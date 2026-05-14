#pragma once

#include "GrpBase.h"

class CGraphicCollisionObject : public CGraphicBase
{
public:
	CGraphicCollisionObject();
	virtual ~CGraphicCollisionObject();

protected:
	bool IntersectTriangle(const XMFLOAT3& c_orig, const XMFLOAT3& c_dir, const XMFLOAT3& c_v0, const XMFLOAT3& c_v1, const XMFLOAT3& c_v2, float* pu, float* pv, float* pt);
	bool IntersectBoundBox(const XMFLOAT4X4* c_pmatWorld, const TBoundBox& c_rboundBox, float* pu, float* pv, float* pt);
	bool IntersectCube(const XMFLOAT4X4* c_pmatWorld, float sx, float sy, float sz, float ex, float ey, float ez, XMFLOAT3& RayOriginal, XMFLOAT3& RayDirection, float* pu, float* pv, float* pt);
	bool IntersectIndexedMesh(const XMFLOAT4X4* c_pmatWorld, const void* vertices, int step, int vtxCount, const void* indices, int idxCount, XMFLOAT3& RayOriginal, XMFLOAT3& RayDirection, float* pu, float* pv, float* pt);
	bool IntersectMesh(const XMFLOAT4X4* c_pmatWorld, const void* vertices, DWORD dwStep, DWORD dwvtxCount, XMFLOAT3& RayOriginal, XMFLOAT3& RayDirection, float* pu, float* pv, float* pt);

	bool IntersectSphere(const XMFLOAT3& c_rv3Position, float fRadius, const XMFLOAT3& c_rv3RayOriginal, const XMFLOAT3& c_rv3RayDirection);
	bool IntersectCylinder(const XMFLOAT3& c_rv3Position, float fRadius, float fHeight, const XMFLOAT3& c_rv3RayOriginal, const XMFLOAT3& c_rv3RayDirection);

	bool IntersectSphere(const XMFLOAT3& c_rv3Position, float fRadius);
	bool IntersectCylinder(const XMFLOAT3& c_rv3Position, float fRadius, float fHeight);
};