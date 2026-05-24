#include "StdAfx.h"
#include "ActorInstance.h"
#include "RaceData.h"



void CActorInstance::__Push(int x, int y)
{
	if (IsResistFallen())
		return;

	const XMFLOAT3& c_rv3Src = TransformComponent().GetPosition();
	const XMFLOAT3 c_v3Dst((float)x, (float)-y, c_rv3Src.z);

	XMFLOAT3 c_v3Delta;
	XMStoreFloat3(&c_v3Delta, XMLoadFloat3(&c_v3Dst) - XMLoadFloat3(&c_rv3Src));

	const int LoopValue = 100;

	XMFLOAT3 inc;
	XMStoreFloat3(&inc, XMLoadFloat3(&c_v3Delta) / float(LoopValue));

	XMFLOAT3 v3Movement(0.0f, 0.0f, 0.0f);

	IPhysicsWorld* pWorld = IPhysicsWorld::GetPhysicsWorld();

	if (!pWorld)
		return;

	for (int i = 0; i < LoopValue; ++i)
	{
		XMFLOAT3 v3Check;
		XMStoreFloat3(&v3Check, XMLoadFloat3(&c_rv3Src) + XMLoadFloat3(&v3Movement));

		if (pWorld->isPhysicalCollision(v3Check))
		{
			ResetBlendingPosition();
			return;
		}

		XMStoreFloat3(&v3Movement, XMLoadFloat3(&v3Movement) + XMLoadFloat3(&inc));
	}

	SetBlendingPosition(c_v3Dst);

	if (!IsUsingSkill())
	{
		int len = sqrtf(c_v3Delta.x * c_v3Delta.x + c_v3Delta.y * c_v3Delta.y);

		if (len > 150.0f)
		{
			InterceptOnceMotion(CRaceMotionData::NAME_DAMAGE_FLYING);
			PushOnceMotion(CRaceMotionData::NAME_STAND_UP);
		}
	}
}

void CActorInstance::TEMP_Push(int x, int y)
{
	__Push(x, y);
}

bool CActorInstance::__IsSyncing()
{
	if (IsDead())
		return TRUE;

	if (IsStun())
		return TRUE;

	if (IsPushing())
		return TRUE;

	return FALSE;
}

bool CActorInstance::IsPushing()
{
	return m_PhysicsObject.isBlending();
}