#include "StdAfx.h"
#include "GrpMath.h"

using namespace DirectX;

float CrossProduct2D(float x1, float y1, float x2, float y2)
{
	return x1 * y2 - y1 * x2;
}

bool IsInTriangle2D(float ax, float ay, float bx, float by, float cx, float cy, float tx, float ty)
{
	float c1 = CrossProduct2D(bx - ax, by - ay, tx - ax, ty - ay);
	float c2 = CrossProduct2D(cx - bx, cy - by, tx - bx, ty - by);
	float c3 = CrossProduct2D(ax - cx, ay - cy, tx - cx, ty - cy);

	if (c1 * c2 > 0.0f && c1 * c3 > 0.0f)
		return true;

	if (c1 * c2 * c3 == 0.0f)
	{
		if (tx < ax)
			if (tx < bx)
				if (tx < cx)
					return false;

		if (tx > ax)
			if (tx > bx)
				if (tx > cx)
					return false;

		if (ty < ay)
			if (ty < by)
				if (ty < cy)
					return false;

		if (ty > ay)
			if (ty > by)
				if (ty > cy)
					return false;

		return true;
	}

	return false;
}

XMFLOAT3* D3DXVec3Rotation(XMFLOAT3* pvtOut, const XMFLOAT3* c_pvtSrc, const XMFLOAT4* c_pqtRot)
{
	const XMVECTOR qtSrc = XMVectorSet(c_pvtSrc->x, c_pvtSrc->y, c_pvtSrc->z, 0.0f);
	const XMVECTOR qtRot = XMLoadFloat4(c_pqtRot);

	XMVECTOR qtRet = XMQuaternionConjugate(qtRot);
	qtRet = XMQuaternionMultiply(qtSrc, qtRet);
	qtRet = XMQuaternionMultiply(qtRot, qtRet);

	pvtOut->x = XMVectorGetX(qtRet);
	pvtOut->y = XMVectorGetY(qtRet);
	pvtOut->z = XMVectorGetZ(qtRet);

	return pvtOut;
}

XMFLOAT3* D3DXVec3Translation(XMFLOAT3* pvtOut, const XMFLOAT3* c_pvtSrc, const XMFLOAT3* c_pvtTrans)
{
	pvtOut->x = c_pvtSrc->x + c_pvtTrans->x;
	pvtOut->y = c_pvtSrc->y + c_pvtTrans->y;
	pvtOut->z = c_pvtSrc->z + c_pvtTrans->z;

	return pvtOut;
}

void GetRotationFromMatrix(XMFLOAT3* pRotation, const XMFLOAT4X4* c_pMatrix)
{
	float sx = c_pMatrix->_32;
	float cx = sqrtf(1.0f - sx * sx);

	if (cx < 0.00001f)
	{
		if (sx > 0.0f)
			pRotation->x = XM_PI / 2.0f;
		else
			pRotation->x = -XM_PI / 2.0f;

		pRotation->y = atan2f(c_pMatrix->_31, c_pMatrix->_11);
		pRotation->z = 0.0f;
	}
	else
	{
		pRotation->x = atan2f(sx, cx);
		pRotation->y = atan2f(-c_pMatrix->_31, c_pMatrix->_33);
		pRotation->z = atan2f(-c_pMatrix->_12, c_pMatrix->_22);
	}
}

void GetPivotAndRotationFromMatrix(XMFLOAT4X4* pMatrix, XMFLOAT3* pPivot, XMFLOAT3* pRotation)
{
	float sx = pMatrix->_32;
	float cx = sqrtf(1.0f - sx * sx);
	float x, y, z;

	if (cx < 0.00001f)
	{
		if (sx > 0.0f)
			x = XM_PI / 2.0f;
		else
			x = -XM_PI / 2.0f;

		y = atan2f(pMatrix->_31, pMatrix->_11);
		z = 0.0f;
	}
	else
	{
		x = atan2f(sx, cx);
		y = atan2f(-pMatrix->_31, pMatrix->_33);
		z = atan2f(-pMatrix->_12, pMatrix->_22);
	}

	pRotation->x = x;
	pRotation->y = y;
	pRotation->z = z;

	pPivot->x = pMatrix->_41;
	pPivot->y = pMatrix->_42;
	pPivot->z = pMatrix->_43;
}

void ExtractMovement(XMFLOAT4X4* pTargetMatrix, XMFLOAT4X4* pSourceMatrix)
{
	XMFLOAT3 v3Pivot;
	XMFLOAT3 v3Rotation;

	GetPivotAndRotationFromMatrix(pSourceMatrix, &v3Pivot, &v3Rotation);

	const XMMATRIX matRotationX = XMMatrixRotationX(v3Rotation.x);
	const XMMATRIX matRotationY = XMMatrixRotationY(v3Rotation.y);
	const XMMATRIX matRotationZ = XMMatrixRotationZ(v3Rotation.z);
	const XMMATRIX matTranslation = XMMatrixTranslation(v3Pivot.x, v3Pivot.y, v3Pivot.z);

	XMStoreFloat4x4(pTargetMatrix, matRotationX * matRotationY * matRotationZ * matTranslation);
}