/* Copyright (C) John W. Ratcliff, 2001. 
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) John W. Ratcliff, 2001"
 */

#include "Stdafx.h"
#include "frustum.h"

//#include "frustum.h"

/*void Frustum::Set(int x1,int y1,int x2,int y2)
{
  mX1 = x1;
  mY1 = y1;
  mX2 = x2;
  mY2 = y2;
}

*/
ViewState Frustum::ViewVolumeTest(const Vector3d& c, float r) const
{
	XMVECTOR center = XMLoadFloat3((XMFLOAT3*)&c);

	if (m_bUsingSphere)
	{
		XMVECTOR v = center - XMLoadFloat3(&m_v3Center);
		float d = XMVectorGetX(XMVector3LengthSq(v));

		float rr = m_fRadius + r;
		if (rr * rr < d) return VS_OUTSIDE;
	}

	float dist[6];

	for (int i = 0; i < 6; ++i)
	{
		XMVECTOR p = XMLoadFloat4(&m_plane[i]);
		dist[i] = XMVectorGetX(XMPlaneDotCoord(p, center));

		if (dist[i] <= -r)
			return VS_OUTSIDE;
	}

	for (int i = 0; i < 6; ++i)
		if (dist[i] <= r)
			return VS_PARTIAL;

	return VS_INSIDE;
}

void Frustum::BuildViewFrustum(const XMFLOAT4X4& mat)
{
    m_bUsingSphere = false;

    XMMATRIX m = XMLoadFloat4x4(&mat);

    XMFLOAT4X4 f;
    XMStoreFloat4x4(&f, m);

    // LEFT
    m_plane[0] = XMFLOAT4(
        f._14 + f._11,
        f._24 + f._21,
        f._34 + f._31,
        f._44 + f._41);

    // RIGHT
    m_plane[1] = XMFLOAT4(
        f._14 - f._11,
        f._24 - f._21,
        f._34 - f._31,
        f._44 - f._41);

    // TOP
    m_plane[2] = XMFLOAT4(
        f._14 - f._12,
        f._24 - f._22,
        f._34 - f._32,
        f._44 - f._42);

    // BOTTOM
    m_plane[3] = XMFLOAT4(
        f._14 + f._12,
        f._24 + f._22,
        f._34 + f._32,
        f._44 + f._42);

    // NEAR
    m_plane[4] = XMFLOAT4(
        f._13,
        f._23,
        f._33,
        f._43);

    // FAR
    m_plane[5] = XMFLOAT4(
        f._14 - f._13,
        f._24 - f._23,
        f._34 - f._33,
        f._44 - f._43);

    for (int i = 0; i < 6; ++i)
    {
        XMVECTOR p = XMLoadFloat4(&m_plane[i]);
        p = XMPlaneNormalize(p);
        XMStoreFloat4(&m_plane[i], p);
    }
}

void Frustum::BuildViewFrustum2(XMFLOAT4X4& mat,
	float fNear, float fFar, float fFov, float fAspect,
	const XMFLOAT3& cam, const XMFLOAT3& look)
{
	float len = fFar - fNear;
	float h = len * tanf(fFov * 0.5f);
	float w = h * fAspect;

	XMVECTOR P = XMVectorSet(0, 0, fNear + len * 0.5f, 0);
	XMVECTOR Q = XMVectorSet(w, h, len, 0);

	float radius = XMVectorGetX(XMVector3Length(P - Q));
	m_fRadius = radius;

	XMVECTOR c = XMLoadFloat3(&cam);
	XMVECTOR l = XMLoadFloat3(&look);

	XMStoreFloat3(&m_v3Center,
		c + l * (fNear + len * 0.5f));

	BuildViewFrustum(mat);
	m_bUsingSphere = true;
}