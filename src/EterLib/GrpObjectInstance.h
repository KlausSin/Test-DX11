#pragma once

#include "GrpColorInstance.h"
#include "GrpScreen.h"
#include "CullingManager.h"
#include "CollisionData.h"
#include "AttributeInstance.h"

#include <qMin32Lib/entity/all.h>
enum
{
	THING_OBJECT = 0xadf21f13,
	TREE_OBJECT = 0x8ac9f7a6,
	ACTOR_OBJECT = 0x29a76c24,
	EFFECT_OBJECT = 0x1cfa97c6,
	DUNGEON_OBJECT = 0x18326035,
};

enum
{
	PORTAL_ID_MAX_NUM = 8,
};

class CGraphicObjectInstance : public CGraphicCollisionObject, public CEntity
{
public:
	CGraphicObjectInstance();
	virtual ~CGraphicObjectInstance();

	virtual int GetType() const = 0;

public:

	void Clear();
	void Update();
	bool Render(const RenderContext& ctx);
	void BlendRender(const RenderContext& ctx);
	void RenderToShadowMap(const RenderContext& ctx);
	void RenderShadow(const RenderContext& ctx);
	void RenderPCBlocker(const RenderContext& ctx);
	void Deform();
	void Transform();

	bool isIntersect(const CRay& c_rRay, float* pu, float* pv, float* pt);


	void Initialize();
	virtual void OnInitialize();

public:
	void UpdateBoundingSphere();
	void RegisterBoundingSphere();
	virtual bool GetBoundingSphere(XMFLOAT3& v3Center, float& fRadius) = 0;

	virtual void OnRender(const RenderContext& ctx) = 0;
	virtual void OnBlendRender(const RenderContext& ctx) = 0;
	virtual void OnRenderToShadowMap(const RenderContext& ctx) = 0;
	virtual void OnRenderShadow(const RenderContext& ctx) = 0;
	virtual void OnRenderPCBlocker(const RenderContext& ctx) = 0;

	virtual void OnClear() {}
	virtual void OnUpdate() {}
	virtual void OnDeform() {}

protected:
	void OnCreateInternal() override;
	void OnDestroyInternal() override;

public:
	CTransformComponent& TransformComponent();
	CRenderComponent& RenderComponent();
	CCollisionComponent& CollisionComponent();
	CAttributeComponent& AttributeComponent();
	CBoundsComponent& BoundsComponent();

	const CTransformComponent& TransformComponent() const;
	const CRenderComponent& RenderComponent() const;
	const CCollisionComponent& CollisionComponent() const;
	const CAttributeComponent& AttributeComponent() const;
	const CBoundsComponent& BoundsComponent() const;

protected:

	CCullingManager::CullingHandle m_CullingHandle;

public:
	void UpdateCollisionData(const CStaticCollisionDataVector* pscdVector = 0);

protected:
	virtual void OnUpdateCollisionData(const CStaticCollisionDataVector* pscdVector) = 0;

public:

	void UpdateHeightInstance(CAttributeInstance* pAttributeInstance = 0);
	bool GetObjectHeight(float fX, float fY, float* pfHeight);

protected:

	virtual void OnUpdateHeighInstance(CAttributeInstance* pAttributeInstance) = 0;
	virtual bool OnGetObjectHeight(float fX, float fY, float* pfHeight) = 0;
};
