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

	VW_API int VW_BuildDirtyChunkMesh(
		VW_World* world,
		int dirtyIndex,
		VW_Mesh* outMesh);

	VW_API int VW_BuildChunkMesh(
		VW_World* world,
		int chunkX,
		int chunkY,
		int chunkZ,
		VW_Mesh* outMesh);

	VW_API void VW_FreeMesh(
		VW_Mesh* mesh);

	VW_API void VW_ClearDirtyChunks(
		VW_World* world);

#ifdef __cplusplus
}
#endif