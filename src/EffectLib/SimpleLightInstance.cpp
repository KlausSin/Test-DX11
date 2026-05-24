#include "Stdafx.h"
#include "EterLib/GrpLightManager.h"

#include "SimpleLightInstance.h"

CDynamicPool<CLightInstance> CLightInstance::ms_kPool;

void CLightInstance::DestroySystem()
{
	ms_kPool.Destroy();
}

CLightInstance* CLightInstance::New()
{
	return ms_kPool.Alloc();
}

void CLightInstance::Delete(CLightInstance* pkData)
{
	pkData->Destroy();
	ms_kPool.Free(pkData);
}

void CLightInstance::OnSetDataPointer(CEffectElementBase* pElement)
{
	Destroy();

	m_pData = ((CLightData*)pElement);

	m_iLoopCount = m_pData->GetLoopCount();

	CLightComponent Light;
	m_pData->InitializeLight(Light);
	CLightManager::Instance().RegisterLight(LIGHT_TYPE_DYNAMIC, &m_LightID, Light);
}

bool CLightInstance::OnUpdate(float fElapsedTime)
{
	if (!isActive())
	{
		Destroy();
		return false;
	}

	if (m_fLocalTime >= m_pData->GetDuration())
	{
		if (m_pData->isLoop() && --m_iLoopCount != 0)
		{
			if (m_iLoopCount < 0)
				m_iLoopCount = 0;
			m_fLocalTime -= m_pData->GetDuration();
		}
		else
		{
			Destroy();
			m_iLoopCount = 1;
			return false;
		}
		/*
		if (!m_pData->isLoop())
		{
			OnClear();
			return false;
		}
		m_fLocalTime -= m_pData->GetDuration();
		*/
	}

	CLight* pLight = CLightManager::Instance().GetLight(m_LightID);

	if (pLight)
	{
		pLight->SetAmbientColor(m_pData->m_cAmbient.x, m_pData->m_cAmbient.y, m_pData->m_cAmbient.z, m_pData->m_cAmbient.w);
		pLight->SetDiffuseColor(m_pData->m_cDiffuse.x, m_pData->m_cDiffuse.y, m_pData->m_cDiffuse.z, m_pData->m_cDiffuse.w);

		float fRange;
		m_pData->GetRange(m_fLocalTime, fRange);
		pLight->SetRange(fRange);

		XMFLOAT3 pos;

		m_pData->GetPosition(m_fLocalTime, pos);

		XMStoreFloat3(&pos, XMVector3TransformCoord(XMLoadFloat3(&pos), XMLoadFloat4x4(mc_pmatLocal)));

		pLight->SetPosition(pos.x, pos.y, pos.z);

	}

	return true;
}

void CLightInstance::OnRender()
{
	//OnUpdate(0);
}

void CLightInstance::OnInitialize()
{
	m_LightID = 0;
	m_dwRangeIndex = 0;
}

void CLightInstance::OnDestroy()
{
	if (m_LightID)
	{
		CLightManager::Instance().DeleteLight(m_LightID);
	}
}

CLightInstance::CLightInstance()
{
	Initialize();
}

CLightInstance::~CLightInstance()
{
	Destroy();
}
