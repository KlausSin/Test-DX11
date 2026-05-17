#include "StdAfx.h"
#include "ActorInstance.h"

const TPixelPosition& CActorInstance::NEW_GetLastPixelPositionRef()
{	
	GetBlendingPosition(&m_kPPosLast);
	m_kPPosLast.y=-m_kPPosLast.y;
	
	return m_kPPosLast;
}

const XMFLOAT3& CActorInstance::GetPositionVectorRef()
{
	m_v3Pos.x=m_x;
	m_v3Pos.y=m_y;
	m_v3Pos.z=m_z;
	return m_v3Pos;
}

const XMFLOAT3&	CActorInstance::GetMovementVectorRef()
{
	if (m_pkHorse)
		return m_pkHorse->GetMovementVectorRef();

	return m_v3Movement;
}

void CActorInstance::NEW_SetAtkPixelPosition(const TPixelPosition& c_rkPPosAtk)
{
	m_kPPosAtk=c_rkPPosAtk;
}

void CActorInstance::SetCurPixelPosition(const TPixelPosition& c_rkPPosCur)
{
	XMFLOAT3 v3PosCur;
	v3PosCur.x=+c_rkPPosCur.x;
	v3PosCur.y=-c_rkPPosCur.y;
	v3PosCur.z=+c_rkPPosCur.z;

	SetPixelPosition(v3PosCur);
}

void CActorInstance::NEW_SetSrcPixelPosition(const TPixelPosition& c_rkPPosSrc)
{
	m_kPPosSrc=c_rkPPosSrc;
}

void CActorInstance::NEW_SetDstPixelPositionZ(float z)
{
	m_kPPosDst.z=z;
}

void CActorInstance::NEW_SetDstPixelPosition(const TPixelPosition& c_rkPPosDst)
{
	m_kPPosDst=c_rkPPosDst;
}

const TPixelPosition& CActorInstance::NEW_GetAtkPixelPositionRef()
{
	return m_kPPosAtk;
}

const TPixelPosition& CActorInstance::NEW_GetSrcPixelPositionRef()
{
	return m_kPPosSrc;
}


const TPixelPosition& CActorInstance::NEW_GetDstPixelPositionRef()
{
	return m_kPPosDst;
}

const TPixelPosition& CActorInstance::NEW_GetCurPixelPositionRef()
{
	m_kPPosCur.x=+m_x;
	m_kPPosCur.y=-m_y;
	m_kPPosCur.z=+m_z;
	
	return m_kPPosCur;
}

void CActorInstance::GetPixelPosition(TPixelPosition * pPixelPosition)
{
	pPixelPosition->x = m_x;
	pPixelPosition->y = m_y;
	pPixelPosition->z = m_z;
}

void CActorInstance::SetPixelPosition(const TPixelPosition& c_rPixelPos)
{
	TPixelPosition nextPos = c_rPixelPos;

	constexpr float characterRadius = 10.0f;
	constexpr float characterHalfHeight = 80.0f;
	constexpr float characterCenterZ = characterRadius + characterHalfHeight;

	if (CPhysicsManager::Instance().IsInitialized())
	{
		float3pos from = { m_x, -m_y, m_z + characterCenterZ };
		float3pos to = { nextPos.x, -nextPos.y, nextPos.z + characterCenterZ };
		quatrot rot = { 0.70710678f, 0.0f, 0.0f, 0.70710678f };

		JoltShapeCastHit hit = CPhysicsManager::Instance().CastCapsule(
			from,
			to,
			characterRadius,
			characterHalfHeight,
			rot,
			this
		);

		if (hit.hit)
		{
			return;
		}
	}

	if (m_pkTree)
		__SetTreePosition(nextPos.x, nextPos.y, nextPos.z);

	if (m_pkHorse)
		m_pkHorse->SetPixelPosition(nextPos);

	m_x = nextPos.x;
	m_y = nextPos.y;
	m_z = nextPos.z;
	m_bNeedUpdateCollision = TRUE;
}


void CActorInstance::__InitializePositionData()
{
	m_dwShakeTime = 0;

	m_x = 0.0f;
	m_y = 0.0f;
	m_z = 0.0f;
	m_bNeedUpdateCollision = FALSE;

	m_kPPosAtk=m_kPPosLast=m_kPPosDst=m_kPPosCur=m_kPPosSrc=TPixelPosition(0.0f, 0.0f, 0.0f);

	__InitializeMovement();
}
