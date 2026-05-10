#include "StdAfx.h"

#include "EterBase/Timer.h"
#include "EterLib/StateManager.h"
#include "EterLib/Camera.h"

#include "SpeedTreeForestDirectX.h"
#include "SpeedTreeConfig.h"
#include "qMin32Lib/ConstantBufferManager.h"
#include "qMin32Lib/DxManager.h"
//

CSpeedTreeForestDirectX::CSpeedTreeForestDirectX()
{
}

CSpeedTreeForestDirectX::~CSpeedTreeForestDirectX()
{
}

bool CSpeedTreeForestDirectX::SetRenderingDevice()
{
	const float c_afLightPosition[4] = { -0.707f, -0.300f, 0.707f, 0.0f };
	const float	c_afLightAmbient[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	const float	c_afLightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const float	c_afLightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	float afLight1[] =
	{
		c_afLightPosition[0], c_afLightPosition[1], c_afLightPosition[2],
		c_afLightDiffuse[0], c_afLightDiffuse[1], c_afLightDiffuse[2],
		c_afLightAmbient[0], c_afLightAmbient[1], c_afLightAmbient[2],
		c_afLightSpecular[0], c_afLightSpecular[1], c_afLightSpecular[2],
		c_afLightPosition[3],
		1.0f, 0.0f, 0.0f
	};

	CSpeedTreeRT::SetNumWindMatrices(c_nNumWindMatrices);
	CSpeedTreeRT::SetLightAttributes(0, afLight1);
	CSpeedTreeRT::SetLightState(0, true);
	return true;
}

void CSpeedTreeForestDirectX::UpdateCompundMatrix(const D3DXVECTOR3& c_rEyeVec, const D3DXMATRIX& c_rmatView, const D3DXMATRIX& c_rmatProj)
{
	D3DXMATRIX matBlendShader;
	D3DXMatrixMultiply(&matBlendShader, &c_rmatView, &c_rmatProj);

	float afDirection[3];
	afDirection[0] = matBlendShader.m[0][2];
	afDirection[1] = matBlendShader.m[1][2];
	afDirection[2] = matBlendShader.m[2][2];
	CSpeedTreeRT::SetCamera(c_rEyeVec, afDirection);

	if (_mgr)
		_mgr->GetCbMgr()->SetSpeedTreeCompoundMatrix(matBlendShader);
}

void CSpeedTreeForestDirectX::Render(unsigned long ulRenderBitVector)
{
	auto& state = STATEMANAGER.GetStateCache();
	auto cb = _mgr->GetCbMgr();

	UpdateSystem(CTimer::Instance().GetCurrentSecond());

	if (m_pMainTreeMap.empty())
		return;

	if (!(ulRenderBitVector & Forest_RenderToShadow) && !(ulRenderBitVector & Forest_RenderToMiniMap))
		UpdateCompundMatrix(CCameraManager::Instance().GetCurrentCamera()->GetEye(), ms_matView, ms_matProj);

#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
	cb->SetLightingEnable(true);
#else
	cb->SetLightingEnable(false);
#endif

	UINT uiCount = 0;
	for (auto& it : m_pMainTreeMap)
	{
		auto ppInstances = it.second->GetInstances(uiCount);
		for (auto& inst : ppInstances)
			inst->Advance();
	}

	cb->SetSpeedTreeLight(m_afLighting);
	cb->SetSpeedTreeFog(m_afFog);

	state.Push();

	state.Sampler.SetFilter(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	state.Sampler.SetAddressUV(1, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

	cb->SetAlphaTestEnable(true);
	state.Raster.SetCullMode(D3D11_CULL_NONE);

	if (ulRenderBitVector & Forest_RenderBranches)
	{
		_mgr->SetShader(VF_BRANCH);

		for (auto& it : m_pMainTreeMap)
		{
			auto pMainTree = it.second;
			auto ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupBranchForTreeType();

			for (auto& inst : ppInstances)
				if (inst->isShow())
					inst->RenderBranches();
		}
	}

	state.Raster.SetCullMode(D3D11_CULL_NONE);

	if (ulRenderBitVector & Forest_RenderFronds)
	{
		_mgr->SetShader(VF_BRANCH);

		for (auto& it : m_pMainTreeMap)
		{
			auto pMainTree = it.second;
			auto ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupFrondForTreeType();

			for (auto& inst : ppInstances)
				if (inst->isShow())
					inst->RenderFronds();
		}
	}

	if (ulRenderBitVector & Forest_RenderLeaves)
	{
		_mgr->SetShader(VF_LEAF);

		DWORD oldAlphaRef = cb->GetAlphaRef();

		if (ulRenderBitVector & Forest_RenderToShadow || ulRenderBitVector & Forest_RenderToMiniMap)
			cb->SetAlphaRef(0);

		for (auto& it : m_pMainTreeMap)
		{
			auto pMainTree = it.second;
			auto ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupLeafForTreeType();

			for (auto& inst : ppInstances)
				if (inst->isShow())
					inst->RenderLeaves();

			pMainTree->EndLeafForTreeType();
		}

		if (ulRenderBitVector & Forest_RenderToShadow || ulRenderBitVector & Forest_RenderToMiniMap)
			cb->SetAlphaRef(oldAlphaRef);
	}

	cb->SetLightingEnable(false);

	if (ulRenderBitVector & Forest_RenderBillboards)
	{
		for (auto& it : m_pMainTreeMap)
		{
			auto ppInstances = it.second->GetInstances(uiCount);

			for (auto& inst : ppInstances)
				if (inst->isShow())
					inst->RenderBillboards();
		}
	}

	state.Restore();
}
