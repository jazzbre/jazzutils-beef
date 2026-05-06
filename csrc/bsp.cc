#include "../sandbox/testbsp/bsp.h"

#define EXPORTAPI extern "C"

namespace {
	struct BSP {
		std::unique_ptr<BSPNode> root;
	};

	using Bsp_TraverseCallback = void(*)(float3* vertices, int count, void* callbackData);

	struct treefronttoback
	{
		BSPNode* root;
		float3 p;
		treefronttoback(BSPNode* root, const float3& p) :root(root), p(p) {}
		struct iterator
		{
			std::vector<BSPNode*> stack;
			float3 p;
			void toleaf(BSPNode* n) { while (n) { stack.push_back(n); n = (dot(float4(p, 1), n->plane()) < 0) ? n->under.get() : n->over.get(); } }
			BSPNode* operator *() const { return stack.size() ? stack.back() : NULL; }
			iterator& operator++() { assert(stack.size()); BSPNode* n = **this; stack.pop_back(); assert(n); toleaf((dot(float4(p, 1), n->plane()) < 0) ? n->over.get() : n->under.get());  return *this; }
			bool operator !=(const iterator& b) { return stack != b.stack; }
		};
		iterator begin() { iterator b; b.p = p;  b.toleaf(root); return b; }
		iterator end() { iterator e; e.p = p;  return e; }
	};

	std::vector<Face> WingMeshToFaces(const WingMesh& m)
	{
		std::vector<Face> faces;
		assert(m.unpacked == 0);
		assert(m.fback.size() == m.faces.size());
		int k = 0;
		for (unsigned int i = 0; i < m.faces.size(); i++)
		{
			Face face;
			face.plane() = m.faces[i];
			face.vertex = m.GenerateFaceVerts(i);
			faces.push_back(face);
		}
		return faces;
	}
}

EXPORTAPI WingMesh* Bsp_CreateWingMesh(const float3* verts, const int3* tris, int n) {
	WingMesh* pm = new WingMesh();
	WingMesh& m = *pm;
	if (n == 0) return pm;
	int verts_count = -1;
	for (int i = 0; i < n; i++) for (int j = 0; j < 3; j++)
	{
		verts_count = std::max(verts_count, tris[i][j]);
	}
	verts_count++;
	m.verts.resize(verts_count);
	for (unsigned int i = 0; i < m.verts.size(); i++)
	{
		m.verts[i] = verts[i];
	}
	for (int i = 0; i < n; i++)
	{
		const int3& t = tris[i];
		WingMesh::HalfEdge e0, e1, e2;
		e0.face = e1.face = e2.face = i;
		int k = m.edges.size();
		e0.id = e1.prev = e2.next = k + 0;
		e1.id = e2.prev = e0.next = k + 1;
		e2.id = e0.prev = e1.next = k + 2;
		e0.v = tris[i][0];
		e1.v = tris[i][1];
		e2.v = tris[i][2];
		m.edges.push_back(e0);
		m.edges.push_back(e1);
		m.edges.push_back(e2);
	}
	m.faces.resize(n);
	for (int i = 0; i < n; i++)
	{
		float3 normal = TriNormal(verts[tris[i][0]], verts[tris[i][1]], verts[tris[i][2]]);
		float  dist = -dot(normal, verts[tris[i][0]]);
		m.faces[i] = float4(normal, dist);
	}
	m.LinkMesh();
	m.InitBackLists();
	return pm;
}

EXPORTAPI WingMesh* Bsp_CreateWingMeshBox(float3* minBounds, float3* maxBounds) {
	float3& bmin = *minBounds;
	float3& bmax = *maxBounds;
	WingMesh* pm = new WingMesh();
	WingMesh& wm = *pm;
	wm.verts = { { bmin.x, bmin.y, bmin.z },{ bmin.x, bmin.y, bmax.z },{ bmin.x, bmax.y, bmin.z },{ bmin.x, bmax.y, bmax.z },
				 { bmax.x, bmin.y, bmin.z },{ bmax.x, bmin.y, bmax.z },{ bmax.x, bmax.y, bmin.z },{ bmax.x, bmax.y, bmax.z }, };
	wm.faces = { {-1,0,0,  bmin.x},{ 1,0,0, -bmax.x},{0,-1,0,  bmin.y},{0, 1,0, -bmax.y},{0,0,-1,  bmin.z},{0,0, 1, -bmax.z}, };
	wm.edges =
	{
		{ 0,0,11, 1, 3,0},{ 1,1,23, 2, 0,0},{ 2,3,15, 3, 1,0},{ 3,2,16, 0, 2,0},
		{ 4,6,13, 5, 7,1},{ 5,7,21, 6, 4,1},{ 6,5, 9, 7, 5,1},{ 7,4,18, 4, 6,1},
		{ 8,0,19, 9,11,2},{ 9,4, 6,10, 8,2},{10,5,20,11, 9,2},{11,1, 0, 8,10,2},
		{12,3,22,13,15,3},{13,7, 4,14,12,3},{14,6,17,15,13,3},{15,2, 2,12,14,3},
		{16,0, 3,17,19,4},{17,2,14,18,16,4},{18,6, 7,19,17,4},{19,4, 8,16,18,4},
		{20,1,10,21,23,5},{21,5, 5,22,20,5},{22,7,12,23,21,5},{23,3, 1,20,22,5},
	};
	wm.InitBackLists();
	wm.SanityCheck();
	return pm;
}

EXPORTAPI void Bsp_DestroyWingMesh(WingMesh* m) {
	if (m)
	{
		delete m;
	}
}

EXPORTAPI BSP* Bsp_Create(WingMesh* mesh, float3* minBounds, float3* maxBounds)
{
	BSP* bsp = new BSP();
	bsp->root = BSPCompile(WingMeshToFaces(*mesh), WingMeshBox(*minBounds, *maxBounds));
	BSPMakeBrep(bsp->root.get(), {});
	return bsp;
}

EXPORTAPI void Bsp_Destroy(BSP* bsp)
{
	if (bsp) {
		delete bsp;
	}
}

EXPORTAPI void Bsp_Intersect(BSP* bsp, const float3* p, const float4* r, float s, BSP* cutter)
{
	if (bsp == nullptr || bsp->root.get() == nullptr)
	{
		return;
	}
	if (cutter == nullptr || cutter->root.get() == nullptr)
	{
		return;
	}
	auto b = BSPDup(cutter->root.get());
	BSPRotate(*b, *r);
	BSPScale(*b, s);
	BSPTranslate(*b, *p);
	bsp->root = BSPIntersect(move(b), move(bsp->root));
}

EXPORTAPI void Bsp_Union(BSP* bsp, const float3* p, const float4* r, float s, BSP* cutter)
{
	if (bsp == nullptr || bsp->root.get() == nullptr)
	{
		return;
	}
	if (cutter == nullptr || cutter->root.get() == nullptr)
	{
		return;
	}
	auto b = BSPDup(cutter->root.get());
	BSPRotate(*b, *r);
	BSPScale(*b, s);
	BSPTranslate(*b, *p);
	bsp->root = BSPUnion(move(b), move(bsp->root));
}

EXPORTAPI void Bsp_Traverse(BSP* bsp, const float3* camera_position, bool backToFront, Bsp_TraverseCallback callback, void* callbackData)
{
	if (bsp == nullptr || bsp->root.get() == nullptr)
	{
		return;
	}
	if (backToFront)
	{
		for (auto n : treebacktofront(bsp->root.get(), *camera_position))
		{
			for (auto& f : n->brep)
			{
				callback(f.vertex.data(), f.vertex.size(), callbackData);
			}
		}
	}
	else {
		for (auto n : treefronttoback(bsp->root.get(), *camera_position))
		{
			for (auto& f : n->brep)
			{
				callback(f.vertex.data(), f.vertex.size(), callbackData);
			}
		}
	}
}

extern float3   HitCheckImpactNormal;;

EXPORTAPI bool Bsp_SphereTest(BSP* bsp, float r, bool solid, float3* v0, float3* v1, float3* impact, float3* impactNormal)
{
	if (bsp == nullptr || bsp->root.get() == nullptr)
	{
		return false;
	}
	int ret = HitCheckSphere(r, bsp->root.get(), solid, *v0, *v1, impact, {});
	if (ret != 0)
	{
		*impactNormal = HitCheckImpactNormal;
		return true;
	}
	return false;
}

EXPORTAPI bool Bsp_RayTest(BSP* bsp, bool solid, float3* v0, float3* v1, float3* impact, float3* impactNormal)
{
	if (bsp == nullptr || bsp->root.get() == nullptr)
	{
		return false;
	}
	int ret = HitCheck(bsp->root.get(), solid, *v0, *v1, impact);
	if (ret != 0)
	{
		*impactNormal = HitCheckImpactNormal;
		return true;
	}
	return false;
}
