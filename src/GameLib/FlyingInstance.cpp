#include "Stdafx.h"
#include "EterLib/GrpMath.h"
#include "EffectLib/EffectManager.h"

#include "MapManager.h"

#include "FlyingData.h"
#include "FlyTrace.h"
#include "FlyingInstance.h"
#include "FlyingObjectManager.h"
#include "FlyTarget.h"
#include "FlyHandler.h"

CDynamicPool<CFlyingInstance> CFlyingInstance::ms_kPool;

CFlyingInstance::CFlyingInstance()
{
	__Initialize();
}

CFlyingInstance::~CFlyingInstance()
{
	Destroy();
}

void CFlyingInstance::__Initialize()
{
	m_qAttachRotation = m_qRot = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

	m_v3Accel = m_v3LocalVelocity = m_v3Velocity = m_v3Position =
		XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_pHandler = NULL;
	m_pData = NULL;
	m_pOwner = NULL;

	m_bAlive = false;
	m_canAttack = false;

	m_dwSkillIndex = 0;
	m_iPierceCount = 0;

	m_fStartTime = 0.0f;
	m_fRemainRange = 0.0f;

	m_bTargetHitted = FALSE;
	m_HittedObjectSet.clear();
}

void CFlyingInstance::Clear()
{
	Destroy();
}

void CFlyingInstance::Destroy()
{
	m_FlyTarget.Clear();

	ClearAttachInstance();

	__Initialize();
}


void CFlyingInstance::BuildAttachInstance()
{
	for (int i = 0; i < m_pData->GetAttachDataCount(); i++)
	{
		CFlyingData::TFlyingAttachData& rfad = m_pData->GetAttachDataReference(i);

		switch (rfad.iType)
		{
		case CFlyingData::FLY_ATTACH_OBJECT:
			// NOT Implemented
			assert(!"FlyingInstance.cpp:BuildAttachInstance Not implemented FLY_ATTACH_OBJECT");
			break;
		case CFlyingData::FLY_ATTACH_EFFECT:
		{
			CEffectManager& rem = CEffectManager::Instance();
			TAttachEffectInstance aei;

			DWORD dwCRC = GetCaseCRC32(rfad.strFilename.c_str(), rfad.strFilename.size());

			aei.iAttachIndex = i;
			aei.dwEffectInstanceIndex = rem.GetEmptyIndex();

			aei.pFlyTrace = NULL;
			if (rfad.bHasTail)
			{
				aei.pFlyTrace = CFlyTrace::New();
				aei.pFlyTrace->Create(rfad);
			}
			rem.CreateEffectInstance(aei.dwEffectInstanceIndex, dwCRC);

			m_vecAttachEffectInstance.push_back(aei);
		}
		break;
		}
	}
}

void CFlyingInstance::Create(CFlyingData* pData, const XMFLOAT3& c_rv3StartPos, const CFlyTarget& c_rkTarget, bool canAttack)
{
	m_FlyTarget = c_rkTarget;
	m_canAttack = canAttack;

	__SetDataPointer(pData, c_rv3StartPos);
	__SetTargetDirection(m_FlyTarget);
}

void CFlyingInstance::__SetTargetDirection(const CFlyTarget& c_rkTarget)
{
	XMFLOAT3 v3TargetPos = c_rkTarget.GetFlyTargetPosition();

	if (m_pData->m_bMaintainParallel)
		v3TargetPos.z += 50.0f;

	XMFLOAT3 v3TargetDir;
	XMStoreFloat3(&v3TargetDir,
		XMVector3Normalize(XMLoadFloat3(&v3TargetPos) - XMLoadFloat3(&m_v3Position)));

	__SetTargetNormalizedDirection(v3TargetDir);
}

void CFlyingInstance::__SetTargetNormalizedDirection(const XMFLOAT3& v3NormalizedDirection)
{
	XMFLOAT4 q = SafeRotationNormalizedArc(
		XMFLOAT3(0.0f, -1.0f, 0.0f),
		v3NormalizedDirection
	);

	XMStoreFloat4(&m_qRot,
		XMQuaternionMultiply(XMLoadFloat4(&m_qRot), XMLoadFloat4(&q)));

	XMVECTOR qv = XMLoadFloat4(&q);

	XMStoreFloat3(&m_v3Velocity,
		XMVector3Rotate(XMLoadFloat3(&m_v3LocalVelocity), qv));

	XMStoreFloat3(&m_v3Accel,
		XMVector3Rotate(XMLoadFloat3(&m_pData->m_v3Accel), qv));
}

// 2004. 3. 26. myevan. 기능을 몰라 일단 주석 처리. 적절한 네이밍이 필요. 게임에서 사용하지 않는다면 툴에서 툴 전용으로 상속받아 만들도록 하자
void CFlyingInstance::SetFlyTarget(const CFlyTarget& cr_Target)
{
	//m_pFlyTarget = pTarget;
	m_FlyTarget = cr_Target;
	//SetStartTargetPosition(m_FlyTarget.GetFlyTargetPosition());

	__SetTargetDirection(m_FlyTarget);
}

void CFlyingInstance::AdjustDirectionForHoming(const XMFLOAT3& v3TargetPosition)
{
	XMFLOAT3 vTargetDir;
	XMStoreFloat3(&vTargetDir,
		XMVector3Normalize(XMLoadFloat3(&v3TargetPosition) - XMLoadFloat3(&m_v3Position)));

	XMFLOAT3 vVel;
	XMStoreFloat3(&vVel, XMVector3Normalize(XMLoadFloat3(&m_v3Velocity)));

	XMFLOAT3 vv1;
	XMStoreFloat3(&vv1, XMLoadFloat3(&vVel) - XMLoadFloat3(&vTargetDir));

	if (XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&vv1))) < 0.001f)
		return;

	XMFLOAT4 q = SafeRotationNormalizedArc(vVel, vTargetDir);

	if (m_pData->m_fHomingMaxAngle > 180.0f)
	{
		XMVECTOR qv = XMLoadFloat4(&q);

		XMStoreFloat3(&m_v3Velocity,
			XMVector3Rotate(XMLoadFloat3(&m_v3Velocity), qv));

		XMStoreFloat3(&m_v3Accel,
			XMVector3Rotate(XMLoadFloat3(&m_v3Accel), qv));

		XMStoreFloat4(&m_qRot,
			XMQuaternionMultiply(qv, XMLoadFloat4(&m_qRot)));

		return;
	}

	float c = cosf(XMConvertToRadians(m_pData->m_fHomingMaxAngle));
	float s = sinf(XMConvertToRadians(m_pData->m_fHomingMaxAngle));

	if (q.w <= -1.0f + 0.0001f)
	{
		q.x = 0.0f;
		q.y = 0.0f;
		q.z = s;
		q.w = c;
	}
	else if (q.w <= c && q.w <= 1.0f - 0.0001f)
	{
		float factor = s / sqrtf(1.0f - q.w * q.w);
		q.x *= factor;
		q.y *= factor;
		q.z *= factor;
		q.w = c;
	}

	XMVECTOR qClamped = XMLoadFloat4(&q);

	XMStoreFloat3(&m_v3Velocity,
		XMVector3Rotate(XMLoadFloat3(&m_v3Velocity), qClamped));

	XMStoreFloat3(&m_v3Accel,
		XMVector3Rotate(XMLoadFloat3(&m_v3Accel), qClamped));

	XMStoreFloat4(&m_qRot,
		XMQuaternionMultiply(XMLoadFloat4(&m_qRot), qClamped));
}

void CFlyingInstance::UpdateAttachInstance()
{
	XMVECTOR dt = XMVectorReplicate(CTimer::Instance().GetElapsedSecond());

	XMVECTOR angVel = XMLoadFloat3(&m_pData->m_v3AngVel);
	XMVECTOR qStep = XMQuaternionRotationRollPitchYawFromVector(angVel * dt);

	XMStoreFloat4(&m_qAttachRotation,
		XMQuaternionMultiply(XMLoadFloat4(&m_qAttachRotation), qStep));

	XMVECTOR q = XMQuaternionMultiply(XMLoadFloat4(&m_qAttachRotation), XMLoadFloat4(&m_qRot));

	CEffectManager& rem = CEffectManager::Instance();

	for (auto it = m_vecAttachEffectInstance.begin(); it != m_vecAttachEffectInstance.end(); ++it)
	{
		auto& rfad = m_pData->GetAttachDataReference(it->iAttachIndex);
		assert(rfad.iType == CFlyingData::FLY_ATTACH_EFFECT);

		rem.SelectEffectInstance(it->dwEffectInstanceIndex);

		XMFLOAT4X4 m;

		switch (rfad.iFlyType)
		{
		case CFlyingData::FLY_ATTACH_TYPE_LINE:
		{
			XMStoreFloat4x4(&m, XMMatrixRotationQuaternion(q));
			m._41 = m_v3Position.x;
			m._42 = m_v3Position.y;
			m._43 = m_v3Position.z;
		}
		break;

		case CFlyingData::FLY_ATTACH_TYPE_MULTI_LINE:
		{
			XMFLOAT3 p(
				-sinf(XMConvertToRadians(rfad.fRoll)) * rfad.fDistance,
				0.0f,
				-cosf(XMConvertToRadians(rfad.fRoll)) * rfad.fDistance
			);

			XMStoreFloat3(&p, XMVector3Rotate(XMLoadFloat3(&p), q));

			XMStoreFloat3(&p, XMLoadFloat3(&p) + XMLoadFloat3(&m_v3Position));

			XMStoreFloat4x4(&m, XMMatrixRotationQuaternion(q));
			m._41 = p.x;
			m._42 = p.y;
			m._43 = p.z;
		}
		break;

		case CFlyingData::FLY_ATTACH_TYPE_SINE:
		{
			float angle = (CTimer::Instance().GetCurrentSecond() - m_fStartTime)
				* 2.0f * XM_PI / rfad.fPeriod;

			XMFLOAT3 p(
				-sinf(XMConvertToRadians(rfad.fRoll)) * rfad.fAmplitude * sinf(angle),
				0.0f,
				-cosf(XMConvertToRadians(rfad.fRoll)) * rfad.fAmplitude * sinf(angle)
			);

			XMStoreFloat3(&p, XMVector3Rotate(XMLoadFloat3(&p), q));
			XMStoreFloat3(&p, XMLoadFloat3(&p) + XMLoadFloat3(&m_v3Position));

			XMStoreFloat4x4(&m, XMMatrixRotationQuaternion(q));
			m._41 = p.x;
			m._42 = p.y;
			m._43 = p.z;
		}
		break;

		case CFlyingData::FLY_ATTACH_TYPE_EXP:
		{
			float dtSec = CTimer::Instance().GetCurrentSecond() - m_fStartTime;
			float angle = dtSec / rfad.fPeriod;

			XMFLOAT3 p(
				-sinf(XMConvertToRadians(rfad.fRoll)) * rfad.fAmplitude * expf(-angle) * angle,
				0.0f,
				-cosf(XMConvertToRadians(rfad.fRoll)) * rfad.fAmplitude * expf(-angle) * angle
			);

			XMStoreFloat3(&p, XMVector3Rotate(XMLoadFloat3(&p), q));
			XMStoreFloat3(&p, XMLoadFloat3(&p) + XMLoadFloat3(&m_v3Position));

			XMStoreFloat4x4(&m, XMMatrixRotationQuaternion(q));
			m._41 = p.x;
			m._42 = p.y;
			m._43 = p.z;
		}
		break;
		}

		rem.SetEffectInstanceGlobalMatrix(m);

		if (it->pFlyTrace)
			it->pFlyTrace->UpdateNewPosition(XMFLOAT3(m._41, m._42, m._43));
	}
}

struct FCheckBackgroundDuringFlying {
	CDynamicSphereInstance s;
	bool bHit;
	FCheckBackgroundDuringFlying(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		s.fRadius = 1.0f;
		s.v3LastPosition = v1;
		s.v3Position = v2;
		bHit = false;
	}
	void operator () (CGraphicObjectInstance* p)
	{
		if (!p)
			return;

		if (!bHit && p->GetType() != ACTOR_OBJECT)
		{
			if (p->CollisionComponent().CollisionDynamicSphere(s))
			{
				bHit = true;
			}
		}
	}
	bool IsHitted()
	{
		return bHit;
	}
};

struct FCheckAnotherMonsterDuringFlying {
	CDynamicSphereInstance s;
	CGraphicObjectInstance* pInst;
	const IActorInstance* pOwner;
	FCheckAnotherMonsterDuringFlying(const IActorInstance* pOwner, const XMFLOAT3& v1, const XMFLOAT3& v2)
		: pOwner(pOwner)
	{
		s.fRadius = 10.0f;
		s.v3LastPosition = v1;
		s.v3Position = v2;
		pInst = 0;
	}
	void operator () (CGraphicObjectInstance* p)
	{
		if (!p)
			return;

		if (!pInst && p->GetType() == ACTOR_OBJECT)
		{
			IActorInstance* pa = (IActorInstance*)p;
			if (pa != pOwner && pa->TestCollisionWithDynamicSphere(s))
			{
				pInst = p;
			}
		}
	}
	bool IsHitted()
	{
		return pInst != 0;
	}
	CGraphicObjectInstance* GetHittedObject()
	{
		return pInst;
	}
};


bool CFlyingInstance::Update()
{
	if (!m_bAlive)
		return false;

	if (m_pData->m_bIsHoming &&
		m_pData->m_fHomingStartTime + m_fStartTime < CTimer::Instance().GetCurrentSecond())
	{
		if (m_FlyTarget.IsObject())
			AdjustDirectionForHoming(m_FlyTarget.GetFlyTargetPosition());
	}

	XMFLOAT3 v3LastPosition = m_v3Position;

	XMVECTOR vel = XMLoadFloat3(&m_v3Velocity);
	XMVECTOR accel = XMLoadFloat3(&m_v3Accel);

	float dt = CTimer::Instance().GetElapsedSecond();

	vel = vel + accel * dt;

	vel.m128_f32[2] += m_pData->m_fGravity * dt;

	XMStoreFloat3(&m_v3Velocity, vel);

	XMFLOAT3 v3Movement;
	XMStoreFloat3(&v3Movement, XMLoadFloat3(&m_v3Velocity) * dt);

	float _fMoveDistance = XMVectorGetX(XMVector3Length(XMLoadFloat3(&v3Movement)));
	float fCollisionSphereRadius = std::max(_fMoveDistance * 2, m_pData->m_fCollisionSphereRadius);
	m_fRemainRange -= _fMoveDistance;
	XMStoreFloat3(&m_v3Position, XMLoadFloat3(&m_v3Position) + XMLoadFloat3(&v3Movement));

	UpdateAttachInstance();

	if (m_fRemainRange < 0)
	{
		if (m_pHandler)
			m_pHandler->OnExplodingOutOfRange();

		__Explode(false);
		return false;
	}

	if (m_FlyTarget.IsObject())
	{
		if (!m_bTargetHitted)
		{
			if (square_distance_between_linesegment_and_point(m_v3Position, v3LastPosition, m_FlyTarget.GetFlyTargetPosition()) < m_pData->m_fBombRange * m_pData->m_fBombRange)
			{
				m_bTargetHitted = TRUE;

				if (m_canAttack)
				{
					IFlyTargetableObject* pVictim = m_FlyTarget.GetFlyTarget();
					if (pVictim)
					{
						pVictim->OnShootDamage();
					}
				}

				if (m_pHandler)
				{
					m_pHandler->OnExplodingAtTarget(m_dwSkillIndex);
				}

				if (m_iPierceCount)
				{
					m_iPierceCount--;
					__Bomb();
				}
				else
				{
					__Explode();
					return false;
				}

				return true;
			}
		}
	}
	else if (m_FlyTarget.IsPosition())
	{
		if (square_distance_between_linesegment_and_point(m_v3Position, v3LastPosition, m_FlyTarget.GetFlyTargetPosition()) < m_pData->m_fBombRange * m_pData->m_fBombRange)
		{
			__Explode();
			return false;
		}
	}

	Vector3d vecStart, vecDir;
	vecStart.Set(v3LastPosition.x, v3LastPosition.y, v3LastPosition.z);
	vecDir.Set(v3Movement.x, v3Movement.y, v3Movement.z);

	CCullingManager& rkCullingMgr = CCullingManager::Instance();

	if (m_pData->m_bHitOnAnotherMonster)
	{
		FCheckAnotherMonsterDuringFlying kCheckAnotherMonsterDuringFlying(m_pOwner, v3LastPosition, m_v3Position);
		rkCullingMgr.ForInRange(vecStart, fCollisionSphereRadius, &kCheckAnotherMonsterDuringFlying);
		if (kCheckAnotherMonsterDuringFlying.IsHitted())
		{
			IActorInstance* pHittedInstance = (IActorInstance*)kCheckAnotherMonsterDuringFlying.GetHittedObject();
			if (m_HittedObjectSet.end() == m_HittedObjectSet.find(pHittedInstance))
			{
				m_HittedObjectSet.insert(pHittedInstance);

				if (m_pHandler)
				{
					m_pHandler->OnExplodingAtAnotherTarget(m_dwSkillIndex, pHittedInstance->GetVirtualID());
				}

				if (m_iPierceCount)
				{
					m_iPierceCount--;
					__Bomb();
				}
				else
				{
					__Explode();
					return false;
				}

				return true;
			}
		}
	}

	if (m_pData->m_bHitOnBackground)
	{
		// 지형 충돌

		if (CFlyingManager::Instance().GetMapManagerPtr())
		{
			float fGroundHeight = CFlyingManager::Instance().GetMapManagerPtr()->GetTerrainHeight(m_v3Position.x, -m_v3Position.y);
			if (fGroundHeight > m_v3Position.z)
			{
				if (m_pHandler)
					m_pHandler->OnExplodingAtBackground();

				__Explode();
				return false;
			}
		}

		// 건물+나무 충돌

		FCheckBackgroundDuringFlying kCheckBackgroundDuringFlying(v3LastPosition, m_v3Position);
		rkCullingMgr.ForInRange(vecStart, fCollisionSphereRadius, &kCheckBackgroundDuringFlying);

		if (kCheckBackgroundDuringFlying.IsHitted())
		{
			if (m_pHandler)
				m_pHandler->OnExplodingAtBackground();

			__Explode();
			return false;
		}
	}

	return true;
}

void CFlyingInstance::ClearAttachInstance()
{
	CEffectManager& rkEftMgr = CEffectManager::Instance();

	TAttachEffectInstanceVector::iterator i;
	for (i = m_vecAttachEffectInstance.begin(); i != m_vecAttachEffectInstance.end(); ++i)
	{
		rkEftMgr.DestroyEffectInstance(i->dwEffectInstanceIndex);

		if (i->pFlyTrace)
			CFlyTrace::Delete(i->pFlyTrace);

		i->iAttachIndex = 0;
		i->dwEffectInstanceIndex = 0;
		i->pFlyTrace = NULL;
	}
	m_vecAttachEffectInstance.clear();
}

void CFlyingInstance::__Explode(bool bBomb)
{
	if (!m_bAlive)
		return;

	m_bAlive = false;

	if (bBomb)
		__Bomb();
}

void CFlyingInstance::__Bomb()
{
	CEffectManager& rkEftMgr = CEffectManager::Instance();
	if (!m_pData->m_dwBombEffectID)
		return;

	DWORD dwEmptyIndex = rkEftMgr.GetEmptyIndex();
	rkEftMgr.CreateEffectInstance(dwEmptyIndex, m_pData->m_dwBombEffectID);

	XMFLOAT4X4 m;
	XMStoreFloat4x4(&m, XMMatrixIdentity());

	m._41 = m_v3Position.x;
	m._42 = m_v3Position.y;
	m._43 = m_v3Position.z;

	rkEftMgr.SelectEffectInstance(dwEmptyIndex);
	rkEftMgr.SetEffectInstanceGlobalMatrix(m);
}

void CFlyingInstance::Render()
{
	if (!m_bAlive)
		return;
	RenderAttachInstance();
}

void CFlyingInstance::RenderAttachInstance()
{
	TAttachEffectInstanceVector::iterator it;
	for (it = m_vecAttachEffectInstance.begin(); it != m_vecAttachEffectInstance.end(); ++it)
	{
		if (it->pFlyTrace)
			it->pFlyTrace->Render();
	}
}

void CFlyingInstance::SetDataPointer(CFlyingData* pData, const XMFLOAT3& v3StartPosition)
{
	__SetDataPointer(pData, v3StartPosition);
}

void CFlyingInstance::__SetDataPointer(CFlyingData* pData, const XMFLOAT3& v3StartPosition)
{
	m_pData = pData;

	m_qRot = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_v3Position = v3StartPosition;
	m_bAlive = true;

	m_fStartTime = CTimer::Instance().GetCurrentSecond();

	XMVECTOR q = XMQuaternionRotationRollPitchYaw(
		0.0f,
		0.0f,
		XMConvertToRadians(pData->m_fConeAngle)
	);

	XMVECTOR qYaw = XMQuaternionRotationRollPitchYaw(
		XMConvertToRadians(pData->m_fRollAngle - 90.0f),
		0.0f,
		0.0f
	);

	XMStoreFloat4(&m_qRot, XMQuaternionMultiply(q, qYaw));

	if (pData->m_bSpreading)
	{
		XMVECTOR axis1 = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		XMVECTOR axis2 = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

		float a1 = (frandom(-XM_PI / 3, XM_PI / 3) + frandom(-XM_PI / 3, XM_PI / 3)) * 0.5f;
		float a2 = frandom(0.0f, XM_2PI);

		XMVECTOR q2 = XMQuaternionRotationAxis(axis1, a1);
		XMVECTOR q1 = XMQuaternionRotationAxis(axis2, a2);

		XMStoreFloat4(&m_qRot, XMQuaternionMultiply(XMQuaternionMultiply(q1, q2), XMLoadFloat4(&m_qRot)));
	}

	m_v3Velocity = m_v3LocalVelocity = XMFLOAT3(0.0f, -pData->m_fInitVel, 0.0f);
	m_v3Accel = pData->m_v3Accel;
	m_fRemainRange = pData->m_fRange;

	m_qAttachRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	BuildAttachInstance();
	UpdateAttachInstance();

	m_iPierceCount = pData->m_iPierceCount;
}
