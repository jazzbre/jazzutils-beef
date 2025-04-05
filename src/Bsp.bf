using System;
using System.Collections;

namespace jazzutils
{
	public static class Bsp
	{
		public typealias BSP = void*;
		public typealias WingMesh = void*;
		public typealias Bsp_TraverseCallbackRange = function void(float* vertices, int count, void* callbackData);

		[CLink] public static extern WingMesh* Bsp_CreateWingMesh(float* verts, int* tris, int n);
		[CLink] public static extern void Bsp_DestroyWingMesh(WingMesh* m);
		[CLink] public static extern BSP* Bsp_Create(WingMesh* mesh, float* minBounds, float* maxBounds);
		[CLink] public static extern void Bsp_Destroy(BSP* bsp);
		[CLink] public static extern void Bsp_Intersect(BSP* bsp, float* p, float* r, float s, BSP* cutter);
		[CLink] public static extern void Bsp_Union(BSP* bsp, float* p, float* r, float s, BSP* cutter);
		[CLink] public static extern void Bsp_Traverse(BSP* bsp, float* camera_position, bool backToFront, Bsp_TraverseCallbackRange callback);
	}
}