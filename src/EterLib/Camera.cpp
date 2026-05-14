// Camera.cpp: implementation of the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "EterBase/Utils.h"
#include "Camera.h"
#include <cmath>

const float c_fDefaultResistance = 0.3f;

CCameraManager aCameraManager;	// CCameraManager Instance

void CCamera::SetCameraMaxDistance(float fMax)
{
	CAMERA_MAX_DISTANCE = fMax;
}

float CCamera::GetTargetHeight()
{
	return m_fTarget_;
}

void CCamera::SetTargetHeight(float fTarget)
{
	m_fTarget_=fTarget;	
}


//////////////////////////////////////////////////////////////////////////
// CCamera
//////////////////////////////////////////////////////////////////////////


CCamera::CCamera() :
m_fEyeGroundHeightRatio(0.3f),
m_fTargetHeightLimitRatio(2.0f),
m_fResistance(c_fDefaultResistance),
m_isLock(false)
{
	m_fDistance						= 1.0f;
	m_eCameraState					= CAMERA_STATE_NORMAL;
	m_eCameraStatePrev				= CAMERA_STATE_NORMAL;
	m_ulNumScreenBuilding			= 0;

	m_fPitchSum = 0.0f;
	m_fRollSum = 0.0f;

	m_fTerrainCollisionRadius		= 50.0f;
	m_fObjectCollisionRadius		= 50.0f;

	m_bDrag							= false;
	
	m_lMousePosX					= -1;
	m_lMousePosY					= -1;

	m_fTarget_						= CAMERA_TARGET_STANDARD;

	m_v3AngularAcceleration			= { 0.0f, 0.0f, 0.0f };
	m_v3AngularVelocity				= { 0.0f, 0.0f, 0.0f };

	m_bProcessTerrainCollision		= true;

    SetViewParams(XMFLOAT3(0.0f,0.0f,1.0f), XMFLOAT3(0.0f,0.0f,0.0f), XMFLOAT3(0.0f,1.0f,0.0f));
}

CCamera::~CCamera()
{
}

void CCamera::Lock()
{
	m_isLock = true;
}

void CCamera::Unlock()
{
	m_isLock = false;
}

bool CCamera::IsLock()
{
	return m_isLock;
}

void CCamera::SetResistance(float fResistance)
{
	m_fResistance = c_fDefaultResistance * fResistance;
}

void CCamera::Wheel(int nLen)
{
	if (IsLock())
		return;

	m_v3AngularVelocity.y = (float)(nLen) * m_fResistance;
}

void CCamera::BeginDrag(int nMouseX, int nMouseY)
{
	if (IsLock())
		return;

	m_bDrag = true;
	m_lMousePosX = nMouseX;
	m_lMousePosY = nMouseY;
	m_fPitchSum = 0.0f;
	m_fRollSum = 0.0f;
}

bool CCamera::IsDraging()
{
	if (IsLock())
		return false;

	return m_bDrag;
}

bool CCamera::EndDrag()
{
	if (IsLock())
		return false;

	m_bDrag = false;

	float fSum=sqrt(m_fPitchSum*m_fPitchSum+m_fRollSum*m_fRollSum);

	m_fPitchSum = 0.0f;
	m_fRollSum = 0.0f;
	
	if (fSum<1.0f)
		return false;

	return true;
}

bool CCamera::Drag(int nMouseX, int nMouseY, LPPOINT lpReturnPoint)
{
	if (IsLock())
		return false;

	if (!m_bDrag)
	{
		m_lMousePosX = nMouseX;
		m_lMousePosY = nMouseY;
		lpReturnPoint->x = m_lMousePosX;
		lpReturnPoint->y = m_lMousePosY;
		return false;
	}
	
	long lMouseX = nMouseX;
	long lMouseY = nMouseY;
	
	float fNewPitchVelocity = (float)(lMouseY - m_lMousePosY) * m_fResistance;
	float fNewRotationVelocity = (float)(lMouseX - m_lMousePosX) * m_fResistance;

	m_fPitchSum += fNewPitchVelocity;
	m_fRollSum += fNewRotationVelocity;
	
	
	if (CAMERA_STATE_CANTGOLEFT == GetCameraState())
		fNewRotationVelocity = fMAX(0.0f, fNewRotationVelocity);
	if (CAMERA_STATE_CANTGORIGHT == GetCameraState())
		fNewRotationVelocity = fMIN(0.0f, fNewRotationVelocity);
	if (CAMERA_STATE_CANTGODOWN == GetCameraState())
		fNewPitchVelocity = fMAX(0.0f, fNewPitchVelocity);
	
	m_v3AngularVelocity.x = fNewRotationVelocity;
	m_v3AngularVelocity.z = fNewPitchVelocity;

	lpReturnPoint->x = m_lMousePosX;
	lpReturnPoint->y = m_lMousePosY;
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Update

void CCamera::SetCameraState(eCameraState eNewCameraState)
{
	if (eNewCameraState == m_eCameraState)
		return;

	m_eCameraStatePrev = m_eCameraState;
	m_eCameraState = eNewCameraState;

/*
	if ((CAMERA_STATE_NORMAL == m_eCameraStatePrev))
	{
		m_fDistanceBackup = m_fDistance;
		m_fPitchBackup = m_fPitch;
		m_fRollBackup = m_fRoll;
	}
	else if ((CAMERA_STATE_CANTGODOWN == m_eCameraStatePrev) && (CAMERA_STATE_CANTGODOWN == m_eCameraState) )
	{
		m_v3EyeBackup = m_v3Eye;
	}
*/
}

void CCamera::IncreaseNumSrcreenBuilding()
{
	++m_ulNumScreenBuilding;
}

void CCamera::ResetNumScreenBuilding()
{
	m_ulNumScreenBuilding = 0;
}

//////////////////////////////////////////////////////////////////////////
// Property
void CCamera::SetViewParams( const XMFLOAT3&v3Eye, const XMFLOAT3& v3Target, const XMFLOAT3& v3Up)
{
	if (IsLock())
		return;

    // Set attributes for the view matrix
    m_v3Eye = v3Eye;
    m_v3Target = v3Target;
    m_v3Up = v3Up;

	// Avoid screen flipping with nan values in the view matrix
	if (!m_v3Eye.y)
		m_v3Eye.y = 0.1f;

	SetViewMatrix();
}

void CCamera::SetEye(const XMFLOAT3& v3Eye)
{
	if (IsLock())
		return;

    m_v3Eye = v3Eye;

	SetViewMatrix();
}

void CCamera::SetTarget(const XMFLOAT3& v3Target)
{
	if (IsLock())
		return;

    m_v3Target = v3Target;

	SetViewMatrix();
}

void CCamera::SetUp(const XMFLOAT3& v3Up)
{
	if (IsLock())
		return;

    m_v3Up = v3Up;

	SetViewMatrix();
}

bool IsNaN(const XMFLOAT3& v)
{
	return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z);
}

void CCamera::SetViewMatrix()
{
	m_v3View = {
		m_v3Target.x - m_v3Eye.x,
		m_v3Target.y - m_v3Eye.y,
		m_v3Target.z - m_v3Eye.z
	};

	if (IsNaN(m_v3View))
		m_v3View = { 0.0f, 0.0f, 1.0f };

	XMFLOAT3 v3CenterRay = { -m_v3View.x, -m_v3View.y, -m_v3View.z };

	CalculateRoll();

	m_fDistance = XMVectorGetX(XMVector3Length(XMLoadFloat3(&m_v3View)));
	if (std::isnan(m_fDistance))
		m_fDistance = 0.0f;

	m_fDistance = std::fmax(0.0f, m_fDistance);
	assert(m_fDistance >= 0.0f);

	if (m_fDistance > FLT_EPSILON)
		XMStoreFloat3(&m_v3View, XMVector3Normalize(XMLoadFloat3(&m_v3View)));
	else
		m_v3View = { 0.0f, 0.0f, 1.0f };

	XMStoreFloat3(&m_v3Cross, XMVector3Cross(XMLoadFloat3(&m_v3Up), XMLoadFloat3(&m_v3View)));

	float crossLength = XMVectorGetX(XMVector3Length(XMLoadFloat3(&m_v3Cross)));
	if (crossLength > FLT_EPSILON)
		XMStoreFloat3(&m_v3Cross, XMVector3Normalize(XMLoadFloat3(&m_v3Cross)));
	else
		m_v3Cross = { 1.0f, 0.0f, 0.0f };

	XMStoreFloat3(&m_v3Up, XMVector3Cross(XMLoadFloat3(&m_v3View), XMLoadFloat3(&m_v3Cross)));

	float upLength = XMVectorGetX(XMVector3Length(XMLoadFloat3(&m_v3Up)));
	if (upLength > FLT_EPSILON)
		XMStoreFloat3(&m_v3Up, XMVector3Normalize(XMLoadFloat3(&m_v3Up)));
	else
		m_v3Up = { 0.0f, 1.0f, 0.0f };

	XMFLOAT3 val = { 0.0f, 0.0f, 1.0f };

	m_fPitch = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_v3Up), XMLoadFloat3(&val)));

	if (m_fPitch >= 1.0f)
		m_fPitch = 1.0f;
	else if (m_fPitch <= -1.0f)
		m_fPitch = -1.0f;

	m_fPitch = acosf(m_fPitch);

	if (std::isnan(m_fPitch))
		m_fPitch = 0.0f;

	m_fPitch *= (180.0f / XM_PI);

	if (m_v3View.z > 0.0f)
		m_fPitch = -m_fPitch;

	XMMATRIX matView = XMMatrixLookAtRH(
		XMLoadFloat3(&m_v3Eye),
		XMLoadFloat3(&m_v3Target),
		XMLoadFloat3(&m_v3Up));

	XMStoreFloat4x4(&m_matView, matView);

	XMVECTOR determinant;
	XMMATRIX matInverseView = XMMatrixInverse(&determinant, matView);
	float fDeterminantD3DMatView = XMVectorGetX(determinant);

	if (std::isnan(fDeterminantD3DMatView) || fabsf(fDeterminantD3DMatView) < FLT_EPSILON)
		XMStoreFloat4x4(&m_matInverseView, XMMatrixIdentity());
	else
		XMStoreFloat4x4(&m_matInverseView, matInverseView);

	m_matBillboard = m_matInverseView;
	m_matBillboard._41 = 0.0f;
	m_matBillboard._42 = 0.0f;
	m_matBillboard._43 = 0.0f;

	m_ViewRay.SetStartPoint(m_v3Target);
	m_ViewRay.SetDirection(v3CenterRay, m_fDistance);

	m_kCameraBottomToTerrainRay.SetStartPoint(m_v3Eye);
	m_kCameraFrontToTerrainRay.SetStartPoint(m_v3Eye);
	m_kCameraBackToTerrainRay.SetStartPoint(m_v3Eye);
	m_kCameraLeftToTerrainRay.SetStartPoint(m_v3Eye);
	m_kCameraRightToTerrainRay.SetStartPoint(m_v3Eye);
	m_kTargetToCameraBottomRay.SetStartPoint(m_v3Target);

	m_kCameraBottomToTerrainRay.SetDirection({ -m_v3Up.x, -m_v3Up.y, -m_v3Up.z }, 2.0f * m_fTerrainCollisionRadius);
	m_kCameraFrontToTerrainRay.SetDirection(m_v3View, 4.0f * m_fTerrainCollisionRadius);
	m_kCameraBackToTerrainRay.SetDirection({ -m_v3View.x, -m_v3View.y, -m_v3View.z }, m_fTerrainCollisionRadius);
	m_kCameraLeftToTerrainRay.SetDirection({ -m_v3Cross.x, -m_v3Cross.y, -m_v3Cross.z }, 3.0f * m_fTerrainCollisionRadius);
	m_kCameraRightToTerrainRay.SetDirection(m_v3Cross, 3.0f * m_fTerrainCollisionRadius);

	XMFLOAT3 temp = {
		v3CenterRay.x - m_fTerrainCollisionRadius * m_v3Up.x,
		v3CenterRay.y - m_fTerrainCollisionRadius * m_v3Up.y,
		v3CenterRay.z - m_fTerrainCollisionRadius * m_v3Up.z
	};

	m_kTargetToCameraBottomRay.SetDirection(temp, XMVectorGetX(XMVector3Length(XMLoadFloat3(&temp))));

	m_kLeftObjectCollisionRay.SetStartPoint(m_v3Target);
	m_kTopObjectCollisionRay.SetStartPoint(m_v3Target);
	m_kRightObjectCollisionRay.SetStartPoint(m_v3Target);
	m_kBottomObjectCollisionRay.SetStartPoint(m_v3Target);

	XMFLOAT3 val1 = {
		v3CenterRay.x + m_fObjectCollisionRadius * m_v3Cross.x,
		v3CenterRay.y + m_fObjectCollisionRadius * m_v3Cross.y,
		v3CenterRay.z + m_fObjectCollisionRadius * m_v3Cross.z
	};
	m_kLeftObjectCollisionRay.SetDirection(val1, XMVectorGetX(XMVector3Length(XMLoadFloat3(&val1))));

	XMFLOAT3 val2 = {
		v3CenterRay.x - m_fObjectCollisionRadius * m_v3Cross.x,
		v3CenterRay.y - m_fObjectCollisionRadius * m_v3Cross.y,
		v3CenterRay.z - m_fObjectCollisionRadius * m_v3Cross.z
	};
	m_kRightObjectCollisionRay.SetDirection(val2, XMVectorGetX(XMVector3Length(XMLoadFloat3(&val2))));

	XMFLOAT3 val3 = {
		v3CenterRay.x + m_fObjectCollisionRadius * m_v3Up.x,
		v3CenterRay.y + m_fObjectCollisionRadius * m_v3Up.y,
		v3CenterRay.z + m_fObjectCollisionRadius * m_v3Up.z
	};
	m_kTopObjectCollisionRay.SetDirection(val3, XMVectorGetX(XMVector3Length(XMLoadFloat3(&val3))));

	XMFLOAT3 val4 = {
		v3CenterRay.x - m_fObjectCollisionRadius * m_v3Up.x,
		v3CenterRay.y - m_fObjectCollisionRadius * m_v3Up.y,
		v3CenterRay.z - m_fObjectCollisionRadius * m_v3Up.z
	};
	m_kBottomObjectCollisionRay.SetDirection(val4, XMVectorGetX(XMVector3Length(XMLoadFloat3(&val4))));
}

void CCamera::Move(const XMFLOAT3& v3Displacement)
{
	if (IsLock())
		return;

	m_v3Eye.x += v3Displacement.x;
	m_v3Eye.y += v3Displacement.y;
	m_v3Eye.z += v3Displacement.z;

	m_v3Target.x += v3Displacement.x;
	m_v3Target.y += v3Displacement.y;
	m_v3Target.z += v3Displacement.z;

	SetViewMatrix();
}

void CCamera::Zoom(float fRatio)
{
	if (IsLock())
		return;

	if (fRatio == 1.0f)
		return;

	XMFLOAT3 v3Temp = {
		m_v3Eye.x - m_v3Target.x,
		m_v3Eye.y - m_v3Target.y,
		m_v3Eye.z - m_v3Target.z
	};

	v3Temp.x *= fRatio;
	v3Temp.y *= fRatio;
	v3Temp.z *= fRatio;

	m_v3Eye.x = v3Temp.x + m_v3Target.x;
	m_v3Eye.y = v3Temp.y + m_v3Target.y;
	m_v3Eye.z = v3Temp.z + m_v3Target.z;

	SetViewMatrix();
}

void CCamera::MoveAlongView(float fDistance)
{
	if (IsLock())
		return;

	XMFLOAT3 v3Temp;
	XMStoreFloat3(&v3Temp, XMVector3Normalize(XMLoadFloat3(&m_v3View)));

	v3Temp.x *= fDistance;
	v3Temp.y *= fDistance;
	v3Temp.z *= fDistance;

	m_v3Eye.x += v3Temp.x;
	m_v3Eye.y += v3Temp.y;
	m_v3Eye.z += v3Temp.z;

	m_v3Target.x += v3Temp.x;
	m_v3Target.y += v3Temp.y;
	m_v3Target.z += v3Temp.z;

	SetViewMatrix();
}

void CCamera::MoveAlongCross(float fDistance)
{
	if (IsLock())
		return;

	XMFLOAT3 v3Temp;
	XMStoreFloat3(&v3Temp, XMVector3Normalize(XMLoadFloat3(&m_v3Cross)));

	v3Temp.x *= fDistance;
	v3Temp.y *= fDistance;
	v3Temp.z *= fDistance;

	m_v3Eye.x += v3Temp.x;
	m_v3Eye.y += v3Temp.y;
	m_v3Eye.z += v3Temp.z;

	m_v3Target.x += v3Temp.x;
	m_v3Target.y += v3Temp.y;
	m_v3Target.z += v3Temp.z;

	SetViewMatrix();
}

void CCamera::MoveAlongUp(float fDistance)
{
	if (IsLock())
		return;

	XMFLOAT3 v3Temp;
	XMStoreFloat3(&v3Temp, XMVector3Normalize(XMLoadFloat3(&m_v3Up)));

	v3Temp.x *= fDistance;
	v3Temp.y *= fDistance;
	v3Temp.z *= fDistance;

	m_v3Eye.x += v3Temp.x;
	m_v3Eye.y += v3Temp.y;
	m_v3Eye.z += v3Temp.z;

	m_v3Target.x += v3Temp.x;
	m_v3Target.y += v3Temp.y;
	m_v3Target.z += v3Temp.z;

	SetViewMatrix();
}


void CCamera::MoveLateral(float fDistance)
{
	if (IsLock())
		return;

	MoveAlongCross(fDistance);
}

void CCamera::MoveFront(float fDistance)
{
	if (IsLock())
		return;

	XMFLOAT3 v3Temp = { m_v3View.x, m_v3View.y, 0.0f };

	XMStoreFloat3(&v3Temp, XMVector3Normalize(XMLoadFloat3(&v3Temp)));

	v3Temp.x *= fDistance;
	v3Temp.y *= fDistance;
	v3Temp.z *= fDistance;

	m_v3Eye.x += v3Temp.x;
	m_v3Eye.y += v3Temp.y;
	m_v3Eye.z += v3Temp.z;

	m_v3Target.x += v3Temp.x;
	m_v3Target.y += v3Temp.y;
	m_v3Target.z += v3Temp.z;

	SetViewMatrix();
}

void CCamera::MoveVertical(float fDistance) 
{
	if (IsLock())
		return;

	m_v3Eye.z += fDistance;
	m_v3Target.z += fDistance;

	SetViewMatrix();
}

void CCamera::RotateEyeAroundTarget(float fPitchDegree, float fRollDegree)
{
	if (IsLock())
		return;

	if (m_fPitch + fPitchDegree > 80.0f)
		fPitchDegree = 80.0f - m_fPitch;
	else if (m_fPitch + fPitchDegree < -80.0f)
		fPitchDegree = -80.0f - m_fPitch;

	XMMATRIX matRotPitch = XMMatrixRotationAxis(XMLoadFloat3(&m_v3Cross), XMConvertToRadians(fPitchDegree));
	XMMATRIX matRotRoll = XMMatrixRotationZ(-XMConvertToRadians(fRollDegree));
	XMMATRIX matRot = matRotPitch * matRotRoll;

	XMFLOAT3 v3Temp = {
		m_v3Eye.x - m_v3Target.x,
		m_v3Eye.y - m_v3Target.y,
		m_v3Eye.z - m_v3Target.z
	};

	XMStoreFloat3(&m_v3Eye, XMVector3TransformCoord(XMLoadFloat3(&v3Temp), matRot));

	m_v3Eye.x += m_v3Target.x;
	m_v3Eye.y += m_v3Target.y;
	m_v3Eye.z += m_v3Target.z;

	SetUp({ 0.0f, 0.0f, 1.0f });

	m_fRoll += fRollDegree;

	if (m_fRoll > 360.0f)
		m_fRoll -= 360.0f;
	else if (m_fRoll < -360.0f)
		m_fRoll += 360.0f;
}

void CCamera::RotateEyeAroundPoint(const XMFLOAT3& v3Point, float fPitchDegree, float fRollDegree)
{
	XMMATRIX matRotPitch = XMMatrixRotationAxis(XMLoadFloat3(&m_v3Cross), XMConvertToRadians(fPitchDegree));
	XMMATRIX matRotRoll = XMMatrixRotationZ(-XMConvertToRadians(fRollDegree));
	XMMATRIX matRot = matRotPitch * matRotRoll;

	XMFLOAT3 v3Temp = {
		m_v3Eye.x - v3Point.x,
		m_v3Eye.y - v3Point.y,
		m_v3Eye.z - v3Point.z
	};

	XMStoreFloat3(&m_v3Eye, XMVector3TransformCoord(XMLoadFloat3(&v3Temp), matRot));

	m_v3Eye.x += v3Point.x;
	m_v3Eye.y += v3Point.y;
	m_v3Eye.z += v3Point.z;

	XMFLOAT3 vv2 = {
		v3Temp.x + m_v3Up.x,
		v3Temp.y + m_v3Up.y,
		v3Temp.z + m_v3Up.z
	};

	XMStoreFloat3(&m_v3Up, XMVector3TransformCoord(XMLoadFloat3(&vv2), matRot));

	m_v3Up.x -= m_v3Eye.x - v3Point.x;
	m_v3Up.y -= m_v3Eye.y - v3Point.y;
	m_v3Up.z -= m_v3Eye.z - v3Point.z;

	v3Temp = {
		m_v3Target.x - v3Point.x,
		m_v3Target.y - v3Point.y,
		m_v3Target.z - v3Point.z
	};

	XMStoreFloat3(&m_v3Target, XMVector3TransformCoord(XMLoadFloat3(&v3Temp), matRot));

	m_v3Target.x += v3Point.x;
	m_v3Target.y += v3Point.y;
	m_v3Target.z += v3Point.z;

	SetViewMatrix();
}

void CCamera::Pitch(const float fPitchDelta)
{
	RotateEyeAroundTarget(fPitchDelta, 0.0f);
}

void CCamera::Roll(const float fRollDelta)
{
	RotateEyeAroundTarget(0.0f, fRollDelta);
}

void CCamera::SetDistance(const float fdistance)
{
	Zoom(fdistance/m_fDistance);
}

void CCamera::CalculateRoll()
{
	XMFLOAT2 v2ViewXY;
	v2ViewXY.x = m_v3View.x;
	v2ViewXY.y = m_v3View.y;

	XMStoreFloat2(&v2ViewXY, XMVector2Normalize(XMLoadFloat2(&v2ViewXY)));

	const XMFLOAT2 vv = { 0.0f, 1.0f };

	float fDot = XMVectorGetX(XMVector2Dot(XMLoadFloat2(&v2ViewXY), XMLoadFloat2(&vv)));

	if (fDot >= 1.0f)
		fDot = 1.0f;
	else if (fDot <= -1.0f)
		fDot = -1.0f;

	fDot = acosf(fDot);
	fDot *= (180.0f / XM_PI);

	float fCross =
		v2ViewXY.x * vv.y -
		v2ViewXY.y * vv.x;

	if (fCross < 0.0f)
		fDot = -fDot;

	m_fRoll = fDot;
}

//////////////////////////////////////////////////////////////////////////
// CCameraMananger
//////////////////////////////////////////////////////////////////////////

CCameraManager::CCameraManager() :
m_pCurrentCamera(NULL),
m_pPreviousCamera(NULL)
{
	AddCamera(DEFAULT_PERSPECTIVE_CAMERA);
	AddCamera(DEFAULT_ORTHO_CAMERA);

	SetCurrentCamera(DEFAULT_PERSPECTIVE_CAMERA);
}

CCameraManager::~CCameraManager()
{
	for (TCameraMap::iterator itor = m_CameraMap.begin(); itor != m_CameraMap.end(); ++itor)
	{
		delete (*itor).second;
	}
	m_CameraMap.clear();
}

CCamera * CCameraManager::GetCurrentCamera()
{
	if (!m_pCurrentCamera)
		assert(false);
	return m_pCurrentCamera;
}

void CCameraManager::SetCurrentCamera(unsigned char ucCameraNum)
{
	if (m_pCurrentCamera != m_CameraMap[ucCameraNum])
		m_pPreviousCamera = m_pCurrentCamera;

	m_pCurrentCamera = m_CameraMap[ucCameraNum];
}

void CCameraManager::ResetToPreviousCamera()
{
	if (!m_pPreviousCamera)
		assert(false);
	m_pCurrentCamera = m_pPreviousCamera;
	m_pPreviousCamera = NULL;
}

bool CCameraManager::isCurrentCamera(unsigned char ucCameraNum)
{
	if (m_CameraMap[ucCameraNum] == m_pCurrentCamera)
		return true;
	return false;
}

// 잡스러운 함수들...
bool CCameraManager::AddCamera(unsigned char ucCameraNum)
{
	if(m_CameraMap.end() != m_CameraMap.find(ucCameraNum))
		return false;
	m_CameraMap.insert(TCameraMap::value_type(ucCameraNum, new CCamera));
	return true;
}

bool CCameraManager::RemoveCamera(unsigned char ucCameraNum)
{
	TCameraMap::iterator itor = m_CameraMap.find(ucCameraNum);
	if(m_CameraMap.end() == itor)
		return false;
	m_CameraMap.erase(itor);
	return true;
}

unsigned char CCameraManager::GetCurrentCameraNum()
{
	if (!m_pCurrentCamera)
		return NO_CURRENT_CAMERA;
	for (TCameraMap::iterator itor = m_CameraMap.begin(); itor != m_CameraMap.end(); ++itor)
		if(m_pCurrentCamera == (*itor).second)
			return (*itor).first;
	return NO_CURRENT_CAMERA;
}

bool CCameraManager::isTerrainCollisionEnable()
{
	return m_pCurrentCamera->isTerrainCollisionEnable();
}

void CCameraManager::SetTerrainCollision(bool bEnable)
{
	m_pCurrentCamera->SetTerrainCollision(bEnable);
}
