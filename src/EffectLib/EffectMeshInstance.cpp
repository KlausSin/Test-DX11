#include "StdAfx.h"
#include "EterLib/StateManager.h"
#include "EterLib/ResourceManager.h"
#include "EffectMeshInstance.h"
#include "Eterlib/GrpMath.h"

CDynamicPool<CEffectMeshInstance>		CEffectMeshInstance::ms_kPool;

void CEffectMeshInstance::DestroySystem()
{
	ms_kPool.Destroy();
}

CEffectMeshInstance* CEffectMeshInstance::New()
{
	return ms_kPool.Alloc();
}

void CEffectMeshInstance::Delete(CEffectMeshInstance* pkMeshInstance)
{
	pkMeshInstance->Destroy();
	ms_kPool.Free(pkMeshInstance);
}

BOOL CEffectMeshInstance::isActive()
{
	if (!CEffectElementBaseInstance::isActive())
		return FALSE;

	if (!m_MeshFrameController.isActive())
		return FALSE;

	for (DWORD j = 0; j < m_TextureInstanceVector.size(); ++j)
	{
		int iCurrentFrame = m_MeshFrameController.GetCurrentFrame();
		if (m_TextureInstanceVector[j].TextureFrameController.isActive(iCurrentFrame))
			return TRUE;
	}

	return FALSE;
}

bool CEffectMeshInstance::OnUpdate(float fElapsedTime)
{
	if (!isActive())
		return false;

	if (m_MeshFrameController.isActive())
		m_MeshFrameController.Update(fElapsedTime);

	for (DWORD j = 0; j < m_TextureInstanceVector.size(); ++j)
	{
		int iCurrentFrame = m_MeshFrameController.GetCurrentFrame();
		if (m_TextureInstanceVector[j].TextureFrameController.isActive(iCurrentFrame))
			m_TextureInstanceVector[j].TextureFrameController.Update(fElapsedTime);
	}

	return true;
}

void CEffectMeshInstance::OnRender()
{
	if (!isActive())
		return;

	CEffectMesh* pEffectMesh = m_roMesh.GetPointer();

	for (DWORD i = 0; i < pEffectMesh->GetMeshCount(); ++i)
	{
		assert(i < m_TextureInstanceVector.size());

		CFrameController& rTextureFrameController = m_TextureInstanceVector[i].TextureFrameController;
		if (!rTextureFrameController.isActive(m_MeshFrameController.GetCurrentFrame()))
			continue;

		int iBillboardType = m_pMeshScript->GetBillboardType(i);

		XMFLOAT4X4 m_matWorld;
		XMStoreFloat4x4(&m_matWorld, XMMatrixIdentity());

		switch (iBillboardType)
		{
		case MESH_BILLBOARD_TYPE_ALL:
		{
			XMVECTOR det;
			XMMATRIX matTemp = XMMatrixRotationX(XMConvertToRadians(90.0f));
			XMMATRIX matViewInv = XMMatrixInverse(&det, XMLoadFloat4x4(&CScreen::GetViewMatrix()));
			XMStoreFloat4x4(&m_matWorld, matTemp * matViewInv);
		}
		break;

		case MESH_BILLBOARD_TYPE_Y:
		{
			XMVECTOR det;
			XMFLOAT4X4 matTemp;
			XMStoreFloat4x4(&matTemp, XMMatrixInverse(&det, XMLoadFloat4x4(&CScreen::GetViewMatrix())));

			m_matWorld._11 = matTemp._11;
			m_matWorld._12 = matTemp._12;
			m_matWorld._21 = matTemp._21;
			m_matWorld._22 = matTemp._22;
		}
		break;

		case MESH_BILLBOARD_TYPE_MOVE:
		{
			XMFLOAT3 Position;
			XMFLOAT3 LastPosition;

			m_pMeshScript->GetPosition(m_fLocalTime, Position);
			m_pMeshScript->GetPosition(m_fLocalTime - CTimer::Instance().GetElapsedSecond(), LastPosition);

			Position.x -= LastPosition.x;
			Position.y -= LastPosition.y;
			Position.z -= LastPosition.z;

			if (XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&Position))) > 0.001f)
			{
				XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&Position));
				XMVECTOR from = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
				XMVECTOR q = XMQuaternionRotationNormal(XMVector3Normalize(XMVector3Cross(from, dir)), acosf(XMVectorGetX(XMVector3Dot(from, dir))));
				XMStoreFloat4x4(&m_matWorld, XMMatrixRotationQuaternion(q));
			}
		}
		break;
		}

		auto& state = STATEMANAGER.GetStateCache();

		if (!m_pMeshScript->isBlendingEnable(i))
		{
			state.Blend.SetBlendEnable(false);
		}
		else
		{
			int iBlendingSrcType = m_pMeshScript->GetBlendingSrcType(i);
			int iBlendingDestType = m_pMeshScript->GetBlendingDestType(i);

			state.Blend.SetBlendEnable(true);
			state.Blend.SetSrcBlend((D3D11_BLEND)iBlendingSrcType);
			state.Blend.SetDestBlend((D3D11_BLEND)iBlendingDestType);
		}

		XMFLOAT3 Position;
		m_pMeshScript->GetPosition(m_fLocalTime, Position);

		m_matWorld._41 = Position.x;
		m_matWorld._42 = Position.y;
		m_matWorld._43 = Position.z;

		XMStoreFloat4x4(&m_matWorld, XMLoadFloat4x4(&m_matWorld) * XMLoadFloat4x4(mc_pmatLocal));
		STATEMANAGER.GetTransform().SetWorld(m_matWorld);

		BYTE byType;
		XMFLOAT4 Color(1.0f, 1.0f, 1.0f, 1.0f);

		m_pMeshScript->GetColorOperationType(i, &byType);
		m_pMeshScript->GetColorFactor(i, &Color);

		TTimeEventTableFloat* TableAlpha;

		float fAlpha = 1.0f;
		if (m_pMeshScript->GetTimeTableAlphaPointer(i, &TableAlpha) && !TableAlpha->empty())
			fAlpha = GetTimeEventBlendValue(m_fLocalTime, *TableAlpha);

		CEffectMesh::TEffectMeshData* pMeshData = pEffectMesh->GetMeshDataPointer(i);

		assert(m_MeshFrameController.GetCurrentFrame() < pMeshData->EffectFrameDataVector.size());
		CEffectMesh::TEffectFrameData& rFrameData = pMeshData->EffectFrameDataVector[m_MeshFrameController.GetCurrentFrame()];

		DWORD dwcurTextureFrame = rTextureFrameController.GetCurrentFrame();
		if (dwcurTextureFrame < m_TextureInstanceVector[i].TextureInstanceVector.size())
		{
			CGraphicImageInstance* pImageInstance = m_TextureInstanceVector[i].TextureInstanceVector[dwcurTextureFrame];
			STATEMANAGER.SetTexture(0, pImageInstance->GetTexturePointer()->GetSRV());
		}

		Color.w = fAlpha * rFrameData.fVisibility;

		auto cb = _mgr->GetCbMgr();
		cb->SetTextureFactor(ColorToUint(Color));

		_mgr->SetShader(VF_EFFECT);
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, rFrameData.dwIndexCount / 3, sizeof(TPTVertex), &rFrameData.PDTVertexVector[0]);
	}
}

void CEffectMeshInstance::OnSetDataPointer(CEffectElementBase * pElement)
{
	CEffectMeshScript * pMesh = (CEffectMeshScript *)pElement;
	m_pMeshScript = pMesh;

	const char * c_szMeshFileName = pMesh->GetMeshFileName();

	m_pEffectMesh = CResourceManager::Instance().GetTyped<CEffectMesh>(c_szMeshFileName);

	if (!m_pEffectMesh)
		return;

	m_roMesh.SetPointer(m_pEffectMesh);

	m_MeshFrameController.Clear();
	m_MeshFrameController.SetMaxFrame(m_roMesh.GetPointer()->GetFrameCount());
	m_MeshFrameController.SetFrameTime(pMesh->GetMeshAnimationFrameDelay());
	m_MeshFrameController.SetLoopFlag(pMesh->isMeshAnimationLoop());
	m_MeshFrameController.SetLoopCount(pMesh->GetMeshAnimationLoopCount());
	m_MeshFrameController.SetStartFrame(0);

	m_TextureInstanceVector.clear();
	m_TextureInstanceVector.resize(m_pEffectMesh->GetMeshCount());
	for (DWORD j = 0; j < m_TextureInstanceVector.size(); ++j)
	{
		CEffectMeshScript::TMeshData * pMeshData;
		if (!m_pMeshScript->GetMeshDataPointer(j, &pMeshData))
			continue;
		
		CEffectMesh* pkEftMesh=m_roMesh.GetPointer();

		if (!pkEftMesh)
			continue;

		std::vector<CGraphicImage*>* pTextureVector = pkEftMesh->GetTextureVectorPointer(j);
		if (!pTextureVector)
			continue;

		std::vector<CGraphicImage*>& rTextureVector = *pTextureVector;

		CFrameController & rFrameController = m_TextureInstanceVector[j].TextureFrameController;
		rFrameController.Clear();
		rFrameController.SetMaxFrame(rTextureVector.size());
		rFrameController.SetFrameTime(pMeshData->fTextureAnimationFrameDelay);
		rFrameController.SetLoopFlag(pMeshData->bTextureAnimationLoopEnable);
		rFrameController.SetStartFrame(pMeshData->dwTextureAnimationStartFrame);

		std::vector<CGraphicImageInstance*> & rImageInstanceVector = m_TextureInstanceVector[j].TextureInstanceVector;
		rImageInstanceVector.clear();
		rImageInstanceVector.reserve(rTextureVector.size());
		for (std::vector<CGraphicImage*>::iterator itor = rTextureVector.begin(); itor != rTextureVector.end(); ++itor)
		{
			CGraphicImage * pImage = *itor;
			CGraphicImageInstance * pImageInstance = CGraphicImageInstance::ms_kPool.Alloc();
			pImageInstance->SetImagePointer(pImage);
			rImageInstanceVector.push_back(pImageInstance);
		}
	}
}

void CEffectMeshInstance_DeleteImageInstance(CGraphicImageInstance * pkInstance)
{
	CGraphicImageInstance::ms_kPool.Free(pkInstance);
}

void CEffectMeshInstance_DeleteTextureInstance(CEffectMeshInstance::TTextureInstance & rkInstance)
{
	std::vector<CGraphicImageInstance*> & rVector = rkInstance.TextureInstanceVector;
	for_each(rVector.begin(), rVector.end(), CEffectMeshInstance_DeleteImageInstance);
	rVector.clear();
}

void CEffectMeshInstance::OnInitialize()
{
}

void CEffectMeshInstance::OnDestroy()
{
	for_each(m_TextureInstanceVector.begin(), m_TextureInstanceVector.end(), CEffectMeshInstance_DeleteTextureInstance);
	m_TextureInstanceVector.clear();
	m_roMesh.SetPointer(NULL);
}

CEffectMeshInstance::CEffectMeshInstance()
{
	Initialize();
}

CEffectMeshInstance::~CEffectMeshInstance()
{
	Destroy();
}