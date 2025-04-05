#include "../sandbox/testbsp/bsp.h"

#define EXPORTAPI extern "C"

namespace {
	struct BSP {
		std::unique_ptr<BSPNode> root;
	};

	using Bsp_TraverseCallback = void(*)(float3* vertices, int count);

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

EXPORTAPI void Bsp_DestroyWingMesh(WingMesh* m) {
	delete m;
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
	auto b = BSPDup(cutter->root.get());
	BSPScale(*b, s);
	BSPTranslate(*b, *p);
	BSPRotate(*b, *r);
	bsp->root = BSPIntersect(move(b), move(bsp->root));
}

EXPORTAPI void Bsp_Union(BSP* bsp, const float3* p, const float4* r, float s, BSP* cutter)
{
	auto b = BSPDup(cutter->root.get());
	BSPScale(*b, s);
	BSPTranslate(*b, *p);
	BSPRotate(*b, *r);
	bsp->root = BSPUnion(move(b), move(bsp->root));
}

EXPORTAPI void Bsp_Traverse(BSP* bsp, const float3* camera_position, bool backToFront, Bsp_TraverseCallback callback)
{
	if (backToFront)
	{
		for (auto n : treebacktofront(bsp->root.get(), *camera_position))
		{
			for (auto& f : n->brep)
			{
				callback(f.vertex.data(), f.vertex.size());
			}
		}
	}
	else {
		for (auto n : treetraverse(bsp->root.get()))
		{
			for (auto& f : n->brep)
			{
				callback(f.vertex.data(), f.vertex.size());
			}
		}
	}
}
