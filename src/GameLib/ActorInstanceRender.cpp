#include "StdAfx.h"
#include "EterLib/StateManager.h"

#include "ActorInstance.h"

bool CActorInstance::ms_isDirLine=false;

bool CActorInstance::IsDirLine()
{
	return ms_isDirLine;
}

void CActorInstance::ShowDirectionLine(bool isVisible)
{
	ms_isDirLine=isVisible;
}

void CActorInstance::SetMaterialColor(DWORD dwColor)
{
	if (m_pkHorse)
		m_pkHorse->SetMaterialColor(dwColor);

	m_dwMtrlColor&=0xff000000;
	m_dwMtrlColor|=(dwColor&0x00ffffff);
}

void CActorInstance::SetMaterialAlpha(DWORD dwAlpha)
{
	m_dwMtrlAlpha=dwAlpha;	
}


void CActorInstance::OnRender()
{
	// Early out if race data is not loaded yet (async loading)
	if (!m_pkCurRaceData)
		return;

	D3DMATERIAL11 kMtrl;
	STATEMANAGER.GetMaterial(&kMtrl);

	kMtrl.Diffuse=D3DXCOLOR(m_dwMtrlColor);	
	STATEMANAGER.SetMaterial(&kMtrl);

	// 현재는 이렇게.. 최종적인 형태는 Diffuse와 Blend의 분리로..
	// 아니면 이런 형태로 가되 Texture & State Sorting 지원으로.. - [levites]
	STATEMANAGER.GetRaster().Push();
	STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);

	switch(m_iRenderMode)
	{
		case RENDER_MODE_NORMAL:
			BeginDiffuseRender();
				RenderWithOneTexture();
			EndDiffuseRender();
			BeginOpacityRender();
				BlendRenderWithOneTexture();
			EndOpacityRender();
			break;
		case RENDER_MODE_BLEND:
			if (m_fAlphaValue == 1.0f)
			{
				BeginDiffuseRender();
					RenderWithOneTexture();
				EndDiffuseRender();
				BeginOpacityRender();
					BlendRenderWithOneTexture();
				EndOpacityRender();
			}
			else if (m_fAlphaValue > 0.0f)
			{
				BeginBlendRender();
					RenderWithOneTexture();
					BlendRenderWithOneTexture();
				EndBlendRender();
			}
			break;
		case RENDER_MODE_ADD:
			BeginAddRender();
				RenderWithOneTexture();
				BlendRenderWithOneTexture();
			EndAddRender();
			break;
		case RENDER_MODE_MODULATE:
			BeginModulateRender();
				RenderWithOneTexture();
				BlendRenderWithOneTexture();
			EndModulateRender();
			break;
	}

	STATEMANAGER.GetRaster().Restore();

	kMtrl.Diffuse = D3DXCOLOR(0xffffffff);
	STATEMANAGER.SetMaterial(&kMtrl);

	if (ms_isDirLine)
	{
		D3DXVECTOR3 kD3DVt3Cur(m_x, m_y, m_z);

		D3DXVECTOR3 kD3DVt3LookDir(0.0f, -1.0f, 0.0f);
		D3DXMATRIX kD3DMatLook;
		D3DXMatrixRotationZ(&kD3DMatLook, D3DXToRadian(GetRotation()));
		D3DXVec3TransformCoord(&kD3DVt3LookDir, &kD3DVt3LookDir, &kD3DMatLook);
		D3DXVec3Scale(&kD3DVt3LookDir, &kD3DVt3LookDir, 200.0f);
		D3DXVec3Add(&kD3DVt3LookDir, &kD3DVt3LookDir, &kD3DVt3Cur);

		D3DXVECTOR3 kD3DVt3AdvDir(0.0f, -1.0f, 0.0f);
		D3DXMATRIX kD3DMatAdv;
		D3DXMatrixRotationZ(&kD3DMatAdv, D3DXToRadian(GetAdvancingRotation()));
		D3DXVec3TransformCoord(&kD3DVt3AdvDir, &kD3DVt3AdvDir, &kD3DMatAdv);
		D3DXVec3Scale(&kD3DVt3AdvDir, &kD3DVt3AdvDir, 200.0f);
		D3DXVec3Add(&kD3DVt3AdvDir, &kD3DVt3AdvDir, &kD3DVt3Cur);

		static CScreen s_kScreen;

		STATEMANAGER.GetStateCache().Push();

		STATEMANAGER.GetDepthStencil().SetDepthEnable(false);
		_mgr->GetCbMgr()->SetLightingEnable(false);

		s_kScreen.SetDiffuseColor(1.0f, 1.0f, 0.0f);
		s_kScreen.RenderLine3d(kD3DVt3Cur.x, kD3DVt3Cur.y, kD3DVt3Cur.z, kD3DVt3AdvDir.x, kD3DVt3AdvDir.y, kD3DVt3AdvDir.z);

		s_kScreen.SetDiffuseColor(0.0f, 1.0f, 1.0f);
		s_kScreen.RenderLine3d(kD3DVt3Cur.x, kD3DVt3Cur.y, kD3DVt3Cur.z, kD3DVt3LookDir.x, kD3DVt3LookDir.y, kD3DVt3LookDir.z);

		STATEMANAGER.GetStateCache().Restore();
	}
}

void CActorInstance::BeginDiffuseRender()
{
	STATEMANAGER.GetBlend().Push();
	STATEMANAGER.GetBlend().SetBlendEnable(false);
}

void CActorInstance::EndDiffuseRender()
{
	STATEMANAGER.GetBlend().Restore();
}

void CActorInstance::BeginOpacityRender()
{
	_mgr->GetCbMgr()->SetAlphaTestEnable(true);
	_mgr->GetCbMgr()->SetAlphaRef(0);
}

void CActorInstance::EndOpacityRender()
{
	_mgr->GetCbMgr()->SetAlphaTestEnable(false);
}

void CActorInstance::BeginBlendRender()
{
	STATEMANAGER.GetBlend().Push();

	STATEMANAGER.GetBlend().SetBlendEnable(true);
	STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);

	_mgr->GetCbMgr()->SetTextureFactor(D3DXCOLOR(1.0f, 1.0f, 1.0f, m_fAlphaValue));
}

void CActorInstance::EndBlendRender()
{
	STATEMANAGER.GetBlend().Restore();
}

void CActorInstance::BeginAddRender()
{
	_mgr->GetCbMgr()->SetTextureFactor(m_AddColor);

	STATEMANAGER.GetBlend().Push();
	STATEMANAGER.GetBlend().SetBlendEnable(false);
}

void CActorInstance::EndAddRender()
{
	STATEMANAGER.GetBlend().Restore();
}

void CActorInstance::RestoreRenderMode()
{
	// NOTE : This is temporary code. I wanna convert this code to that restore the mode to
	//        model's default setting which had has as like specular or normal. - [levites]
	m_iRenderMode = RENDER_MODE_NORMAL;
	if (m_kBlendAlpha.m_isBlending)
	{
		m_kBlendAlpha.m_iOldRenderMode = m_iRenderMode;
	}
}


void CActorInstance::SetAddRenderMode()
{
	m_iRenderMode = RENDER_MODE_ADD;
	if (m_kBlendAlpha.m_isBlending)
	{
		m_kBlendAlpha.m_iOldRenderMode = m_iRenderMode;
	}
}

void CActorInstance::SetRenderMode(int iRenderMode)
{
	m_iRenderMode = iRenderMode;
	if (m_kBlendAlpha.m_isBlending)
	{
		m_kBlendAlpha.m_iOldRenderMode = iRenderMode;
	}
}

void CActorInstance::SetAddColor(const D3DXCOLOR & c_rColor)
{
	m_AddColor = c_rColor;
	m_AddColor.a = 1.0f;
}

void CActorInstance::BeginModulateRender()
{
	_mgr->GetCbMgr()->SetTextureFactor(m_AddColor);

	STATEMANAGER.GetBlend().Push();
	STATEMANAGER.GetBlend().SetBlendEnable(false);
}

void CActorInstance::EndModulateRender()
{
	STATEMANAGER.GetBlend().Restore();
}

void CActorInstance::SetModulateRenderMode()
{
	m_iRenderMode = RENDER_MODE_MODULATE;
	if (m_kBlendAlpha.m_isBlending)
	{
		m_kBlendAlpha.m_iOldRenderMode = m_iRenderMode;
	}
}

void CActorInstance::RenderCollisionData()
{
	static CScreen s_Screen;

	_mgr->GetCbMgr()->SetLightingEnable(false);

	STATEMANAGER.GetStateCache().Push();

	STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);

	if (m_pAttributeInstance)
	{
		for (DWORD col = 0; col < GetCollisionInstanceCount(); ++col)
		{
			CBaseCollisionInstance* pInstance = GetCollisionInstanceData(col);
			pInstance->Render();
		}
	}

	STATEMANAGER.GetDepthStencil().SetDepthEnable(false);

	s_Screen.SetColorOperation();
	s_Screen.SetDiffuseColor(1.0f, 0.0f, 0.0f);

	TCollisionPointInstanceList::iterator itor;

	s_Screen.SetDiffuseColor(1.0f, isShow() ? 1.0f : 0.0f, 0.0f);

	D3DXVECTOR3 center;
	float r;
	GetBoundingSphere(center, r);
	s_Screen.RenderCircle3d(center.x, center.y, center.z, r);

	s_Screen.SetDiffuseColor(0.0f, 0.0f, 1.0f);
	itor = m_DefendingPointInstanceList.begin();
	for (; itor != m_DefendingPointInstanceList.end(); ++itor)
	{
		const TCollisionPointInstance& c_rInstance = *itor;

		for (DWORD i = 0; i < c_rInstance.SphereInstanceVector.size(); ++i)
		{
			const CDynamicSphereInstance& c_rSphereInstance = c_rInstance.SphereInstanceVector[i];

			s_Screen.RenderCircle3d(
				c_rSphereInstance.v3Position.x,
				c_rSphereInstance.v3Position.y,
				c_rSphereInstance.v3Position.z,
				c_rSphereInstance.fRadius);
		}
	}

	s_Screen.SetDiffuseColor(0.0f, 1.0f, 0.0f);
	itor = m_BodyPointInstanceList.begin();
	for (; itor != m_BodyPointInstanceList.end(); ++itor)
	{
		const TCollisionPointInstance& c_rInstance = *itor;

		for (DWORD i = 0; i < c_rInstance.SphereInstanceVector.size(); ++i)
		{
			const CDynamicSphereInstance& c_rSphereInstance = c_rInstance.SphereInstanceVector[i];

			s_Screen.RenderCircle3d(
				c_rSphereInstance.v3Position.x,
				c_rSphereInstance.v3Position.y,
				c_rSphereInstance.v3Position.z,
				c_rSphereInstance.fRadius);
		}
	}

	s_Screen.SetDiffuseColor(1.0f, 0.0f, 0.0f);

	{
		CDynamicSphereInstanceVector::iterator itor = m_kSplashArea.SphereInstanceVector.begin();

		for (; itor != m_kSplashArea.SphereInstanceVector.end(); ++itor)
		{
			const CDynamicSphereInstance& c_rInstance = *itor;

			s_Screen.RenderCircle3d(
				c_rInstance.v3Position.x,
				c_rInstance.v3Position.y,
				c_rInstance.v3Position.z,
				c_rInstance.fRadius);
		}
	}

	STATEMANAGER.GetStateCache().Restore();

	_mgr->GetCbMgr()->SetLightingEnable(true);
}


void CActorInstance::RenderToShadowMap()
{
	if (RENDER_MODE_BLEND == m_iRenderMode)
	if (GetAlphaValue() < 0.5f)
		return;

	CGraphicThingInstance::RenderToShadowMap();

	if (m_pkHorse)
		m_pkHorse->RenderToShadowMap();
}
