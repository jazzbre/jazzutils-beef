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

		[CRepr]
		public struct VW_RayCastHit
		{
			public int32 hit;

			public int32 x;
			public int32 y;
			public int32 z;

			public uint8 material;

			public float distance;

			public float positionX;
			public float positionY;
			public float positionZ;

			public int32 normalX;
			public int32 normalY;
			public int32 normalZ;
		}

		[CRepr]
		public struct VW_CollisionVoxel
		{
			public int32 x;
			public int32 y;
			public int32 z;

			public uint8 material;
			public uint8 exposedFaces;
		}

		[CRepr]
		public struct VW_CollisionBox
		{
			public int32 minX;
			public int32 minY;
			public int32 minZ;

			public int32 maxX;
			public int32 maxY;
			public int32 maxZ;

			public int32 normalX;
			public int32 normalY;
			public int32 normalZ;

			public uint8 material;
		}

		[CRepr]
		public struct VW_PackedQuad
		{
			public uint32 data0;
			public uint32 data1;

			public enum Axis : uint8
			{
				X = 0,
				Y = 1,
				Z = 2
			}

			public int32 X
			{
				get { return (int32)((data0 >> 0) & 63); }
			}

			public int32 Y
			{
				get { return (int32)((data0 >> 6) & 63); }
			}

			public int32 Z
			{
				get { return (int32)((data0 >> 12) & 63); }
			}

			public int32 Width
			{
				get { return (int32)((data0 >> 18) & 63); }
			}

			public int32 Height
			{
				get { return (int32)((data0 >> 24) & 63); }
			}

			public int32 AxisIndex
			{
				get { return (int32)((data0 >> 30) & 3); }
			}

			public Axis QuadAxis
			{
				get { return (Axis)((data0 >> 30) & 3); }
			}

			public uint8 Material
			{
				get { return (uint8)(data1 & 255); }
			}

			public int32 Sign
			{
				get
				{
					return (((data1 >> 8) & 1) != 0) ? 1 : -1;
				}
			}

			public bool IsPositive
			{
				get { return ((data1 >> 8) & 1) != 0; }
			}

			public bool IsNegative
			{
				get { return ((data1 >> 8) & 1) == 0; }
			}

			public void GetOrigin(out int32 x, out int32 y, out int32 z)
			{
				x = X;
				y = Y;
				z = Z;
			}

			public void GetSize(out int32 width, out int32 height)
			{
				width = Width;
				height = Height;
			}

			public void GetNormal(out int32 nx, out int32 ny, out int32 nz)
			{
				nx = 0;
				ny = 0;
				nz = 0;

				int32 sign = Sign;

				switch (AxisIndex)
				{
				case 0:
					nx = sign;
					break;

				case 1:
					ny = sign;
					break;

				case 2:
					nz = sign;
					break;

				default:
					break;
				}
			}

			public void GetUAxis(out int32 ux, out int32 uy, out int32 uz)
			{
				ux = 0;
				uy = 0;
				uz = 0;

				int32 uAxis = (AxisIndex + 1) % 3;

				switch (uAxis)
				{
				case 0:
					ux = 1;
					break;

				case 1:
					uy = 1;
					break;

				case 2:
					uz = 1;
					break;

				default:
					break;
				}
			}

			public void GetVAxis(out int32 vx, out int32 vy, out int32 vz)
			{
				vx = 0;
				vy = 0;
				vz = 0;

				int32 vAxis = (AxisIndex + 2) % 3;

				switch (vAxis)
				{
				case 0:
					vx = 1;
					break;

				case 1:
					vy = 1;
					break;

				case 2:
					vz = 1;
					break;

				default:
					break;
				}
			}

			public void GetCorner(
				int32 corner,
				out int32 x,
				out int32 y,
				out int32 z)
			{
				x = X;
				y = Y;
				z = Z;

				int32 axis = AxisIndex;
				int32 uAxis = (axis + 1) % 3;
				int32 vAxis = (axis + 2) % 3;

				bool addU = corner == 1 || corner == 2;
				bool addV = corner == 2 || corner == 3;

				if (addU)
				{
					switch (uAxis)
					{
					case 0:
						x += Width;
						break;
					case 1:
						y += Width;
						break;
					case 2:
						z += Width;
						break;
					default:
						break;
					}
				}

				if (addV)
				{
					switch (vAxis)
					{
					case 0:
						x += Height;
						break;
					case 1:
						y += Height;
						break;
					case 2:
						z += Height;
						break;
					default:
						break;
					}
				}
			}

			public void GetTriangleCorner(
				int32 triangleVertexIndex,
				out int32 x,
				out int32 y,
				out int32 z)
			{
				// Same winding as:
				// 0, 1, 2
				// 0, 2, 3
				int32 corner = 0;

				switch (triangleVertexIndex)
				{
				case 0:
					corner = 0;
					break;
				case 1:
					corner = 1;
					break;
				case 2:
					corner = 2;
					break;
				case 3:
					corner = 0;
					break;
				case 4:
					corner = 2;
					break;
				case 5:
					corner = 3;
					break;
				default:
					corner = 0;
					break;
				}

				GetCorner(corner, out x, out y, out z);
			}

			public void GetAabb(
				out int32 minX,
				out int32 minY,
				out int32 minZ,
				out int32 maxX,
				out int32 maxY,
				out int32 maxZ)
			{
				int32 x0;
				int32 y0;
				int32 z0;

				int32 x2;
				int32 y2;
				int32 z2;

				GetCorner(0, out x0, out y0, out z0);
				GetCorner(2, out x2, out y2, out z2);

				minX = Math.Min(x0, x2);
				minY = Math.Min(y0, y2);
				minZ = Math.Min(z0, z2);

				maxX = Math.Max(x0, x2);
				maxY = Math.Max(y0, y2);
				maxZ = Math.Max(z0, z2);
			}

			public bool ContainsVoxelLocal(int32 vx, int32 vy, int32 vz)
			{
				// This checks whether a voxel-space point lies on/inside the quad rectangle plane.
				// Useful for CPU-side collision tests against packed quads.
				int32 axis = AxisIndex;
				int32 sign = Sign;

				int32 px = X;
				int32 py = Y;
				int32 pz = Z;

				int32 uAxis = (axis + 1) % 3;
				int32 vAxis = (axis + 2) % 3;

				int32 pAxis = 0;
				int32 vAxisCoord = 0;

				switch (axis)
				{
				case 0:
					pAxis = px;
					vAxisCoord = vx;
					break;
				case 1:
					pAxis = py;
					vAxisCoord = vy;
					break;
				case 2:
					pAxis = pz;
					vAxisCoord = vz;
					break;
				default:
					return false;
				}

				// For collision with voxel centers, this may need adjustment depending on usage.
				if (vAxisCoord != pAxis && vAxisCoord != pAxis - sign)
					return false;

				int32 u0 = 0;
				int32 u = 0;

				switch (uAxis)
				{
				case 0:
					u0 = px;
					u = vx;
					break;
				case 1:
					u0 = py;
					u = vy;
					break;
				case 2:
					u0 = pz;
					u = vz;
					break;
				default:
					return false;
				}

				int32 vv0 = 0;
				int32 vv = 0;

				switch (vAxis)
				{
				case 0:
					vv0 = px;
					vv = vx;
					break;
				case 1:
					vv0 = py;
					vv = vy;
					break;
				case 2:
					vv0 = pz;
					vv = vz;
					break;
				default:
					return false;
				}

				return u >= u0 && u < u0 + Width &&
					vv >= vv0 && vv < vv0 + Height;
			}

			public bool RayIntersectsLocal(
				float ox,
				float oy,
				float oz,
				float dx,
				float dy,
				float dz,
				float maxDistance,
				out float hitDistance)
			{
				hitDistance = 0.0f;

				int32 axis = AxisIndex;

				float plane = 0.0f;
				float originAxis = 0.0f;
				float dirAxis = 0.0f;

				switch (axis)
				{
				case 0:
					plane = (float)X;
					originAxis = ox;
					dirAxis = dx;
					break;

				case 1:
					plane = (float)Y;
					originAxis = oy;
					dirAxis = dy;
					break;

				case 2:
					plane = (float)Z;
					originAxis = oz;
					dirAxis = dz;
					break;

				default:
					return false;
				}

				if (Math.Abs(dirAxis) <= 0.000001f)
					return false;

				float t = (plane - originAxis) / dirAxis;

				if (t < 0.0f || t > maxDistance)
					return false;

				float hx = ox + dx * t;
				float hy = oy + dy * t;
				float hz = oz + dz * t;

				int32 uAxis = (axis + 1) % 3;
				int32 vAxis = (axis + 2) % 3;

				float u = 0.0f;
				float v = 0.0f;
				float u0 = 0.0f;
				float v0 = 0.0f;

				switch (uAxis)
				{
				case 0:
					u = hx;
					u0 = (float)X;
					break;
				case 1:
					u = hy;
					u0 = (float)Y;
					break;
				case 2:
					u = hz;
					u0 = (float)Z;
					break;
				default:
					return false;
				}

				switch (vAxis)
				{
				case 0:
					v = hx;
					v0 = (float)X;
					break;
				case 1:
					v = hy;
					v0 = (float)Y;
					break;
				case 2:
					v = hz;
					v0 = (float)Z;
					break;
				default:
					return false;
				}

				if (u < u0 || u > u0 + (float)Width)
					return false;

				if (v < v0 || v > v0 + (float)Height)
					return false;

				hitDistance = t;
				return true;
			}

			public uint32 GetDrawVertexCount()
			{
				return 6;
			}

			public static uint32 GetMeshDrawVertexCount(uint32 quadCount)
			{
				return quadCount * 6;
			}

			public void GetCollisionBoxLocal(out VW_CollisionBox collisionBox)
			{
				collisionBox = default(VW_CollisionBox);

				int32 axis = AxisIndex;
				int32 sign = Sign;

				collisionBox.material = Material;
				GetNormal(out collisionBox.normalX, out collisionBox.normalY, out collisionBox.normalZ);

				int32 minX = X;
				int32 minY = Y;
				int32 minZ = Z;

				int32 maxX = X;
				int32 maxY = Y;
				int32 maxZ = Z;

				int32 uAxis = (axis + 1) % 3;
				int32 vAxis = (axis + 2) % 3;

				// Axis thickness: 1 voxel into the solid side.
				switch (axis)
				{
				case 0:
					if (sign > 0)
					{
						minX = X - 1;
						maxX = X;
					}
					else
					{
						minX = X;
						maxX = X + 1;
					}
					break;

				case 1:
					if (sign > 0)
					{
						minY = Y - 1;
						maxY = Y;
					}
					else
					{
						minY = Y;
						maxY = Y + 1;
					}
					break;

				case 2:
					if (sign > 0)
					{
						minZ = Z - 1;
						maxZ = Z;
					}
					else
					{
						minZ = Z;
						maxZ = Z + 1;
					}
					break;

				default:
					break;
				}

				// U extent.
				switch (uAxis)
				{
				case 0:
					minX = X;
					maxX = X + Width;
					break;

				case 1:
					minY = Y;
					maxY = Y + Width;
					break;

				case 2:
					minZ = Z;
					maxZ = Z + Width;
					break;

				default:
					break;
				}

				// V extent.
				switch (vAxis)
				{
				case 0:
					minX = X;
					maxX = X + Height;
					break;

				case 1:
					minY = Y;
					maxY = Y + Height;
					break;

				case 2:
					minZ = Z;
					maxZ = Z + Height;
					break;

				default:
					break;
				}

				collisionBox.minX = minX;
				collisionBox.minY = minY;
				collisionBox.minZ = minZ;

				collisionBox.maxX = maxX;
				collisionBox.maxY = maxY;
				collisionBox.maxZ = maxZ;
			}

			public void GetCollisionBoxWorld(
				int32 chunkBaseX,
				int32 chunkBaseY,
				int32 chunkBaseZ,
				out VW_CollisionBox collisionBox)
			{
				GetCollisionBoxLocal(out collisionBox);

				collisionBox.minX += chunkBaseX;
				collisionBox.maxX += chunkBaseX;

				collisionBox.minY += chunkBaseY;
				collisionBox.maxY += chunkBaseY;

				collisionBox.minZ += chunkBaseZ;
				collisionBox.maxZ += chunkBaseZ;
			}

			public override void ToString(String strBuffer)
			{
				int32 nx;
				int32 ny;
				int32 nz;

				GetNormal(out nx, out ny, out nz);

				strBuffer.AppendF(
					"PackedQuad(pos=({}, {}, {}), size=({}, {}), axis={}, sign={}, normal=({}, {}, {}), material={})",
					X,
					Y,
					Z,
					Width,
					Height,
					AxisIndex,
					Sign,
					nx,
					ny,
					nz,
					Material);
			}
		}

		[CRepr]
		public struct VW_PackedQuadMesh
		{
			public VW_PackedQuad* quads;
			public uint32 quadCount;

			public int32 chunkX;
			public int32 chunkY;
			public int32 chunkZ;
			public int32 chunkLinearIndex;
		}

		public struct VW_MeshBuildScratch;

		public typealias VW_DrillSphereCallback = function uint8(int32 x, int32 y, int32 z, uint8 material, void* userData);

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
			public static extern int32 VW_DrillSphereWithCallback(
				void* world,
				float cx,
				float cy,
				float cz,
				float radius,
				VW_DrillSphereCallback callback, void* userData);

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

			[CLink]
			public static extern int32 VW_BuildChunkMeshLODWithScratch(
				void* world,
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				int32 lod,
				VW_MeshBuildScratch* scratch,
				VW_Mesh* outMesh);

			[CLink]
			public static extern int32 VW_BuildDirtyChunkMeshLODWithScratch(
				void* world,
				int32 dirtyIndex,
				int32 lod,
				VW_MeshBuildScratch* scratch,
				VW_Mesh* outMesh);

			[CLink]
			public static extern int32 VW_RayCast(
				void* world,
				float originX,
				float originY,
				float originZ,
				float dirX,
				float dirY,
				float dirZ,
				float maxDistance,
				VW_RayCastHit* outHit);

			[CLink]
			public static extern int32 VW_CountCollisionVoxelsInChunk(
				void* world,
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ);

			[CLink]
			public static extern int32 VW_GetCollisionVoxelsInChunk(
				void* world,
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				VW_CollisionVoxel* outVoxels,
				int32 maxVoxels);

			[CLink]
			public static extern int32 VW_BuildChunkPackedQuadsWithScratch(
				void* world,
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				VW_MeshBuildScratch* scratch,
				VW_PackedQuadMesh* outMesh);

			[CLink]
			public static extern int32 VW_BuildChunkPackedQuadsLODWithScratch(
				void* world,
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				int32 lod,
				VW_MeshBuildScratch* scratch,
				VW_PackedQuadMesh* outMesh);

			[CLink]
			public static extern int32 VW_BuildDirtyChunkPackedQuadsWithScratch(
				void* world,
				int32 dirtyIndex,
				VW_MeshBuildScratch* scratch,
				VW_PackedQuadMesh* outMesh);

			[CLink]
			public static extern int32 VW_BuildDirtyChunkPackedQuadsLODWithScratch(
				void* world,
				int32 dirtyIndex,
				int32 lod,
				VW_MeshBuildScratch* scratch,
				VW_PackedQuadMesh* outMesh);

			[CLink]
			public static extern void VW_FreePackedQuadMesh(
				VW_PackedQuadMesh* mesh);
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

			public typealias DrillSphereCallback = delegate uint8(int32 x, int32 y, int32 z, uint8 material);

			private static uint8 DrillSphereCallbackFunction(int32 x, int32 y, int32 z, uint8 material, void* userData)
			{
				var callback = (DrillSphereCallback)Internal.UnsafeCastToObject(userData);
				return callback(x, y, z, material);
			}

			public int32 DrillSphereWithCallback(void* world, float cx, float cy, float cz, float radius, DrillSphereCallback callback)
			{
				return VoxelNative.VW_DrillSphereWithCallback(world, cx, cy, cz, radius, => DrillSphereCallbackFunction, Internal.UnsafeCastToPtr(callback));
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

			public bool BuildChunkMeshWithScratch(int32 chunkX, int32 chunkY, int32 chunkZ, VoxelMeshBuildScratch scratch, out VW_Mesh mesh)
			{
				mesh = default;

				return VoxelNative.VW_BuildChunkMeshWithScratch(mHandle, chunkX, chunkY, chunkZ, scratch.Handle, &mesh) != 0;
			}

			public bool BuildDirtyChunkMeshWithScratch(int32 dirtyIndex, VoxelMeshBuildScratch scratch, out VW_Mesh mesh)
			{
				mesh = default;

				return VoxelNative.VW_BuildDirtyChunkMeshWithScratch(mHandle, dirtyIndex, scratch.Handle, &mesh) != 0;
			}

			public bool BuildChunkMeshLODWithScratch(
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				int32 lod,
				VoxelMeshBuildScratch scratch,
				out VW_Mesh mesh)
			{
				mesh = default(VW_Mesh);

				return VoxelNative.VW_BuildChunkMeshLODWithScratch(
					mHandle,
					chunkX,
					chunkY,
					chunkZ,
					lod,
					scratch.Handle,
					&mesh) != 0;
			}

			public bool BuildDirtyChunkMeshLODWithScratch(int32 dirtyIndex, int32 lod, VoxelMeshBuildScratch scratch, out VW_Mesh mesh)
			{
				mesh = default;

				return VoxelNative.VW_BuildDirtyChunkMeshLODWithScratch(mHandle, dirtyIndex, lod, scratch.Handle, &mesh) != 0;
			}

			public void FreeMesh(ref VW_Mesh mesh)
			{
				VoxelNative.VW_FreeMesh(&mesh);
			}

			public void ClearDirtyChunks()
			{
				VoxelNative.VW_ClearDirtyChunks(mHandle);
			}

			public bool RayCast(
				float originX,
				float originY,
				float originZ,
				float dirX,
				float dirY,
				float dirZ,
				float maxDistance,
				out VW_RayCastHit hit)
			{
				hit = default(VW_RayCastHit);

				return VoxelNative.VW_RayCast(
					mHandle,
					originX,
					originY,
					originZ,
					dirX,
					dirY,
					dirZ,
					maxDistance,
					&hit) != 0;
			}

			public int32 CountCollisionVoxelsInChunk(int32 chunkX, int32 chunkY, int32 chunkZ)
			{
				return VoxelNative.VW_CountCollisionVoxelsInChunk(
					mHandle,
					chunkX,
					chunkY,
					chunkZ);
			}

			public int32 GetCollisionVoxelsInChunk(
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				VW_CollisionVoxel* outVoxels,
				int32 maxVoxels)
			{
				return VoxelNative.VW_GetCollisionVoxelsInChunk(
					mHandle,
					chunkX,
					chunkY,
					chunkZ,
					outVoxels,
					maxVoxels);
			}

			public bool BuildChunkPackedQuads(
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				VoxelMeshBuildScratch scratch,
				out VW_PackedQuadMesh mesh)
			{
				mesh = default(VW_PackedQuadMesh);

				if (scratch == null || scratch.Handle == null)
					return false;

				return VoxelNative.VW_BuildChunkPackedQuadsWithScratch(
					mHandle,
					chunkX,
					chunkY,
					chunkZ,
					scratch.Handle,
					&mesh) != 0;
			}

			public bool BuildChunkPackedQuadsLOD(
				int32 chunkX,
				int32 chunkY,
				int32 chunkZ,
				int32 lod,
				VoxelMeshBuildScratch scratch,
				out VW_PackedQuadMesh mesh)
			{
				mesh = default(VW_PackedQuadMesh);

				if (scratch == null || scratch.Handle == null)
					return false;

				return VoxelNative.VW_BuildChunkPackedQuadsLODWithScratch(
					mHandle,
					chunkX,
					chunkY,
					chunkZ,
					lod,
					scratch.Handle,
					&mesh) != 0;
			}

			public bool BuildDirtyChunkPackedQuads(
				int32 dirtyIndex,
				VoxelMeshBuildScratch scratch,
				out VW_PackedQuadMesh mesh)
			{
				mesh = default(VW_PackedQuadMesh);

				if (scratch == null || scratch.Handle == null)
					return false;

				return VoxelNative.VW_BuildDirtyChunkPackedQuadsWithScratch(
					mHandle,
					dirtyIndex,
					scratch.Handle,
					&mesh) != 0;
			}

			public bool BuildDirtyChunkPackedQuadsLOD(
				int32 dirtyIndex,
				int32 lod,
				VoxelMeshBuildScratch scratch,
				out VW_PackedQuadMesh mesh)
			{
				mesh = default(VW_PackedQuadMesh);

				if (scratch == null || scratch.Handle == null)
					return false;

				return VoxelNative.VW_BuildDirtyChunkPackedQuadsLODWithScratch(
					mHandle,
					dirtyIndex,
					lod,
					scratch.Handle,
					&mesh) != 0;
			}

			public void FreePackedQuadMesh(ref VW_PackedQuadMesh mesh)
			{
				VoxelNative.VW_FreePackedQuadMesh(&mesh);
			}
		}
	}
}