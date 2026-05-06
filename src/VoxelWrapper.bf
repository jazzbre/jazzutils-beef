using System;

namespace jazzutils
{
	namespace VoxelWrapper
	{
		[CRepr]
		public struct VW_Vec3
		{
			public float x;
			public float y;
			public float z;

			public this(float x, float y, float z)
			{
				this.x = x;
				this.y = y;
				this.z = z;
			}
		}

		[CRepr]
		public struct VW_ChunkCoord
		{
			public int32 x;
			public int32 y;
			public int32 z;

			public this(int32 x, int32 y, int32 z)
			{
				this.x = x;
				this.y = y;
				this.z = z;
			}
		}

		[CRepr]
		public struct VW_Vertex
		{
			public float x;
			public float y;
			public float z;

			public float nx;
			public float ny;
			public float nz;

			public float u;
			public float v;

			public uint32 material;
		}

		[CRepr]
		public struct VW_Mesh
		{
			public VW_Vertex* vertices;
			public uint32 vertexCount;

			public uint32* indices;
			public uint32 indexCount;

			public int32 chunkX;
			public int32 chunkY;
			public int32 chunkZ;
			public int32 chunkLinearIndex;
		}

		public struct VW_MeshBuildScratch;

		public static class VoxelNative
		{
			[CLink]
			public static extern void* VW_CreateWorld(
				int32 sizeX,
				int32 sizeY,
				int32 sizeZ,
				int32 chunkSize);

			[CLink]
			public static extern void VW_DestroyWorld(
				void* world);

			[CLink]
			public static extern int32 VW_SetData(
				void* world,
				uint8* voxels,
				int32 sizeX,
				int32 sizeY,
				int32 sizeZ);

			[CLink]
			public static extern int32 VW_GetSizeX(
				void* world);

			[CLink]
			public static extern int32 VW_GetSizeY(
				void* world);

			[CLink]
			public static extern int32 VW_GetSizeZ(
				void* world);

			[CLink]
			public static extern int32 VW_GetChunkSize(
				void* world);

			[CLink]
			public static extern int32 VW_GetChunkCountX(
				void* world);

			[CLink]
			public static extern int32 VW_GetChunkCountY(
				void* world);

			[CLink]
			public static extern int32 VW_GetChunkCountZ(
				void* world);

			[CLink]
			public static extern int32 VW_GetChunkLinearIndex(
				void* world,
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ);

			[CLink]
			public static extern int32 VW_GetVoxel(
				void* world,
				int32 x,
				int32 y,
				int32 z);

			[CLink]
			public static extern int32 VW_SetVoxel(
				void* world,
				int32 x,
				int32 y,
				int32 z,
				uint8 material);

			[CLink]
			public static extern int32 VW_DrillSphere(
				void* world,
				float cx,
				float cy,
				float cz,
				float radius,
				uint8 replacementMaterial);

			[CLink]
			public static extern int32 VW_DrillCapsule(
				void* world,
				VW_Vec3 a,
				VW_Vec3 b,
				float radius,
				uint8 replacementMaterial);

			[CLink]
			public static extern int32 VW_GetDirtyChunkCount(
				void* world);

			[CLink]
			public static extern int32 VW_GetDirtyChunkCoord(
				void* world,
				int32 dirtyIndex,
				VW_ChunkCoord* outCoord);

			[CLink]
			public static extern void VW_FreeMesh(
				VW_Mesh* mesh);

			[CLink]
			public static extern void VW_ClearDirtyChunks(
				void* world);

			[CLink]
			public static extern VW_MeshBuildScratch* VW_CreateMeshBuildScratch();

			[CLink]
			public static extern void VW_DestroyMeshBuildScratch(
				VW_MeshBuildScratch* scratch);

			[CLink]
			public static extern void VW_ClearMeshBuildScratch(
				VW_MeshBuildScratch* scratch);

			[CLink]
			public static extern int32 VW_BuildChunkMeshWithScratch(
				void* world,
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				VW_MeshBuildScratch* scratch,
				VW_Mesh* outMesh);

			[CLink]
			public static extern int32 VW_BuildDirtyChunkMeshWithScratch(
				void* world,
				int32 dirtyIndex,
				VW_MeshBuildScratch* scratch,
				VW_Mesh* outMesh);
		}

		public class VoxelMeshBuildScratch
		{
			public VW_MeshBuildScratch* Handle;

			public this()
			{
				Handle = VoxelNative.VW_CreateMeshBuildScratch();
			}

			public ~this()
			{
				if (Handle != null)
				{
					VoxelNative.VW_DestroyMeshBuildScratch(Handle);
					Handle = null;
				}
			}

			public void Clear()
			{
				VoxelNative.VW_ClearMeshBuildScratch(Handle);
			}
		}

		public class VoxelWorld
		{
			private void* mHandle;

			public bool IsValid
			{
				get
				{
					return mHandle != null;
				}
			}

			public void* Handle
			{
				get
				{
					return mHandle;
				}
			}

			public this(int32 sizeX, int32 sizeY, int32 sizeZ, int32 chunkSize = 32)
			{
				mHandle = VoxelNative.VW_CreateWorld(sizeX, sizeY, sizeZ, chunkSize);
			}

			public ~this()
			{
				Destroy();
			}

			public void Destroy()
			{
				if (mHandle != null)
				{
					VoxelNative.VW_DestroyWorld(mHandle);
					mHandle = null;
				}
			}

			public int32 SizeX
			{
				get
				{
					return VoxelNative.VW_GetSizeX(mHandle);
				}
			}

			public int32 SizeY
			{
				get
				{
					return VoxelNative.VW_GetSizeY(mHandle);
				}
			}

			public int32 SizeZ
			{
				get
				{
					return VoxelNative.VW_GetSizeZ(mHandle);
				}
			}

			public int32 ChunkSize
			{
				get
				{
					return VoxelNative.VW_GetChunkSize(mHandle);
				}
			}

			public int32 ChunkCountX
			{
				get
				{
					return VoxelNative.VW_GetChunkCountX(mHandle);
				}
			}

			public int32 ChunkCountY
			{
				get
				{
					return VoxelNative.VW_GetChunkCountY(mHandle);
				}
			}

			public int32 ChunkCountZ
			{
				get
				{
					return VoxelNative.VW_GetChunkCountZ(mHandle);
				}
			}

			public int32 DirtyChunkCount
			{
				get
				{
					return VoxelNative.VW_GetDirtyChunkCount(mHandle);
				}
			}

			public bool SetData(uint8* voxels, int32 sizeX, int32 sizeY, int32 sizeZ)
			{
				return VoxelNative.VW_SetData(mHandle, voxels, sizeX, sizeY, sizeZ) != 0;
			}

			public int32 GetVoxel(int32 x, int32 y, int32 z)
			{
				return VoxelNative.VW_GetVoxel(mHandle, x, y, z);
			}

			public bool SetVoxel(int32 x, int32 y, int32 z, uint8 material)
			{
				return VoxelNative.VW_SetVoxel(mHandle, x, y, z, material) != 0;
			}

			public int32 DrillSphere(float cx, float cy, float cz, float radius, uint8 replacementMaterial = 0)
			{
				return VoxelNative.VW_DrillSphere(mHandle, cx, cy, cz, radius, replacementMaterial);
			}

			public int32 DrillCapsule(VW_Vec3 a, VW_Vec3 b, float radius, uint8 replacementMaterial = 0)
			{
				return VoxelNative.VW_DrillCapsule(mHandle, a, b, radius, replacementMaterial);
			}

			public bool GetDirtyChunkCoord(int32 dirtyIndex, out VW_ChunkCoord coord)
			{
				coord = default;

				return VoxelNative.VW_GetDirtyChunkCoord(mHandle, dirtyIndex, &coord) != 0;
			}

			public int32 GetChunkLinearIndex(int32 chunkX, int32 chunkY, int32 chunkZ)
			{
				return VoxelNative.VW_GetChunkLinearIndex(mHandle, chunkX, chunkY, chunkZ);
			}

			public bool BuildDirtyChunkMeshWithScratch(int32 dirtyIndex, VoxelMeshBuildScratch scratch, out VW_Mesh mesh)
			{
				mesh = default;

				return VoxelNative.VW_BuildDirtyChunkMeshWithScratch(mHandle, dirtyIndex, scratch.Handle, &mesh) != 0;
			}

			public bool BuildChunkMeshWithScratch(int32 chunkX, int32 chunkY, int32 chunkZ, VoxelMeshBuildScratch scratch, out VW_Mesh mesh)
			{
				mesh = default;

				return VoxelNative.VW_BuildChunkMeshWithScratch(mHandle, chunkX, chunkY, chunkZ, scratch.Handle, &mesh) != 0;
			}

			public void FreeMesh(ref VW_Mesh mesh)
			{
				VoxelNative.VW_FreeMesh(&mesh);
			}

			public void ClearDirtyChunks()
			{
				VoxelNative.VW_ClearDirtyChunks(mHandle);
			}
		}
	}
}