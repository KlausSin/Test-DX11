#include "StdAfx.h"
#include "PhysicsObject.h"

const float c_fFrameTime = 0.02f;
const float EPSILON		 = 0.001f;

IPhysicsWorld* IPhysicsWorld::ms_pWorld = NULL;
IObjectManager* IObjectManager::ms_ObjManager = NULL;

void CPhysicsObject::Update(float fElapsedTime)
{
	if (m_xPushingPosition.isPlaying())
		m_xPushingPosition.Interpolate(fElapsedTime);
	if (m_yPushingPosition.isPlaying())
		m_yPushingPosition.Interpolate(fElapsedTime);
}

void CPhysicsObject::Accumulate(XMFLOAT3* pv3Position)
{
	float fForce = 0.0f;

	if (fabs(m_v3Velocity.x) < EPSILON ||
		fabs(m_v3Velocity.y) < EPSILON ||
		fabs(m_v3Velocity.z) < EPSILON)
	{
		fForce -= (m_fMass * m_fFriction);
	}

	XMVECTOR dir = XMLoadFloat3(&m_v3Direction);
	float s = fForce / m_fMass;

	XMStoreFloat3(&m_v3Acceleration, dir * s);
	XMStoreFloat3(&m_v3Velocity,
		XMLoadFloat3(&m_v3Velocity) + XMLoadFloat3(&m_v3Acceleration));

	XMFLOAT3 vel;
	XMStoreFloat3(&vel, XMLoadFloat3(&m_v3Velocity));

	if (vel.x * m_v3Direction.x < EPSILON) { vel.x = 0.0f; m_v3Direction.x = 0.0f; }
	if (vel.y * m_v3Direction.y < EPSILON) { vel.y = 0.0f; m_v3Direction.y = 0.0f; }
	if (vel.z * m_v3Direction.z < EPSILON) { vel.z = 0.0f; m_v3Direction.z = 0.0f; }

	m_v3Velocity = vel;

	XMStoreFloat3(pv3Position,
		XMLoadFloat3(pv3Position) + XMLoadFloat3(&m_v3Velocity));
}

void CPhysicsObject::IncreaseExternalForce(const XMFLOAT3 & c_rvBasePosition, float fForce)
{	
	// Accumulate Acceleration by External Force
	XMVECTOR dir = XMLoadFloat3(&m_v3Direction);
	float s = fForce / m_fMass;

	XMStoreFloat3(&m_v3Acceleration, dir * s);
	XMStoreFloat3(&m_v3Velocity, dir * s);
/*
	Tracenf("force %f, mass %f, accel (%f, %f, %f)", fForce, m_fMass, 
		m_v3Acceleration.x, 
		m_v3Acceleration.y,
		m_v3Acceleration.z);
*/
	// NOTE : 최종 위치를 구해둔다. 근데 100보다 크다면? ;
	const int LoopValue = 100;
	XMFLOAT3 v3Movement(0.0f, 0.0f, 0.0f);

	for(int i = 0; i < LoopValue; ++i)
	{
		Accumulate(&v3Movement);

		// VICTIM_COLLISION_TEST
		IPhysicsWorld* pWorld = IPhysicsWorld::GetPhysicsWorld();
		if (pWorld)
		{
			XMFLOAT3 pos;
			XMStoreFloat3(&pos, XMLoadFloat3(&c_rvBasePosition) + XMLoadFloat3(&v3Movement));

			if (pWorld->isPhysicalCollision(pos))
			{
				Initialize();
				return;

				//for (float fRatio = 0.0f; fRatio < 1.0f; fRatio += 0.1f)
				//{
				//	// 좀더 정밀하게 체크한다
				//	if (pWorld->isPhysicalCollision(c_rvBasePosition + v3Movement * fRatio))
				//	{
				//		v3Movement = D3DXVECTOR3 (0.0f, 0.0f, 0.0f);
				//		break;
				//	}
				//}
				//break;
			}
		}
		// VICTIM_COLLISION_TEST_END

		if (fabs(m_v3Velocity.x) < EPSILON &&
			fabs(m_v3Velocity.y) < EPSILON &&
			fabs(m_v3Velocity.z) < EPSILON )
			break;
	}	

	SetLastPosition(v3Movement, float(LoopValue) * c_fFrameTime);

	if( m_pActorInstance )
	{
		IObjectManager* pObjectManager = IObjectManager::GetObjectManager();
		pObjectManager->AdjustCollisionWithOtherObjects( m_pActorInstance );
	}
}

void CPhysicsObject::SetLastPosition(const TPixelPosition & c_rPosition, float fBlendingTime)
{
	m_v3LastPosition.x = float(c_rPosition.x);
	m_v3LastPosition.y = float(c_rPosition.y);
	m_v3LastPosition.z = float(c_rPosition.z);
	m_xPushingPosition.Setup(0.0f, c_rPosition.x, fBlendingTime);
	m_yPushingPosition.Setup(0.0f, c_rPosition.y, fBlendingTime);
}

void CPhysicsObject::GetLastPosition(TPixelPosition * pPosition)
{
	pPosition->x = (m_v3LastPosition.x);
	pPosition->y = (m_v3LastPosition.y);
	pPosition->z = (m_v3LastPosition.z);
}

void CPhysicsObject::SetDirection(const XMFLOAT3 & c_rv3Direction)
{
	m_v3Direction.x = c_rv3Direction.x;
	m_v3Direction.y = c_rv3Direction.y;
	m_v3Direction.z = c_rv3Direction.z;
}

float CPhysicsObject::GetXMovement()
{
	return m_xPushingPosition.GetChangingValue();
}

float CPhysicsObject::GetYMovement()
{
	return m_yPushingPosition.GetChangingValue();
}

bool CPhysicsObject::isBlending()
{
	// NOTE : IncreaseExternalForce() 에 의해 밀리는 처리중인가?
	if (XMVectorGetX(XMVector3Length(XMLoadFloat3(&m_v3Velocity))) != 0.0f)
		return true;;

	// NOTE : SetLastPosition() 에 의해 밀리는 처리중인가?
	if (m_xPushingPosition.isPlaying() ||
		m_yPushingPosition.isPlaying())
		return true;

	return false;
}

void CPhysicsObject::Initialize()
{
	m_fMass = 1.0f;
	m_fFriction = 0.3f;
	m_v3Direction = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_v3Acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_v3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_v3LastPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_xPushingPosition.Initialize();
	m_yPushingPosition.Initialize();
}

CPhysicsObject::CPhysicsObject()
{
	m_pActorInstance = NULL;
}

CPhysicsObject::~CPhysicsObject()
{
}
