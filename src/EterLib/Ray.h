#pragma once

#include "DirectXMath.h"
using namespace DirectX;

class CRay
{
public:
	CRay(const XMFLOAT3& v3Start, const XMFLOAT3& v3Dir, float fRayRange)
		: m_v3Start(v3Start), m_v3Direction(v3Dir)
	{
		assert(fRayRange >= 0);
		m_fRayRange = fRayRange;

		XMVECTOR dir = XMLoadFloat3(&m_v3Direction);
		dir = XMVector3Normalize(dir);
		XMStoreFloat3(&m_v3Direction, dir);

		XMVECTOR start = XMLoadFloat3(&m_v3Start);
		XMStoreFloat3(&m_v3End, start + dir * fRayRange);
	}

	CRay() {}

	void SetStartPoint(const XMFLOAT3& v3Start)
	{
		m_v3Start = v3Start;
	}

	void SetDirection(const XMFLOAT3& v3Dir, float fRayRange)
	{
		assert(fRayRange >= 0);

		m_v3Direction = v3Dir;

		XMVECTOR dir = XMLoadFloat3(&m_v3Direction);
		dir = XMVector3Normalize(dir);
		XMStoreFloat3(&m_v3Direction, dir);

		m_fRayRange = fRayRange;

		XMVECTOR start = XMLoadFloat3(&m_v3Start);
		XMStoreFloat3(&m_v3End, start + dir * fRayRange);
	}

	void GetStartPoint(XMFLOAT3* pv3Start) const
	{
		*pv3Start = m_v3Start;
	}

	void GetDirection(XMFLOAT3* pv3Dir, float* pfRayRange) const
	{
		*pv3Dir = m_v3Direction;
		*pfRayRange = m_fRayRange;
	}

	void GetEndPoint(XMFLOAT3* pv3End) const
	{
		*pv3End = m_v3End;
	}

	const CRay& operator=(const CRay& rhs)
	{
		assert(rhs.m_fRayRange >= 0);

		m_v3Start = rhs.m_v3Start;
		m_v3Direction = rhs.m_v3Direction;
		m_fRayRange = rhs.m_fRayRange;
		m_v3End = rhs.m_v3End;

		return *this;
	}

private:
	XMFLOAT3 m_v3Start;
	XMFLOAT3 m_v3End;
	XMFLOAT3 m_v3Direction;
	float m_fRayRange;
};
