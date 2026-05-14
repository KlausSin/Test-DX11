#pragma once

#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

float CrossProduct2D(float x1, float y1, float x2, float y2);

bool IsInTriangle2D(float ax, float ay, float bx, float by, float cx, float cy, float tx, float ty);

XMFLOAT3* D3DXVec3Rotation(XMFLOAT3* pvtOut, const XMFLOAT3* c_pvtSrc, const XMFLOAT4* c_pqtRot);
XMFLOAT3* D3DXVec3Translation(XMFLOAT3* pvtOut, const XMFLOAT3* c_pvtSrc, const XMFLOAT3* c_pvtTrans);

void GetRotationFromMatrix(XMFLOAT3* pRotation, const XMFLOAT4X4* c_pMatrix);
void GetPivotAndRotationFromMatrix(XMFLOAT4X4* pMatrix, XMFLOAT3* pPivot, XMFLOAT3* pRotation);
void ExtractMovement(XMFLOAT4X4* pTargetMatrix, XMFLOAT4X4* pSourceMatrix);

inline XMFLOAT3* D3DXVec3Blend(XMFLOAT3* pvtOut, const XMFLOAT3* c_pvtSrc1, const XMFLOAT3* c_pvtSrc2, float d)
{
	pvtOut->x = c_pvtSrc1->x + d * (c_pvtSrc2->x - c_pvtSrc1->x);
	pvtOut->y = c_pvtSrc1->y + d * (c_pvtSrc2->y - c_pvtSrc1->y);
	pvtOut->z = c_pvtSrc1->z + d * (c_pvtSrc2->z - c_pvtSrc1->z);

	return pvtOut;
}

inline XMFLOAT4* D3DXQuaternionBlend(XMFLOAT4* pqtOut, const XMFLOAT4* c_pqtSrc1, const XMFLOAT4* c_pqtSrc2, float d)
{
	pqtOut->x = c_pqtSrc1->x + d * (c_pqtSrc2->x - c_pqtSrc1->x);
	pqtOut->y = c_pqtSrc1->y + d * (c_pqtSrc2->y - c_pqtSrc1->y);
	pqtOut->z = c_pqtSrc1->z + d * (c_pqtSrc2->z - c_pqtSrc1->z);
	pqtOut->w = c_pqtSrc1->w + d * (c_pqtSrc2->w - c_pqtSrc1->w);

	return pqtOut;
}

inline float ClampDegree(float fDegree)
{
	if (fDegree >= 360.0f)
		fDegree -= 360.0f;

	if (fDegree < 0.0f)
		fDegree += 360.0f;

	return fDegree;
}

inline float GetVector3Distance(const XMFLOAT3& c_rv3Source, const XMFLOAT3& c_rv3Target)
{
	return
		(c_rv3Source.x - c_rv3Target.x) * (c_rv3Source.x - c_rv3Target.x) +
		(c_rv3Source.y - c_rv3Target.y) * (c_rv3Source.y - c_rv3Target.y);
}

inline XMFLOAT4 SafeRotationNormalizedArc(const XMFLOAT3& vFrom, const XMFLOAT3& vTo)
{
	if (vFrom.x == vTo.x && vFrom.y == vTo.y && vFrom.z == vTo.z)
		return XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	if (vFrom.x == -vTo.x && vFrom.y == -vTo.y && vFrom.z == -vTo.z)
		return XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);

	XMVECTOR from = XMLoadFloat3(&vFrom);
	XMVECTOR to = XMLoadFloat3(&vTo);

	XMVECTOR c = XMVector3Cross(from, to);
	float d = XMVectorGetX(XMVector3Dot(from, to));
	float s = sqrtf((1.0f + d) * 2.0f);

	XMFLOAT3 cross;
	XMStoreFloat3(&cross, c);

	return XMFLOAT4(cross.x / s, cross.y / s, cross.z / s, s * 0.5f);
}

inline XMFLOAT4 RotationNormalizedArc(const XMFLOAT3& vFrom, const XMFLOAT3& vTo)
{
	XMVECTOR from = XMLoadFloat3(&vFrom);
	XMVECTOR to = XMLoadFloat3(&vTo);

	XMVECTOR c = XMVector3Cross(from, to);
	float d = XMVectorGetX(XMVector3Dot(from, to));
	float s = sqrtf((1.0f + d) * 2.0f);

	XMFLOAT3 cross;
	XMStoreFloat3(&cross, c);

	return XMFLOAT4(cross.x / s, cross.y / s, cross.z / s, s * 0.5f);
}

inline XMFLOAT4 RotationArc(const XMFLOAT3& vFrom, const XMFLOAT3& vTo)
{
	XMVECTOR from = XMVector3Normalize(XMLoadFloat3(&vFrom));
	XMVECTOR to = XMVector3Normalize(XMLoadFloat3(&vTo));

	XMFLOAT3 vnFrom, vnTo;
	XMStoreFloat3(&vnFrom, from);
	XMStoreFloat3(&vnTo, to);

	return RotationNormalizedArc(vnFrom, vnTo);
}

inline float square_distance_between_linesegment_and_point(const XMFLOAT3& p1, const XMFLOAT3& p2, const XMFLOAT3& x)
{
	XMVECTOR vp1 = XMLoadFloat3(&p1);
	XMVECTOR vp2 = XMLoadFloat3(&p2);
	XMVECTOR vx = XMLoadFloat3(&x);

	XMVECTOR v1 = vp2 - vp1;
	float l = XMVectorGetX(XMVector3LengthSq(v1));

	XMVECTOR v2 = vx - vp1;
	XMVECTOR v3 = vp2 - vp1;

	float d = XMVectorGetX(XMVector3Dot(v2, v3));

	if (d <= 0.0f)
	{
		return XMVectorGetX(XMVector3LengthSq(v2));
	}
	else if (d >= l)
	{
		XMVECTOR v4 = vx - vp2;
		return XMVectorGetX(XMVector3LengthSq(v4));
	}
	else
	{
		XMVECTOR c = XMVector3Cross(v2, v3);
		return XMVectorGetX(XMVector3LengthSq(c)) / l;
	}
}

inline XMFLOAT3* Vec3TransformQuaternionSafe(XMFLOAT3* pvout, const XMFLOAT3* pv, const XMFLOAT4* pq)
{
	XMVECTOR v = XMVector3Cross(XMLoadFloat3(pv), XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(pq)));
	v *= -2.0f * pq->w;

	XMVECTOR qvec = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(pq));
	XMVECTOR pvvec = XMLoadFloat3(pv);

	v += (pq->w * pq->w - XMVectorGetX(XMVector3LengthSq(qvec))) * pvvec;
	v += 2.0f * XMVectorGetX(XMVector3Dot(qvec, pvvec)) * qvec;

	XMStoreFloat3(pvout, v);

	return pvout;
}

inline XMFLOAT3* Vec3TransformQuaternion(XMFLOAT3* pvout, const XMFLOAT3* pv, const XMFLOAT4* pq)
{
	XMVECTOR qvec = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(pq));
	XMVECTOR pvvec = XMLoadFloat3(pv);

	XMVECTOR v = XMVector3Cross(pvvec, qvec);
	v *= -2.0f * pq->w;
	v += (pq->w * pq->w - XMVectorGetX(XMVector3LengthSq(qvec))) * pvvec;
	v += 2.0f * XMVectorGetX(XMVector3Dot(qvec, pvvec)) * qvec;

	XMStoreFloat3(pvout, v);

	return pvout;
}