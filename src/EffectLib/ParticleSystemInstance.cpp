#include "StdAfx.h"
#include "EterBase/Random.h"
#include "EterLib/StateManager.h"
#include "ParticleSystemData.h"
#include "ParticleSystemInstance.h"
#include "ParticleInstance.h"

CDynamicPool<CParticleSystemInstance>	CParticleSystemInstance::ms_kPool;

void CParticleSystemInstance::DestroySystem()
{
	ms_kPool.Destroy();

	CParticleInstance::DestroySystem();
	//CRayParticleInstance::DestroySystem();
}

CParticleSystemInstance* CParticleSystemInstance::New()
{
	return ms_kPool.Alloc();
}

void CParticleSystemInstance::Delete(CParticleSystemInstance* pkPSInst)
{
	pkPSInst->Destroy();
	ms_kPool.Free(pkPSInst);
}



DWORD CParticleSystemInstance::GetEmissionCount()
{
	return m_dwCurrentEmissionCount;
}

void CParticleSystemInstance::CreateParticles(float fElapsedTime)
{
	using namespace DirectX;

	float fEmissionCount;
	m_pEmitterProperty->GetEmissionCountPerSecond(m_fLocalTime, &fEmissionCount);

	float fCreatingValue = fEmissionCount * fElapsedTime + m_fEmissionResidue;
	int iCreatingCount = int(fCreatingValue);
	m_fEmissionResidue = fCreatingValue - iCreatingCount;

	int icurEmissionCount = GetEmissionCount();
	int iMaxEmissionCount = int(m_pEmitterProperty->GetMaxEmissionCount());
	int iNextEmissionCount = int(icurEmissionCount + iCreatingCount);
	iCreatingCount -= std::max(0, iNextEmissionCount - iMaxEmissionCount);

	float fLifeTime = 0.0f;
	float fEmittingSize = 0.0f;
	XMFLOAT3 _v3TimePosition;
	XMFLOAT3 _v3Velocity;
	float fVelocity = 0.0f;
	XMFLOAT2 v2HalfSize;
	float fLieRotation = 0.0f;

	if (iCreatingCount)
	{
		m_pEmitterProperty->GetParticleLifeTime(m_fLocalTime, &fLifeTime);
		if (fLifeTime == 0.0f)
			return;

		m_pEmitterProperty->GetEmittingSize(m_fLocalTime, &fEmittingSize);
		m_pData->GetPosition(m_fLocalTime, _v3TimePosition);

		m_pEmitterProperty->GetEmittingDirectionX(m_fLocalTime, &_v3Velocity.x);
		m_pEmitterProperty->GetEmittingDirectionY(m_fLocalTime, &_v3Velocity.y);
		m_pEmitterProperty->GetEmittingDirectionZ(m_fLocalTime, &_v3Velocity.z);

		m_pEmitterProperty->GetEmittingVelocity(m_fLocalTime, &fVelocity);

		m_pEmitterProperty->GetParticleSizeX(m_fLocalTime, &v2HalfSize.x);
		m_pEmitterProperty->GetParticleSizeY(m_fLocalTime, &v2HalfSize.y);

		if (BILLBOARD_TYPE_LIE == m_pParticleProperty->m_byBillboardType && mc_pmatLocal)
		{
			float fsx = mc_pmatLocal->_32;
			float fcx = sqrtf(1.0f - fsx * fsx);

			if (fcx >= 0.00001f)
				fLieRotation = XMConvertToDegrees(atan2f(-mc_pmatLocal->_12, mc_pmatLocal->_22));
		}
	}

	for (int i = 0; i < iCreatingCount; ++i)
	{
		CParticleInstance* pInstance = CParticleInstance::New();
		pInstance->m_pParticleProperty = m_pParticleProperty;
		pInstance->m_pEmitterProperty = m_pEmitterProperty;

		pInstance->m_fLifeTime = fLifeTime;
		pInstance->m_fLastLifeTime = fLifeTime;

		switch (m_pEmitterProperty->GetEmitterShape())
		{
		case CEmitterProperty::EMITTER_SHAPE_POINT:
			pInstance->m_v3Position = { 0.0f, 0.0f, 0.0f };
			break;

		case CEmitterProperty::EMITTER_SHAPE_ELLIPSE:
		{
			pInstance->m_v3Position = { frandom(-500.0f, 500.0f), frandom(-500.0f, 500.0f), 0.0f };
			XMStoreFloat3(&pInstance->m_v3Position, XMVector3Normalize(XMLoadFloat3(&pInstance->m_v3Position)));

			float scale = m_pEmitterProperty->isEmitFromEdge()
				? m_pEmitterProperty->m_fEmittingRadius + fEmittingSize
				: frandom(0.0f, m_pEmitterProperty->m_fEmittingRadius) + fEmittingSize;

			pInstance->m_v3Position.x *= scale;
			pInstance->m_v3Position.y *= scale;
			pInstance->m_v3Position.z *= scale;
		}
		break;

		case CEmitterProperty::EMITTER_SHAPE_SQUARE:
			pInstance->m_v3Position.x = frandom(-m_pEmitterProperty->m_v3EmittingSize.x / 2.0f, m_pEmitterProperty->m_v3EmittingSize.x / 2.0f) + fEmittingSize;
			pInstance->m_v3Position.y = frandom(-m_pEmitterProperty->m_v3EmittingSize.y / 2.0f, m_pEmitterProperty->m_v3EmittingSize.y / 2.0f) + fEmittingSize;
			pInstance->m_v3Position.z = frandom(-m_pEmitterProperty->m_v3EmittingSize.z / 2.0f, m_pEmitterProperty->m_v3EmittingSize.z / 2.0f) + fEmittingSize;
			break;

		case CEmitterProperty::EMITTER_SHAPE_SPHERE:
		{
			pInstance->m_v3Position = { frandom(-500.0f, 500.0f), frandom(-500.0f, 500.0f), frandom(-500.0f, 500.0f) };
			XMStoreFloat3(&pInstance->m_v3Position, XMVector3Normalize(XMLoadFloat3(&pInstance->m_v3Position)));

			float scale = m_pEmitterProperty->isEmitFromEdge()
				? m_pEmitterProperty->m_fEmittingRadius + fEmittingSize
				: frandom(0.0f, m_pEmitterProperty->m_fEmittingRadius) + fEmittingSize;

			pInstance->m_v3Position.x *= scale;
			pInstance->m_v3Position.y *= scale;
			pInstance->m_v3Position.z *= scale;
		}
		break;
		}

		XMFLOAT3 v3TimePosition = _v3TimePosition;

		pInstance->m_v3Position.x += v3TimePosition.x;
		pInstance->m_v3Position.y += v3TimePosition.y;
		pInstance->m_v3Position.z += v3TimePosition.z;

		if (mc_pmatLocal && !m_pParticleProperty->m_bAttachFlag)
		{
			XMStoreFloat3(&pInstance->m_v3Position, XMVector3TransformCoord(XMLoadFloat3(&pInstance->m_v3Position), XMLoadFloat4x4(mc_pmatLocal)));
			XMStoreFloat3(&v3TimePosition, XMVector3TransformCoord(XMLoadFloat3(&v3TimePosition), XMLoadFloat4x4(mc_pmatLocal)));
		}

		pInstance->m_v3StartPosition = v3TimePosition;

		pInstance->m_v3Velocity = { 0.0f, 0.0f, 0.0f };

		if (CEmitterProperty::EMITTER_ADVANCED_TYPE_INNER == m_pEmitterProperty->GetEmitterAdvancedType())
		{
			XMFLOAT3 dir = {
				pInstance->m_v3Position.x - v3TimePosition.x,
				pInstance->m_v3Position.y - v3TimePosition.y,
				pInstance->m_v3Position.z - v3TimePosition.z
			};

			XMStoreFloat3(&pInstance->m_v3Velocity, XMVector3Normalize(XMLoadFloat3(&dir)));
			pInstance->m_v3Velocity.x *= -100.0f;
			pInstance->m_v3Velocity.y *= -100.0f;
			pInstance->m_v3Velocity.z *= -100.0f;
		}
		else if (CEmitterProperty::EMITTER_ADVANCED_TYPE_OUTER == m_pEmitterProperty->GetEmitterAdvancedType())
		{
			if (m_pEmitterProperty->GetEmitterShape() == CEmitterProperty::EMITTER_SHAPE_POINT)
			{
				pInstance->m_v3Velocity = { frandom(-100.0f, 100.0f), frandom(-100.0f, 100.0f), frandom(-100.0f, 100.0f) };
			}
			else
			{
				XMFLOAT3 dir = {
					pInstance->m_v3Position.x - v3TimePosition.x,
					pInstance->m_v3Position.y - v3TimePosition.y,
					pInstance->m_v3Position.z - v3TimePosition.z
				};

				XMStoreFloat3(&pInstance->m_v3Velocity, XMVector3Normalize(XMLoadFloat3(&dir)));
				pInstance->m_v3Velocity.x *= 100.0f;
				pInstance->m_v3Velocity.y *= 100.0f;
				pInstance->m_v3Velocity.z *= 100.0f;
			}
		}

		XMFLOAT3 v3Velocity = _v3Velocity;

		if (mc_pmatLocal && !m_pParticleProperty->m_bAttachFlag)
			XMStoreFloat3(&v3Velocity, XMVector3TransformNormal(XMLoadFloat3(&v3Velocity), XMLoadFloat4x4(mc_pmatLocal)));

		pInstance->m_v3Velocity.x += v3Velocity.x;
		pInstance->m_v3Velocity.y += v3Velocity.y;
		pInstance->m_v3Velocity.z += v3Velocity.z;

		if (m_pEmitterProperty->m_v3EmittingDirection.x > 0.0f)
			pInstance->m_v3Velocity.x += frandom(-m_pEmitterProperty->m_v3EmittingDirection.x / 2.0f, m_pEmitterProperty->m_v3EmittingDirection.x / 2.0f) * 1000.0f;
		if (m_pEmitterProperty->m_v3EmittingDirection.y > 0.0f)
			pInstance->m_v3Velocity.y += frandom(-m_pEmitterProperty->m_v3EmittingDirection.y / 2.0f, m_pEmitterProperty->m_v3EmittingDirection.y / 2.0f) * 1000.0f;
		if (m_pEmitterProperty->m_v3EmittingDirection.z > 0.0f)
			pInstance->m_v3Velocity.z += frandom(-m_pEmitterProperty->m_v3EmittingDirection.z / 2.0f, m_pEmitterProperty->m_v3EmittingDirection.z / 2.0f) * 1000.0f;

		pInstance->m_v3Velocity.x *= fVelocity;
		pInstance->m_v3Velocity.y *= fVelocity;
		pInstance->m_v3Velocity.z *= fVelocity;

		pInstance->m_v2HalfSize = v2HalfSize;

		pInstance->m_fRotation = frandom(m_pParticleProperty->m_wRotationRandomStartingBegin, m_pParticleProperty->m_wRotationRandomStartingEnd);

		if (BILLBOARD_TYPE_LIE == m_pParticleProperty->m_byBillboardType && mc_pmatLocal)
			pInstance->m_fRotation += fLieRotation;

		pInstance->m_byFrameIndex = 0;
		pInstance->m_byTextureAnimationType = m_pParticleProperty->GetTextureAnimationType();

		if (m_pParticleProperty->GetTextureAnimationFrameCount() > 1)
		{
			if (CParticleProperty::TEXTURE_ANIMATION_TYPE_RANDOM_DIRECTION == m_pParticleProperty->GetTextureAnimationType())
			{
				if (random() & 1)
				{
					pInstance->m_byFrameIndex = 0;
					pInstance->m_byTextureAnimationType = CParticleProperty::TEXTURE_ANIMATION_TYPE_CW;
				}
				else
				{
					pInstance->m_byFrameIndex = m_pParticleProperty->GetTextureAnimationFrameCount() - 1;
					pInstance->m_byTextureAnimationType = CParticleProperty::TEXTURE_ANIMATION_TYPE_CCW;
				}
			}

			if (m_pParticleProperty->m_bTexAniRandomStartFrameFlag)
				pInstance->m_byFrameIndex = random_range(0, m_pParticleProperty->GetTextureAnimationFrameCount() - 1);
		}

		pInstance->m_v3LastPosition = {
			pInstance->m_v3Position.x - pInstance->m_v3Velocity.x * fElapsedTime,
			pInstance->m_v3Position.y - pInstance->m_v3Velocity.y * fElapsedTime,
			pInstance->m_v3Position.z - pInstance->m_v3Velocity.z * fElapsedTime
		};

		pInstance->m_v2Scale.x = m_pParticleProperty->m_TimeEventScaleX.front().m_Value;
		pInstance->m_v2Scale.y = m_pParticleProperty->m_TimeEventScaleY.front().m_Value;
		pInstance->m_Color = m_pParticleProperty->m_TimeEventColor.front().m_Value;

		m_ParticleInstanceListVector[pInstance->m_byFrameIndex].push_back(pInstance);
		m_dwCurrentEmissionCount++;
	}
}

bool CParticleSystemInstance::OnUpdate(float fElapsedTime)
{
	using namespace DirectX;

	bool bMakeParticle = true;

	if (m_fLocalTime >= m_pEmitterProperty->GetCycleLength())
	{
		if (m_pEmitterProperty->isCycleLoop() && --m_iLoopCount != 0)
		{
			if (m_iLoopCount < 0)
				m_iLoopCount = 0;

			m_fLocalTime = m_fLocalTime - m_pEmitterProperty->GetCycleLength();
		}
		else
		{
			bMakeParticle = false;
			m_iLoopCount = 1;

			if (GetEmissionCount() == 0)
				return false;
		}
	}

	int dwFrameIndex;
	int dwFrameCount = m_pParticleProperty->GetTextureAnimationFrameCount();

	float fAngularVelocity;
	m_pEmitterProperty->GetEmittingAngularVelocity(m_fLocalTime, &fAngularVelocity);

	if (fAngularVelocity && !m_pParticleProperty->m_bAttachFlag)
	{
		XMFLOAT3 zAxis = { 0.0f, 0.0f, 1.0f };
		XMStoreFloat3(&m_pParticleProperty->m_v3ZAxis, XMVector3TransformNormal(XMLoadFloat3(&zAxis), XMLoadFloat4x4(mc_pmatLocal)));
	}

	for (dwFrameIndex = 0; dwFrameIndex < dwFrameCount; dwFrameIndex++)
	{
		TParticleInstanceList::iterator itor = m_ParticleInstanceListVector[dwFrameIndex].begin();

		for (; itor != m_ParticleInstanceListVector[dwFrameIndex].end();)
		{
			CParticleInstance* pInstance = *itor;

			if (!pInstance->Update(fElapsedTime, fAngularVelocity)) [[unlikely]]
			{
				pInstance->DeleteThis();

				itor = m_ParticleInstanceListVector[dwFrameIndex].erase(itor);
				m_dwCurrentEmissionCount--;
			}
			else [[likely]]
			{
				if (pInstance->m_byFrameIndex != dwFrameIndex)
				{
					m_ParticleInstanceListVector[dwFrameCount + pInstance->m_byFrameIndex].push_back(*itor);
					itor = m_ParticleInstanceListVector[dwFrameIndex].erase(itor);
				}
				else
				{
					++itor;
				}
			}
		}
	}

	if (isActive() && bMakeParticle)
		CreateParticles(fElapsedTime);

	for (dwFrameIndex = 0; dwFrameIndex < dwFrameCount; ++dwFrameIndex)
	{
		m_ParticleInstanceListVector[dwFrameIndex].splice(
			m_ParticleInstanceListVector[dwFrameIndex].end(),
			m_ParticleInstanceListVector[dwFrameIndex + dwFrameCount]);

		m_ParticleInstanceListVector[dwFrameIndex + dwFrameCount].clear();
	}

	return true;
}

namespace NParticleRenderer
{
	struct TwoSideRenderer
	{
		const XMFLOAT4X4* pmat;

		TwoSideRenderer(const XMFLOAT4X4* pmat = nullptr)
			: pmat(pmat)
		{
		}

		inline void operator () (CParticleInstance* pInstance)
		{
			pInstance->Transform(pmat, XMConvertToRadians(-30.0f));
			STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, sizeof(TPTVertex), pInstance->GetParticleMeshPointer());

			pInstance->Transform(pmat, XMConvertToRadians(+30.0f));
			STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, sizeof(TPTVertex), pInstance->GetParticleMeshPointer());
		}
	};

	struct ThreeSideRenderer
	{
		const XMFLOAT4X4* pmat;

		ThreeSideRenderer(const XMFLOAT4X4* pmat = nullptr)
			: pmat(pmat)
		{
		}

		inline void operator () (CParticleInstance* pInstance)
		{
			pInstance->Transform(pmat);
			STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, sizeof(TPTVertex), pInstance->GetParticleMeshPointer());

			pInstance->Transform(pmat, XMConvertToRadians(-60.0f));
			STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, sizeof(TPTVertex), pInstance->GetParticleMeshPointer());

			pInstance->Transform(pmat, XMConvertToRadians(+60.0f));
			STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, sizeof(TPTVertex), pInstance->GetParticleMeshPointer());
		}
	};

	struct NormalRenderer
	{
		inline void operator () (CParticleInstance* pInstance)
		{
			pInstance->Transform();
			STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, sizeof(TPTVertex), pInstance->GetParticleMeshPointer());
		}
	};

	struct AttachRenderer
	{
		const XMFLOAT4X4* pmat;

		AttachRenderer(const XMFLOAT4X4* pmat)
			: pmat(pmat)
		{
		}

		inline void operator () (CParticleInstance* pInstance)
		{
			pInstance->Transform(pmat);
			STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, sizeof(TPTVertex), pInstance->GetParticleMeshPointer());
		}
	};
}

void CParticleSystemInstance::OnRender()
{
	CScreen::Identity();

	auto& state = STATEMANAGER.GetStateCache();

	state.Blend.SetSrcBlend((D3D11_BLEND)m_pParticleProperty->m_bySrcBlendType);
	state.Blend.SetDestBlend((D3D11_BLEND)m_pParticleProperty->m_byDestBlendType);

	if (m_pParticleProperty->m_byBillboardType < BILLBOARD_TYPE_2FACE)
	{
		if (!m_pParticleProperty->m_bAttachFlag)
		{
			auto obj = NParticleRenderer::NormalRenderer();
			ForEachParticleRendering(obj);
		}
		else
		{
			auto obj = NParticleRenderer::AttachRenderer(mc_pmatLocal);
			ForEachParticleRendering(obj);
		}
	}
	else if (m_pParticleProperty->m_byBillboardType == BILLBOARD_TYPE_2FACE)
	{
		if (!m_pParticleProperty->m_bAttachFlag)
		{
			auto obj = NParticleRenderer::TwoSideRenderer();
			ForEachParticleRendering(obj);
		}
		else
		{
			auto obj = NParticleRenderer::TwoSideRenderer(mc_pmatLocal);
			ForEachParticleRendering(obj);
		}
	}
	else if (m_pParticleProperty->m_byBillboardType == BILLBOARD_TYPE_3FACE)
	{
		if (!m_pParticleProperty->m_bAttachFlag)
		{
			auto obj = NParticleRenderer::ThreeSideRenderer();
			ForEachParticleRendering(obj);
		}
		else
		{
			auto obj = NParticleRenderer::ThreeSideRenderer(mc_pmatLocal);
			ForEachParticleRendering(obj);
		}
	}
}

void CParticleSystemInstance::OnSetDataPointer(CEffectElementBase * pElement)
{
	m_pData = (CParticleSystemData *)pElement;

	m_dwCurrentEmissionCount = 0;
	m_pParticleProperty = m_pData->GetParticlePropertyPointer();
	m_pEmitterProperty = m_pData->GetEmitterPropertyPointer();
	m_iLoopCount = m_pEmitterProperty->GetLoopCount();
	m_ParticleInstanceListVector.resize(m_pParticleProperty->GetTextureAnimationFrameCount()*2+2);

	/////

	assert(m_kVct_pkImgInst.empty());
	m_kVct_pkImgInst.reserve(m_pParticleProperty->m_ImageVector.size());
	for (DWORD i = 0; i < m_pParticleProperty->m_ImageVector.size(); ++i)
	{
		CGraphicImage * pImage = m_pParticleProperty->m_ImageVector[i];

		CGraphicImageInstance* pkImgInstNew = CGraphicImageInstance::New();
		pkImgInstNew->SetImagePointer(pImage);
		m_kVct_pkImgInst.push_back(pkImgInstNew);
	}
}

void CParticleSystemInstance::OnInitialize()
{
	m_dwCurrentEmissionCount = 0;
	m_iLoopCount = 0;
	m_fEmissionResidue = 0.0f;
}

void CParticleSystemInstance::OnDestroy()
{
	// 2004. 3. 1. myevan. 파티클 제거 루틴
	TParticleInstanceListVector::iterator i;
	for(i = m_ParticleInstanceListVector.begin(); i!=m_ParticleInstanceListVector.end(); ++i)
	{
		TParticleInstanceList& rkLst_kParticleInst=*i;

		TParticleInstanceList::iterator j;
		for(j = rkLst_kParticleInst.begin(); j!=rkLst_kParticleInst.end(); ++j)
		{
			CParticleInstance* pkParticleInst=*j;
			pkParticleInst->DeleteThis();
		}

		rkLst_kParticleInst.clear();	
	}
	m_ParticleInstanceListVector.clear();

	std::for_each(m_kVct_pkImgInst.begin(), m_kVct_pkImgInst.end(), CGraphicImageInstance::Delete);
	m_kVct_pkImgInst.clear();
}

CParticleSystemInstance::CParticleSystemInstance()
{
	Initialize();
}

CParticleSystemInstance::~CParticleSystemInstance()
{
	assert(m_ParticleInstanceListVector.empty());
	assert(m_kVct_pkImgInst.empty());
}
