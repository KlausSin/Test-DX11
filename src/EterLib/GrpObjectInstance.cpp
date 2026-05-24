#include "StdAfx.h"
#include "GrpObjectInstance.h"
#include "EterBase/Timer.h"

void CGraphicObjectInstance::OnCreateInternal()
{
	AddComponent<CTransformComponent>();
	AddComponent<CRenderComponent>();
	AddComponent<CCollisionComponent>();
	AddComponent<CAttributeComponent>();
	AddComponent<CBoundsComponent>();
}

void CGraphicObjectInstance::OnDestroyInternal()
{
}

CTransformComponent& CGraphicObjectInstance::TransformComponent()
{ 
	return GetComponent<CTransformComponent>();
}

CRenderComponent& CGraphicObjectInstance::RenderComponent() 
{ 
	return GetComponent<CRenderComponent>(); 
}

CCollisionComponent& CGraphicObjectInstance::CollisionComponent() 
{ 
	return GetComponent<CCollisionComponent>();
}

CAttributeComponent& CGraphicObjectInstance::AttributeComponent() 
{ 
	return GetComponent<CAttributeComponent>();
}

CBoundsComponent& CGraphicObjectInstance::BoundsComponent()
{ 
	return GetComponent<CBoundsComponent>();
}

const CTransformComponent& CGraphicObjectInstance::TransformComponent() const
{
	return GetComponent<CTransformComponent>();
}

const CRenderComponent& CGraphicObjectInstance::RenderComponent() const
{ 
	return GetComponent<CRenderComponent>();
}

const CCollisionComponent& CGraphicObjectInstance::CollisionComponent() const 
{
	return GetComponent<CCollisionComponent>(); 
}

const CAttributeComponent& CGraphicObjectInstance::AttributeComponent() const 
{
	return GetComponent<CAttributeComponent>();
}

const CBoundsComponent& CGraphicObjectInstance::BoundsComponent() const
{ 
	return GetComponent<CBoundsComponent>();
}

void CGraphicObjectInstance::OnInitialize()
{
}

void CGraphicObjectInstance::Clear()
{
	if (m_CullingHandle)
	{
		CCullingManager::Instance().Unregister(m_CullingHandle);
		m_CullingHandle = NULL;
	}

	AttributeComponent().ClearHeightInstance();
	CollisionComponent().Clear();


	OnClear();
}

bool CGraphicObjectInstance::Render(const RenderContext& ctx)
{
	if (!RenderComponent().IsVisible())
		return false;

	OnRender(ctx);
	return true;
}

void CGraphicObjectInstance::BlendRender(const RenderContext& ctx)
{
	if (!RenderComponent().IsVisible())
		return;

	OnBlendRender(ctx);
}

void CGraphicObjectInstance::RenderToShadowMap(const RenderContext& ctx)
{
	if (!RenderComponent().IsVisible())
		return;

	OnRenderToShadowMap(ctx);
}

void CGraphicObjectInstance::RenderShadow(const RenderContext& ctx)
{
	if (!RenderComponent().IsVisible())
		return;

	OnRenderShadow(ctx);
}

void CGraphicObjectInstance::RenderPCBlocker(const RenderContext& ctx)
{
	if (!RenderComponent().IsVisible())
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
	if (!RenderComponent().IsVisible())
		return;

	OnDeform();
}

void CGraphicObjectInstance::Transform()
{
	TransformComponent().UpdateMatrix();
}

bool CGraphicObjectInstance::isIntersect(const CRay& c_rRay, float* pu, float* pv, float* pt)
{
	XMFLOAT3 v3Start, v3Dir;
	float fRayRange;

	c_rRay.GetStartPoint(&v3Start);
	c_rRay.GetDirection(&v3Dir, &fRayRange);

	const auto& bounds = BoundsComponent();

	const XMFLOAT3& min = bounds.GetTBBoxMin();
	const XMFLOAT3& max = bounds.GetTBBoxMax();

	TPosition posVertices[8];

	posVertices[0] = TPosition(min.x, min.y, min.z);
	posVertices[1] = TPosition(max.x, min.y, min.z);
	posVertices[2] = TPosition(min.x, max.y, min.z);
	posVertices[3] = TPosition(max.x, max.y, min.z);

	posVertices[4] = TPosition(min.x, min.y, max.z);
	posVertices[5] = TPosition(max.x, min.y, max.z);
	posVertices[6] = TPosition(min.x, max.y, max.z);
	posVertices[7] = TPosition(max.x, max.y, max.z);

	TIndex Indices[36] =
	{
		0, 1, 2, 1, 3, 2,
		2, 0, 6, 0, 4, 6,
		0, 1, 4, 1, 5, 4,
		1, 3, 5, 3, 7, 5,
		3, 2, 7, 2, 6, 7,
		4, 5, 6, 5, 7, 6
	};

	int triCount = 12;
	WORD* pcurIdx = (WORD*)Indices;

	while (triCount--)
	{
		if (IntersectTriangle(
			v3Start,
			v3Dir,
			posVertices[pcurIdx[0]],
			posVertices[pcurIdx[1]],
			posVertices[pcurIdx[2]],
			pu,
			pv,
			pt))
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
	CreateEntity();
	Initialize();
}

void CGraphicObjectInstance::Initialize()
{
	if (m_CullingHandle)
		CCullingManager::Instance().Unregister(m_CullingHandle);

	m_CullingHandle = 0;

	CollisionComponent().Clear();
	Clear();

	OnInitialize();
}

CGraphicObjectInstance::~CGraphicObjectInstance()
{
	if (m_CullingHandle)
	{
		CCullingManager::Instance().Unregister(m_CullingHandle);
		m_CullingHandle = 0;
	}

	CollisionComponent().Clear();
}

void CGraphicObjectInstance::UpdateBoundingSphere()
{
	if (m_CullingHandle)
	{
		XMFLOAT3 center;
		float radius;
		GetBoundingSphere(center, radius);

		if (radius != m_CullingHandle->GetRadius())
			m_CullingHandle->NewPosRadius((Vector3d&)center, radius);
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

void CGraphicObjectInstance::UpdateCollisionData(const CStaticCollisionDataVector* pscdVector)
{
	CollisionComponent().Clear();
	OnUpdateCollisionData(pscdVector);
}

void CGraphicObjectInstance::UpdateHeightInstance(CAttributeInstance* pAttributeInstance)
{
	AttributeComponent().ClearHeightInstance();
	OnUpdateHeighInstance(pAttributeInstance);
}


bool CGraphicObjectInstance::GetObjectHeight(float fX, float fY, float* pfHeight)
{
	if (!AttributeComponent().HasHeightInstance())
		return false;

	return OnGetObjectHeight(fX, fY, pfHeight);
}
