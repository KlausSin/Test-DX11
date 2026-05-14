#include "Stdafx.h"
#include "CollisionData.h"
#include "Pool.h"
#include "GrpScreen.h"
#include "GrpMath.h"
#include "lineintersect_utils.h"
#include "StateManager.h"
const float gc_fReduceMove = 0.5f;


CDynamicPool<CSphereCollisionInstance> gs_sci;
CDynamicPool<CCylinderCollisionInstance> gs_cci;
CDynamicPool<CPlaneCollisionInstance> gs_pci;
CDynamicPool<CAABBCollisionInstance> gs_aci;
CDynamicPool<COBBCollisionInstance> gs_oci;

void DestroyCollisionInstanceSystem()
{
	gs_sci.Destroy();
	gs_cci.Destroy();
	gs_pci.Destroy();
	gs_aci.Destroy();
	gs_oci.Destroy();
}

/////////////////////////////////////////////
// Base
CBaseCollisionInstance* CBaseCollisionInstance::BuildCollisionInstance(const CStaticCollisionData* c_pCollisionData, const XMFLOAT4X4* pMat)
{
	XMMATRIX matWorld = XMLoadFloat4x4(pMat);

	switch (c_pCollisionData->dwType)
	{
	case COLLISION_TYPE_PLANE:
	{
		CPlaneCollisionInstance* ppci = gs_pci.Alloc();

		XMMATRIX matRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&c_pCollisionData->quatRotation));
		XMMATRIX matTranslationLocal = XMMatrixTranslation(c_pCollisionData->v3Position.x, c_pCollisionData->v3Position.y, c_pCollisionData->v3Position.z);
		XMMATRIX matTransform = matRotation * matTranslationLocal * matWorld;

		TPlaneData& PlaneData = ppci->GetAttribute();

		XMStoreFloat3(&PlaneData.v3Position, XMVector3TransformCoord(XMLoadFloat3(&c_pCollisionData->v3Position), matWorld));

		float fHalfWidth = c_pCollisionData->fDimensions[0] / 2.0f;
		float fHalfLength = c_pCollisionData->fDimensions[1] / 2.0f;

		PlaneData.v3QuadPosition[0] = XMFLOAT3(-fHalfWidth, -fHalfLength, 0.0f);
		PlaneData.v3QuadPosition[1] = XMFLOAT3(+fHalfWidth, -fHalfLength, 0.0f);
		PlaneData.v3QuadPosition[2] = XMFLOAT3(-fHalfWidth, +fHalfLength, 0.0f);
		PlaneData.v3QuadPosition[3] = XMFLOAT3(+fHalfWidth, +fHalfLength, 0.0f);

		for (uint32_t i = 0; i < 4; ++i)
			XMStoreFloat3(&PlaneData.v3QuadPosition[i], XMVector3TransformCoord(XMLoadFloat3(&PlaneData.v3QuadPosition[i]), matTransform));

		XMVECTOR v3Line0 = XMVector3Normalize(XMLoadFloat3(&PlaneData.v3QuadPosition[1]) - XMLoadFloat3(&PlaneData.v3QuadPosition[0]));
		XMVECTOR v3Line1 = XMVector3Normalize(XMLoadFloat3(&PlaneData.v3QuadPosition[2]) - XMLoadFloat3(&PlaneData.v3QuadPosition[0]));
		XMVECTOR v3Line2 = XMVector3Normalize(XMLoadFloat3(&PlaneData.v3QuadPosition[1]) - XMLoadFloat3(&PlaneData.v3QuadPosition[3]));
		XMVECTOR v3Line3 = XMVector3Normalize(XMLoadFloat3(&PlaneData.v3QuadPosition[2]) - XMLoadFloat3(&PlaneData.v3QuadPosition[3]));

		XMVECTOR v3Normal = XMVector3Normalize(XMVector3Cross(v3Line0, v3Line1));
		XMStoreFloat3(&PlaneData.v3Normal, v3Normal);

		XMStoreFloat3(&PlaneData.v3InsideVector[0], XMVector3Cross(v3Normal, v3Line0));
		XMStoreFloat3(&PlaneData.v3InsideVector[1], XMVector3Cross(v3Line1, v3Normal));
		XMStoreFloat3(&PlaneData.v3InsideVector[2], XMVector3Cross(v3Line2, v3Normal));
		XMStoreFloat3(&PlaneData.v3InsideVector[3], XMVector3Cross(v3Normal, v3Line3));

		return ppci;
	}
	break;

	case COLLISION_TYPE_BOX:
		assert(false && "COLLISION_TYPE_BOX not implemented");
		break;

	case COLLISION_TYPE_AABB:
	{
		CAABBCollisionInstance* paci = gs_aci.Alloc();

		XMFLOAT3 v3Pos = c_pCollisionData->v3Position;

		TAABBData& AABBData = paci->GetAttribute();
		AABBData.v3Min = XMFLOAT3(v3Pos.x - c_pCollisionData->fDimensions[0], v3Pos.y - c_pCollisionData->fDimensions[1], v3Pos.z - c_pCollisionData->fDimensions[2]);
		AABBData.v3Max = XMFLOAT3(v3Pos.x + c_pCollisionData->fDimensions[0], v3Pos.y + c_pCollisionData->fDimensions[1], v3Pos.z + c_pCollisionData->fDimensions[2]);

		XMStoreFloat3(&AABBData.v3Min, XMVector3TransformCoord(XMLoadFloat3(&AABBData.v3Min), matWorld));
		XMStoreFloat3(&AABBData.v3Max, XMVector3TransformCoord(XMLoadFloat3(&AABBData.v3Max), matWorld));

		return paci;
	}
	break;

	case COLLISION_TYPE_OBB:
	{
		COBBCollisionInstance* poci = gs_oci.Alloc();

		XMFLOAT3 v3Min(
			c_pCollisionData->v3Position.x - c_pCollisionData->fDimensions[0],
			c_pCollisionData->v3Position.y - c_pCollisionData->fDimensions[1],
			c_pCollisionData->v3Position.z - c_pCollisionData->fDimensions[2]);

		XMFLOAT3 v3Max(
			c_pCollisionData->v3Position.x + c_pCollisionData->fDimensions[0],
			c_pCollisionData->v3Position.y + c_pCollisionData->fDimensions[1],
			c_pCollisionData->v3Position.z + c_pCollisionData->fDimensions[2]);

		XMVECTOR vMin = XMVector3TransformCoord(XMLoadFloat3(&v3Min), matWorld);
		XMVECTOR vMax = XMVector3TransformCoord(XMLoadFloat3(&v3Max), matWorld);
		XMVECTOR vPosition = (vMin + vMax) * 0.5f;

		XMFLOAT3 v3Position;
		XMStoreFloat3(&v3Position, vPosition);

		TOBBData& OBBData = poci->GetAttribute();
		OBBData.v3Min = XMFLOAT3(v3Position.x - c_pCollisionData->fDimensions[0], v3Position.y - c_pCollisionData->fDimensions[1], v3Position.z - c_pCollisionData->fDimensions[2]);
		OBBData.v3Max = XMFLOAT3(v3Position.x + c_pCollisionData->fDimensions[0], v3Position.y + c_pCollisionData->fDimensions[1], v3Position.z + c_pCollisionData->fDimensions[2]);

		XMFLOAT4X4 matRot;
		XMStoreFloat4x4(&matRot, matWorld);
		matRot._41 = 0.0f;
		matRot._42 = 0.0f;
		matRot._43 = 0.0f;
		matRot._44 = 1.0f;
		OBBData.matRot = matRot;

		return poci;
	}
	break;

	case COLLISION_TYPE_SPHERE:
	{
		CSphereCollisionInstance* psci = gs_sci.Alloc();

		XMMATRIX matTranslationLocal = XMMatrixTranslation(c_pCollisionData->v3Position.x, c_pCollisionData->v3Position.y, c_pCollisionData->v3Position.z) * matWorld;

		TSphereData& SphereData = psci->GetAttribute();
		SphereData.v3Position = XMFLOAT3(XMVectorGetX(matTranslationLocal.r[3]), XMVectorGetY(matTranslationLocal.r[3]), XMVectorGetZ(matTranslationLocal.r[3]));
		SphereData.fRadius = c_pCollisionData->fDimensions[0];

		return psci;
	}
	break;

	case COLLISION_TYPE_CYLINDER:
	{
		CCylinderCollisionInstance* pcci = gs_cci.Alloc();

		XMMATRIX matTranslationLocal = XMMatrixTranslation(c_pCollisionData->v3Position.x, c_pCollisionData->v3Position.y, c_pCollisionData->v3Position.z) * matWorld;

		TCylinderData& CylinderData = pcci->GetAttribute();
		CylinderData.fRadius = c_pCollisionData->fDimensions[0];
		CylinderData.fHeight = c_pCollisionData->fDimensions[1];
		CylinderData.v3Position = XMFLOAT3(XMVectorGetX(matTranslationLocal.r[3]), XMVectorGetY(matTranslationLocal.r[3]), XMVectorGetZ(matTranslationLocal.r[3]));

		return pcci;
	}
	break;
	}

	assert(false && "NOT_REACHED");
	return nullptr;
}

void CBaseCollisionInstance::Destroy()
{
	OnDestroy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*------------------------------------------------------Sphere---------------------------------------------------------------*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSphereData & CSphereCollisionInstance::GetAttribute()
{
	return m_attribute;
}

const TSphereData & CSphereCollisionInstance::GetAttribute() const
{
	return m_attribute;
}

void CSphereCollisionInstance::Render(D3D11_FILL_MODE D3D11_FILL_MODE)
{
	static CScreen s;
	_mgr->GetCbMgr()->SetTextureFactor(0xffffffff);
}

void CSphereCollisionInstance::OnDestroy()
{
	gs_sci.Free(this);
}

bool CSphereCollisionInstance::OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
	const float fRadiusSum = m_attribute.fRadius + s.fRadius;

	if (square_distance_between_linesegment_and_point(s.v3LastPosition, s.v3Position, m_attribute.v3Position) < fRadiusSum * fRadiusSum)
	{
		if (GetVector3Distance(s.v3Position, m_attribute.v3Position) < GetVector3Distance(s.v3LastPosition, m_attribute.v3Position))
			return true;
	}

	return false;
}

bool CSphereCollisionInstance::OnCollisionDynamicSphere(const CDynamicSphereInstance & s) const 
{
	//Tracef("OnCollisionDynamicSphere\n");
	
	if (square_distance_between_linesegment_and_point(s.v3LastPosition,s.v3Position,m_attribute.v3Position)<(m_attribute.fRadius+s.fRadius)*(m_attribute.fRadius+s.fRadius))
	{
		return true;
	}
	
	return false;
}

XMFLOAT3 CSphereCollisionInstance::OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const
{
	const XMVECTOR vPos = XMLoadFloat3(&s.v3Position);
	const XMVECTOR vLastPos = XMLoadFloat3(&s.v3LastPosition);
	const XMVECTOR vAttrPos = XMLoadFloat3(&m_attribute.v3Position);

	const XMVECTOR _vv__ = vPos - vAttrPos;
	const float fRadiusSum = s.fRadius + m_attribute.fRadius;

	if (XMVectorGetX(XMVector3LengthSq(_vv__)) >= fRadiusSum * fRadiusSum)
		return XMFLOAT3(0.0f, 0.0f, 0.0f);

	XMVECTOR c;
	const XMVECTOR _vv__2 = vPos - vLastPos;
	const XMVECTOR _vv_s_2 = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	c = XMVector3Cross(_vv__2, _vv_s_2);

	float sum = -XMVectorGetX(XMVector3Dot(c, _vv__));
	float mul = fRadiusSum * fRadiusSum - XMVectorGetX(XMVector3LengthSq(_vv__));

	if (sum * sum - 4.0f * mul <= 0.0f)
		return XMFLOAT3(0.0f, 0.0f, 0.0f);

	float sq = sqrtf(sum * sum - 4.0f * mul);
	float t1 = -sum - sq;
	float t2 = -sum + sq;

	t1 *= 0.5f;
	t2 *= 0.5f;

	XMFLOAT3 result;

	if (fabsf(t1) <= fabsf(t2))
		XMStoreFloat3(&result, c * (gc_fReduceMove * t1));
	else
		XMStoreFloat3(&result, c * (gc_fReduceMove * t2));

	return result;
}

/////////////////////////////////////////////
// Plane
TPlaneData & CPlaneCollisionInstance::GetAttribute()
{
	return m_attribute;
}

const TPlaneData & CPlaneCollisionInstance::GetAttribute() const
{
	return m_attribute;
}

bool CPlaneCollisionInstance::OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
	const XMVECTOR vPos = XMLoadFloat3(&s.v3Position);
	const XMVECTOR vLastPos = XMLoadFloat3(&s.v3LastPosition);
	const XMVECTOR vPlanePos = XMLoadFloat3(&m_attribute.v3Position);
	const XMVECTOR vNormal = XMLoadFloat3(&m_attribute.v3Normal);

	const XMVECTOR v3SpherePosition = vPos - vPlanePos;
	const XMVECTOR v3SphereLastPosition = vLastPos - vPlanePos;

	float fPosition1 = XMVectorGetX(XMVector3Dot(vNormal, v3SpherePosition));
	float fPosition2 = XMVectorGetX(XMVector3Dot(vNormal, v3SphereLastPosition));

	if ((fPosition1 > 0.0f && fPosition2 < 0.0f) || (fPosition1 < 0.0f && fPosition2 > 0.0f) || (fPosition1 <= s.fRadius && fPosition1 >= -s.fRadius))
	{
		const XMVECTOR vQuad0 = XMLoadFloat3(&m_attribute.v3QuadPosition[0]);
		const XMVECTOR vQuad3 = XMLoadFloat3(&m_attribute.v3QuadPosition[3]);

		const XMVECTOR v3QuadPosition1 = vPos - vQuad0;
		const XMVECTOR v3QuadPosition2 = vPos - vQuad3;

		if (XMVectorGetX(XMVector3Dot(v3QuadPosition1, XMLoadFloat3(&m_attribute.v3InsideVector[0]))) > -s.fRadius)
			if (XMVectorGetX(XMVector3Dot(v3QuadPosition1, XMLoadFloat3(&m_attribute.v3InsideVector[1]))) > -s.fRadius)
				if (XMVectorGetX(XMVector3Dot(v3QuadPosition2, XMLoadFloat3(&m_attribute.v3InsideVector[2]))) > -s.fRadius)
					if (XMVectorGetX(XMVector3Dot(v3QuadPosition2, XMLoadFloat3(&m_attribute.v3InsideVector[3]))) > -s.fRadius)
					{
						const XMVECTOR _vv__3 = vPos - vPlanePos;
						const XMVECTOR _vv__4 = vLastPos - vPlanePos;

						if (fabsf(XMVectorGetX(XMVector3Dot(_vv__3, vNormal))) < fabsf(XMVectorGetX(XMVector3Dot(_vv__4, vNormal))))
							return true;
					}
	}

	return false;
}

bool CPlaneCollisionInstance::OnCollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
	const XMVECTOR vPos = XMLoadFloat3(&s.v3Position);
	const XMVECTOR vLastPos = XMLoadFloat3(&s.v3LastPosition);
	const XMVECTOR vPlanePos = XMLoadFloat3(&m_attribute.v3Position);
	const XMVECTOR vNormal = XMLoadFloat3(&m_attribute.v3Normal);

	const XMVECTOR v3SpherePosition = vPos - vPlanePos;
	const XMVECTOR v3SphereLastPosition = vLastPos - vPlanePos;

	float fPosition1 = XMVectorGetX(XMVector3Dot(vNormal, v3SpherePosition));
	float fPosition2 = XMVectorGetX(XMVector3Dot(vNormal, v3SphereLastPosition));

	if ((fPosition1 > 0.0f && fPosition2 < 0.0f) || (fPosition1 < 0.0f && fPosition2 > 0.0f) || (fPosition1 <= s.fRadius && fPosition1 >= -s.fRadius))
	{
		const XMVECTOR v3QuadPosition1 = vPos - XMLoadFloat3(&m_attribute.v3QuadPosition[0]);
		const XMVECTOR v3QuadPosition2 = vPos - XMLoadFloat3(&m_attribute.v3QuadPosition[3]);

		if (XMVectorGetX(XMVector3Dot(v3QuadPosition1, XMLoadFloat3(&m_attribute.v3InsideVector[0]))) > -s.fRadius)
			if (XMVectorGetX(XMVector3Dot(v3QuadPosition1, XMLoadFloat3(&m_attribute.v3InsideVector[1]))) > -s.fRadius)
				if (XMVectorGetX(XMVector3Dot(v3QuadPosition2, XMLoadFloat3(&m_attribute.v3InsideVector[2]))) > -s.fRadius)
					if (XMVectorGetX(XMVector3Dot(v3QuadPosition2, XMLoadFloat3(&m_attribute.v3InsideVector[3]))) > -s.fRadius)
						return true;
	}

	return false;
}

XMFLOAT3 CPlaneCollisionInstance::OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const
{
	const XMVECTOR vPos = XMLoadFloat3(&s.v3Position);
	const XMVECTOR vLastPos = XMLoadFloat3(&s.v3LastPosition);
	const XMVECTOR vPlanePos = XMLoadFloat3(&m_attribute.v3Position);
	const XMVECTOR vNormal = XMLoadFloat3(&m_attribute.v3Normal);

	const XMVECTOR advance = vPos - vLastPos;

	float d = XMVectorGetX(XMVector3Dot(vNormal, advance));
	if (d >= -0.0001f && d <= 0.0001f)
		return XMFLOAT3(0.0f, 0.0f, 0.0f);

	const XMVECTOR vv = vPos - vPlanePos;
	float t = -XMVectorGetX(XMVector3Dot(vNormal, vv)) / d;

	XMFLOAT3 result;

	if (XMVectorGetX(XMVector3Dot(vNormal, advance)) >= 0.0f)
		XMStoreFloat3(&result, advance * t - vNormal * s.fRadius);
	else
		XMStoreFloat3(&result, advance * t + vNormal * s.fRadius);

	return result;
}

void CPlaneCollisionInstance::Render(D3D11_FILL_MODE /*D3D11_FILL_MODE*/)
{
	static CScreen s;
	s.RenderBar3d(m_attribute.v3QuadPosition);
}

void CPlaneCollisionInstance::OnDestroy()
{
	gs_pci.Free(this);
}

/////////////////////////////////////////////
// Cylinder
TCylinderData & CCylinderCollisionInstance::GetAttribute()
{
	return m_attribute;
}

const TCylinderData & CCylinderCollisionInstance::GetAttribute() const
{
	return m_attribute;
}

bool CCylinderCollisionInstance::CollideCylinderVSDynamicSphere(const TCylinderData& c_rattribute, const CDynamicSphereInstance& s) const
{
	if (s.v3Position.z + s.fRadius < c_rattribute.v3Position.z)
		return false;

	if (s.v3Position.z - s.fRadius > c_rattribute.v3Position.z + c_rattribute.fHeight)
		return false;

	XMFLOAT3 oa, ob;

	IntersectLineSegments(
		c_rattribute.v3Position,
		XMFLOAT3(c_rattribute.v3Position.x, c_rattribute.v3Position.y, c_rattribute.v3Position.z + c_rattribute.fHeight),
		s.v3LastPosition,
		s.v3Position,
		oa,
		ob
	);

	const XMVECTOR vv = XMLoadFloat3(&oa) - XMLoadFloat3(&ob);
	const float fRadiusSum = c_rattribute.fRadius + s.fRadius;

	return XMVectorGetX(XMVector3LengthSq(vv)) <= fRadiusSum * fRadiusSum;
}

bool CCylinderCollisionInstance::OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
	if (CollideCylinderVSDynamicSphere(m_attribute, s))
	{
		if (GetVector3Distance(s.v3Position, m_attribute.v3Position) < GetVector3Distance(s.v3LastPosition, m_attribute.v3Position))
			return true;
	}

	const XMVECTOR vDistance = XMLoadFloat3(&s.v3Position) - XMLoadFloat3(&s.v3LastPosition);
	float fDistance = XMVectorGetX(XMVector3Length(vDistance));

	if (s.fRadius <= 0.0001f)
		return false;

	if (fDistance >= s.fRadius * 2.0f)
	{
		TCylinderData cylinder = m_attribute;
		cylinder.v3Position = s.v3LastPosition;

		int iStep = int(fDistance / s.fRadius * 2.0f);

		XMFLOAT3 v3Step;
		XMStoreFloat3(&v3Step, vDistance / float(iStep));

		for (int i = 0; i < iStep; ++i)
		{
			XMStoreFloat3(&cylinder.v3Position, XMLoadFloat3(&cylinder.v3Position) + XMLoadFloat3(&v3Step));

			if (CollideCylinderVSDynamicSphere(cylinder, s))
				return true;
		}
	}

	return false;
}

bool CCylinderCollisionInstance::OnCollisionDynamicSphere(const CDynamicSphereInstance & s) const
{
	//Tracef("OnCollisionDynamicSphere\n");
	
	return (CollideCylinderVSDynamicSphere(m_attribute, s));
}

XMFLOAT3 CCylinderCollisionInstance::OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const
{
	XMFLOAT3 v3Position = m_attribute.v3Position;
	v3Position.z = s.v3Position.z;

	const XMVECTOR vPos = XMLoadFloat3(&s.v3Position);
	const XMVECTOR vCylPos = XMLoadFloat3(&v3Position);

	const XMVECTOR vv = vPos - vCylPos;
	const float fRadiusSum = s.fRadius + m_attribute.fRadius;

	if (XMVectorGetX(XMVector3LengthSq(vv)) >= fRadiusSum * fRadiusSum)
		return XMFLOAT3(0.0f, 0.0f, 0.0f);

	XMVECTOR advance = vPos - XMLoadFloat3(&s.v3LastPosition);
	advance = XMVectorSetZ(advance, 0.0f);

	const XMVECTOR vssa = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	const XMVECTOR c = XMVector3Cross(advance, vssa);

	const XMVECTOR svsvs = vPos - vCylPos;

	float sum = -XMVectorGetX(XMVector3Dot(c, svsvs));
	float mul = fRadiusSum * fRadiusSum - XMVectorGetX(XMVector3LengthSq(svsvs));

	if (sum * sum - 4.0f * mul <= 0.0f)
		return XMFLOAT3(0.0f, 0.0f, 0.0f);

	float sq = sqrtf(sum * sum - 4.0f * mul);
	float t1 = -sum - sq;
	float t2 = -sum + sq;

	t1 *= 0.5f;
	t2 *= 0.5f;

	XMFLOAT3 result;

	if (fabsf(t1) <= fabsf(t2))
		XMStoreFloat3(&result, c * (gc_fReduceMove * t1));
	else
		XMStoreFloat3(&result, c * (gc_fReduceMove * t2));

	return result;
}

void CCylinderCollisionInstance::Render(D3D11_FILL_MODE D3D11_FILL_MODE)
{
	static CScreen s;
	_mgr->GetCbMgr()->SetTextureFactor(0xffffffff);

}

void CCylinderCollisionInstance::OnDestroy()
{
	gs_cci.Free(this);
}

/////////////////////////////////////////////
// AABB (Aligned Axis Bounding Box)
TAABBData & CAABBCollisionInstance::GetAttribute()
{
	return m_attribute;
}

const TAABBData & CAABBCollisionInstance::GetAttribute() const
{

	return m_attribute;
}

bool CAABBCollisionInstance::OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
	XMFLOAT3 v;
	XMFLOAT3 v3center;

	XMStoreFloat3(&v3center, (XMLoadFloat3(&m_attribute.v3Min) + XMLoadFloat3(&m_attribute.v3Max)) * 0.5f);

	v = s.v3Position;

	if (v.x < m_attribute.v3Min.x) v.x = m_attribute.v3Min.x;
	if (v.x > m_attribute.v3Max.x) v.x = m_attribute.v3Max.x;
	if (v.y < m_attribute.v3Min.y) v.x = m_attribute.v3Min.y;
	if (v.y > m_attribute.v3Max.y) v.x = m_attribute.v3Max.y;
	if (v.z < m_attribute.v3Min.z) v.z = m_attribute.v3Min.z;
	if (v.z > m_attribute.v3Max.z) v.z = m_attribute.v3Max.z;

	if (GetVector3Distance(v, s.v3Position) <= s.fRadius * s.fRadius)
		return true;

	v = s.v3LastPosition;

	if (v.x < m_attribute.v3Min.x) v.x = m_attribute.v3Min.x;
	if (v.x > m_attribute.v3Max.x) v.x = m_attribute.v3Max.x;
	if (v.y < m_attribute.v3Min.y) v.x = m_attribute.v3Min.y;
	if (v.y > m_attribute.v3Max.y) v.x = m_attribute.v3Max.y;
	if (v.z < m_attribute.v3Min.z) v.z = m_attribute.v3Min.z;
	if (v.z > m_attribute.v3Max.z) v.z = m_attribute.v3Max.z;

	if (GetVector3Distance(v, s.v3LastPosition) <= s.fRadius * s.fRadius)
		return true;

	return false;
}

bool CAABBCollisionInstance::OnCollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
	XMFLOAT3 v = s.v3Position;

	if (v.x < m_attribute.v3Min.x) v.x = m_attribute.v3Min.x;
	if (v.x > m_attribute.v3Max.x) v.x = m_attribute.v3Max.x;
	if (v.y < m_attribute.v3Min.y) v.x = m_attribute.v3Min.y;
	if (v.y > m_attribute.v3Max.y) v.x = m_attribute.v3Max.y;
	if (v.z < m_attribute.v3Min.z) v.z = m_attribute.v3Min.z;
	if (v.z > m_attribute.v3Max.z) v.z = m_attribute.v3Max.z;

	if (v.x > m_attribute.v3Min.x && v.x < m_attribute.v3Max.x &&
		v.y > m_attribute.v3Min.y && v.y < m_attribute.v3Max.y &&
		v.z > m_attribute.v3Min.z && v.z < m_attribute.v3Max.z)
		return true;

	if (GetVector3Distance(v, s.v3Position) <= s.fRadius * s.fRadius)
		return true;

	v = s.v3LastPosition;

	if (v.x < m_attribute.v3Min.x) v.x = m_attribute.v3Min.x;
	if (v.x > m_attribute.v3Max.x) v.x = m_attribute.v3Max.x;
	if (v.y < m_attribute.v3Min.y) v.x = m_attribute.v3Min.y;
	if (v.y > m_attribute.v3Max.y) v.x = m_attribute.v3Max.y;
	if (v.z < m_attribute.v3Min.z) v.z = m_attribute.v3Min.z;
	if (v.z > m_attribute.v3Max.z) v.z = m_attribute.v3Max.z;

	if (v.x > m_attribute.v3Min.x && v.x < m_attribute.v3Max.x &&
		v.y > m_attribute.v3Min.y && v.y < m_attribute.v3Max.y &&
		v.z > m_attribute.v3Min.z && v.z < m_attribute.v3Max.z)
		return true;

	if (GetVector3Distance(v, s.v3LastPosition) <= s.fRadius * s.fRadius)
		return true;

	return false;
}

XMFLOAT3 CAABBCollisionInstance::OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const
{
	XMFLOAT3 v3Temp;

	if (s.v3Position.x + s.fRadius <= m_attribute.v3Min.x) { v3Temp.x = m_attribute.v3Min.x; }
	else if (s.v3Position.x - s.fRadius >= m_attribute.v3Max.x) { v3Temp.x = m_attribute.v3Max.x; }
	else if (s.v3Position.x + s.fRadius >= m_attribute.v3Min.x && s.v3Position.x + s.fRadius <= m_attribute.v3Max.x) { v3Temp.x = s.v3Position.x + s.fRadius; }
	else { v3Temp.x = s.v3Position.x - s.fRadius; }

	if (s.v3Position.y + s.fRadius <= m_attribute.v3Min.y) { v3Temp.y = m_attribute.v3Min.y; }
	else if (s.v3Position.y - s.fRadius >= m_attribute.v3Max.y) { v3Temp.y = m_attribute.v3Max.y; }
	else if (s.v3Position.y + s.fRadius >= m_attribute.v3Min.y && s.v3Position.y + s.fRadius <= m_attribute.v3Max.y) { v3Temp.y = s.v3Position.y + s.fRadius; }
	else { v3Temp.y = s.v3Position.y - s.fRadius; }

	if (s.v3Position.z + s.fRadius <= m_attribute.v3Min.z) { v3Temp.z = m_attribute.v3Min.z; }
	else if (s.v3Position.z - s.fRadius >= m_attribute.v3Max.z) { v3Temp.z = m_attribute.v3Max.z; }
	else if (s.v3Position.z + s.fRadius >= m_attribute.v3Min.z && s.v3Position.z + s.fRadius <= m_attribute.v3Max.z) { v3Temp.z = s.v3Position.z + s.fRadius; }
	else { v3Temp.z = s.v3Position.z - s.fRadius; }

	const XMVECTOR vv = XMLoadFloat3(&v3Temp) - XMLoadFloat3(&s.v3Position);

	if (XMVectorGetX(XMVector3LengthSq(vv)) < s.fRadius * s.fRadius)
		return XMFLOAT3(0.0f, 0.0f, 0.0f);

	return XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void CAABBCollisionInstance::Render(D3D11_FILL_MODE D3D11_FILL_MODE)
{
	static CScreen s;
	_mgr->GetCbMgr()->SetTextureFactor(0xffffffff);

	s.RenderCube(m_attribute.v3Min.x, m_attribute.v3Min.y, m_attribute.v3Min.z, m_attribute.v3Max.x, m_attribute.v3Max.y, m_attribute.v3Max.z);
	return;
}

void CAABBCollisionInstance::OnDestroy()
{
	gs_aci.Free(this);
}

/////////////////////////////////////////////
// OBB

TOBBData & COBBCollisionInstance::GetAttribute()
{
	return m_attribute;
}

const TOBBData & COBBCollisionInstance::GetAttribute() const
{

	return m_attribute;
}

bool COBBCollisionInstance::OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
	XMFLOAT3 v3Center;
	XMStoreFloat3(&v3Center, (XMLoadFloat3(&m_attribute.v3Min) + XMLoadFloat3(&m_attribute.v3Max)) * 0.5f);

	XMMATRIX matRot = XMLoadFloat4x4(&m_attribute.matRot);

	XMVECTOR vSphere = XMLoadFloat3(&s.v3Position) - XMLoadFloat3(&v3Center);
	vSphere = XMVector3TransformCoord(vSphere, matRot);
	vSphere += XMLoadFloat3(&v3Center);

	XMFLOAT3 v3Sphere;
	XMStoreFloat3(&v3Sphere, vSphere);

	XMFLOAT3 v3Point = v3Sphere;

	if (v3Point.x < m_attribute.v3Min.x) { v3Point.x = m_attribute.v3Min.x; }
	if (v3Point.x > m_attribute.v3Max.x) { v3Point.x = m_attribute.v3Max.x; }
	if (v3Point.y < m_attribute.v3Min.y) { v3Point.y = m_attribute.v3Min.y; }
	if (v3Point.y > m_attribute.v3Max.y) { v3Point.y = m_attribute.v3Max.y; }
	if (v3Point.z < m_attribute.v3Min.z) { v3Point.z = m_attribute.v3Min.z; }
	if (v3Point.z > m_attribute.v3Max.z) { v3Point.z = m_attribute.v3Max.z; }

	if (GetVector3Distance(v3Point, v3Sphere) <= s.fRadius * s.fRadius)
		return true;

	vSphere = XMLoadFloat3(&s.v3LastPosition) - XMLoadFloat3(&v3Center);
	vSphere = XMVector3TransformCoord(vSphere, matRot);
	vSphere += XMLoadFloat3(&v3Center);

	XMStoreFloat3(&v3Sphere, vSphere);

	v3Point = v3Sphere;

	if (v3Point.x < m_attribute.v3Min.x) { v3Point.x = m_attribute.v3Min.x; }
	if (v3Point.x > m_attribute.v3Max.x) { v3Point.x = m_attribute.v3Max.x; }
	if (v3Point.y < m_attribute.v3Min.y) { v3Point.y = m_attribute.v3Min.y; }
	if (v3Point.y > m_attribute.v3Max.y) { v3Point.y = m_attribute.v3Max.y; }
	if (v3Point.z < m_attribute.v3Min.z) { v3Point.z = m_attribute.v3Min.z; }
	if (v3Point.z > m_attribute.v3Max.z) { v3Point.z = m_attribute.v3Max.z; }

	if (GetVector3Distance(v3Point, v3Sphere) <= s.fRadius * s.fRadius)
		return true;

	return false;
}

bool COBBCollisionInstance::OnCollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
	XMFLOAT3 v3Center;
	XMStoreFloat3(&v3Center, (XMLoadFloat3(&m_attribute.v3Min) + XMLoadFloat3(&m_attribute.v3Max)) * 0.5f);

	XMMATRIX matRot = XMLoadFloat4x4(&m_attribute.matRot);

	XMVECTOR vSphere = XMLoadFloat3(&s.v3Position) - XMLoadFloat3(&v3Center);
	vSphere = XMVector3TransformCoord(vSphere, matRot);
	vSphere += XMLoadFloat3(&v3Center);

	XMFLOAT3 v3Sphere;
	XMStoreFloat3(&v3Sphere, vSphere);

	XMFLOAT3 v3Point = v3Sphere;

	if (v3Point.x < m_attribute.v3Min.x) { v3Point.x = m_attribute.v3Min.x; }
	if (v3Point.x > m_attribute.v3Max.x) { v3Point.x = m_attribute.v3Max.x; }
	if (v3Point.y < m_attribute.v3Min.y) { v3Point.y = m_attribute.v3Min.y; }
	if (v3Point.y > m_attribute.v3Max.y) { v3Point.y = m_attribute.v3Max.y; }
	if (v3Point.z < m_attribute.v3Min.z) { v3Point.z = m_attribute.v3Min.z; }
	if (v3Point.z > m_attribute.v3Max.z) { v3Point.z = m_attribute.v3Max.z; }

	if (GetVector3Distance(v3Point, v3Sphere) <= s.fRadius * s.fRadius)
		return true;

	vSphere = XMLoadFloat3(&s.v3LastPosition) - XMLoadFloat3(&v3Center);
	vSphere = XMVector3TransformCoord(vSphere, matRot);
	vSphere += XMLoadFloat3(&v3Center);

	XMStoreFloat3(&v3Sphere, vSphere);

	v3Point = v3Sphere;

	if (v3Point.x < m_attribute.v3Min.x) { v3Point.x = m_attribute.v3Min.x; }
	if (v3Point.x > m_attribute.v3Max.x) { v3Point.x = m_attribute.v3Max.x; }
	if (v3Point.y < m_attribute.v3Min.y) { v3Point.y = m_attribute.v3Min.y; }
	if (v3Point.y > m_attribute.v3Max.y) { v3Point.y = m_attribute.v3Max.y; }
	if (v3Point.z < m_attribute.v3Min.z) { v3Point.z = m_attribute.v3Min.z; }
	if (v3Point.z > m_attribute.v3Max.z) { v3Point.z = m_attribute.v3Max.z; }

	if (GetVector3Distance(v3Point, v3Sphere) <= s.fRadius * s.fRadius)
		return true;

	return false;
}

XMFLOAT3 COBBCollisionInstance::OnGetCollisionMovementAdjust(const CDynamicSphereInstance & s) const
{

	return XMFLOAT3(.0f, .0f, .0f);
	
}

void COBBCollisionInstance::Render(D3D11_FILL_MODE D3D11_FILL_MODE)
{
	static CScreen s;
	_mgr->GetCbMgr()->SetTextureFactor(0xffffffff);
	s.RenderCube(m_attribute.v3Min.x, m_attribute.v3Min.y, m_attribute.v3Min.z, m_attribute.v3Max.x, m_attribute.v3Max.y, m_attribute.v3Max.z, m_attribute.matRot);
	return;
}

void COBBCollisionInstance::OnDestroy()
{
	gs_oci.Free(this);
}