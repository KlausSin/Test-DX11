#include "StdAfx.h"
#include "EterBase/Utils.h"
#include "EterBase/Timer.h"
#include "GrpBase.h"
#include "Camera.h"
#include "StateManager.h"
#include "qMin32Lib/All.h"

void PixelPositionToD3DXVECTOR3(const XMFLOAT3& c_rkPPosSrc, XMFLOAT3* pv3Dst)
{
	pv3Dst->x = +c_rkPPosSrc.x;
	pv3Dst->y = -c_rkPPosSrc.y;
	pv3Dst->z = +c_rkPPosSrc.z;
}

void D3DXVECTOR3ToPixelPosition(const XMFLOAT3& c_rv3Src, XMFLOAT3* pv3Dst)
{
	pv3Dst->x = +c_rv3Src.x;
	pv3Dst->y = -c_rv3Src.y;
	pv3Dst->z = +c_rv3Src.z;
}

HWND CGraphicBase::ms_hWnd;
HDC CGraphicBase::ms_hDC;

std::vector<XMFLOAT4X4> CGraphicBase::ms_matStack;
D3D11_VIEWPORT			CGraphicBase::ms_Viewport;

HRESULT					CGraphicBase::ms_hLastResult = NULL;

int						CGraphicBase::ms_iWidth;
int						CGraphicBase::ms_iHeight;

DWORD					CGraphicBase::ms_faceCount = 0;

XMFLOAT4X4				CGraphicBase::ms_matIdentity;
XMFLOAT4X4				CGraphicBase::ms_matView;
XMFLOAT4X4				CGraphicBase::ms_matProj;
XMFLOAT4X4				CGraphicBase::ms_matInverseView;
XMFLOAT4X4				CGraphicBase::ms_matInverseViewYAxis;
XMFLOAT4X4				CGraphicBase::ms_matWorld;
XMFLOAT4X4				CGraphicBase::ms_matWorldView;
XMFLOAT4X4				CGraphicBase::ms_matScreen0;
XMFLOAT4X4				CGraphicBase::ms_matScreen1;
XMFLOAT4X4				CGraphicBase::ms_matScreen2;

XMFLOAT3				CGraphicBase::ms_vtPickRayOrig;
XMFLOAT3				CGraphicBase::ms_vtPickRayDir;

float					CGraphicBase::ms_fFieldOfView;
float					CGraphicBase::ms_fNearY;
float					CGraphicBase::ms_fFarY;
float					CGraphicBase::ms_fAspect;

DWORD					CGraphicBase::ms_dwWavingEndTime;
int						CGraphicBase::ms_iWavingPower;
DWORD					CGraphicBase::ms_dwFlashingEndTime;
XMFLOAT4 				CGraphicBase::ms_FlashingColor;

// Terrain picking용 Ray... CCamera 이용하는 버전.. 기존의 Ray와 통합 필요...
CRay					CGraphicBase::ms_Ray;
bool					CGraphicBase::ms_bSupportDXT = true;
bool					CGraphicBase::ms_isLowTextureMemory = false;
bool					CGraphicBase::ms_isHighTextureMemory = false;

// 2004.11.18.myevan.DynamicVertexBuffer로 교체
/*
std::vector<TIndex>		CGraphicBase::ms_lineIdxVector;
std::vector<TIndex>		CGraphicBase::ms_lineTriIdxVector;
std::vector<TIndex>		CGraphicBase::ms_lineRectIdxVector;
std::vector<TIndex>		CGraphicBase::ms_lineCubeIdxVector;

std::vector<TIndex>		CGraphicBase::ms_fillTriIdxVector;
std::vector<TIndex>		CGraphicBase::ms_fillRectIdxVector;
std::vector<TIndex>		CGraphicBase::ms_fillCubeIdxVector;
*/

// D3D11
ID3D11Device*			CGraphicBase::ms_lpd3d11Device = NULL;
ID3D11DeviceContext*	CGraphicBase::ms_lpd3d11Context = NULL;
IDXGISwapChain*			CGraphicBase::ms_lpd3d11SwapChain = NULL;
ID3D11RenderTargetView*	CGraphicBase::ms_lpd3d11RTV = NULL;
ID3D11DepthStencilView*	CGraphicBase::ms_lpd3d11DSV = NULL;
ID3D11Texture2D*		CGraphicBase::ms_lpd3d11DepthStencil = NULL;
UniquePtr<DxManager>	CGraphicBase::m_mgr = nullptr;

VBufferPtr	CGraphicBase::ms_alpd3dPDTVB[PDT_VERTEXBUFFER_NUM];

IBufferPtr	CGraphicBase::ms_alpd3dDefIB[DEFAULT_IB_NUM];

bool CGraphicBase::IsLowTextureMemory()
{
	return ms_isLowTextureMemory;
}

bool CGraphicBase::IsHighTextureMemory()
{
	return ms_isHighTextureMemory;
}

bool CGraphicBase::IsFastTNL()
{
	// D3D11 feature level 11_0 always has hardware T&L
	return true;
}

bool CGraphicBase::IsTLVertexClipping()
{
	// D3D11 always clips transformed vertices
	return true;
}

void CGraphicBase::GetBackBufferSize(UINT* puWidth, UINT* puHeight)
{
	*puWidth = (UINT)ms_iWidth;
	*puHeight = (UINT)ms_iHeight;
}

void CGraphicBase::SetDefaultIndexBuffer(UINT eDefIB)
{
	if (eDefIB>=DEFAULT_IB_NUM)
		return;

	m_mgr->SetIndexBuffer(ms_alpd3dDefIB[eDefIB]);
}

bool CGraphicBase::SetPDTStream(SPDTVertex* pSrcVertices, UINT uVtxCount)
{
	if (!uVtxCount || uVtxCount >= PDT_VERTEX_NUM)
		return false;

	static DWORD s_dwVBPos = 0;

	if (s_dwVBPos >= PDT_VERTEXBUFFER_NUM)
		s_dwVBPos = 0;

	auto vb = ms_alpd3dPDTVB[s_dwVBPos];

	++s_dwVBPos;

	if (!vb)
		return false;

	if (!vb->Update(pSrcVertices, uVtxCount))
		return false;

	m_mgr->SetVertexBuffer(vb, sizeof(TPDTVertex));
	return true;
}

DWORD CGraphicBase::GetAvailableTextureMemory()
{
	if (!ms_lpd3d11Device)
		return 0;

	static DWORD s_dwNextUpdateTime = 0;
	static DWORD s_dwTexMemSize = 0;

	DWORD dwCurTime = ELTimer_GetMSec();
	if (s_dwNextUpdateTime < dwCurTime)
	{
		s_dwNextUpdateTime = dwCurTime + 5000;

		// Query DXGI for video memory
		IDXGIDevice* pDxgiDevice = NULL;
		if (SUCCEEDED(ms_lpd3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDxgiDevice)))
		{
			IDXGIAdapter* pDxgiAdapter = NULL;
			if (SUCCEEDED(pDxgiDevice->GetAdapter(&pDxgiAdapter)))
			{
				DXGI_ADAPTER_DESC desc;
				if (SUCCEEDED(pDxgiAdapter->GetDesc(&desc)))
					s_dwTexMemSize = (DWORD)desc.DedicatedVideoMemory;
				pDxgiAdapter->Release();
			}
			pDxgiDevice->Release();
		}
	}

	return s_dwTexMemSize;
}

const XMFLOAT4X4& CGraphicBase::GetViewMatrix()
{
	return ms_matView;
}

const XMFLOAT4X4& CGraphicBase::GetIdentityMatrix()
{
	return ms_matIdentity;
}

void CGraphicBase::SetEyeCamera(float xEye, float yEye, float zEye, float xCenter, float yCenter, float zCenter, float xUp, float yUp, float zUp)
{
	XMFLOAT3 vectorEye = { xEye, yEye, zEye };
	XMFLOAT3 vectorCenter = { xCenter, yCenter, zCenter };
	XMFLOAT3 vectorUp = { xUp, yUp, zUp };

	CCameraManager::Instance().GetCurrentCamera()->SetViewParams(vectorEye, vectorCenter, vectorUp);
	UpdateViewMatrix();
}

void CGraphicBase::SetSimpleCamera(float x, float y, float z, float pitch, float roll)
{
	CCamera* pCamera = CCameraManager::Instance().GetCurrentCamera();
	XMFLOAT3 vectorEye = { x, y, z };

	pCamera->SetViewParams({ 0.0f, y, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
	pCamera->RotateEyeAroundTarget(pitch, roll);
	pCamera->Move(vectorEye);

	UpdateViewMatrix();

	ms_matWorld = STATEMANAGER.GetTransform().GetWorld();
	XMStoreFloat4x4(&ms_matWorldView, XMLoadFloat4x4(&ms_matWorld) * XMLoadFloat4x4(&ms_matView));
}

void CGraphicBase::SetAroundCamera(float distance, float pitch, float roll, float lookAtZ)
{
	CCamera* pCamera = CCameraManager::Instance().GetCurrentCamera();

	pCamera->SetViewParams({ 0.0f, -distance, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
	pCamera->RotateEyeAroundTarget(pitch, roll);

	XMFLOAT3 v3Target = pCamera->GetTarget();
	v3Target.z = lookAtZ;
	pCamera->SetTarget(v3Target);

	UpdateViewMatrix();

	ms_matWorld = STATEMANAGER.GetTransform().GetWorld();
	XMStoreFloat4x4(&ms_matWorldView, XMLoadFloat4x4(&ms_matWorld) * XMLoadFloat4x4(&ms_matView));
}

void CGraphicBase::SetPositionCamera(float fx, float fy, float fz, float distance, float pitch, float roll)
{
	if (ms_dwWavingEndTime > CTimer::Instance().GetCurrentMillisecond())
	{
		if (ms_iWavingPower > 0)
		{
			fx += float(rand() % ms_iWavingPower) / 10.0f;
			fy += float(rand() % ms_iWavingPower) / 10.0f;
			fz += float(rand() % ms_iWavingPower) / 10.0f;
		}
	}

	CCamera* pCamera = CCameraManager::Instance().GetCurrentCamera();
	if (!pCamera)
		return;

	pCamera->SetViewParams({ 0.0f, -distance, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
	pitch = fMIN(80.0f, fMAX(-80.0f, pitch));
	pCamera->RotateEyeAroundTarget(pitch, roll);
	pCamera->Move({ fx, fy, fz });

	UpdateViewMatrix();

	ms_matWorld = STATEMANAGER.GetTransform().GetWorld();
	XMStoreFloat4x4(&ms_matWorldView, XMLoadFloat4x4(&ms_matWorld) * XMLoadFloat4x4(&ms_matView));
}

void CGraphicBase::SetOrtho2D(float hres, float vres, float zres)
{
	XMStoreFloat4x4(&ms_matProj, XMMatrixOrthographicOffCenterRH(0.0f, hres, vres, 0.0f, 0.0f, zres));
	UpdateProjMatrix();
}

void CGraphicBase::SetOrtho3D(float hres, float vres, float zmin, float zmax)
{
	XMStoreFloat4x4(&ms_matProj, XMMatrixOrthographicRH(hres, vres, zmin, zmax));
	UpdateProjMatrix();
}

void CGraphicBase::SetPerspective(float fov, float aspect, float nearz, float farz)
{
	ms_fFieldOfView = fov;
	ms_fAspect = aspect;
	ms_fNearY = nearz;
	ms_fFarY = farz;

	XMStoreFloat4x4(&ms_matProj, XMMatrixPerspectiveFovRH(XMConvertToRadians(fov), ms_fAspect, nearz, farz));
	UpdateProjMatrix();
}

void CGraphicBase::UpdateProjMatrix()
{
	STATEMANAGER.GetTransform().SetProjection(ms_matProj);
}

void CGraphicBase::UpdateViewMatrix()
{
	CCamera* pkCamera = CCameraManager::Instance().GetCurrentCamera();
	if (!pkCamera)
		return;

	ms_matView = pkCamera->GetViewMatrix();
	STATEMANAGER.GetTransform().SetView(ms_matView);

	XMVECTOR det;
	XMStoreFloat4x4(&ms_matInverseView, XMMatrixInverse(&det, XMLoadFloat4x4(&ms_matView)));

	ms_matInverseViewYAxis._11 = ms_matInverseView._11;
	ms_matInverseViewYAxis._12 = ms_matInverseView._12;
	ms_matInverseViewYAxis._21 = ms_matInverseView._21;
	ms_matInverseViewYAxis._22 = ms_matInverseView._22;
}

void CGraphicBase::UpdatePipeLineMatrix()
{
	UpdateProjMatrix();
	UpdateViewMatrix();
}

void CGraphicBase::SetViewport(DWORD dwX, DWORD dwY, DWORD dwWidth, DWORD dwHeight, float fMinZ, float fMaxZ)
{
	ms_Viewport.TopLeftX = (float)dwX;
	ms_Viewport.TopLeftY = (float)dwY;
	ms_Viewport.Width = (float)dwWidth;
	ms_Viewport.Height = (float)dwHeight;
	ms_Viewport.MinDepth = fMinZ;
	ms_Viewport.MaxDepth = fMaxZ;
	ms_lpd3d11Context->RSSetViewports(1, &ms_Viewport);
}

void CGraphicBase::GetTargetPosition(float * px, float * py, float * pz)
{
	*px = CCameraManager::Instance().GetCurrentCamera()->GetTarget().x;
	*py = CCameraManager::Instance().GetCurrentCamera()->GetTarget().y;
	*pz = CCameraManager::Instance().GetCurrentCamera()->GetTarget().z;
}

void CGraphicBase::GetCameraPosition(float * px, float * py, float * pz)
{
	*px = CCameraManager::Instance().GetCurrentCamera()->GetEye().x;
	*py = CCameraManager::Instance().GetCurrentCamera()->GetEye().y;
	*pz = CCameraManager::Instance().GetCurrentCamera()->GetEye().z;
}

void CGraphicBase::GetMatrix(XMFLOAT4X4* pRetMatrix) const
{
	assert(!ms_matStack.empty());
	*pRetMatrix = ms_matStack.back();
}

const XMFLOAT4X4* CGraphicBase::GetMatrixPointer() const
{
	assert(!ms_matStack.empty());
	return &ms_matStack.back();
}

void CGraphicBase::GetSphereMatrix(XMFLOAT4X4* pMatrix, float fValue)
{
	XMStoreFloat4x4(pMatrix, XMMatrixIdentity());

	pMatrix->_11 = fValue * ms_matWorldView._11;
	pMatrix->_21 = fValue * ms_matWorldView._21;
	pMatrix->_31 = fValue * ms_matWorldView._31;
	pMatrix->_41 = fValue;

	pMatrix->_12 = -fValue * ms_matWorldView._12;
	pMatrix->_22 = -fValue * ms_matWorldView._22;
	pMatrix->_32 = -fValue * ms_matWorldView._32;
	pMatrix->_42 = -fValue;
}

float CGraphicBase::GetFOV()
{
	return ms_fFieldOfView;
}

void CGraphicBase::PushMatrix()
{
	if (ms_matStack.empty())
	{
		XMFLOAT4X4 mat;
		XMStoreFloat4x4(&mat, XMMatrixIdentity());
		ms_matStack.push_back(mat);
	}
	else
	{
		ms_matStack.push_back(ms_matStack.back());
	}
}

void CGraphicBase::PopMatrix()
{
	assert(!ms_matStack.empty());
	ms_matStack.pop_back();
}

void CGraphicBase::LoadMatrix(const XMFLOAT4X4& c_rSrcMatrix)
{
	if (ms_matStack.empty())
		ms_matStack.push_back(c_rSrcMatrix);
	else
		ms_matStack.back() = c_rSrcMatrix;
}

void CGraphicBase::MultMatrix(const XMFLOAT4X4* pMat)
{
	assert(!ms_matStack.empty());
	XMStoreFloat4x4(&ms_matStack.back(), XMLoadFloat4x4(&ms_matStack.back()) * XMLoadFloat4x4(pMat));
}

void CGraphicBase::MultMatrixLocal(const XMFLOAT4X4* pMat)
{
	assert(!ms_matStack.empty());
	XMStoreFloat4x4(&ms_matStack.back(), XMLoadFloat4x4(pMat) * XMLoadFloat4x4(&ms_matStack.back()));
}

void CGraphicBase::Translate(float x, float y, float z)
{
	assert(!ms_matStack.empty());
	XMStoreFloat4x4(&ms_matStack.back(), XMLoadFloat4x4(&ms_matStack.back()) * XMMatrixTranslation(x, y, z));
}

void CGraphicBase::Scale(float x, float y, float z)
{
	assert(!ms_matStack.empty());
	XMStoreFloat4x4(&ms_matStack.back(), XMLoadFloat4x4(&ms_matStack.back()) * XMMatrixScaling(x, y, z));
}

void CGraphicBase::Rotate(float degree, float x, float y, float z)
{
	assert(!ms_matStack.empty());
	XMVECTOR axis = XMVectorSet(x, y, z, 0.0f);
	XMStoreFloat4x4(&ms_matStack.back(), XMLoadFloat4x4(&ms_matStack.back()) * XMMatrixRotationAxis(axis, XMConvertToRadians(degree)));
}

void CGraphicBase::RotateLocal(float degree, float x, float y, float z)
{
	assert(!ms_matStack.empty());
	XMVECTOR axis = XMVectorSet(x, y, z, 0.0f);
	XMStoreFloat4x4(&ms_matStack.back(), XMMatrixRotationAxis(axis, XMConvertToRadians(degree)) * XMLoadFloat4x4(&ms_matStack.back()));
}

void CGraphicBase::RotateYawPitchRollLocal(float fYaw, float fPitch, float fRoll)
{
	assert(!ms_matStack.empty());
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	XMStoreFloat4x4(&ms_matStack.back(), rot * XMLoadFloat4x4(&ms_matStack.back()));
}

DWORD CGraphicBase::GetColor(float r, float g, float b, float a)
{
	BYTE argb[4] =
	{
		(BYTE) (255.0f * b),
		(BYTE) (255.0f * g),
		(BYTE) (255.0f * r),
		(BYTE) (255.0f * a)
	};

	return *((DWORD *) argb);
}

void CGraphicBase::InitScreenEffect()
{
	ms_dwWavingEndTime = 0;
	ms_dwFlashingEndTime = 0;
	ms_iWavingPower = 0;
	ms_FlashingColor = { 0.0f, 0.0f, 0.0f, 0.0f };
}

void CGraphicBase::SetScreenEffectWaving(float fDuringTime, int iPower)
{
	ms_dwWavingEndTime = CTimer::Instance().GetCurrentMillisecond() + long(fDuringTime * 1000.0f);
	ms_iWavingPower = iPower;
}

void CGraphicBase::SetScreenEffectFlashing(float fDuringTime, const XMFLOAT4& c_rColor)
{
	ms_dwFlashingEndTime = CTimer::Instance().GetCurrentMillisecond() + long(fDuringTime * 1000.0f);
	ms_FlashingColor = c_rColor;
}

UniquePtr<DxManager>& CGraphicBase::GetDxManager()
{
	return m_mgr;
}

DWORD CGraphicBase::GetFaceCount()
{
	return ms_faceCount;
}

void CGraphicBase::ResetFaceCount()
{
	ms_faceCount = 0;
}

HRESULT CGraphicBase::GetLastResult()
{
	return ms_hLastResult;
}

CGraphicBase::CGraphicBase()
{
}

CGraphicBase::~CGraphicBase()
{
}
