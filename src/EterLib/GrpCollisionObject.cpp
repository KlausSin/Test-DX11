#include "StdAfx.h"
#include "GrpCollisionObject.h"

using namespace DirectX;

bool CGraphicCollisionObject::IntersectBoundBox(const XMFLOAT4X4* c_pmatWorld, const TBoundBox& c_rboundBox, float* pu, float* pv, float* pt)
{
	return IntersectCube(c_pmatWorld, c_rboundBox.sx, c_rboundBox.sy, c_rboundBox.sz, c_rboundBox.ex, c_rboundBox.ey, c_rboundBox.ez, ms_vtPickRayOrig, ms_vtPickRayDir, pu, pv, pt);
}

bool CGraphicCollisionObject::IntersectCube(const XMFLOAT4X4* c_pmatWorld, float sx, float sy, float sz, float ex, float ey, float ez,
	XMFLOAT3& RayOriginal, XMFLOAT3& RayDirection, float* pu, float* pv, float* pt)
{
	TPosition posVertices[8];

	posVertices[0] = TPosition(sx, sy, sz);
	posVertices[1] = TPosition(ex, sy, sz);
	posVertices[2] = TPosition(sx, ey, sz);
	posVertices[3] = TPosition(ex, ey, sz);
	posVertices[4] = TPosition(sx, sy, ez);
	posVertices[5] = TPosition(ex, sy, ez);
	posVertices[6] = TPosition(sx, ey, ez);
	posVertices[7] = TPosition(ex, ey, ez);

	static const WORD c_awFillCubeIndices[36] = {
		0, 1, 2, 1, 3, 2,
		2, 0, 6, 0, 4, 6,
		0, 1, 4, 1, 5, 4,
		1, 3, 5, 3, 7, 5,
		3, 2, 7, 2, 6, 7,
		4, 5, 6, 5, 7, 6,
	};

	return IntersectIndexedMesh(c_pmatWorld, posVertices, sizeof(TPosition), 8, c_awFillCubeIndices, 36, RayOriginal, RayDirection, pu, pv, pt);
}

const int c_iLimitVertexCount = 1024;

bool CGraphicCollisionObject::IntersectIndexedMesh(const XMFLOAT4X4* c_pmatWorld, const void* vertices, int step, int vtxCount, const void* indices, int idxCount,
	XMFLOAT3& RayOriginal, XMFLOAT3& RayDirection, float* pu, float* pv, float* pt)
{
	static XMFLOAT3 s_v3PositionArray[c_iLimitVertexCount];
	static DWORD s_dwPositionCount;

	if (vtxCount > c_iLimitVertexCount)
	{
		Tracef("The vertex count of mesh which is worked collision detection is too much : %d / %d", vtxCount, c_iLimitVertexCount);
		return false;
	}

	s_dwPositionCount = 0;

	const XMMATRIX matWorld = XMLoadFloat4x4(c_pmatWorld);
	char* pcurVtx = (char*)vertices;

	while (vtxCount--)
	{
		float* pos = (float*)pcurVtx;
		XMStoreFloat3(&s_v3PositionArray[s_dwPositionCount++], XMVector3TransformCoord(XMLoadFloat3((XMFLOAT3*)pos), matWorld));
		pcurVtx += step;
	}

	WORD* pcurIdx = (WORD*)indices;

	int triCount = idxCount / 3;
	while (triCount--)
	{
		if (IntersectTriangle(RayOriginal, RayDirection, s_v3PositionArray[pcurIdx[0]], s_v3PositionArray[pcurIdx[1]], s_v3PositionArray[pcurIdx[2]], pu, pv, pt))
			return true;

		pcurIdx += 3;
	}

	return false;
}

bool CGraphicCollisionObject::IntersectMesh(const XMFLOAT4X4* c_pmatWorld, const void* vertices, DWORD dwStep, DWORD dwvtxCount, XMFLOAT3& RayOriginal, XMFLOAT3& RayDirection, float* pu, float* pv, float* pt)
{
	char* pcurVtx = (char*)vertices;

	XMFLOAT3 v3Vertex[3];
	const XMMATRIX matWorld = XMLoadFloat4x4(c_pmatWorld);

	for (DWORD i = 0; i < dwvtxCount; i += 3)
	{
		XMStoreFloat3(&v3Vertex[0], XMVector3TransformCoord(XMLoadFloat3((XMFLOAT3*)pcurVtx), matWorld));
		pcurVtx += dwStep;

		XMStoreFloat3(&v3Vertex[1], XMVector3TransformCoord(XMLoadFloat3((XMFLOAT3*)pcurVtx), matWorld));
		pcurVtx += dwStep;

		XMStoreFloat3(&v3Vertex[2], XMVector3TransformCoord(XMLoadFloat3((XMFLOAT3*)pcurVtx), matWorld));
		pcurVtx += dwStep;

		if (IntersectTriangle(RayOriginal, RayDirection, v3Vertex[0], v3Vertex[1], v3Vertex[2], pu, pv, pt))
			return true;
	}

	return false;
}

bool CGraphicCollisionObject::IntersectTriangle(const XMFLOAT3& c_orig,
	const XMFLOAT3& c_dir,
	const XMFLOAT3& c_v0,
	const XMFLOAT3& c_v1,
	const XMFLOAT3& c_v2,
	float* pu,
	float* pv,
	float* pt)
{
	const XMVECTOR orig = XMLoadFloat3(&c_orig);
	const XMVECTOR dir = XMLoadFloat3(&c_dir);
	const XMVECTOR v0 = XMLoadFloat3(&c_v0);
	const XMVECTOR v1 = XMLoadFloat3(&c_v1);
	const XMVECTOR v2 = XMLoadFloat3(&c_v2);

	const XMVECTOR edge1 = v1 - v0;
	const XMVECTOR edge2 = v2 - v0;
	const XMVECTOR pvec = XMVector3Cross(dir, edge2);

	float det = XMVectorGetX(XMVector3Dot(edge1, pvec));
	XMVECTOR tvec;

	if (det > 0.0f)
		tvec = orig - v0;
	else
	{
		tvec = v0 - orig;
		det = -det;
	}

	if (det < 0.0001f)
		return false;

	float u = XMVectorGetX(XMVector3Dot(tvec, pvec));
	if (u < 0.0f || u > det)
		return false;

	const XMVECTOR qvec = XMVector3Cross(tvec, edge1);

	float v = XMVectorGetX(XMVector3Dot(dir, qvec));
	if (v < 0.0f || u + v > det)
		return false;

	float t = XMVectorGetX(XMVector3Dot(edge2, qvec));
	float fInvDet = 1.0f / det;

	t *= fInvDet;
	u *= fInvDet;
	v *= fInvDet;

	XMFLOAT3 spot;
	XMStoreFloat3(&spot, edge1 * u + edge2 * v + v0);

	*pu = spot.x;
	*pv = spot.y;
	*pt = t;

	return true;
}

bool CGraphicCollisionObject::IntersectSphere(const XMFLOAT3& c_rv3Position, float fRadius, const XMFLOAT3& c_rv3RayOriginal, const XMFLOAT3& c_rv3RayDirection)
{
	const XMVECTOR v3RayOriginal = XMLoadFloat3(&c_rv3RayOriginal) - XMLoadFloat3(&c_rv3Position);
	const XMVECTOR rayDirection = XMLoadFloat3(&c_rv3RayDirection);

	float a = XMVectorGetX(XMVector3Dot(rayDirection, rayDirection));
	float b = 2.0f * XMVectorGetX(XMVector3Dot(v3RayOriginal, rayDirection));
	float c = XMVectorGetX(XMVector3Dot(v3RayOriginal, v3RayOriginal)) - fRadius * fRadius;

	float D = b * b - 4.0f * a * c;

	return D >= 0.0f;
}

bool CGraphicCollisionObject::IntersectCylinder(const XMFLOAT3& c_rv3Position, float fRadius, float fHeight, const XMFLOAT3& c_rv3RayOriginal, const XMFLOAT3& c_rv3RayDirection)
{
	XMFLOAT3 v3RayOriginal;
	XMStoreFloat3(&v3RayOriginal, XMLoadFloat3(&c_rv3RayOriginal) - XMLoadFloat3(&c_rv3Position));

	float a = c_rv3RayDirection.x * c_rv3RayDirection.x + c_rv3RayDirection.y * c_rv3RayDirection.y;
	float b = 2.0f * (v3RayOriginal.x * c_rv3RayDirection.x + v3RayOriginal.y * c_rv3RayDirection.y);
	float c = v3RayOriginal.x * v3RayOriginal.x + v3RayOriginal.y * v3RayOriginal.y - fRadius * fRadius;

	float D = b * b - 4.0f * a * c;
	if (D > 0.0f)
		if (0.0f != a)
		{
			float tPlus = (-b + sqrtf(D)) / (2.0f * a);
			float tMinus = (-b - sqrtf(D)) / (2.0f * a);
			float fzPlus = v3RayOriginal.z + tPlus * c_rv3RayDirection.z;
			float fzMinus = v3RayOriginal.z + tMinus * c_rv3RayDirection.z;

			if (fzPlus > 0.0f && fzPlus <= fHeight)
				return true;
			if (fzMinus > 0.0f && fzMinus <= fHeight)
				return true;
			if (fzMinus * fzPlus < 0.0f)
				return true;
		}

	return false;
}

bool CGraphicCollisionObject::IntersectSphere(const XMFLOAT3& c_rv3Position, float fRadius)
{
	return CGraphicCollisionObject::IntersectSphere(c_rv3Position, fRadius, ms_vtPickRayOrig, ms_vtPickRayDir);
}

bool CGraphicCollisionObject::IntersectCylinder(const XMFLOAT3& c_rv3Position, float fRadius, float fHeight)
{
	return CGraphicCollisionObject::IntersectCylinder(c_rv3Position, fRadius, fHeight, ms_vtPickRayOrig, ms_vtPickRayDir);
}

CGraphicCollisionObject::CGraphicCollisionObject()
{
}

CGraphicCollisionObject::~CGraphicCollisionObject()
{
}