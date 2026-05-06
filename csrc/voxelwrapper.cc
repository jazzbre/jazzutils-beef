#include "VoxelWrapper.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <unordered_set>
#include <vector>

struct ChunkCoordInternal
{
	int x;
	int y;
	int z;

	bool operator==(const ChunkCoordInternal& o) const
	{
		return x == o.x && y == o.y && z == o.z;
	}
};

struct ChunkCoordHash
{
	size_t operator()(const ChunkCoordInternal& c) const
	{
		uint32_t hx = (uint32_t)c.x * 73856093u;
		uint32_t hy = (uint32_t)c.y * 19349663u;
		uint32_t hz = (uint32_t)c.z * 83492791u;
		return (size_t)(hx ^ hy ^ hz);
	}
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

	std::vector<ChunkCoordInternal> dirtyList;
	std::unordered_set<ChunkCoordInternal, ChunkCoordHash> dirtySet;
};

static bool InBounds(const VW_World* w, int x, int y, int z)
{
	return w &&
		x >= 0 && y >= 0 && z >= 0 &&
		x < w->sizeX &&
		y < w->sizeY &&
		z < w->sizeZ;
}

static int VoxelIndex(const VW_World* w, int x, int y, int z)
{
	return x + y * w->sizeX + z * w->sizeX * w->sizeY;
}

static uint8_t GetVoxelInternal(const VW_World* w, int x, int y, int z)
{
	if (!InBounds(w, x, y, z))
		return 0;

	return w->voxels[(size_t)VoxelIndex(w, x, y, z)];
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

static void MarkDirtyChunk(VW_World* w, int chunkX, int chunkY, int chunkZ)
{
	if (!w)
		return;

	if (chunkX < 0 || chunkY < 0 || chunkZ < 0 ||
		chunkX >= w->chunksX ||
		chunkY >= w->chunksY ||
		chunkZ >= w->chunksZ)
	{
		return;
	}

	ChunkCoordInternal c{ chunkX, chunkY, chunkZ };

	if (w->dirtySet.insert(c).second)
		w->dirtyList.push_back(c);
}

static void MarkDirtyForVoxel(VW_World* w, int x, int y, int z)
{
	if (!w || !InBounds(w, x, y, z))
		return;

	const int cs = w->chunkSize;

	const int cx = x / cs;
	const int cy = y / cs;
	const int cz = z / cs;

	MarkDirtyChunk(w, cx, cy, cz);

	if ((x % cs) == 0)
		MarkDirtyChunk(w, cx - 1, cy, cz);

	if ((x % cs) == cs - 1)
		MarkDirtyChunk(w, cx + 1, cy, cz);

	if ((y % cs) == 0)
		MarkDirtyChunk(w, cx, cy - 1, cz);

	if ((y % cs) == cs - 1)
		MarkDirtyChunk(w, cx, cy + 1, cz);

	if ((z % cs) == 0)
		MarkDirtyChunk(w, cx, cy, cz - 1);

	if ((z % cs) == cs - 1)
		MarkDirtyChunk(w, cx, cy, cz + 1);
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
	VW_Vec3 ab = Sub(b, a);
	float denom = Dot(ab, ab);

	if (denom <= 0.000001f)
		return LengthSq(Sub(p, a));

	float t = Dot(Sub(p, a), ab) / denom;
	t = Clamp01(t);

	VW_Vec3 q = Add(a, Mul(ab, t));
	return LengthSq(Sub(p, q));
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

static bool BuildChunkMeshGreedy(
	VW_World* w,
	ChunkCoordInternal c,
	std::vector<VW_Vertex>& outVertices,
	std::vector<uint32_t>& outIndices)
{
	if (!w)
		return false;

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

		std::vector<int> mask((size_t)maskW * (size_t)maskH);

		for (int plane = minCoord[axis]; plane <= maxCoord[axis]; plane++)
		{
			std::fill(mask.begin(), mask.end(), 0);

			for (int v = vMin; v < vMax; v++)
			{
				for (int u = uMin; u < uMax; u++)
				{
					int aPos[3] = { 0, 0, 0 };
					int bPos[3] = { 0, 0, 0 };

					aPos[axis] = plane - 1;
					bPos[axis] = plane;

					aPos[uAxis] = u;
					bPos[uAxis] = u;

					aPos[vAxis] = v;
					bPos[vAxis] = v;

					const uint8_t a = GetVoxelInternal(w, aPos[0], aPos[1], aPos[2]);
					const uint8_t b = GetVoxelInternal(w, bPos[0], bPos[1], bPos[2]);

					const bool aSolid = a != 0;
					const bool bSolid = b != 0;

					int value = 0;

					if (aSolid && !bSolid)
					{
						// Positive face of voxel A.
						value = (int)a | (1 << 8);
					}
					else if (!aSolid && bSolid)
					{
						// Negative face of voxel B.
						value = (int)b | (2 << 8);
					}

					const int mi = (u - uMin) + (v - vMin) * maskW;
					mask[(size_t)mi] = value;
				}
			}

			for (int y = 0; y < maskH;)
			{
				for (int x = 0; x < maskW;)
				{
					const int value = mask[(size_t)(x + y * maskW)];

					if (value == 0)
					{
						x++;
						continue;
					}

					int quadW = 1;

					while (x + quadW < maskW &&
						mask[(size_t)((x + quadW) + y * maskW)] == value)
					{
						quadW++;
					}

					int quadH = 1;
					bool stop = false;

					while (y + quadH < maskH && !stop)
					{
						for (int k = 0; k < quadW; k++)
						{
							if (mask[(size_t)((x + k) + (y + quadH) * maskW)] != value)
							{
								stop = true;
								break;
							}
						}

						if (!stop)
							quadH++;
					}

					for (int yy = 0; yy < quadH; yy++)
					{
						for (int xx = 0; xx < quadW; xx++)
							mask[(size_t)((x + xx) + (y + yy) * maskW)] = 0;
					}

					const uint8_t material = (uint8_t)(value & 0xFF);
					const int sign = ((value >> 8) == 1) ? +1 : -1;

					const int u0 = uMin + x;
					const int v0 = vMin + y;

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

					x += quadW;
				}

				y++;
			}
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

		world->dirtyList.clear();
		world->dirtySet.clear();

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

		const int idx = VoxelIndex(world, x, y, z);

		if (world->voxels[(size_t)idx] == material)
			return 1;

		world->voxels[(size_t)idx] = material;
		MarkDirtyForVoxel(world, x, y, z);

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

		for (int z = minZ; z <= maxZ; z++)
		{
			for (int y = minY; y <= maxY; y++)
			{
				for (int x = minX; x <= maxX; x++)
				{
					const float px = (float)x + 0.5f;
					const float py = (float)y + 0.5f;
					const float pz = (float)z + 0.5f;

					const float dx = px - cx;
					const float dy = py - cy;
					const float dz = pz - cz;

					if ((dx * dx + dy * dy + dz * dz) <= r2)
					{
						const int idx = VoxelIndex(world, x, y, z);

						if (world->voxels[(size_t)idx] != replacementMaterial)
						{
							world->voxels[(size_t)idx] = replacementMaterial;
							MarkDirtyForVoxel(world, x, y, z);
							changed++;
						}
					}
				}
			}
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
						const int idx = VoxelIndex(world, x, y, z);

						if (world->voxels[(size_t)idx] != replacementMaterial)
						{
							world->voxels[(size_t)idx] = replacementMaterial;
							MarkDirtyForVoxel(world, x, y, z);
							changed++;
						}
					}
				}
			}
		}

		return changed;
	}

	VW_API int VW_GetDirtyChunkCount(
		VW_World* world)
	{
		if (!world)
			return 0;

		return (int)world->dirtyList.size();
	}

	VW_API int VW_GetDirtyChunkCoord(
		VW_World* world,
		int dirtyIndex,
		VW_ChunkCoord* outCoord)
	{
		if (!world || !outCoord)
			return 0;

		if (dirtyIndex < 0 || dirtyIndex >= (int)world->dirtyList.size())
			return 0;

		const ChunkCoordInternal c = world->dirtyList[(size_t)dirtyIndex];

		outCoord->x = c.x;
		outCoord->y = c.y;
		outCoord->z = c.z;

		return 1;
	}

	VW_API int VW_BuildChunkMesh(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ,
		VW_Mesh* outMesh)
	{
		if (!world || !outMesh)
			return 0;

		if (ChunkLinearIndex(world, chunkX, chunkY, chunkZ) < 0)
			return 0;

		std::memset(outMesh, 0, sizeof(VW_Mesh));

		std::vector<VW_Vertex> vertices;
		std::vector<uint32_t> indices;

		ChunkCoordInternal c;
		c.x = chunkX;
		c.y = chunkY;
		c.z = chunkZ;

		if (!BuildChunkMeshGreedy(world, c, vertices, indices))
			return 0;

		outMesh->chunkX = chunkX;
		outMesh->chunkY = chunkY;
		outMesh->chunkZ = chunkZ;
		outMesh->chunkLinearIndex = ChunkLinearIndex(world, chunkX, chunkY, chunkZ);

		outMesh->vertexCount = (uint32_t)vertices.size();
		outMesh->indexCount = (uint32_t)indices.size();

		if (!vertices.empty())
		{
			const size_t bytes = vertices.size() * sizeof(VW_Vertex);
			outMesh->vertices = (VW_Vertex*)std::malloc(bytes);
			std::memcpy(outMesh->vertices, vertices.data(), bytes);
		}

		if (!indices.empty())
		{
			const size_t bytes = indices.size() * sizeof(uint32_t);
			outMesh->indices = (uint32_t*)std::malloc(bytes);
			std::memcpy(outMesh->indices, indices.data(), bytes);
		}

		return 1;
	}

	VW_API int VW_BuildDirtyChunkMesh(
		VW_World* world,
		int dirtyIndex,
		VW_Mesh* outMesh)
	{
		if (!world || !outMesh)
			return 0;

		if (dirtyIndex < 0 || dirtyIndex >= (int)world->dirtyList.size())
			return 0;

		const ChunkCoordInternal c = world->dirtyList[(size_t)dirtyIndex];

		return VW_BuildChunkMesh(world, c.x, c.y, c.z, outMesh);
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

		world->dirtyList.clear();
		world->dirtySet.clear();
	}

}