#include "StdAfx.h"
#include "DungeonBlock.h"

#include "EterLib/StateManager.h"

class CDungeonModelInstance : public CGrannyModelInstance
{
public:
	CDungeonModelInstance() = default;
	virtual ~CDungeonModelInstance() = default;

	void RenderDungeonBlock(const RenderContext& ctx)
	{
		if (IsEmpty())
			return;

		_mgr->SetShader(VF_MESH, HAS_TEX2);

		auto vb = m_pModel->GetVertexBuffer();
		if (vb)
		{
			_mgr->SetVertexBuffer(vb, sizeof(TPNT2Vertex));
			RenderMeshNodeListWithTwoTexture(ctx, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
		}
	}

	void RenderDungeonBlockShadow(const RenderContext& ctx)
	{
		if (IsEmpty())
			return;

		STATEMANAGER.GetBlend().Push();

		STATEMANAGER.GetBlend().SetBlendEnable(true);
		STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_ZERO);
		STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_SRC_COLOR);

		_mgr->SetShader(VF_MESH, HAS_TEX2);

		auto lpd3dRigidPNTVtxBuf = m_pModel->GetVertexBuffer();
		if (lpd3dRigidPNTVtxBuf)
		{
			_mgr->SetVertexBuffer(lpd3dRigidPNTVtxBuf, sizeof(TPNT2Vertex));
			RenderMeshNodeListWithoutTexture(ctx, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
		}

		STATEMANAGER.GetBlend().Restore();
	}
};


struct FUpdate
{
	float fElapsedTime;
	XMFLOAT4X4* pmatWorld;

	void operator()(CGrannyModelInstance* pInstance)
	{
		pInstance->Update(CGrannyModelInstance::ANIFPS_MIN);
		pInstance->UpdateLocalTime(fElapsedTime);
		pInstance->Deform(pmatWorld);
	}
};

void CDungeonBlock::Update()
{
	Transform();

	FUpdate Update;
	Update.fElapsedTime = 0.0f;
	Update.pmatWorld = &m_worldMatrix;
	for_each(m_ModelInstanceContainer.begin(), m_ModelInstanceContainer.end(), Update);
}

struct FRender
{
	const RenderContext& ctx;

	void operator()(CDungeonModelInstance* pInstance) const
	{
		if (pInstance)
			pInstance->RenderDungeonBlock(ctx);
	}
};

void CDungeonBlock::Render(const RenderContext& ctx)
{
	std::for_each(m_ModelInstanceContainer.begin(), m_ModelInstanceContainer.end(), FRender{ctx});
}

struct FRenderShadow
{
	const RenderContext& ctx;

	void operator()(CDungeonModelInstance* pInstance) const
	{
		if (pInstance)
			pInstance->RenderDungeonBlockShadow(ctx);
	}
};

void CDungeonBlock::OnRenderShadow(const RenderContext& ctx)
{
	std::for_each(m_ModelInstanceContainer.begin(), m_ModelInstanceContainer.end(), FRenderShadow{ ctx });
}

struct FBoundBox
{
	XMFLOAT3* m_pv3Min;
	XMFLOAT3* m_pv3Max;

	FBoundBox(XMFLOAT3* pv3Min, XMFLOAT3* pv3Max)
	{
		m_pv3Min = pv3Min;
		m_pv3Max = pv3Max;
	}

	void operator()(CGrannyModelInstance* pInstance)
	{
		pInstance->GetBoundBox(m_pv3Min, m_pv3Max);
	}
};

bool CDungeonBlock::GetBoundingSphere(XMFLOAT3& v3Center, float& fRadius)
{
	v3Center = m_v3Center;
	fRadius = m_fRadius;

	XMStoreFloat3(&v3Center, XMVector3TransformCoord(XMLoadFloat3(&v3Center), XMLoadFloat4x4(&GetTransform())));

	return true;
}

void CDungeonBlock::OnUpdateCollisionData(const CStaticCollisionDataVector * pscdVector)
{
	assert(pscdVector);
	CStaticCollisionDataVector::const_iterator it;
	for(it = pscdVector->begin();it!=pscdVector->end();++it)
	{
		AddCollision(&(*it),&GetTransform());
	}
}

void CDungeonBlock::OnUpdateHeighInstance(CAttributeInstance * pAttributeInstance)
{
	assert(pAttributeInstance);
	SetHeightInstance(pAttributeInstance);	
}

bool CDungeonBlock::OnGetObjectHeight(float fX, float fY, float * pfHeight)
{
	if (m_pHeightAttributeInstance && m_pHeightAttributeInstance->GetHeight(fX, fY, pfHeight))
		return true;
	return false;
}

void CDungeonBlock::BuildBoundingSphere()
{
	XMFLOAT3 v3Min, v3Max;
	for_each(m_ModelInstanceContainer.begin(), m_ModelInstanceContainer.end(), FBoundBox(&v3Min, &v3Max));

	XMStoreFloat3(&m_v3Center, (XMLoadFloat3(&v3Min) + XMLoadFloat3(&v3Max)) * 0.5f);

	XMFLOAT3 vv;
	XMStoreFloat3(&vv, XMLoadFloat3(&v3Max) - XMLoadFloat3(&v3Min));

	m_fRadius = XMVectorGetX(XMVector3Length(XMLoadFloat3(&vv))) * 0.5f + 150.0f;
}

bool CDungeonBlock::Intersect(float * pfu, float * pfv, float * pft)
{
	TModelInstanceContainer::iterator itor = m_ModelInstanceContainer.begin();
	for (; itor != m_ModelInstanceContainer.end(); ++itor)
	{
		CDungeonModelInstance * pInstance = *itor;
		if (pInstance->Intersect(&CGraphicObjectInstance::GetTransform(), pfu, pfv, pft))
			return true;
	}

	return false;
}

void CDungeonBlock::GetBoundBox(XMFLOAT3 * pv3Min, XMFLOAT3 * pv3Max)
{
	pv3Min->x = +10000000.0f;
	pv3Min->y = +10000000.0f;
	pv3Min->z = +10000000.0f;
	pv3Max->x = -10000000.0f;
	pv3Max->y = -10000000.0f;
	pv3Max->z = -10000000.0f;

	TModelInstanceContainer::iterator itor = m_ModelInstanceContainer.begin();
	for (; itor != m_ModelInstanceContainer.end(); ++itor)
	{
		CDungeonModelInstance * pInstance = *itor;

		XMFLOAT3 v3Min;
		XMFLOAT3 v3Max;
		pInstance->GetBoundBox(&v3Min, &v3Max);

		pv3Min->x = std::min(v3Min.x, pv3Min->x);
		pv3Min->y = std::min(v3Min.x, pv3Min->y);
		pv3Min->z = std::min(v3Min.x, pv3Min->z);
		pv3Max->x = std::max(v3Max.x, pv3Max->x);
		pv3Max->y = std::max(v3Max.x, pv3Max->y);
		pv3Max->z = std::max(v3Max.x, pv3Max->z);
	}
}

bool CDungeonBlock::Load(const char * c_szFileName)
{
	Destroy();

	m_pThing = CResourceManager::Instance().GetTyped<CGraphicThing>(c_szFileName);

	m_pThing->AddReference();
	if (m_pThing->GetModelCount() <= 0)
	{
		TraceError("CDungeonBlock::Load(filename=%s) - model count is %d\n", c_szFileName, m_pThing->GetModelCount());
		return false;
	}

	m_ModelInstanceContainer.reserve(m_pThing->GetModelCount());

	for (int i = 0; i < m_pThing->GetModelCount(); ++i)
	{
		CDungeonModelInstance* pModelInstance = new CDungeonModelInstance;
		pModelInstance->SetMainModelPointer(m_pThing->GetModelPointer(i));
		m_ModelInstanceContainer.push_back(pModelInstance);
	}

	return true;
}

void CDungeonBlock::__Initialize()
{
	m_v3Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_fRadius = 0.0f;

	m_pThing = NULL;
}

void CDungeonBlock::Destroy()
{
	if (m_pThing)
	{
		m_pThing->Release();
		m_pThing = NULL;
	}

	stl_wipe(m_ModelInstanceContainer);

	__Initialize();
}

CDungeonBlock::CDungeonBlock()
{
	__Initialize();
}
CDungeonBlock::~CDungeonBlock()
{
	Destroy();
}
