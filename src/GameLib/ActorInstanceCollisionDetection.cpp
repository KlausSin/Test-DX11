#include "StdAfx.h"
#include "EterLib/GrpMath.h"

#include "ActorInstance.h"

void CActorInstance::__InitializeCollisionData()
{
	m_canSkipCollision=false;
}

void CActorInstance::EnableSkipCollision()
{
	m_canSkipCollision=true;
}

void CActorInstance::DisableSkipCollision()
{
	m_canSkipCollision=false;
}

bool CActorInstance::CanSkipCollision()
{
	return m_canSkipCollision;
}

void CActorInstance::UpdatePointInstance()
{
	// Optimized: Cache end iterator
	TCollisionPointInstanceListIterator itor;
	TCollisionPointInstanceListIterator end = m_DefendingPointInstanceList.end();
	for (itor = m_DefendingPointInstanceList.begin(); itor != end; ++itor)
		UpdatePointInstance(&(*itor));
}

void CActorInstance::UpdatePointInstance(TCollisionPointInstance* pPointInstance)
{
	if (!pPointInstance)
	{
		assert(!"CActorInstance::UpdatePointInstance - pPointInstance is NULL");
		return;
	}

	XMFLOAT4X4 matBone;

	if (pPointInstance->isAttached)
	{
		if (pPointInstance->dwModelIndex >= m_LODControllerVector.size())
			return;

		CGrannyLODController* pGrnLODController = m_LODControllerVector[pPointInstance->dwModelIndex];
		if (!pGrnLODController)
			return;

		CGrannyModelInstance* pModelInstance = pGrnLODController->GetModelInstance();
		if (!pModelInstance)
			return;

		XMFLOAT4X4* pmatBone = (XMFLOAT4X4*)pModelInstance->GetBoneMatrixPointer(pPointInstance->dwBoneIndex);

		matBone = *(XMFLOAT4X4*)pModelInstance->GetCompositeBoneMatrixPointer(pPointInstance->dwBoneIndex);

		matBone._41 = pmatBone->_41;
		matBone._42 = pmatBone->_42;
		matBone._43 = pmatBone->_43;

		XMStoreFloat4x4(&matBone, XMLoadFloat4x4(&matBone) * XMLoadFloat4x4(&TransformComponent().GetWorldMatrix()));
	}
	else
	{
		matBone = TransformComponent().GetWorldMatrix();
	}

	CSphereCollisionInstanceVector::const_iterator sit = pPointInstance->c_pCollisionData->SphereDataVector.begin();
	CSphereCollisionInstanceVector::const_iterator sit_end = pPointInstance->c_pCollisionData->SphereDataVector.end();
	CDynamicSphereInstanceVector::iterator dit = pPointInstance->SphereInstanceVector.begin();

	for (; sit != sit_end; ++sit, ++dit)
	{
		const TSphereData& c = sit->GetAttribute();

		XMFLOAT4X4 matPoint;

		XMStoreFloat4x4(&matPoint, XMMatrixTranslation(c.v3Position.x, c.v3Position.y, c.v3Position.z));
		XMStoreFloat4x4(&matPoint, XMLoadFloat4x4(&matPoint) * XMLoadFloat4x4(&matBone));

		dit->v3LastPosition = dit->v3Position;

		dit->v3Position.x = matPoint._41;
		dit->v3Position.y = matPoint._42;
		dit->v3Position.z = matPoint._43;
	}
}

void CActorInstance::UpdateAdvancingPointInstance()
{
	XMFLOAT3 v3Movement = m_v3Movement;
	if (m_pkHorse)
		v3Movement = m_pkHorse->m_v3Movement;

	if (m_pkHorse)
		m_pkHorse->UpdateAdvancingPointInstance();

	XMFLOAT4X4 matPoint;
	XMFLOAT4X4 matCenter;

	TCollisionPointInstanceListIterator itor = m_BodyPointInstanceList.begin();
	TCollisionPointInstanceListIterator itor_end = m_BodyPointInstanceList.end();
	for (; itor != itor_end; ++itor)
	{
		TCollisionPointInstance& rInstance = *itor;

		if (rInstance.isAttached)
		{
			if (rInstance.dwModelIndex >= m_LODControllerVector.size())
			{
				Tracenf("CActorInstance::UpdateAdvancingPointInstance - rInstance.dwModelIndex=%d >= m_LODControllerVector.size()=%d", rInstance.dwModelIndex, m_LODControllerVector.size());
				continue;
			}

			CGrannyLODController* pGrnLODController = m_LODControllerVector[rInstance.dwModelIndex];
			if (!pGrnLODController)
			{
				Tracenf("CActorInstance::UpdateAdvancingPointInstance - m_LODControllerVector[rInstance.dwModelIndex=%d] is NULL", rInstance.dwModelIndex);
				continue;
			}

			CGrannyModelInstance* pModelInstance = pGrnLODController->GetModelInstance();
			if (!pModelInstance)
				continue;

			matCenter = *(XMFLOAT4X4*)pModelInstance->GetBoneMatrixPointer(rInstance.dwBoneIndex);
			XMStoreFloat4x4(&matCenter, XMLoadFloat4x4(&matCenter) * XMLoadFloat4x4(&TransformComponent().GetWorldMatrix()));
		}
		else
		{
			matCenter = TransformComponent().GetWorldMatrix();
		}

		const NRaceData::TCollisionData* c_pCollisionData = rInstance.c_pCollisionData;
		if (c_pCollisionData)
		{
			for (DWORD j = 0; j < c_pCollisionData->SphereDataVector.size(); ++j)
			{
				const TSphereData& c = c_pCollisionData->SphereDataVector[j].GetAttribute();
				CDynamicSphereInstance& rSphereInstance = rInstance.SphereInstanceVector[j];

				XMStoreFloat4x4(&matPoint, XMMatrixTranslation(c.v3Position.x, c.v3Position.y, c.v3Position.z));
				XMStoreFloat4x4(&matPoint, XMLoadFloat4x4(&matPoint) * XMLoadFloat4x4(&matCenter));

				rSphereInstance.v3LastPosition.x = matPoint._41;
				rSphereInstance.v3LastPosition.y = matPoint._42;
				rSphereInstance.v3LastPosition.z = matPoint._43;

				rSphereInstance.v3Position = rSphereInstance.v3LastPosition;
				XMStoreFloat3(&rSphereInstance.v3Position, XMLoadFloat3(&rSphereInstance.v3Position) + XMLoadFloat3(&v3Movement));
			}
		}
	}
}

bool CActorInstance::CheckCollisionDetection(const CDynamicSphereInstanceVector * c_pAttackingSphereVector, XMFLOAT3 * pv3Position)
{
	if (!c_pAttackingSphereVector)
	{
		assert(!"CActorInstance::CheckCollisionDetection - c_pAttackingSphereVector is NULL"); // 레퍼런스로 교체하시오
		return false;
	}

	// Optimized: Cache end iterator and vector sizes
	TCollisionPointInstanceListIterator itor;
	TCollisionPointInstanceListIterator itor_end = m_DefendingPointInstanceList.end();
	DWORD attackSize = c_pAttackingSphereVector->size();
	for (itor = m_DefendingPointInstanceList.begin(); itor != itor_end; ++itor)
	{
		const CDynamicSphereInstanceVector * c_pDefendingSphereVector = &(*itor).SphereInstanceVector;
		DWORD defendSize = c_pDefendingSphereVector->size();

		for (DWORD i = 0; i < attackSize; ++i)
		for (DWORD j = 0; j < defendSize; ++j)
		{
			const CDynamicSphereInstance & c_rAttackingSphere = c_pAttackingSphereVector->at(i);
			const CDynamicSphereInstance & c_rDefendingSphere = c_pDefendingSphereVector->at(j);

			if (DetectCollisionDynamicSphereVSDynamicSphere(c_rAttackingSphere, c_rDefendingSphere))
			{
				// FIXME : 두 원의 교점을 찾아내는 식으로 바꿔야 한다.
				XMStoreFloat3(pv3Position, (XMLoadFloat3(&c_rAttackingSphere.v3Position) + XMLoadFloat3(&c_rDefendingSphere.v3Position)) * 0.5f);
				return true;
			}
		}
	}

	return false;
}

bool CActorInstance::CreateCollisionInstancePiece(DWORD dwAttachingModelIndex, const NRaceData::TAttachingData * c_pAttachingData, TCollisionPointInstance * pPointInstance)
{
	if (!c_pAttachingData)
	{
		assert(!"CActorInstance::CreateCollisionInstancePiece - c_pAttachingData is NULL"); // 레퍼런스로 교체하시오
		return false;
	}

	if (!c_pAttachingData->pCollisionData)
	{
		assert(!"CActorInstance::CreateCollisionInstancePiece - c_pAttachingData->pCollisionData is NULL"); // 레퍼런스로 교체하시오
		return false;
	}

	if (!pPointInstance)
	{
		assert(!"CActorInstance::CreateCollisionInstancePiece - pPointInstance is NULL"); // 레퍼런스로 교체하시오
		return false;
	}

	pPointInstance->dwModelIndex = dwAttachingModelIndex;
	pPointInstance->isAttached = FALSE;
	pPointInstance->dwBoneIndex = 0;
	pPointInstance->c_pCollisionData = c_pAttachingData->pCollisionData;

	if (c_pAttachingData->isAttaching)
	{
		int iAttachingBoneIndex;

		CGrannyModelInstance * pModelInstance = m_LODControllerVector[dwAttachingModelIndex]->GetModelInstance();

		if (pModelInstance && pModelInstance->GetBoneIndexByName(c_pAttachingData->strAttachingBoneName.c_str(),
												&iAttachingBoneIndex))
		{
			pPointInstance->isAttached = TRUE;
			pPointInstance->dwBoneIndex = iAttachingBoneIndex;
		}
		else
		{
			//TraceError("CActorInstance::CreateCollisionInstancePiece: Cannot get matrix of bone %s ModelInstance 0x%p",	c_pAttachingData->strAttachingBoneName.c_str(), pModelInstance);
			pPointInstance->isAttached = TRUE;
			pPointInstance->dwBoneIndex = 0;
		}
	}


	const CSphereCollisionInstanceVector & c_rSphereDataVector = c_pAttachingData->pCollisionData->SphereDataVector;

	pPointInstance->SphereInstanceVector.clear();
	pPointInstance->SphereInstanceVector.reserve(c_rSphereDataVector.size());

	// Optimized: Cache end iterator
	CSphereCollisionInstanceVector::const_iterator it;
	CSphereCollisionInstanceVector::const_iterator it_end = c_rSphereDataVector.end();
	CDynamicSphereInstance dsi;

	dsi.v3LastPosition = XMFLOAT3(0.0f,0.0f,0.0f);
	dsi.v3Position = XMFLOAT3(0.0f,0.0f,0.0f);
	for (it = c_rSphereDataVector.begin(); it!=it_end; ++it)
	{
		const TSphereData & c_rSphereData = it->GetAttribute();
		dsi.fRadius = c_rSphereData.fRadius;
		pPointInstance->SphereInstanceVector.push_back(dsi);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CActorInstance::__SplashAttackProcess(CActorInstance& rVictim)
{
	XMFLOAT3 v3Distance(rVictim.m_x - m_x, rVictim.m_z - m_z, rVictim.m_z - m_z);
	float fDistanceSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&v3Distance)));

	if (fDistanceSq >= 1000.0f * 1000.0f)
		return FALSE;

	if (!__IsInSplashTime())
		return FALSE;

	const CRaceMotionData::TMotionAttackingEventData* c_pAttackingEvent = m_kSplashArea.c_pAttackingEvent;
	const NRaceData::TAttackData& c_rAttackData = c_pAttackingEvent->AttackData;
	THittedInstanceMap& rHittedInstanceMap = m_kSplashArea.HittedInstanceMap;

	if (rHittedInstanceMap.end() != rHittedInstanceMap.find(&rVictim))
		return FALSE;

	if (NRaceData::ATTACK_TYPE_SNIPE == c_rAttackData.iAttackType)
	{
		if (__IsFlyTargetPC())
			if (!__IsSameFlyTarget(&rVictim))
				return FALSE;
	}

	XMFLOAT3 v3HitPosition;

	if (rVictim.CheckCollisionDetection(&m_kSplashArea.SphereInstanceVector, &v3HitPosition))
	{
		rHittedInstanceMap.insert(std::make_pair(&rVictim, GetLocalTime() + c_rAttackData.fInvisibleTime));

		int iCurrentHitCount = rHittedInstanceMap.size();
		int iMaxHitCount = (0 == c_rAttackData.iHitLimitCount ? 16 : c_rAttackData.iHitLimitCount);

		if (iCurrentHitCount > iMaxHitCount)
			return FALSE;

		NEW_SetAtkPixelPosition(NEW_GetCurPixelPositionRef());
		__ProcessDataAttackSuccess(c_rAttackData, rVictim, v3HitPosition, m_kSplashArea.uSkill, m_kSplashArea.isEnableHitProcess);
		return TRUE;
	}

	return FALSE;
}

BOOL CActorInstance::__NormalAttackProcess(CActorInstance & rVictim)
{
	// Check Distance
	// NOTE - 일단 근접 체크만 하고 있음
	// Optimized: Already using squared distance comparison
	XMFLOAT3 v3Distance(rVictim.m_x - m_x, rVictim.m_z - m_z, rVictim.m_z - m_z);
	float fDistanceSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&v3Distance)));

	extern bool IS_HUGE_RACE(unsigned int vnum);
	if (IS_HUGE_RACE(rVictim.GetRace()))
	{
		if (fDistanceSq >= 500.0f*500.0f)
			return FALSE;
	}
	else
	{
		if (fDistanceSq >= 300.0f*300.0f)
			return FALSE;
	}

	if (!isValidAttacking())
		return FALSE;

	const float c_fAttackRadius = 20.0f;
	const NRaceData::TMotionAttackData * pad = m_pkCurRaceMotionData->GetMotionAttackDataPointer();

	const float motiontime = GetAttackingElapsedTime();

	NRaceData::THitDataContainer::const_iterator itorHitData = pad->HitDataContainer.begin();
	for (; itorHitData != pad->HitDataContainer.end(); ++itorHitData)
	{
		const NRaceData::THitData & c_rHitData = *itorHitData;

		// NOTE : 이미 맞았는지 체크
		THitDataMap::iterator itHitData = m_HitDataMap.find(&c_rHitData);
		if (itHitData != m_HitDataMap.end())
		{
			THittedInstanceMap & rHittedInstanceMap = itHitData->second;

			THittedInstanceMap::iterator itInstance;
			if ((itInstance=rHittedInstanceMap.find(&rVictim)) != rHittedInstanceMap.end())
			{
				if (pad->iMotionType==NRaceData::MOTION_TYPE_COMBO || itInstance->second > GetLocalTime())
					continue;
			}
		}

		NRaceData::THitTimePositionMap::const_iterator range_start, range_end;
		range_start = c_rHitData.mapHitPosition.lower_bound(motiontime-CTimer::Instance().GetElapsedSecond());
		range_end = c_rHitData.mapHitPosition.upper_bound(motiontime);
		float c = cosf(XMConvertToRadians(GetRotation()));
		float s = sinf(XMConvertToRadians(GetRotation()));

		for(;range_start!=range_end;++range_start)
		{
			const CDynamicSphereInstance& dsiSrc=range_start->second;

			CDynamicSphereInstance dsi;
			dsi = dsiSrc;
			dsi.fRadius = c_fAttackRadius;

			{
				XMFLOAT3 v3SrcDir;
				XMStoreFloat3(&v3SrcDir, (XMLoadFloat3(&dsiSrc.v3Position) - XMLoadFloat3(&dsiSrc.v3LastPosition)) * __GetReachScale());

				XMFLOAT3 v3Src;
				XMStoreFloat3(&v3Src, XMLoadFloat3(&dsiSrc.v3LastPosition) + XMLoadFloat3(&v3SrcDir));

				XMFLOAT3& v3Dst = dsi.v3Position;

				v3Dst.x = v3Src.x * c - v3Src.y * s;
				v3Dst.y = v3Src.x * s + v3Src.y * c;

				XMStoreFloat3(&v3Dst, XMLoadFloat3(&v3Dst) + XMLoadFloat3(&TransformComponent().GetPosition()));
			}

			{
				const XMFLOAT3& v3Src = dsiSrc.v3LastPosition;
				XMFLOAT3& v3Dst = dsi.v3LastPosition;

				v3Dst.x = v3Src.x * c - v3Src.y * s;
				v3Dst.y = v3Src.x * s + v3Src.y * c;

				XMStoreFloat3(&v3Dst, XMLoadFloat3(&v3Dst) + XMLoadFloat3(&TransformComponent().GetPosition()));
			}

			
			TCollisionPointInstanceList::iterator cpit;
			for(cpit = rVictim.m_DefendingPointInstanceList.begin(); cpit!=rVictim.m_DefendingPointInstanceList.end();++cpit)
			{
				int index = 0;
				const CDynamicSphereInstanceVector & c_DefendingSphereVector = cpit->SphereInstanceVector;
				CDynamicSphereInstanceVector::const_iterator dsit;
				for(dsit = c_DefendingSphereVector.begin(); dsit!= c_DefendingSphereVector.end();++dsit, ++index)
				{
					const CDynamicSphereInstance& sub = *dsit;
					if (DetectCollisionDynamicZCylinderVSDynamicZCylinder(dsi, sub))
					{
						THitDataMap::iterator itHitData = m_HitDataMap.find(&c_rHitData);
						if (itHitData == m_HitDataMap.end())
						{
							THittedInstanceMap HittedInstanceMap;
							HittedInstanceMap.insert(std::make_pair(&rVictim, GetLocalTime()+pad->fInvisibleTime));
							//HittedInstanceMap.insert(std::make_pair(&rVictim, GetLocalTime()+HIT_COOL_TIME));
							m_HitDataMap.insert(std::make_pair(&c_rHitData, HittedInstanceMap));

							//Tracef(" ----------- First Hit\n");
						}
						else
						{
							itHitData->second.insert(std::make_pair(&rVictim, GetLocalTime()+pad->fInvisibleTime));
							//itHitData->second.insert(std::make_pair(&rVictim, GetLocalTime()+HIT_COOL_TIME));

							//Tracef(" ----------- Next Hit : %d\n", itHitData->second.size());

							int iCurrentHitCount = itHitData->second.size();
							// NOTE : 보통 공격은 16명이 한계
							if (NRaceData::MOTION_TYPE_COMBO == pad->iMotionType || NRaceData::MOTION_TYPE_NORMAL == pad->iMotionType)
							{
								if (iCurrentHitCount > 16)
								{
									//Tracef(" Type NORMAL :: Overflow - Can't process, skip\n");
									return FALSE;
								}
							}
							else
							{
								if (iCurrentHitCount > pad->iHitLimitCount)
								{
									//Tracef(" Type SKILL :: Overflow - Can't process, skip\n");
									return FALSE;
								}
							}
						}

						XMFLOAT3 v3HitPosition;
						XMStoreFloat3(&v3HitPosition, (XMLoadFloat3(&TransformComponent().GetPosition()) + XMLoadFloat3(&rVictim.TransformComponent().GetPosition())) * 0.5f);

						extern bool IS_HUGE_RACE(unsigned int vnum);

						if (IS_HUGE_RACE(rVictim.GetRace()))
						{
							XMStoreFloat3(&v3HitPosition, (XMLoadFloat3(&TransformComponent().GetPosition()) + XMLoadFloat3(&sub.v3Position)) * 0.5f);
						}
						
						__ProcessDataAttackSuccess(*pad, rVictim, v3HitPosition, m_kCurMotNode.uSkill);
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

BOOL CActorInstance::AttackingProcess(CActorInstance & rVictim)
{
	if (rVictim.__isInvisible())
		return FALSE;

	if (__SplashAttackProcess(rVictim))
		return TRUE;

	if (__NormalAttackProcess(rVictim))
		return TRUE;

	return FALSE;
}

BOOL CActorInstance::TestPhysicsBlendingCollision(CActorInstance& rVictim)
{
	if (rVictim.IsDead())
		return FALSE;

	TPixelPosition kPPosLast;
	GetBlendingPosition(&kPPosLast);

	XMFLOAT3 v3Distance(rVictim.m_x - kPPosLast.x, rVictim.m_y - kPPosLast.y, rVictim.m_z - kPPosLast.z);
	float fDistanceSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&v3Distance)));

	if (fDistanceSq > 800.0f * 800.0f)
		return FALSE;

	TCollisionPointInstanceList* pMainList;
	TCollisionPointInstanceList* pVictimList;

	if (isAttacking() || IsWaiting())
	{
		pMainList = &m_DefendingPointInstanceList;
		pVictimList = &rVictim.m_DefendingPointInstanceList;
	}
	else
	{
		pMainList = &m_BodyPointInstanceList;
		pVictimList = &rVictim.m_BodyPointInstanceList;
	}

	TPixelPosition kPDelta;
	m_PhysicsObject.GetLastPosition(&kPDelta);

	XMFLOAT3 prevLastPosition, prevPosition;
	const int nSubCheckCount = 50;

	TCollisionPointInstanceListIterator itorMain = pMainList->begin();
	TCollisionPointInstanceListIterator itorMain_end = pMainList->end();
	TCollisionPointInstanceListIterator itorVictim = pVictimList->begin();
	TCollisionPointInstanceListIterator itorVictim_end = pVictimList->end();

	for (; itorMain != itorMain_end; ++itorMain)
	{
		for (; itorVictim != itorVictim_end; ++itorVictim)
		{
			CDynamicSphereInstanceVector& c_rMainSphereVector = (*itorMain).SphereInstanceVector;
			CDynamicSphereInstanceVector& c_rVictimSphereVector = (*itorVictim).SphereInstanceVector;
			DWORD mainSize = c_rMainSphereVector.size();

			for (DWORD i = 0; i < mainSize; ++i)
			{
				CDynamicSphereInstance& c_rMainSphere = c_rMainSphereVector[i];

				prevLastPosition = c_rMainSphere.v3LastPosition;
				prevPosition = c_rMainSphere.v3Position;

				c_rMainSphere.v3LastPosition = prevPosition;

				DWORD victimSize = c_rVictimSphereVector.size();
				for (int i = 1; i <= nSubCheckCount; ++i)
				{
					XMStoreFloat3(&c_rMainSphere.v3Position, XMLoadFloat3(&prevPosition) + XMLoadFloat3(&kPDelta) * (float(i) / float(nSubCheckCount)));

					for (DWORD j = 0; j < victimSize; ++j)
					{
						CDynamicSphereInstance& c_rVictimSphere = c_rVictimSphereVector[j];

						if (DetectCollisionDynamicSphereVSDynamicSphere(c_rMainSphere, c_rVictimSphere))
						{
							BOOL bResult = GetVector3Distance(c_rMainSphere.v3Position, c_rVictimSphere.v3Position) <= GetVector3Distance(c_rMainSphere.v3LastPosition, c_rVictimSphere.v3Position);

							c_rMainSphere.v3LastPosition = prevLastPosition;
							c_rMainSphere.v3Position = prevPosition;

							return bResult;
						}
					}
				}

				c_rMainSphere.v3LastPosition = prevLastPosition;
				c_rMainSphere.v3Position = prevPosition;
			}
		}
	}

	return FALSE;
}


BOOL CActorInstance::TestActorCollision(CActorInstance & rVictim)
{
/*
	if (m_pkHorse)
	{
		if (m_pkHorse->TestActorCollision(rVictim))
			return TRUE;

		return FALSE;
	}
*/

	if (rVictim.IsDead())
		return FALSE;

	XMFLOAT3 v3Distance(rVictim.m_x - m_x, rVictim.m_y - m_y, rVictim.m_z - m_z);
	float fDistanceSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&v3Distance)));
	if (fDistanceSq > 800.0f*800.0f)
		return FALSE;
	
	TCollisionPointInstanceList * pMainList;
	TCollisionPointInstanceList * pVictimList;
	if (isAttacking() || IsWaiting())
	{
		pMainList = &m_DefendingPointInstanceList;
		pVictimList = &rVictim.m_DefendingPointInstanceList;
	}
	else
	{
		pMainList = &m_BodyPointInstanceList;
		pVictimList = &rVictim.m_BodyPointInstanceList;
	}

	// Optimized: Cache end iterators and vector sizes
	TCollisionPointInstanceListIterator itorMain = pMainList->begin();
	TCollisionPointInstanceListIterator itorMain_end = pMainList->end();
	TCollisionPointInstanceListIterator itorVictim = pVictimList->begin();
	TCollisionPointInstanceListIterator itorVictim_end = pVictimList->end();
	for (; itorMain != itorMain_end; ++itorMain)
	for (; itorVictim != itorVictim_end; ++itorVictim)
	{
		const CDynamicSphereInstanceVector & c_rMainSphereVector = (*itorMain).SphereInstanceVector;
		const CDynamicSphereInstanceVector & c_rVictimSphereVector = (*itorVictim).SphereInstanceVector;
		DWORD mainSize = c_rMainSphereVector.size();
		DWORD victimSize = c_rVictimSphereVector.size();

		for (DWORD i = 0; i < mainSize; ++i)
		for (DWORD j = 0; j < victimSize; ++j)
		{
			const CDynamicSphereInstance & c_rMainSphere = c_rMainSphereVector[i];
			const CDynamicSphereInstance & c_rVictimSphere = c_rVictimSphereVector[j];

			if (DetectCollisionDynamicSphereVSDynamicSphere(c_rMainSphere, c_rVictimSphere))
			{
				if (GetVector3Distance(c_rMainSphere.v3Position, c_rVictimSphere.v3Position) <=
					GetVector3Distance(c_rMainSphere.v3LastPosition, c_rVictimSphere.v3Position))
				{
					return TRUE;
				}
				return FALSE;
			}
		}
	}

	return FALSE;
}

bool CActorInstance::AvoidObject(const CGraphicObjectInstance& c_rkBGObj)
{	
#ifdef __MOVIE_MODE__
	if (IsMovieMode())
		return false;
#endif	

	if (this==&c_rkBGObj)
		return false;

	if (!__TestObjectCollision(&c_rkBGObj))
		return false;

	__AdjustCollisionMovement(&c_rkBGObj);
	return true;
}

bool CActorInstance::IsBlockObject(const CGraphicObjectInstance& c_rkBGObj)
{
	if (this==&c_rkBGObj)
		return false;
	
	if (!__TestObjectCollision(&c_rkBGObj))
		return false;
	
	return true;
}

void CActorInstance::BlockMovement()
{
	if (m_pkHorse)
	{
		m_pkHorse->__InitializeMovement();
		return;
	}
	
	__InitializeMovement();
}

BOOL CActorInstance::__TestObjectCollision(const CGraphicObjectInstance * c_pObjectInstance)
{
	if (m_pkHorse)
	{
		if (m_pkHorse->__TestObjectCollision(c_pObjectInstance))
			return TRUE;

		return FALSE;
	}

	if (m_canSkipCollision)
		return FALSE;

	if (m_v3Movement.x == 0.0f && m_v3Movement.y == 0.0f && m_v3Movement.z == 0.0f)
		return FALSE;

	// Optimized: Cache end iterator and vector size
	TCollisionPointInstanceListIterator itorMain = m_BodyPointInstanceList.begin();
	TCollisionPointInstanceListIterator itorMain_end = m_BodyPointInstanceList.end();
	for (; itorMain != itorMain_end; ++itorMain)
	{
		const CDynamicSphereInstanceVector & c_rMainSphereVector = (*itorMain).SphereInstanceVector;
		DWORD mainSize = c_rMainSphereVector.size();
		for (DWORD i = 0; i < mainSize; ++i)
		{
			const CDynamicSphereInstance & c_rMainSphere = c_rMainSphereVector[i];

			if (c_pObjectInstance->CollisionComponent().MovementCollisionDynamicSphere(c_rMainSphere))
			{
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}


bool CActorInstance::TestCollisionWithDynamicSphere(const CDynamicSphereInstance & dsi)
{
	// Optimized: Cache end iterator and vector size
	TCollisionPointInstanceListIterator itorMain = m_BodyPointInstanceList.begin();
	TCollisionPointInstanceListIterator itorMain_end = m_BodyPointInstanceList.end();
	for (; itorMain != itorMain_end; ++itorMain)
	{
		const CDynamicSphereInstanceVector & c_rMainSphereVector = (*itorMain).SphereInstanceVector;
		DWORD mainSize = c_rMainSphereVector.size();
		for (DWORD i = 0; i < mainSize; ++i)
		{
			const CDynamicSphereInstance & c_rMainSphere = c_rMainSphereVector[i];
			
			if (DetectCollisionDynamicSphereVSDynamicSphere(c_rMainSphere, dsi))
			{
				return true;
			}
		}
	}
	
	return false;
}
