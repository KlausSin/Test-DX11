#include "StdAfx.h"
#include "GrpObjectInstance.h"
#include "EterBase/Timer.h"

void CGraphicObjectInstance::OnInitialize()
{	
	ZeroMemory(m_abyPortalID, sizeof(m_abyPortalID));
}

void CGraphicObjectInstance::Clear()
{
	if (m_CullingHandle)
	{
		CCullingManager::Instance().Unregister(m_CullingHandle);
		m_CullingHandle = NULL;
	}

	ClearHeightInstance();

	m_isVisible = TRUE;

	m_v3Position.x = m_v3Position.y = m_v3Position.z = 0.0f;
	m_v3Scale.x = m_v3Scale.y = m_v3Scale.z = 1.0f;
	//m_fRotation = 0.0f;
	m_fYaw = m_fPitch = m_fRoll = 0.0f;

	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());

#ifdef ENABLE_OBJ_SCALLING
	m_v3ScalePosition.x = m_v3ScalePosition.y = m_v3ScalePosition.z = 0.0f;
	XMStoreFloat4x4(&m_ScaleMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_PositionMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_TransformMatrix, XMMatrixIdentity());
#endif

	ZeroMemory(m_abyPortalID, sizeof(m_abyPortalID));

	OnClear();
}

bool CGraphicObjectInstance::Render(const RenderContext& ctx)
{
	if (!isShow())
		return false;

	OnRender(ctx);
	return true;
}

void CGraphicObjectInstance::BlendRender(const RenderContext& ctx)
{
	if (!isShow())
		return;

	OnBlendRender(ctx);
}

void CGraphicObjectInstance::RenderToShadowMap(const RenderContext& ctx)
{
	if (!isShow())
		return;

	OnRenderToShadowMap(ctx);
}

void CGraphicObjectInstance::RenderShadow(const RenderContext& ctx)
{
	if (!isShow())
		return;

	OnRenderShadow(ctx);
}

void CGraphicObjectInstance::RenderPCBlocker(const RenderContext& ctx)
{
	if (!isShow())
		return;

	OnRenderPCBlocker(ctx);
}

void CGraphicObjectInstance::Update()
{
	OnUpdate();

	UpdateBoundingSphere();
}

void CGraphicObjectInstance::Deform()
{
	if (!isShow())
		return;

	OnDeform();
}


void CGraphicObjectInstance::Transform()
{
#ifndef ENABLE_OBJ_SCALLING

	m_worldMatrix = m_mRotation;

	m_worldMatrix._41 += m_v3Position.x;
	m_worldMatrix._42 += m_v3Position.y;
	m_worldMatrix._43 += m_v3Position.z;

#else

	XMFLOAT4X4 tmp1;
	XMStoreFloat4x4(
		&tmp1,
		XMMatrixMultiply(
			XMLoadFloat4x4(&m_PositionMatrix),
			XMLoadFloat4x4(&m_mRotation)
		)
	);

	m_worldMatrix = tmp1;

	m_worldMatrix._41 += m_v3Position.x;
	m_worldMatrix._42 += m_v3Position.y;
	m_worldMatrix._43 += m_v3Position.z;

	XMStoreFloat4x4(
		&tmp1,
		XMMatrixMultiply(
			XMLoadFloat4x4(&m_PositionMatrix),
			XMLoadFloat4x4(&m_ScaleMatrix)
		)
	);

	XMFLOAT4X4 tmp2;
	XMStoreFloat4x4(
		&tmp2,
		XMMatrixMultiply(
			XMLoadFloat4x4(&tmp1),
			XMLoadFloat4x4(&m_mRotation)
		)
	);

	m_TransformMatrix = tmp2;

	m_TransformMatrix._41 = m_v3ScalePosition.x + m_v3Position.x + m_TransformMatrix._41;
	m_TransformMatrix._42 = m_v3ScalePosition.y + m_v3Position.y + m_TransformMatrix._42;
	m_TransformMatrix._43 = m_v3ScalePosition.z + m_v3Position.z + m_TransformMatrix._43;

#endif
}

const XMFLOAT3 & CGraphicObjectInstance::GetPosition() const
{
	return m_v3Position;
}

const XMFLOAT3 & CGraphicObjectInstance::GetScale() const
{
	return m_v3Scale;
}

float CGraphicObjectInstance::GetRotation()
{
	return GetRoll();
}

float CGraphicObjectInstance::GetYaw()
{
	return m_fYaw;
}

float CGraphicObjectInstance::GetPitch()
{
	return m_fPitch;
}

float CGraphicObjectInstance::GetRoll()
{
	return m_fRoll;
}

XMFLOAT4X4 & CGraphicObjectInstance::GetTransform()
{
	return m_worldMatrix;
}

void CGraphicObjectInstance::SetRotationQuaternion(const XMFLOAT4& q)
{
	XMStoreFloat4x4(&m_mRotation, XMMatrixRotationQuaternion(XMLoadFloat4(&q)));
}

void CGraphicObjectInstance::SetRotationMatrix(const XMFLOAT4X4& m)
{
	m_mRotation = m;
}

void CGraphicObjectInstance::SetRotation(float fRotation)
{
	m_fYaw = 0;
	m_fPitch = 0;
	m_fRoll = fRotation;

	XMStoreFloat4x4(&m_mRotation, XMMatrixRotationZ(XMConvertToRadians(fRotation)));
}

void CGraphicObjectInstance::SetRotation(float fYaw, float fPitch, float fRoll)
{
	m_fYaw = fYaw;
	m_fPitch = fPitch;
	m_fRoll = fRoll;

	XMStoreFloat4x4(&m_mRotation, XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll)));
}

void CGraphicObjectInstance::SetPosition(float x, float y, float z)
{
	m_v3Position.x = x;
	m_v3Position.y = y;
	m_v3Position.z = z;	
}

void CGraphicObjectInstance::SetPosition(const XMFLOAT3 & newposition)
{
	m_v3Position = newposition;
}

#ifdef ENABLE_OBJ_SCALLING
void CGraphicObjectInstance::SetScalePosition(float x, float y, float z)
{
	m_v3ScalePosition.x = x;
	m_v3ScalePosition.y = y;
	m_v3ScalePosition.z = z;
}

void CGraphicObjectInstance::SetScale(float x, float y, float z, bool bScaleNow)
#else
void CGraphicObjectInstance::SetScale(float x, float y, float z)
#endif
{
	m_v3Scale.x = x;
	m_v3Scale.y = y;
	m_v3Scale.z = z;

#ifdef ENABLE_OBJ_SCALLING
	if (bScaleNow)
		XMStoreFloat4x4(&m_ScaleMatrix, XMMatrixScaling(m_v3Scale.x, m_v3Scale.y, m_v3Scale.z)); 
#endif
}

void CGraphicObjectInstance::Show()
{
	m_isVisible = true;
}

void CGraphicObjectInstance::Hide()
{
	m_isVisible = false;
}

void CGraphicObjectInstance::ApplyAlwaysHidden() {
	m_isAlwaysHidden = true;
}

void CGraphicObjectInstance::ReleaseAlwaysHidden() {
	m_isAlwaysHidden = false;
}

bool CGraphicObjectInstance::isShow()
{
	return m_isVisible;
}

// 

//////////////////////////////////////////////////////////////////////////

XMFLOAT4 & CGraphicObjectInstance::GetWTBBoxVertex(const unsigned char & c_rucNumTBBoxVertex)
{
	return m_v4TBBox[c_rucNumTBBoxVertex];
}

bool CGraphicObjectInstance::isIntersect(const CRay & c_rRay, float * pu, float * pv, float * pt)
{
	XMFLOAT3 v3Start, v3Dir;
	float fRayRange;
	c_rRay.GetStartPoint(&v3Start);
	c_rRay.GetDirection(&v3Dir, &fRayRange);

	TPosition posVertices[8];

	posVertices[0] = TPosition(m_v3TBBoxMin.x, m_v3TBBoxMin.y, m_v3TBBoxMin.z);
	posVertices[1] = TPosition(m_v3TBBoxMax.x, m_v3TBBoxMin.y, m_v3TBBoxMin.z);
	posVertices[2] = TPosition(m_v3TBBoxMin.x, m_v3TBBoxMax.y, m_v3TBBoxMin.z);
	posVertices[3] = TPosition(m_v3TBBoxMax.x, m_v3TBBoxMax.y, m_v3TBBoxMin.z);
	posVertices[4] = TPosition(m_v3TBBoxMin.x, m_v3TBBoxMin.y, m_v3TBBoxMax.z);
	posVertices[5] = TPosition(m_v3TBBoxMax.x, m_v3TBBoxMin.y, m_v3TBBoxMax.z);
	posVertices[6] = TPosition(m_v3TBBoxMin.x, m_v3TBBoxMax.y, m_v3TBBoxMax.z);
	posVertices[7] = TPosition(m_v3TBBoxMax.x, m_v3TBBoxMax.y, m_v3TBBoxMax.z);

	TIndex Indices[36] = {0, 1, 2, 1, 3, 2,
						  2, 0, 6, 0, 4, 6, 
						  0, 1, 4, 1, 5, 4,
						  1, 3, 5, 3, 7, 5,
						  3, 2, 7, 2, 6, 7,
						  4, 5, 6, 5, 7, 6};

	int triCount = 12;
	WORD* pcurIdx = (WORD*)Indices;

	while (triCount--)
	{
		if (IntersectTriangle(v3Start, v3Dir, 
			posVertices[pcurIdx[0]],
			posVertices[pcurIdx[1]],
			posVertices[pcurIdx[2]],
			pu, pv, pt))
		{
			return true;
		}
		
		pcurIdx += 3;
	}
	
	return false;
}

CGraphicObjectInstance::CGraphicObjectInstance()
{
	m_CullingHandle = 0;
	Initialize();
}

void CGraphicObjectInstance::Initialize()
{
	if (m_CullingHandle)
		CCullingManager::Instance().Unregister(m_CullingHandle);

	m_CullingHandle = 0;

	m_pHeightAttributeInstance = NULL;

	m_isVisible = TRUE;

	m_BlockCamera = false;
	m_isAlwaysHidden = false;

	m_v3Position.x = m_v3Position.y = m_v3Position.z = 0.0f;

#ifdef ENABLE_OBJ_SCALLING
	m_v3Scale.x = m_v3Scale.y = m_v3Scale.z = 0.0f;
#else
	m_v3Scale.x = m_v3Scale.y = m_v3Scale.z = 1.0f;
#endif

	m_fYaw = m_fPitch = m_fRoll = 0.0f;

	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_mRotation, XMMatrixIdentity());

#ifdef ENABLE_OBJ_SCALLING

	m_v3ScalePosition.x = m_v3ScalePosition.y = m_v3ScalePosition.z = 0.0f;

	XMStoreFloat4x4(&m_ScaleMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_PositionMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_TransformMatrix, XMMatrixIdentity());

#endif

	m_v3TBBoxMin = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_v3TBBoxMax = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_v3BBoxMin = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_v3BBoxMax = XMFLOAT3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < 8; ++i)
		m_v4TBBox[i] = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

	memset(m_abyPortalID, 0, sizeof(m_abyPortalID));

	ClearCollision();
	OnInitialize();
}

CGraphicObjectInstance::~CGraphicObjectInstance()
{
	Initialize();
}

void CGraphicObjectInstance::UpdateBoundingSphere()
{
	if (m_CullingHandle)
	{
		XMFLOAT3 center;
		float radius;
		GetBoundingSphere(center,radius);
		if (radius != m_CullingHandle->GetRadius())
			m_CullingHandle->NewPosRadius((Vector3d&)center,radius);
		else
			m_CullingHandle->NewPos((Vector3d&)center);
	}
}

void CGraphicObjectInstance::RegisterBoundingSphere()
{
	if (m_CullingHandle)
		CCullingManager::Instance().Unregister(m_CullingHandle);

	m_CullingHandle = CCullingManager::Instance().Register(this);
}

void CGraphicObjectInstance::AddCollision(const CStaticCollisionData * pscd, const XMFLOAT4X4* pMat)
{
	m_StaticCollisionInstanceVector.push_back(CBaseCollisionInstance::BuildCollisionInstance(pscd, pMat));
}

void CGraphicObjectInstance::ClearCollision()
{
	CCollisionInstanceVector::iterator it;
	for(it = m_StaticCollisionInstanceVector.begin();it!=m_StaticCollisionInstanceVector.end();++it)
	{
		(*it)->Destroy();
	}
	m_StaticCollisionInstanceVector.clear();
}

bool CGraphicObjectInstance::CollisionDynamicSphere(const CDynamicSphereInstance & s) const
{
	CCollisionInstanceVector::const_iterator it;
	for(it = m_StaticCollisionInstanceVector.begin();it!=m_StaticCollisionInstanceVector.end();++it)
	{
		if ((*it)->CollisionDynamicSphere(s))
			return true;
	}
	return false;
}

bool CGraphicObjectInstance::MovementCollisionDynamicSphere(const CDynamicSphereInstance & s) const
{
	CCollisionInstanceVector::const_iterator it;
	for(it = m_StaticCollisionInstanceVector.begin();it!=m_StaticCollisionInstanceVector.end();++it)
	{
		if ((*it)->MovementCollisionDynamicSphere(s))
			return true;
	}
	return false;
}

XMFLOAT3 CGraphicObjectInstance::GetCollisionMovementAdjust(const CDynamicSphereInstance & s) const
{
	CCollisionInstanceVector::const_iterator it;
	for(it = m_StaticCollisionInstanceVector.begin();it!=m_StaticCollisionInstanceVector.end();++it)
	{
		if ((*it)->MovementCollisionDynamicSphere(s))
			return (*it)->GetCollisionMovementAdjust(s);
	}
	
	return XMFLOAT3(0.0f,0.0f,0.0f);
}

void CGraphicObjectInstance::UpdateCollisionData(const CStaticCollisionDataVector * pscdVector)
{
	ClearCollision();
	OnUpdateCollisionData(pscdVector);
}

DWORD CGraphicObjectInstance::GetCollisionInstanceCount()
{
	return m_StaticCollisionInstanceVector.size();
}

CBaseCollisionInstance * CGraphicObjectInstance::GetCollisionInstanceData(DWORD dwIndex)
{
	if (dwIndex>m_StaticCollisionInstanceVector.size())
	{
		return 0;
	}
	return m_StaticCollisionInstanceVector[dwIndex];
}

//////////////////////////////////////////////////////////////////////////
// Height

void CGraphicObjectInstance::SetHeightInstance(CAttributeInstance * pAttributeInstance)
{
	m_pHeightAttributeInstance = pAttributeInstance;
}

void CGraphicObjectInstance::ClearHeightInstance()
{
	m_pHeightAttributeInstance = NULL;
}

void CGraphicObjectInstance::UpdateHeightInstance(CAttributeInstance * pAttributeInstance)
{
	ClearHeightInstance();
	OnUpdateHeighInstance(pAttributeInstance);
}

bool CGraphicObjectInstance::IsObjectHeight()
{
	if (m_pHeightAttributeInstance)
		return true;

	return false;
}

bool CGraphicObjectInstance::GetObjectHeight(float fX, float fY, float * pfHeight)
{
	if (!m_pHeightAttributeInstance)
		return false;

	return OnGetObjectHeight(fX, fY, pfHeight);
}

void CGraphicObjectInstance::SetPortal(DWORD dwIndex, int iID)
{
	if (dwIndex >= PORTAL_ID_MAX_NUM)
	{
		assert(dwIndex < PORTAL_ID_MAX_NUM);
		return;
	}

	m_abyPortalID[dwIndex] = iID;
}

int CGraphicObjectInstance::GetPortal(DWORD dwIndex)
{
	if (dwIndex >= PORTAL_ID_MAX_NUM)
	{
		assert(dwIndex < PORTAL_ID_MAX_NUM);
		return 0;
	}

	return m_abyPortalID[dwIndex];
}
