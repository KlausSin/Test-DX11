#include "StdAfx.h"
#include "PythonBackground.h"
#include "EterLib/Camera.h"

//////////////////////////////////////////////////////////////////////////
// 메세지

extern void SetHeightLog(bool isLog);

float CCamera::CAMERA_MIN_DISTANCE = 200.0f;
float CCamera::CAMERA_MAX_DISTANCE = 2500.0f;

void CCamera::ProcessTerrainCollision()
{
	auto& bg = CPythonBackground::Instance();
	XMFLOAT3 hit;

	if (bg.GetPickingPointWithRayOnlyTerrain(m_kTargetToCameraBottomRay, &hit))
	{
		SetCameraState(CAMERA_STATE_CANTGODOWN);

		XMVECTOR eye = XMLoadFloat3(&m_v3Eye);
		XMVECTOR up = XMLoadFloat3(&m_v3Up);

		XMVECTOR check = eye - up * (2.0f * m_fTerrainCollisionRadius);

		XMFLOAT3 chk;
		XMStoreFloat3(&chk, check);
		chk.z = bg.GetHeight(floorf(chk.x), floorf(chk.y));

		XMVECTOR vNewEye = XMLoadFloat3(&chk) + up * (2.0f * m_fTerrainCollisionRadius);

		XMFLOAT3 newEye;
		XMStoreFloat3(&newEye, vNewEye);

		if (newEye.z > m_v3Eye.z)
			SetEye(newEye);
	}
	else
		SetCameraState(CAMERA_STATE_NORMAL);

	if (bg.GetPickingPointWithRayOnlyTerrain(m_kCameraBottomToTerrainRay, &hit))
	{
		SetCameraState(CAMERA_STATE_CANTGODOWN);

		XMVECTOR eye = XMLoadFloat3(&m_v3Eye);
		XMVECTOR col = XMLoadFloat3(&hit);
		XMVECTOR up = XMLoadFloat3(&m_v3Up);

		XMVECTOR d = eye - col;

		if (XMVectorGetX(XMVector3Length(d)) < 2.0f * m_fTerrainCollisionRadius)
		{
			XMVECTOR vNewEye = col + up * (2.0f * m_fTerrainCollisionRadius);

			XMFLOAT3 newEye;
			XMStoreFloat3(&newEye, vNewEye);

			SetEye(newEye);
		}
	}
	else
		SetCameraState(CAMERA_STATE_NORMAL);
}

struct CameraCollisionChecker
{
	bool m_isBlocked;
	std::vector<XMFLOAT3>* m_pkVct_v3Position;
	CDynamicSphereInstance * m_pdsi;

	CameraCollisionChecker(CDynamicSphereInstance * pdsi, std::vector<XMFLOAT3>* pkVct_v3Position) : m_pdsi(pdsi), m_pkVct_v3Position(pkVct_v3Position), m_isBlocked(false)
	{
	}
	void operator () (CGraphicObjectInstance* pOpponent)
	{
		if (pOpponent->CollisionComponent().CollisionDynamicSphere(*m_pdsi))
 		{
			m_pkVct_v3Position->push_back(pOpponent->TransformComponent().GetPosition());
			m_isBlocked = true;
 		}
	}
};

void CCamera::ProcessBuildingCollision()
{
	float fMoveAmountSmall = 2.0f;
	float fMoveAmountLarge = 4.0f;

	XMFLOAT3 v3CheckVector;

	CDynamicSphereInstance s;
	s.fRadius = m_fObjectCollisionRadius;
	s.v3LastPosition = m_v3Eye;

	Vector3d aVector3d;
	aVector3d.Set(m_v3Eye.x, m_v3Eye.y, m_v3Eye.z);

	// 뒤

	CCullingManager & rkCullingMgr = CCullingManager::Instance();

	{
		XMStoreFloat3(&v3CheckVector,
			XMLoadFloat3(&m_v3Eye) - XMLoadFloat3(&m_v3View) * m_fObjectCollisionRadius
		);
		s.v3Position = v3CheckVector;

		std::vector<XMFLOAT3> kVct_kPosition;
		CameraCollisionChecker kCameraCollisionChecker(&s, &kVct_kPosition);
		rkCullingMgr.ForInRange(aVector3d, m_fObjectCollisionRadius, &kCameraCollisionChecker);
		bool bCollide = kCameraCollisionChecker.m_isBlocked;

		if (bCollide)
		{
			if (m_v3AngularVelocity.y > 0.0f)
			{
				m_v3AngularVelocity.y = 0.0f;
				m_v3AngularVelocity.z += fMoveAmountLarge;
			}
	//		m_v3AngularVelocity.y = fMAX(fMoveAmountSmall, m_v3AngularVelocity.y);
	//// 		m_v3AngularVelocity.y += fMoveAmountSmall;

			if (kVct_kPosition.size() > 1)
			{
	//			m_v3AngularVelocity.z = fMAX(fMoveAmountSmall, m_v3AngularVelocity.z);
 				m_v3AngularVelocity.z += fMoveAmountSmall;
			}
			else
			{
				XMVECTOR d3dd = XMLoadFloat3(&kVct_kPosition[0]) - XMLoadFloat3(&m_v3Eye);

				XMVECTOR vCross = XMVector3Cross(d3dd, XMLoadFloat3(&m_v3View));

				float fDot = XMVectorGetX(XMVector3Dot(vCross, XMLoadFloat3(&m_v3Up)));
				if (fDot < 0)
				{
	//				m_v3AngularVelocity.x = fMIN(-fMoveAmountSmall, m_v3AngularVelocity.x);
 					m_v3AngularVelocity.x -= fMoveAmountSmall;
				}
				else if(fDot > 0)
				{
	//				m_v3AngularVelocity.x = fMAX(fMoveAmountSmall, m_v3AngularVelocity.x);
 					m_v3AngularVelocity.x += fMoveAmountSmall;
				}
				else
				{
	//				m_v3AngularVelocity.z = fMAX(fMoveAmountSmall, m_v3AngularVelocity.z);
 					m_v3AngularVelocity.z += fMoveAmountSmall;
				}
			}
		}
	}

	// 위
	{
		XMVECTOR eye = XMLoadFloat3(&m_v3Eye);
		XMVECTOR up = XMLoadFloat3(&m_v3Up);

		XMStoreFloat3(&v3CheckVector,
			eye + up * (2.0f * m_fObjectCollisionRadius)
		);

		s.v3Position = v3CheckVector;

		std::vector<XMFLOAT3> kVct_kPosition;
		CameraCollisionChecker kCameraCollisionChecker(&s, &kVct_kPosition);
		rkCullingMgr.ForInRange(aVector3d, m_fObjectCollisionRadius, &kCameraCollisionChecker);

		bool bCollide = kCameraCollisionChecker.m_isBlocked;

		if (bCollide)
		{
			m_v3AngularVelocity.z = std::min(-fMoveAmountSmall, m_v3AngularVelocity.y);
		}
	}

	// 좌
	{
		XMVECTOR eye = XMLoadFloat3(&m_v3Eye);
		XMVECTOR cross = XMLoadFloat3(&m_v3Cross);

		XMStoreFloat3(&v3CheckVector,
			eye + cross * (3.0f * m_fObjectCollisionRadius)
		);

		s.v3Position = v3CheckVector;

		std::vector<XMFLOAT3> kVct_kPosition;
		CameraCollisionChecker kCameraCollisionChecker(&s, &kVct_kPosition);
		rkCullingMgr.ForInRange(aVector3d, m_fObjectCollisionRadius, &kCameraCollisionChecker);

		bool bCollide = kCameraCollisionChecker.m_isBlocked;

		if (bCollide)
		{
			if (m_v3AngularVelocity.x > 0.0f)
			{
				m_v3AngularVelocity.x = 0.0f;
				m_v3AngularVelocity.y += fMoveAmountLarge;
			}
		}
	}

	// 우
	{
		XMVECTOR eye = XMLoadFloat3(&m_v3Eye);
		XMVECTOR up = XMLoadFloat3(&m_v3Up);
		XMVECTOR view = XMLoadFloat3(&m_v3View);
		XMVECTOR cross = XMLoadFloat3(&m_v3Cross);

		std::vector<XMFLOAT3> kVct_kPosition;

		// LEFT
		{
			XMStoreFloat3(&v3CheckVector,
				eye - cross * (3.0f * m_fObjectCollisionRadius)
			);

			s.v3Position = v3CheckVector;

			CameraCollisionChecker checker(&s, &kVct_kPosition);
			rkCullingMgr.ForInRange(aVector3d, m_fObjectCollisionRadius, &checker);

			if (checker.m_isBlocked)
			{
				if (m_v3AngularVelocity.x < 0.0f)
				{
					m_v3AngularVelocity.x = 0.0f;
					m_v3AngularVelocity.y += fMoveAmountLarge;
				}
			}
		}

		// DOWN
		{
			XMStoreFloat3(&v3CheckVector,
				eye - up * (2.0f * m_fTerrainCollisionRadius)
			);

			s.v3Position = v3CheckVector;

			CameraCollisionChecker checker(&s, &kVct_kPosition);
			rkCullingMgr.ForInRange(aVector3d, m_fObjectCollisionRadius, &checker);

			if (checker.m_isBlocked)
			{
				if (m_v3AngularVelocity.z < 0.0f)
				{
					m_v3AngularVelocity.z = 0.0f;
					m_v3AngularVelocity.y += fMoveAmountLarge;
				}
			}
		}

		// FRONT
		{
			XMStoreFloat3(&v3CheckVector,
				eye + view * (4.0f * m_fObjectCollisionRadius)
			);

			s.v3Position = v3CheckVector;

			CameraCollisionChecker checker(&s, &kVct_kPosition);
			rkCullingMgr.ForInRange(aVector3d, m_fObjectCollisionRadius, &checker);

			bool bCollide = checker.m_isBlocked;

			if (bCollide)
			{
				if (m_v3AngularVelocity.y < 0.0f)
				{
					m_v3AngularVelocity.y = 0.0f;
					m_v3AngularVelocity.z += fMoveAmountLarge;
				}

				if (kVct_kPosition.size() > 1)
				{
					m_v3AngularVelocity.z += fMoveAmountLarge;
				}
				else if (!kVct_kPosition.empty())
				{
					XMVECTOR hit = XMLoadFloat3(&kVct_kPosition[0]);
					XMVECTOR d = hit - eye;

					XMVECTOR cros = XMVector3Cross(d, view);
					float fDot = XMVectorGetX(XMVector3Dot(cros, up));

					if (fDot < 0)
						m_v3AngularVelocity.x -= fMoveAmountSmall;
					else if (fDot > 0)
						m_v3AngularVelocity.x += fMoveAmountSmall;
					else
						m_v3AngularVelocity.z += fMoveAmountSmall;
				}
			}
		}
	}
}

void CCamera::Update()
{
	RotateEyeAroundTarget(m_v3AngularVelocity.z, m_v3AngularVelocity.x);

	float dist = GetDistance();
	float newDist = std::clamp(dist - m_v3AngularVelocity.y,
		CAMERA_MIN_DISTANCE,
		CAMERA_MAX_DISTANCE);

	SetDistance(newDist);

	if (m_bProcessTerrainCollision)
		ProcessTerrainCollision();

	XMVECTOR v = XMLoadFloat3(&m_v3AngularVelocity);
	v = XMVectorScale(v, 0.5f);
	XMStoreFloat3(&m_v3AngularVelocity, v);

	if (fabs(m_v3AngularVelocity.x) < 1.0f) m_v3AngularVelocity.x = 0.0f;
	if (fabs(m_v3AngularVelocity.y) < 1.0f) m_v3AngularVelocity.y = 0.0f;
	if (fabs(m_v3AngularVelocity.z) < 1.0f) m_v3AngularVelocity.z = 0.0f;

	const float movable = CAMERA_MAX_DISTANCE - CAMERA_MIN_DISTANCE;
	const float delta = CAMERA_TARGET_FACE - CAMERA_TARGET_STANDARD;

	float cur = CAMERA_MAX_DISTANCE - GetDistance();
	float target = CAMERA_TARGET_STANDARD + delta * cur / movable;

	SetTargetHeight(target);

#ifdef __20040725_CAMERA_WORK__
	m_MovementPosition += m_MovementSpeed;

	if (m_MovementPosition.m_fViewDir != 0.0f)
		MoveFront(m_MovementPosition.m_fViewDir);
	if (m_MovementPosition.m_fCrossDir != 0.0f)
		MoveAlongCross(m_MovementPosition.m_fCrossDir);
	if (m_MovementPosition.m_fUpDir != 0.0f)
		MoveVertical(m_MovementPosition.m_fUpDir);
#endif
}
