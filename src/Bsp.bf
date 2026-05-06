using System;
using System.Collections;

namespace jazzutils
{
	public static class Bsp
	{
		public typealias BSP = void*;
		public typealias WingMesh = void*;
		public typealias Bsp_TraverseCallbackRange = function void(float* vertices, int32 count, void* callbackData);

		[CLink] public static extern WingMesh Bsp_CreateWingMesh(float* verts, int32* tris, int32 n);
		[CLink] public static extern WingMesh Bsp_CreateWingMeshBox(float* minBounds, float* maxBounds);
		[CLink] public static extern void Bsp_DestroyWingMesh(WingMesh m);
		[CLink] public static extern BSP Bsp_Create(WingMesh mesh, float* minBounds, float* maxBounds);
		[CLink] public static extern void Bsp_Destroy(BSP bsp);
		[CLink] public static extern void Bsp_Intersect(BSP bsp, float* p, float* r, float s, BSP cutter);
		[CLink] public static extern void Bsp_Union(BSP bsp, float* p, float* r, float s, BSP cutter);
		[CLink] public static extern void Bsp_Traverse(BSP bsp, float* camera_position, bool backToFront, Bsp_TraverseCallbackRange callback, void* callbackData);
		[CLink] public static extern bool Bsp_SphereTest(BSP bsp, float r, bool solid, float* v0, float* v1, float* impact, float* impactNormal);
		[CLink] public static extern bool Bsp_RayTest(BSP bsp, bool solid, float* v0, float* v1, float* impact, float* impactNormal);
	}
}