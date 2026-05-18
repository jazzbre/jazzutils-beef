#pragma once

#include <stdint.h>

#define VW_API

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct VW_Vec3
	{
		float x;
		float y;
		float z;
	} VW_Vec3;

	typedef struct VW_ChunkCoord
	{
		int x;
		int y;
		int z;
	} VW_ChunkCoord;

	typedef struct VW_Vertex
	{
		float x;
		float y;
		float z;

		float nx;
		float ny;
		float nz;

		float u;
		float v;

		uint32_t material;
	} VW_Vertex;

	typedef struct VW_Mesh
	{
		VW_Vertex* vertices;
		uint32_t vertexCount;

		uint32_t* indices;
		uint32_t indexCount;

		int chunkX;
		int chunkY;
		int chunkZ;
		int chunkLinearIndex;
	} VW_Mesh;

	typedef struct VW_World VW_World;
	typedef struct VW_MeshBuildScratch VW_MeshBuildScratch;

	VW_API VW_World* VW_CreateWorld(
		int sizeX,
		int sizeY,
		int sizeZ,
		int chunkSize);

	VW_API void VW_DestroyWorld(
		VW_World* world);

	VW_API int VW_SetData(
		VW_World* world,
		const uint8_t* voxels,
		int sizeX,
		int sizeY,
		int sizeZ);

	VW_API int VW_GetSizeX(
		VW_World* world);

	VW_API int VW_GetSizeY(
		VW_World* world);

	VW_API int VW_GetSizeZ(
		VW_World* world);

	VW_API int VW_GetChunkSize(
		VW_World* world);

	VW_API int VW_GetChunkCountX(
		VW_World* world);

	VW_API int VW_GetChunkCountY(
		VW_World* world);

	VW_API int VW_GetChunkCountZ(
		VW_World* world);

	VW_API int VW_GetChunkLinearIndex(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ);

	VW_API int VW_GetChunkSolidVoxelCount(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ);

	VW_API int VW_IsChunkEmpty(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ);

	VW_API uint32_t VW_GetChunkVersion(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ);

	VW_API int VW_GetVoxel(
		VW_World* world,
		int x,
		int y,
		int z);

	VW_API int VW_SetVoxel(
		VW_World* world,
		int x,
		int y,
		int z,
		uint8_t material);

	VW_API int VW_DrillSphere(
		VW_World* world,
		float cx,
		float cy,
		float cz,
		float radius,
		uint8_t replacementMaterial);

	using VW_DrillSphereCallback = uint8_t(*)(int x, int y, int z, uint8_t material, void* userData);
	VW_API int VW_DrillSphereWithCallback(
		VW_World* world,
		float cx,
		float cy,
		float cz,
		float radius,
		VW_DrillSphereCallback callback, void* userData);

	VW_API int VW_DrillCapsule(
		VW_World* world,
		VW_Vec3 a,
		VW_Vec3 b,
		float radius,
		uint8_t replacementMaterial);

	VW_API int VW_GetDirtyChunkCount(
		VW_World* world);

	VW_API int VW_GetDirtyChunkCoord(
		VW_World* world,
		int dirtyIndex,
		VW_ChunkCoord* outCoord);

	VW_API int VW_BuildChunkMeshWithScratch(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ,
		VW_MeshBuildScratch* scratch,
		VW_Mesh* outMesh);

	VW_API int VW_BuildDirtyChunkMeshWithScratch(
		VW_World* world,
		int dirtyIndex,
		VW_MeshBuildScratch* scratch,
		VW_Mesh* outMesh);

	VW_API int VW_BuildChunkMeshLODWithScratch(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ,
		int lod,
		VW_MeshBuildScratch* scratch,
		VW_Mesh* outMesh);

	VW_API int VW_BuildDirtyChunkMeshLODWithScratch(
		VW_World* world,
		int dirtyIndex,
		int lod,
		VW_MeshBuildScratch* scratch,
		VW_Mesh* outMesh);

	VW_API void VW_FreeMesh(
		VW_Mesh* mesh);

	VW_API void VW_ClearDirtyChunks(
		VW_World* world);

	VW_API VW_MeshBuildScratch* VW_CreateMeshBuildScratch(void);

	VW_API void VW_DestroyMeshBuildScratch(
		VW_MeshBuildScratch* scratch);

	VW_API void VW_ClearMeshBuildScratch(
		VW_MeshBuildScratch* scratch);

	// Ray cast

	typedef struct VW_RayCastHit
	{
		int hit;

		int x;
		int y;
		int z;

		uint8_t material;

		float distance;

		float positionX;
		float positionY;
		float positionZ;

		int normalX;
		int normalY;
		int normalZ;
	} VW_RayCastHit;

	VW_API int VW_RayCast(
		VW_World* world,
		float originX,
		float originY,
		float originZ,
		float dirX,
		float dirY,
		float dirZ,
		float maxDistance,
		VW_RayCastHit* outHit);

	typedef struct VW_CollisionVoxel
	{
		int x;
		int y;
		int z;

		uint8_t material;

		// Bit mask of exposed faces:
		// bit 0 = +X
		// bit 1 = -X
		// bit 2 = +Y
		// bit 3 = -Y
		// bit 4 = +Z
		// bit 5 = -Z
		uint8_t exposedFaces;
	} VW_CollisionVoxel;

	VW_API int VW_CountCollisionVoxelsInChunk(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ);

	VW_API int VW_GetCollisionVoxelsInChunk(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ,
		VW_CollisionVoxel* outVoxels,
		int maxVoxels);

	// Return non-zero to continue visiting, 0 to stop.
	using VW_CollisionVoxelVisitor = int(*)(int x, int y, int z, uint8_t material, uint8_t exposedFaces, void* userData);
	using VW_CollisionVoxelRangeFilter = int(*)(int minX, int minY, int minZ, int maxX, int maxY, int maxZ, void* userData);
	using VW_CollisionVoxelRangeVisitor = int(*)(int minX, int minY, int minZ, int maxX, int maxY, int maxZ, void* userData);

	// Visits collision voxels in [min, max) voxel coordinates.
	VW_API int VW_VisitCollisionVoxelsInRange(
		VW_World* world,
		int minX,
		int minY,
		int minZ,
		int maxX,
		int maxY,
		int maxZ,
		VW_CollisionVoxelVisitor visitor,
		void* userData);

	// Visits collision voxels in [min, max) voxel coordinates and only emits voxels
	// with at least one bit from requiredExposedFaces set. Use 0 to accept any exposed face.
	VW_API int VW_VisitCollisionVoxelsInRangeWithExposedFaces(
		VW_World* world,
		int minX,
		int minY,
		int minZ,
		int maxX,
		int maxY,
		int maxZ,
		uint8_t requiredExposedFaces,
		VW_CollisionVoxelVisitor visitor,
		void* userData);

	// Visits collision voxels in [min, max) voxel coordinates, filtering whole chunk ranges
	// before individual voxels are emitted.
	VW_API int VW_VisitCollisionVoxelsInRangeWithExposedFacesAndRangeFilter(
		VW_World* world,
		int minX,
		int minY,
		int minZ,
		int maxX,
		int maxY,
		int maxZ,
		uint8_t requiredExposedFaces,
		VW_CollisionVoxelRangeFilter rangeFilter,
		void* rangeFilterUserData,
		VW_CollisionVoxelVisitor visitor,
		void* userData);

	// Visits occupied collision voxel ranges in [min, max). Ranges are contiguous
	// x-axis spans on a single y/z row.
	VW_API int VW_VisitCollisionVoxelRangesInRangeWithExposedFacesAndRangeFilter(
		VW_World* world,
		int minX,
		int minY,
		int minZ,
		int maxX,
		int maxY,
		int maxZ,
		uint8_t requiredExposedFaces,
		VW_CollisionVoxelRangeFilter rangeFilter,
		void* rangeFilterUserData,
		VW_CollisionVoxelRangeVisitor visitor,
		void* userData);

	// Visits collision voxels in chunks touched by a swept axis-aligned voxel-space box.
	VW_API int VW_VisitCollisionVoxelsForBoxCast(
		VW_World* world,
		float minX,
		float minY,
		float minZ,
		float maxX,
		float maxY,
		float maxZ,
		float dirX,
		float dirY,
		float dirZ,
		float maxFraction,
		VW_CollisionVoxelVisitor visitor,
		void* userData);

	typedef struct VW_PackedQuad
	{
		// Packed integer voxel-space data:
		//
		// bits  0..5   = x
		// bits  6..11  = y
		// bits 12..17  = z
		// bits 18..23  = width
		// bits 24..29  = height
		// bits 30..31  = axis
		uint32_t data0;

		// bits  0..7   = material
		// bits  8..9   = sign encoded: 0 = negative, 1 = positive
		// bits 10..31  = reserved
		uint32_t data1;
	} VW_PackedQuad;

	typedef struct VW_PackedQuadMesh
	{
		VW_PackedQuad* quads;
		uint32_t quadCount;

		int chunkX;
		int chunkY;
		int chunkZ;
		int chunkLinearIndex;
	} VW_PackedQuadMesh;

	VW_API int VW_BuildChunkPackedQuadsLODWithScratch(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ,
		int lod,
		VW_MeshBuildScratch* scratch,
		VW_PackedQuadMesh* outMesh);

	VW_API int VW_BuildChunkPackedQuadsWithScratch(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ,
		VW_MeshBuildScratch* scratch,
		VW_PackedQuadMesh* outMesh);

	VW_API int VW_BuildDirtyChunkPackedQuadsLODWithScratch(
		VW_World* world,
		int dirtyIndex,
		int lod,
		VW_MeshBuildScratch* scratch,
		VW_PackedQuadMesh* outMesh);

	VW_API int VW_BuildDirtyChunkPackedQuadsWithScratch(
		VW_World* world,
		int dirtyIndex,
		VW_MeshBuildScratch* scratch,
		VW_PackedQuadMesh* outMesh);

	VW_API void VW_FreePackedQuadMesh(
		VW_PackedQuadMesh* mesh);

#ifdef __cplusplus
}
#endif
