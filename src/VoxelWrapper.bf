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

		public static class VoxelNative
		{
			// Change this to "VoxelWrapper" if you link/import without the .dll suffix.
			public const StringView DllName = "VoxelWrapper.dll";

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
			public static extern int32 VW_BuildDirtyChunkMesh(
				void* world,
				int32 dirtyIndex,
				VW_Mesh* outMesh);

			[CLink]
			public static extern int32 VW_BuildChunkMesh(
				void* world,
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				VW_Mesh* outMesh);

			[CLink]
			public static extern void VW_FreeMesh(
				VW_Mesh* mesh);

			[CLink]
			public static extern void VW_ClearDirtyChunks(
				void* world);
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

			public bool BuildDirtyChunkMesh(int32 dirtyIndex, out VW_Mesh mesh)
			{
				mesh = default;

				return VoxelNative.VW_BuildDirtyChunkMesh(mHandle, dirtyIndex, &mesh) != 0;
			}

			public bool BuildChunkMesh(int32 chunkX, int32 chunkY, int32 chunkZ, out VW_Mesh mesh)
			{
				mesh = default;

				return VoxelNative.VW_BuildChunkMesh(mHandle, chunkX, chunkY, chunkZ, &mesh) != 0;
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

		public static class VoxelExample
		{
			public static void ExampleUsage()
			{
				int32 sizeX = 128;
				int32 sizeY = 128;
				int32 sizeZ = 128;
				int32 chunkSize = 32;

				VoxelWorld world = scope VoxelWorld(sizeX, sizeY, sizeZ, chunkSize);

				int32 voxelCount = sizeX * sizeY * sizeZ;
				uint8[] voxels = scope uint8[voxelCount];

				for (int32 i = 0; i < voxelCount; i++)
					voxels[i] = 1;

				uint8* voxelPtr = voxels.Ptr;
				world.SetData(voxelPtr, sizeX, sizeY, sizeZ);

				world.DrillSphere(64.0f, 64.0f, 64.0f, 8.0f, 0);

				int32 dirtyCount = world.DirtyChunkCount;

				for (int32 dirtyIndex = 0; dirtyIndex < dirtyCount; dirtyIndex++)
				{
					VW_ChunkCoord coord;

					if (!world.GetDirtyChunkCoord(dirtyIndex, out coord))
						continue;

					int32 renderChunkIndex = world.GetChunkLinearIndex(coord.x, coord.y, coord.z);

					VW_Mesh mesh;

					if (world.BuildChunkMesh(coord.x, coord.y, coord.z, out mesh))
					{
						// Replace this chunk in your renderer:
						//
						// renderChunks[renderChunkIndex].Upload(
						//     mesh.vertices,
						//     mesh.vertexCount,
						//     mesh.indices,
						//     mesh.indexCount);
						//
						// If mesh.vertexCount == 0, the chunk became empty.
						// In that case, delete/clear the render chunk mesh.

						world.FreeMesh(ref mesh);
					}
				}

				world.ClearDirtyChunks();
			}
		}
	}
}