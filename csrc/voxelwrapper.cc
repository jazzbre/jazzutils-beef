#include "VoxelWrapper.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

struct ChunkCoordInternal
{
	int x;
	int y;
	int z;
};

struct VW_Chunk
{
	// occupancyX[z * chunkSize + y] packs X bits.
	// occupancyY[z * chunkSize + x] packs Y bits.
	// occupancyZ[y * chunkSize + x] packs Z bits.
	std::vector<uint64_t> occupancyX;
	std::vector<uint64_t> occupancyY;
	std::vector<uint64_t> occupancyZ;

	int solidVoxelCount = 0;
	uint32_t version = 0;
};

struct VW_MeshBuildScratch
{
	std::vector<VW_Vertex> vertices;
	std::vector<uint32_t> indices;

	std::vector<uint64_t> positiveRows;
	std::vector<uint64_t> negativeRows;
};

struct VW_World
{
	int sizeX = 0;
	int sizeY = 0;
	int sizeZ = 0;

	int chunkSize = 32;

	int chunksX = 0;
	int chunksY = 0;
	int chunksZ = 0;

	std::vector<uint8_t> voxels;
	std::vector<VW_Chunk> chunks;

	std::vector<int> dirtyChunkIndices;
	std::vector<uint8_t> dirtyFlags;
};

static bool InBounds(const VW_World* w, int x, int y, int z)
{
	return w &&
		x >= 0 &&
		y >= 0 &&
		z >= 0 &&
		x < w->sizeX &&
		y < w->sizeY &&
		z < w->sizeZ;
}

static int VoxelIndex(const VW_World* w, int x, int y, int z)
{
	return x + y * w->sizeX + z * w->sizeX * w->sizeY;
}

static int ChunkLinearIndex(const VW_World* w, int chunkX, int chunkY, int chunkZ)
{
	if (!w)
		return -1;

	if (chunkX < 0 || chunkY < 0 || chunkZ < 0 ||
		chunkX >= w->chunksX ||
		chunkY >= w->chunksY ||
		chunkZ >= w->chunksZ)
	{
		return -1;
	}

	return chunkX + chunkY * w->chunksX + chunkZ * w->chunksX * w->chunksY;
}

static ChunkCoordInternal ChunkCoordFromLinearIndex(const VW_World* w, int index)
{
	ChunkCoordInternal c{ 0, 0, 0 };

	if (!w || index < 0)
		return c;

	c.x = index % w->chunksX;
	c.y = (index / w->chunksX) % w->chunksY;
	c.z = index / (w->chunksX * w->chunksY);

	return c;
}

static VW_Chunk* GetChunk(VW_World* w, int chunkX, int chunkY, int chunkZ)
{
	const int idx = ChunkLinearIndex(w, chunkX, chunkY, chunkZ);

	if (idx < 0)
		return nullptr;

	return &w->chunks[(size_t)idx];
}

static const VW_Chunk* GetChunkConst(const VW_World* w, int chunkX, int chunkY, int chunkZ)
{
	const int idx = ChunkLinearIndex(w, chunkX, chunkY, chunkZ);

	if (idx < 0)
		return nullptr;

	return &w->chunks[(size_t)idx];
}

static uint8_t GetVoxelInternal(const VW_World* w, int x, int y, int z)
{
	if (!InBounds(w, x, y, z))
		return 0;

	return w->voxels[(size_t)VoxelIndex(w, x, y, z)];
}

static uint64_t MakeLowBitsMask(int bitCount)
{
	if (bitCount <= 0)
		return 0ull;

	if (bitCount >= 64)
		return ~0ull;

	return (1ull << bitCount) - 1ull;
}

static int CountTrailingZeros64(uint64_t value)
{
	if (value == 0)
		return 64;

#if defined(_MSC_VER) && defined(_M_X64)
	unsigned long index = 0;
	_BitScanForward64(&index, value);
	return (int)index;
#elif defined(_MSC_VER) && defined(_M_IX86)
	const uint32_t low = (uint32_t)(value & 0xFFFFFFFFull);

	if (low != 0)
	{
		unsigned long index = 0;
		_BitScanForward(&index, low);
		return (int)index;
	}

	const uint32_t high = (uint32_t)(value >> 32);

	unsigned long index = 0;
	_BitScanForward(&index, high);
	return 32 + (int)index;
#elif defined(__GNUC__) || defined(__clang__)
	return __builtin_ctzll(value);
#else
	int count = 0;

	while ((value & 1ull) == 0ull)
	{
		value >>= 1;
		count++;
	}

	return count;
#endif
}

static void MarkDirtyChunkIndex(VW_World* w, int chunkIndex)
{
	if (!w)
		return;

	if (chunkIndex < 0 || chunkIndex >= (int)w->chunks.size())
		return;

	if (w->dirtyFlags[(size_t)chunkIndex] == 0)
	{
		w->dirtyFlags[(size_t)chunkIndex] = 1;
		w->dirtyChunkIndices.push_back(chunkIndex);
	}
}

static void MarkDirtyChunk(VW_World* w, int chunkX, int chunkY, int chunkZ)
{
	const int index = ChunkLinearIndex(w, chunkX, chunkY, chunkZ);
	MarkDirtyChunkIndex(w, index);
}

static void MarkDirtyVoxelBounds(
	VW_World* w,
	int minX,
	int minY,
	int minZ,
	int maxX,
	int maxY,
	int maxZ)
{
	if (!w)
		return;

	if (minX > maxX || minY > maxY || minZ > maxZ)
		return;

	minX = std::max(0, minX - 1);
	minY = std::max(0, minY - 1);
	minZ = std::max(0, minZ - 1);

	maxX = std::min(w->sizeX - 1, maxX + 1);
	maxY = std::min(w->sizeY - 1, maxY + 1);
	maxZ = std::min(w->sizeZ - 1, maxZ + 1);

	const int cs = w->chunkSize;

	const int minChunkX = minX / cs;
	const int minChunkY = minY / cs;
	const int minChunkZ = minZ / cs;

	const int maxChunkX = maxX / cs;
	const int maxChunkY = maxY / cs;
	const int maxChunkZ = maxZ / cs;

	for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
	{
		for (int cy = minChunkY; cy <= maxChunkY; cy++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
				MarkDirtyChunk(w, cx, cy, cz);
		}
	}
}

static bool SetVoxelMaterialInternal(
	VW_World* w,
	int x,
	int y,
	int z,
	uint8_t material)
{
	if (!InBounds(w, x, y, z))
		return false;

	const int voxelIndex = VoxelIndex(w, x, y, z);
	const uint8_t oldMaterial = w->voxels[(size_t)voxelIndex];

	if (oldMaterial == material)
		return false;

	const int cs = w->chunkSize;

	const int chunkX = x / cs;
	const int chunkY = y / cs;
	const int chunkZ = z / cs;

	VW_Chunk* chunk = GetChunk(w, chunkX, chunkY, chunkZ);

	if (!chunk)
		return false;

	const int lx = x - chunkX * cs;
	const int ly = y - chunkY * cs;
	const int lz = z - chunkZ * cs;

	const uint64_t bitX = 1ull << lx;
	const uint64_t bitY = 1ull << ly;
	const uint64_t bitZ = 1ull << lz;

	uint64_t& rowX = chunk->occupancyX[(size_t)(lz * cs + ly)];
	uint64_t& rowY = chunk->occupancyY[(size_t)(lz * cs + lx)];
	uint64_t& rowZ = chunk->occupancyZ[(size_t)(ly * cs + lx)];

	const bool oldSolid = oldMaterial != 0;
	const bool newSolid = material != 0;

	if (!oldSolid && newSolid)
		chunk->solidVoxelCount++;
	else if (oldSolid && !newSolid)
		chunk->solidVoxelCount--;

	if (newSolid)
	{
		rowX |= bitX;
		rowY |= bitY;
		rowZ |= bitZ;
	}
	else
	{
		rowX &= ~bitX;
		rowY &= ~bitY;
		rowZ &= ~bitZ;
	}

	w->voxels[(size_t)voxelIndex] = material;
	chunk->version++;

	return true;
}

static void ClearAllOccupancy(VW_World* w)
{
	if (!w)
		return;

	for (VW_Chunk& chunk : w->chunks)
	{
		std::fill(chunk.occupancyX.begin(), chunk.occupancyX.end(), 0ull);
		std::fill(chunk.occupancyY.begin(), chunk.occupancyY.end(), 0ull);
		std::fill(chunk.occupancyZ.begin(), chunk.occupancyZ.end(), 0ull);

		chunk.solidVoxelCount = 0;
		chunk.version++;
	}
}

static void RebuildAllOccupancy(VW_World* w)
{
	if (!w)
		return;

	ClearAllOccupancy(w);

	const int cs = w->chunkSize;

	for (int z = 0; z < w->sizeZ; z++)
	{
		for (int y = 0; y < w->sizeY; y++)
		{
			for (int x = 0; x < w->sizeX; x++)
			{
				const uint8_t material = GetVoxelInternal(w, x, y, z);

				if (material == 0)
					continue;

				const int chunkX = x / cs;
				const int chunkY = y / cs;
				const int chunkZ = z / cs;

				VW_Chunk* chunk = GetChunk(w, chunkX, chunkY, chunkZ);

				if (!chunk)
					continue;

				const int lx = x - chunkX * cs;
				const int ly = y - chunkY * cs;
				const int lz = z - chunkZ * cs;

				chunk->occupancyX[(size_t)(lz * cs + ly)] |= 1ull << lx;
				chunk->occupancyY[(size_t)(lz * cs + lx)] |= 1ull << ly;
				chunk->occupancyZ[(size_t)(ly * cs + lx)] |= 1ull << lz;

				chunk->solidVoxelCount++;
			}
		}
	}
}

static float Dot(VW_Vec3 a, VW_Vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static VW_Vec3 Sub(VW_Vec3 a, VW_Vec3 b)
{
	VW_Vec3 r;
	r.x = a.x - b.x;
	r.y = a.y - b.y;
	r.z = a.z - b.z;
	return r;
}

static VW_Vec3 Add(VW_Vec3 a, VW_Vec3 b)
{
	VW_Vec3 r;
	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;
	return r;
}

static VW_Vec3 Mul(VW_Vec3 a, float s)
{
	VW_Vec3 r;
	r.x = a.x * s;
	r.y = a.y * s;
	r.z = a.z * s;
	return r;
}

static float Clamp01(float v)
{
	if (v < 0.0f)
		return 0.0f;

	if (v > 1.0f)
		return 1.0f;

	return v;
}

static float LengthSq(VW_Vec3 v)
{
	return Dot(v, v);
}

static float PointSegmentDistanceSq(VW_Vec3 p, VW_Vec3 a, VW_Vec3 b)
{
	const VW_Vec3 ab = Sub(b, a);
	const float denom = Dot(ab, ab);

	if (denom <= 0.000001f)
		return LengthSq(Sub(p, a));

	float t = Dot(Sub(p, a), ab) / denom;
	t = Clamp01(t);

	const VW_Vec3 q = Add(a, Mul(ab, t));
	return LengthSq(Sub(p, q));
}

static uint64_t BuildOccupancyRowFallback(
	const VW_World* w,
	int axis,
	int axisCoord,
	int uAxis,
	int vAxis,
	int uMin,
	int vCoord,
	int width)
{
	uint64_t bits = 0ull;

	for (int i = 0; i < width; i++)
	{
		int p[3] = { 0, 0, 0 };

		p[axis] = axisCoord;
		p[uAxis] = uMin + i;
		p[vAxis] = vCoord;

		if (GetVoxelInternal(w, p[0], p[1], p[2]) != 0)
			bits |= 1ull << i;
	}

	return bits;
}

static uint64_t BuildOccupancyRowCached(
	const VW_World* w,
	ChunkCoordInternal c,
	const int minCoord[3],
	const int maxCoord[3],
	int axis,
	int axisCoord,
	int uAxis,
	int vAxis,
	int uMin,
	int vCoord,
	int width)
{
	if (!w || width <= 0 || width > 64)
		return 0ull;

	if (axisCoord < minCoord[axis] || axisCoord >= maxCoord[axis])
	{
		return BuildOccupancyRowFallback(
			w,
			axis,
			axisCoord,
			uAxis,
			vAxis,
			uMin,
			vCoord,
			width);
	}

	if (vCoord < minCoord[vAxis] || vCoord >= maxCoord[vAxis])
	{
		return BuildOccupancyRowFallback(
			w,
			axis,
			axisCoord,
			uAxis,
			vAxis,
			uMin,
			vCoord,
			width);
	}

	if (uMin < minCoord[uAxis] || (uMin + width) > maxCoord[uAxis])
	{
		return BuildOccupancyRowFallback(
			w,
			axis,
			axisCoord,
			uAxis,
			vAxis,
			uMin,
			vCoord,
			width);
	}

	const VW_Chunk* chunk = GetChunkConst(w, c.x, c.y, c.z);

	if (!chunk)
		return 0ull;

	const int cs = w->chunkSize;

	int fixed[3] = { 0, 0, 0 };
	fixed[axis] = axisCoord - minCoord[axis];
	fixed[vAxis] = vCoord - minCoord[vAxis];

	const int localUStart = uMin - minCoord[uAxis];

	uint64_t row = 0ull;

	if (uAxis == 0)
	{
		const int ly = fixed[1];
		const int lz = fixed[2];

		row = chunk->occupancyX[(size_t)(lz * cs + ly)];
		row >>= localUStart;
	}
	else if (uAxis == 1)
	{
		const int lx = fixed[0];
		const int lz = fixed[2];

		row = chunk->occupancyY[(size_t)(lz * cs + lx)];
		row >>= localUStart;
	}
	else
	{
		const int lx = fixed[0];
		const int ly = fixed[1];

		row = chunk->occupancyZ[(size_t)(ly * cs + lx)];
		row >>= localUStart;
	}

	return row & MakeLowBitsMask(width);
}

static uint8_t GetFaceMaterial(
	const VW_World* w,
	int axis,
	int sign,
	int plane,
	int uAxis,
	int vAxis,
	int u,
	int v)
{
	int p[3] = { 0, 0, 0 };

	if (sign > 0)
		p[axis] = plane - 1;
	else
		p[axis] = plane;

	p[uAxis] = u;
	p[vAxis] = v;

	return GetVoxelInternal(w, p[0], p[1], p[2]);
}

static bool FaceRunHasMaterial(
	const VW_World* w,
	int axis,
	int sign,
	int plane,
	int uAxis,
	int vAxis,
	int u0,
	int v,
	int width,
	uint8_t material)
{
	for (int i = 0; i < width; i++)
	{
		const uint8_t m = GetFaceMaterial(
			w,
			axis,
			sign,
			plane,
			uAxis,
			vAxis,
			u0 + i,
			v);

		if (m != material)
			return false;
	}

	return true;
}

static void AddGreedyQuad(
	std::vector<VW_Vertex>& vertices,
	std::vector<uint32_t>& indices,
	int axis,
	int sign,
	int plane,
	int u0,
	int v0,
	int width,
	int height,
	uint8_t material)
{
	const int uAxis = (axis + 1) % 3;
	const int vAxis = (axis + 2) % 3;

	int p0[3] = { 0, 0, 0 };
	int p1[3] = { 0, 0, 0 };
	int p2[3] = { 0, 0, 0 };
	int p3[3] = { 0, 0, 0 };

	p0[axis] = plane;
	p1[axis] = plane;
	p2[axis] = plane;
	p3[axis] = plane;

	p0[uAxis] = u0;
	p0[vAxis] = v0;

	p1[uAxis] = u0 + width;
	p1[vAxis] = v0;

	p2[uAxis] = u0 + width;
	p2[vAxis] = v0 + height;

	p3[uAxis] = u0;
	p3[vAxis] = v0 + height;

	float nx = 0.0f;
	float ny = 0.0f;
	float nz = 0.0f;

	if (axis == 0)
		nx = (float)sign;
	else if (axis == 1)
		ny = (float)sign;
	else
		nz = (float)sign;

	const uint32_t base = (uint32_t)vertices.size();

	VW_Vertex v[4] = {};

	v[0].x = (float)p0[0];
	v[0].y = (float)p0[1];
	v[0].z = (float)p0[2];

	v[1].x = (float)p1[0];
	v[1].y = (float)p1[1];
	v[1].z = (float)p1[2];

	v[2].x = (float)p2[0];
	v[2].y = (float)p2[1];
	v[2].z = (float)p2[2];

	v[3].x = (float)p3[0];
	v[3].y = (float)p3[1];
	v[3].z = (float)p3[2];

	for (int i = 0; i < 4; i++)
	{
		v[i].nx = nx;
		v[i].ny = ny;
		v[i].nz = nz;
		v[i].material = material;
	}

	v[0].u = 0.0f;
	v[0].v = 0.0f;

	v[1].u = (float)width;
	v[1].v = 0.0f;

	v[2].u = (float)width;
	v[2].v = (float)height;

	v[3].u = 0.0f;
	v[3].v = (float)height;

	vertices.push_back(v[0]);
	vertices.push_back(v[1]);
	vertices.push_back(v[2]);
	vertices.push_back(v[3]);

	if (sign > 0)
	{
		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);

		indices.push_back(base + 0);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
	}
	else
	{
		indices.push_back(base + 0);
		indices.push_back(base + 2);
		indices.push_back(base + 1);

		indices.push_back(base + 0);
		indices.push_back(base + 3);
		indices.push_back(base + 2);
	}
}

static void ConsumeBitGreedyFaceRows(
	VW_World* w,
	std::vector<VW_Vertex>& outVertices,
	std::vector<uint32_t>& outIndices,
	std::vector<uint64_t>& faceRows,
	int axis,
	int sign,
	int plane,
	int uAxis,
	int vAxis,
	int uMin,
	int vMin,
	int maskW,
	int maskH)
{
	for (int y = 0; y < maskH; y++)
	{
		while (faceRows[(size_t)y] != 0ull)
		{
			const uint64_t row = faceRows[(size_t)y];
			const int x = CountTrailingZeros64(row);

			const int u0 = uMin + x;
			const int v0 = vMin + y;

			const uint8_t material = GetFaceMaterial(
				w,
				axis,
				sign,
				plane,
				uAxis,
				vAxis,
				u0,
				v0);

			if (material == 0)
			{
				faceRows[(size_t)y] &= ~(1ull << x);
				continue;
			}

			int quadW = 1;

			while (x + quadW < maskW)
			{
				const uint64_t bit = 1ull << (x + quadW);

				if ((faceRows[(size_t)y] & bit) == 0ull)
					break;

				const uint8_t m = GetFaceMaterial(
					w,
					axis,
					sign,
					plane,
					uAxis,
					vAxis,
					uMin + x + quadW,
					v0);

				if (m != material)
					break;

				quadW++;
			}

			const uint64_t quadMask =
				quadW >= 64
				? ~0ull
				: (((1ull << quadW) - 1ull) << x);

			int quadH = 1;

			while (y + quadH < maskH)
			{
				const uint64_t nextRow = faceRows[(size_t)(y + quadH)];

				if ((nextRow & quadMask) != quadMask)
					break;

				const int nextV = vMin + y + quadH;

				if (!FaceRunHasMaterial(
					w,
					axis,
					sign,
					plane,
					uAxis,
					vAxis,
					u0,
					nextV,
					quadW,
					material))
				{
					break;
				}

				quadH++;
			}

			for (int yy = 0; yy < quadH; yy++)
				faceRows[(size_t)(y + yy)] &= ~quadMask;

			AddGreedyQuad(
				outVertices,
				outIndices,
				axis,
				sign,
				plane,
				u0,
				v0,
				quadW,
				quadH,
				material);
		}
	}
}

static bool BuildChunkMeshBitGreedy(
	VW_World* w,
	ChunkCoordInternal c,
	VW_MeshBuildScratch& scratch)
{
	scratch.vertices.clear();
	scratch.indices.clear();

	if (!w)
		return false;

	if (w->chunkSize > 64)
		return false;

	const int chunkIndex = ChunkLinearIndex(w, c.x, c.y, c.z);

	if (chunkIndex < 0)
		return false;

	const VW_Chunk& chunk = w->chunks[(size_t)chunkIndex];

	if (chunk.solidVoxelCount == 0)
		return true;

	const int cs = w->chunkSize;

	int minCoord[3];
	int maxCoord[3];

	minCoord[0] = c.x * cs;
	minCoord[1] = c.y * cs;
	minCoord[2] = c.z * cs;

	maxCoord[0] = std::min(minCoord[0] + cs, w->sizeX);
	maxCoord[1] = std::min(minCoord[1] + cs, w->sizeY);
	maxCoord[2] = std::min(minCoord[2] + cs, w->sizeZ);

	if (minCoord[0] >= maxCoord[0] ||
		minCoord[1] >= maxCoord[1] ||
		minCoord[2] >= maxCoord[2])
	{
		return false;
	}

	// Reuse capacity across builds on the same thread.
	if (scratch.vertices.capacity() < 1024)
		scratch.vertices.reserve(1024);

	if (scratch.indices.capacity() < 1536)
		scratch.indices.reserve(1536);

	for (int axis = 0; axis < 3; axis++)
	{
		const int uAxis = (axis + 1) % 3;
		const int vAxis = (axis + 2) % 3;

		const int uMin = minCoord[uAxis];
		const int vMin = minCoord[vAxis];

		const int uMax = maxCoord[uAxis];
		const int vMax = maxCoord[vAxis];

		const int maskW = uMax - uMin;
		const int maskH = vMax - vMin;

		if (maskW <= 0 || maskH <= 0 || maskW > 64)
			continue;

		const uint64_t validMask = MakeLowBitsMask(maskW);

		scratch.positiveRows.resize((size_t)maskH);
		scratch.negativeRows.resize((size_t)maskH);

		std::vector<uint64_t>& positiveRows = scratch.positiveRows;
		std::vector<uint64_t>& negativeRows = scratch.negativeRows;

		for (int plane = minCoord[axis]; plane <= maxCoord[axis]; plane++)
		{
			for (int y = 0; y < maskH; y++)
			{
				const int vCoord = vMin + y;

				const uint64_t before = BuildOccupancyRowCached(
					w,
					c,
					minCoord,
					maxCoord,
					axis,
					plane - 1,
					uAxis,
					vAxis,
					uMin,
					vCoord,
					maskW);

				const uint64_t after = BuildOccupancyRowCached(
					w,
					c,
					minCoord,
					maxCoord,
					axis,
					plane,
					uAxis,
					vAxis,
					uMin,
					vCoord,
					maskW);

				positiveRows[(size_t)y] = (before & ~after) & validMask;
				negativeRows[(size_t)y] = (~before & after) & validMask;
			}

			ConsumeBitGreedyFaceRows(
				w,
				scratch.vertices,
				scratch.indices,
				positiveRows,
				axis,
				+1,
				plane,
				uAxis,
				vAxis,
				uMin,
				vMin,
				maskW,
				maskH);

			ConsumeBitGreedyFaceRows(
				w,
				scratch.vertices,
				scratch.indices,
				negativeRows,
				axis,
				-1,
				plane,
				uAxis,
				vAxis,
				uMin,
				vMin,
				maskW,
				maskH);
		}
	}

	return true;
}

extern "C"
{

	VW_API VW_World* VW_CreateWorld(
		int sizeX,
		int sizeY,
		int sizeZ,
		int chunkSize)
	{
		if (sizeX <= 0 || sizeY <= 0 || sizeZ <= 0)
			return nullptr;

		if (chunkSize <= 0)
			chunkSize = 32;

		if (chunkSize > 64)
			chunkSize = 64;

		VW_World* w = new VW_World();

		w->sizeX = sizeX;
		w->sizeY = sizeY;
		w->sizeZ = sizeZ;
		w->chunkSize = chunkSize;

		w->chunksX = (sizeX + chunkSize - 1) / chunkSize;
		w->chunksY = (sizeY + chunkSize - 1) / chunkSize;
		w->chunksZ = (sizeZ + chunkSize - 1) / chunkSize;

		const size_t voxelCount = (size_t)sizeX * (size_t)sizeY * (size_t)sizeZ;
		w->voxels.resize(voxelCount, 0);

		const int chunkCount = w->chunksX * w->chunksY * w->chunksZ;
		w->chunks.resize((size_t)chunkCount);
		w->dirtyFlags.resize((size_t)chunkCount, 0);

		const size_t rowCount = (size_t)chunkSize * (size_t)chunkSize;

		for (VW_Chunk& chunk : w->chunks)
		{
			chunk.occupancyX.resize(rowCount, 0ull);
			chunk.occupancyY.resize(rowCount, 0ull);
			chunk.occupancyZ.resize(rowCount, 0ull);
			chunk.solidVoxelCount = 0;
			chunk.version = 0;
		}

		return w;
	}

	VW_API void VW_DestroyWorld(
		VW_World* world)
	{
		delete world;
	}

	VW_API int VW_SetData(
		VW_World* world,
		const uint8_t* voxels,
		int sizeX,
		int sizeY,
		int sizeZ)
	{
		if (!world || !voxels)
			return 0;

		if (sizeX != world->sizeX ||
			sizeY != world->sizeY ||
			sizeZ != world->sizeZ)
		{
			return 0;
		}

		const size_t voxelCount = (size_t)sizeX * (size_t)sizeY * (size_t)sizeZ;
		std::memcpy(world->voxels.data(), voxels, voxelCount);

		RebuildAllOccupancy(world);

		world->dirtyChunkIndices.clear();
		std::fill(world->dirtyFlags.begin(), world->dirtyFlags.end(), 0);

		for (int z = 0; z < world->chunksZ; z++)
		{
			for (int y = 0; y < world->chunksY; y++)
			{
				for (int x = 0; x < world->chunksX; x++)
					MarkDirtyChunk(world, x, y, z);
			}
		}

		return 1;
	}

	VW_API int VW_GetSizeX(
		VW_World* world)
	{
		return world ? world->sizeX : 0;
	}

	VW_API int VW_GetSizeY(
		VW_World* world)
	{
		return world ? world->sizeY : 0;
	}

	VW_API int VW_GetSizeZ(
		VW_World* world)
	{
		return world ? world->sizeZ : 0;
	}

	VW_API int VW_GetChunkSize(
		VW_World* world)
	{
		return world ? world->chunkSize : 0;
	}

	VW_API int VW_GetChunkCountX(
		VW_World* world)
	{
		return world ? world->chunksX : 0;
	}

	VW_API int VW_GetChunkCountY(
		VW_World* world)
	{
		return world ? world->chunksY : 0;
	}

	VW_API int VW_GetChunkCountZ(
		VW_World* world)
	{
		return world ? world->chunksZ : 0;
	}

	VW_API int VW_GetChunkLinearIndex(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ)
	{
		return ChunkLinearIndex(world, chunkX, chunkY, chunkZ);
	}

	VW_API int VW_GetVoxel(
		VW_World* world,
		int x,
		int y,
		int z)
	{
		if (!world || !InBounds(world, x, y, z))
			return 0;

		return (int)world->voxels[(size_t)VoxelIndex(world, x, y, z)];
	}

	VW_API int VW_SetVoxel(
		VW_World* world,
		int x,
		int y,
		int z,
		uint8_t material)
	{
		if (!world || !InBounds(world, x, y, z))
			return 0;

		if (!SetVoxelMaterialInternal(world, x, y, z, material))
			return 1;

		MarkDirtyVoxelBounds(world, x, y, z, x, y, z);

		return 1;
	}

	VW_API int VW_DrillSphere(
		VW_World* world,
		float cx,
		float cy,
		float cz,
		float radius,
		uint8_t replacementMaterial)
	{
		if (!world || radius <= 0.0f)
			return 0;

		const int minX = std::max(0, (int)std::floor(cx - radius));
		const int minY = std::max(0, (int)std::floor(cy - radius));
		const int minZ = std::max(0, (int)std::floor(cz - radius));

		const int maxX = std::min(world->sizeX - 1, (int)std::ceil(cx + radius));
		const int maxY = std::min(world->sizeY - 1, (int)std::ceil(cy + radius));
		const int maxZ = std::min(world->sizeZ - 1, (int)std::ceil(cz + radius));

		const float r2 = radius * radius;
		int changed = 0;

		int changedMinX = world->sizeX;
		int changedMinY = world->sizeY;
		int changedMinZ = world->sizeZ;

		int changedMaxX = -1;
		int changedMaxY = -1;
		int changedMaxZ = -1;

		for (int z = minZ; z <= maxZ; z++)
		{
			const float pz = (float)z + 0.5f;
			const float dz = pz - cz;
			const float dz2 = dz * dz;

			for (int y = minY; y <= maxY; y++)
			{
				const float py = (float)y + 0.5f;
				const float dy = py - cy;
				const float dy2 = dy * dy;

				const float remaining = r2 - dz2 - dy2;

				if (remaining < 0.0f)
					continue;

				const float xExtent = std::sqrt(remaining);

				const int rowMinX = std::max(minX, (int)std::floor(cx - xExtent));
				const int rowMaxX = std::min(maxX, (int)std::ceil(cx + xExtent));

				for (int x = rowMinX; x <= rowMaxX; x++)
				{
					const float px = (float)x + 0.5f;
					const float dx = px - cx;

					if (dx * dx > remaining)
						continue;

					if (SetVoxelMaterialInternal(world, x, y, z, replacementMaterial))
					{
						changed++;

						changedMinX = std::min(changedMinX, x);
						changedMinY = std::min(changedMinY, y);
						changedMinZ = std::min(changedMinZ, z);

						changedMaxX = std::max(changedMaxX, x);
						changedMaxY = std::max(changedMaxY, y);
						changedMaxZ = std::max(changedMaxZ, z);
					}
				}
			}
		}

		if (changed > 0)
		{
			MarkDirtyVoxelBounds(
				world,
				changedMinX,
				changedMinY,
				changedMinZ,
				changedMaxX,
				changedMaxY,
				changedMaxZ);
		}

		return changed;
	}

	VW_API int VW_DrillCapsule(
		VW_World* world,
		VW_Vec3 a,
		VW_Vec3 b,
		float radius,
		uint8_t replacementMaterial)
	{
		if (!world || radius <= 0.0f)
			return 0;

		const int minX = std::max(0, (int)std::floor(std::min(a.x, b.x) - radius));
		const int minY = std::max(0, (int)std::floor(std::min(a.y, b.y) - radius));
		const int minZ = std::max(0, (int)std::floor(std::min(a.z, b.z) - radius));

		const int maxX = std::min(world->sizeX - 1, (int)std::ceil(std::max(a.x, b.x) + radius));
		const int maxY = std::min(world->sizeY - 1, (int)std::ceil(std::max(a.y, b.y) + radius));
		const int maxZ = std::min(world->sizeZ - 1, (int)std::ceil(std::max(a.z, b.z) + radius));

		const float r2 = radius * radius;
		int changed = 0;

		int changedMinX = world->sizeX;
		int changedMinY = world->sizeY;
		int changedMinZ = world->sizeZ;

		int changedMaxX = -1;
		int changedMaxY = -1;
		int changedMaxZ = -1;

		for (int z = minZ; z <= maxZ; z++)
		{
			for (int y = minY; y <= maxY; y++)
			{
				for (int x = minX; x <= maxX; x++)
				{
					VW_Vec3 p;
					p.x = (float)x + 0.5f;
					p.y = (float)y + 0.5f;
					p.z = (float)z + 0.5f;

					if (PointSegmentDistanceSq(p, a, b) <= r2)
					{
						if (SetVoxelMaterialInternal(world, x, y, z, replacementMaterial))
						{
							changed++;

							changedMinX = std::min(changedMinX, x);
							changedMinY = std::min(changedMinY, y);
							changedMinZ = std::min(changedMinZ, z);

							changedMaxX = std::max(changedMaxX, x);
							changedMaxY = std::max(changedMaxY, y);
							changedMaxZ = std::max(changedMaxZ, z);
						}
					}
				}
			}
		}

		if (changed > 0)
		{
			MarkDirtyVoxelBounds(
				world,
				changedMinX,
				changedMinY,
				changedMinZ,
				changedMaxX,
				changedMaxY,
				changedMaxZ);
		}

		return changed;
	}

	VW_API int VW_GetDirtyChunkCount(
		VW_World* world)
	{
		if (!world)
			return 0;

		return (int)world->dirtyChunkIndices.size();
	}

	VW_API int VW_GetDirtyChunkCoord(
		VW_World* world,
		int dirtyIndex,
		VW_ChunkCoord* outCoord)
	{
		if (!world || !outCoord)
			return 0;

		if (dirtyIndex < 0 || dirtyIndex >= (int)world->dirtyChunkIndices.size())
			return 0;

		const int chunkIndex = world->dirtyChunkIndices[(size_t)dirtyIndex];
		const ChunkCoordInternal c = ChunkCoordFromLinearIndex(world, chunkIndex);

		outCoord->x = c.x;
		outCoord->y = c.y;
		outCoord->z = c.z;

		return 1;
	}

	VW_API int VW_BuildChunkMeshWithScratch(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ,
		VW_MeshBuildScratch* scratch,
		VW_Mesh* outMesh)
	{
		if (!world || !scratch || !outMesh)
			return 0;

		const int chunkIndex = ChunkLinearIndex(world, chunkX, chunkY, chunkZ);

		if (chunkIndex < 0)
			return 0;

		std::memset(outMesh, 0, sizeof(VW_Mesh));

		ChunkCoordInternal c;
		c.x = chunkX;
		c.y = chunkY;
		c.z = chunkZ;

		if (!BuildChunkMeshBitGreedy(world, c, *scratch))
			return 0;

		std::vector<VW_Vertex>& vertices = scratch->vertices;
		std::vector<uint32_t>& indices = scratch->indices;

		outMesh->chunkX = chunkX;
		outMesh->chunkY = chunkY;
		outMesh->chunkZ = chunkZ;
		outMesh->chunkLinearIndex = chunkIndex;

		outMesh->vertexCount = (uint32_t)vertices.size();
		outMesh->indexCount = (uint32_t)indices.size();

		if (!vertices.empty())
		{
			const size_t bytes = vertices.size() * sizeof(VW_Vertex);
			outMesh->vertices = (VW_Vertex*)std::malloc(bytes);

			if (!outMesh->vertices)
			{
				std::memset(outMesh, 0, sizeof(VW_Mesh));
				return 0;
			}

			std::memcpy(outMesh->vertices, vertices.data(), bytes);
		}

		if (!indices.empty())
		{
			const size_t bytes = indices.size() * sizeof(uint32_t);
			outMesh->indices = (uint32_t*)std::malloc(bytes);

			if (!outMesh->indices)
			{
				std::free(outMesh->vertices);
				std::memset(outMesh, 0, sizeof(VW_Mesh));
				return 0;
			}

			std::memcpy(outMesh->indices, indices.data(), bytes);
		}

		return 1;
	}

	VW_API int VW_BuildDirtyChunkMeshWithScratch(
		VW_World* world,
		int dirtyIndex,
		VW_MeshBuildScratch* scratch,
		VW_Mesh* outMesh)
	{
		if (!world || !scratch || !outMesh)
			return 0;

		if (dirtyIndex < 0 || dirtyIndex >= (int)world->dirtyChunkIndices.size())
			return 0;

		const int chunkIndex = world->dirtyChunkIndices[(size_t)dirtyIndex];
		const ChunkCoordInternal c = ChunkCoordFromLinearIndex(world, chunkIndex);

		return VW_BuildChunkMeshWithScratch(
			world,
			c.x,
			c.y,
			c.z,
			scratch,
			outMesh);
	}

	VW_API void VW_FreeMesh(
		VW_Mesh* mesh)
	{
		if (!mesh)
			return;

		std::free(mesh->vertices);
		std::free(mesh->indices);

		mesh->vertices = nullptr;
		mesh->indices = nullptr;

		mesh->vertexCount = 0;
		mesh->indexCount = 0;

		mesh->chunkX = 0;
		mesh->chunkY = 0;
		mesh->chunkZ = 0;
		mesh->chunkLinearIndex = -1;
	}

	VW_API void VW_ClearDirtyChunks(
		VW_World* world)
	{
		if (!world)
			return;

		for (int chunkIndex : world->dirtyChunkIndices)
		{
			if (chunkIndex >= 0 && chunkIndex < (int)world->dirtyFlags.size())
				world->dirtyFlags[(size_t)chunkIndex] = 0;
		}

		world->dirtyChunkIndices.clear();
	}

	VW_API VW_MeshBuildScratch* VW_CreateMeshBuildScratch(void)
	{
		VW_MeshBuildScratch* scratch = new VW_MeshBuildScratch();

		scratch->vertices.reserve(1024);
		scratch->indices.reserve(1536);
		scratch->positiveRows.reserve(64);
		scratch->negativeRows.reserve(64);

		return scratch;
	}

	VW_API void VW_DestroyMeshBuildScratch(
		VW_MeshBuildScratch* scratch)
	{
		delete scratch;
	}

	VW_API void VW_ClearMeshBuildScratch(
		VW_MeshBuildScratch* scratch)
	{
		if (!scratch)
			return;

		scratch->vertices.clear();
		scratch->indices.clear();
		scratch->positiveRows.clear();
		scratch->negativeRows.clear();
	}

}