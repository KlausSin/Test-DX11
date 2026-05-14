#include "stdafx.h"
#include "EterLib/StateManager.h"
#include "EterLib/Camera.h"

#include "FlyingData.h"
#include "FlyTrace.h"

CDynamicPool<CFlyTrace>		CFlyTrace::ms_kPool;		

void CFlyTrace::DestroySystem()
{
	ms_kPool.Destroy();
}

CFlyTrace* CFlyTrace::New()
{
	return ms_kPool.Alloc();
}

void CFlyTrace::Delete(CFlyTrace* pkInst)
{
	pkInst->Destroy();
	ms_kPool.Free(pkInst);
}

CFlyTrace::CFlyTrace()
{
	__Initialize();

	/*
	// Code for texture
	CGraphicImage * pImage = CResourceManager::Instance().GetTyped<CGraphicImage>("d:/ray.jpg");
	m_ImageInstance.SetImagePointer(pImage);
	
	CGraphicTexture * pTexture = m_ImageInstance.GetTexturePointer();
	m_lpTexture = pTexture->GetSRV();
	*/
}

CFlyTrace::~CFlyTrace()
{
	Destroy();
}

				
void CFlyTrace::__Initialize()
{
	m_bRectShape=false;
	m_dwColor=0;
	m_fSize=0.0f;
	m_fTailLength=0.0f;	
}

void CFlyTrace::Destroy()
{
	m_TimePositionDeque.clear();

	__Initialize();
}

void CFlyTrace::UpdateNewPosition(const XMFLOAT3 & v3Position)
{
	m_TimePositionDeque.push_front(TTimePosition(CTimer::Instance().GetCurrentSecond(),v3Position));
	//Tracenf("%f %f",m_TimePositionDeque.back().first, CTimer::Instance().GetCurrentSecond());
	while(!m_TimePositionDeque.empty() && m_TimePositionDeque.back().first+m_fTailLength<CTimer::Instance().GetCurrentSecond())
	{
		m_TimePositionDeque.pop_back();
	}
}

void CFlyTrace::Create(const CFlyingData::TFlyingAttachData & rFlyingAttachData)
{
	//assert(rFlyingAttachData.bHasTail);
	m_dwColor = rFlyingAttachData.dwTailColor;
	m_fTailLength = rFlyingAttachData.fTailLength;
	m_fSize = rFlyingAttachData.fTailSize;
	m_bRectShape = rFlyingAttachData.bRectShape;
}


void CFlyTrace::Update()
{ 
	
}

//1. 알파를 쓰려면 색깔만 줄수있다.
//2. 텍스쳐를 쓰려면 알파 없다-_-


struct TFlyVertex
{
	XMFLOAT3 p;
	DWORD c;
	XMFLOAT2 t;
	TFlyVertex(){};
	TFlyVertex(const XMFLOAT3& p, DWORD c, const XMFLOAT2 & t):p(p),c(c),t(t){}
};

struct TFlyVertexSet
{
	TFlyVertex v[6];
	TFlyVertexSet(TFlyVertex * pv)
	{
		memcpy(v,pv,sizeof(v));
	}
	bool operator < (const TFlyVertexSet& ) const
	{
		return false;
	}
	TFlyVertexSet & operator = ( const TFlyVertexSet& rhs )
	{
		memcpy(v,rhs.v,sizeof(v));
		return *this;
	}
};

typedef std::vector<std::pair<float, TFlyVertexSet> > TFlyVertexSetVector;


void CFlyTrace::Render()
{
	if (m_TimePositionDeque.size() <= 1)
		return;

	TFlyVertexSetVector VSVector;

	STATEMANAGER.GetStateCache().Push();

	STATEMANAGER.GetDepthStencil().SetDepthFunc(D3D11_COMPARISON_LESS);
	STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);

	STATEMANAGER.GetBlend().SetBlendEnable(true);
	STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_ONE);
	STATEMANAGER.GetBlend().SetBlendOp(D3D11_BLEND_OP_ADD);

	_mgr->GetCbMgr()->SetAlphaTestEnable(true);
	_mgr->GetCbMgr()->SetAlphaRef(0x00000000);
	_mgr->GetCbMgr()->SetLightingEnable(false);

	XMFLOAT4X4 matWorld;
	XMStoreFloat4x4(&matWorld, XMMatrixIdentity());

	STATEMANAGER.GetTransform().SetWorld(matWorld);

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);
	_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);

	XMFLOAT4X4 m;
	CScreen s;
	s.UpdateViewMatrix();

	CCamera* pCurrentCamera = CCameraManager::Instance().GetCurrentCamera();
	if (!pCurrentCamera)
	{
		STATEMANAGER.GetStateCache().Restore();
		return;
	}

	XMStoreFloat4x4(&m, XMMatrixIdentity());

	XMFLOAT3 F = pCurrentCamera->GetView();
	m._31 = F.x;
	m._32 = F.y;
	m._33 = F.z;

	Frustum& frustum = s.GetFrustum();

	auto it1 = m_TimePositionDeque.begin();
	auto it2 = it1;
	++it2;

	for (; it2 != m_TimePositionDeque.end(); ++it2, ++it1)
	{
		const XMFLOAT3& rkOld = it1->second;
		const XMFLOAT3& rkNew = it2->second;

		XMFLOAT3 B = {
			rkNew.x - rkOld.x,
			rkNew.y - rkOld.y,
			rkNew.z - rkOld.z
		};

		float radius = std::max(fabsf(B.x), std::max(fabsf(B.y), fabsf(B.z))) / 2.0f;

		Vector3d c(
			it1->second.x + B.x * 0.5f,
			it1->second.y + B.y * 0.5f,
			it1->second.z + B.z * 0.5f);

		if (frustum.ViewVolumeTest(c, radius) == VS_OUTSIDE)
			continue;

		float rate1 = 1.0f - (CTimer::Instance().GetCurrentSecond() - it1->first) / m_fTailLength;
		float rate2 = 1.0f - (CTimer::Instance().GetCurrentSecond() - it2->first) / m_fTailLength;

		float size1 = m_fSize;
		float size2 = m_fSize;

		if (!m_bRectShape)
		{
			size1 *= rate1;
			size2 *= rate2;
		}

		TFlyVertex v[6] =
		{
			TFlyVertex(XMFLOAT3(0.0f, size1, 0.0f), m_dwColor, XMFLOAT2(0.0f, 0.0f)),
			TFlyVertex(XMFLOAT3(-size1, 0.0f, 0.0f), m_dwColor, XMFLOAT2(0.0f, 0.5f)),
			TFlyVertex(XMFLOAT3(size1, 0.0f, 0.0f), m_dwColor, XMFLOAT2(0.5f, 0.0f)),
			TFlyVertex(XMFLOAT3(-size2, 0.0f, 0.0f), m_dwColor, XMFLOAT2(0.5f, 1.0f)),
			TFlyVertex(XMFLOAT3(size2, 0.0f, 0.0f), m_dwColor, XMFLOAT2(1.0f, 0.5f)),
			TFlyVertex(XMFLOAT3(0.0f, -size2, 0.0f), m_dwColor, XMFLOAT2(1.0f, 1.0f)),
		};

		XMFLOAT3 E = {
			pCurrentCamera->GetEye().x - it1->second.x,
			pCurrentCamera->GetEye().y - it1->second.y,
			pCurrentCamera->GetEye().z - it1->second.z
		};

		XMFLOAT3 P;
		XMStoreFloat3(&P, XMVector3Cross(XMLoadFloat3(&B), XMLoadFloat3(&E)));

		XMFLOAT3 U;
		XMStoreFloat3(&U, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&F), XMLoadFloat3(&P))));

		XMFLOAT3 R;
		XMStoreFloat3(&R, XMVector3Cross(XMLoadFloat3(&F), XMLoadFloat3(&U)));

		m._21 = U.x;
		m._22 = U.y;
		m._23 = U.z;
		m._11 = R.x;
		m._12 = R.y;
		m._13 = R.z;

		for (int i = 0; i < 6; ++i)
			XMStoreFloat3(&v[i].p, XMVector3TransformNormal(XMLoadFloat3(&v[i].p), XMLoadFloat4x4(&m)));

		for (int i = 0; i < 3; ++i)
		{
			v[i].p.x += it1->second.x;
			v[i].p.y += it1->second.y;
			v[i].p.z += it1->second.z;
		}

		for (int i = 3; i < 6; ++i)
		{
			v[i].p.x += it2->second.x;
			v[i].p.y += it2->second.y;
			v[i].p.z += it2->second.z;
		}

		float sortValue = -XMVectorGetX(XMVector3Dot(XMLoadFloat3(&E), XMLoadFloat3(&pCurrentCamera->GetView())));
		VSVector.push_back(std::make_pair(sortValue, TFlyVertexSet(v)));
	}

	std::sort(VSVector.begin(), VSVector.end());

	for (auto it = VSVector.begin(); it != VSVector.end(); ++it)
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 4, sizeof(TVertex), it->second.v);

	STATEMANAGER.GetStateCache().Restore();
}
