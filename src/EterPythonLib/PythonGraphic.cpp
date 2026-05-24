#include "StdAfx.h"
#include "EterLib/StateManager.h"
#include "PythonGraphic.h"
#include <string>
#include <DirectXTex/DirectXTex.h>
#include <qMin32Lib/entity/LightComponent.h>

void CPythonGraphic::Destroy()
{	
}

float CPythonGraphic::GetOrthoDepth()
{
	return m_fOrthoDepth;
}

void CPythonGraphic::SetInterfaceRenderState()
{
	auto& state = STATEMANAGER.GetStateCache();
	auto cb = _mgr->GetCbMgr();

	STATEMANAGER.GetTransform().SetProjection(ms_matIdentity);
	STATEMANAGER.GetTransform().SetView(ms_matIdentity);
	STATEMANAGER.GetTransform().SetWorld(ms_matIdentity);

	state.Sampler.SetFilter(0, D3D11_FILTER_MIN_MAG_MIP_POINT);

	state.Blend.SetBlendEnable(true);
	state.Blend.SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
	state.Blend.SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);

	state.Raster.SetScissorEnable(false);

	CPythonGraphic::Instance().SetBlendOperation();
	CPythonGraphic::Instance().SetOrtho2D(ms_iWidth, ms_iHeight, GetOrthoDepth());

	cb->SetEntityLightingEnable(FALSE);
}

void CPythonGraphic::SetGameRenderState()
{
	auto& state = STATEMANAGER.GetStateCache();
	auto cb = _mgr->GetCbMgr();

	state.Sampler.SetFilter(0, D3D11_FILTER_ANISOTROPIC);
	state.Sampler.SetMaxAnisotropy(0, 8);

	state.Blend.SetBlendEnable(false);

	cb->SetEntityLightingEnable(TRUE);
}

void CPythonGraphic::SetCursorPosition(int x, int y)
{
	CScreen::SetCursorPosition(x, y, ms_iWidth, ms_iHeight);
}

void CPythonGraphic::SetOmniLight()
{
	auto cb = _mgr->GetCbMgr();
	cb->ClearEntityLights();
	cb->SetEntityLightingEnable(TRUE);
	cb->SetEntityGlobalAmbient(0xFF4C4C4C);

	CLightComponent& spot = cb->CreateEntityLight();

	spot.SetType(LIGHT_TYPE_SPOT);
	spot.SetPosition(XMFLOAT3(50.0f, 150.0f, 350.0f));
	spot.SetDirection(XMFLOAT3(-0.15f, -0.3f, -0.9f));
	spot.SetTheta(XMConvertToRadians(30.0f));
	spot.SetPhi(XMConvertToRadians(45.0f));
	spot.SetFalloff(1.0f);
	spot.SetAttenuation(0.0f, 0.005f, 0.0f);
	spot.SetDiffuse(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	spot.SetAmbient(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	spot.SetRange(500.0f);
	spot.SetEnable(true);

	CLightComponent& point = cb->CreateEntityLight();

	point.SetType(LIGHT_TYPE_POINT);
	point.SetPosition(XMFLOAT3(0.0f, 200.0f, 200.0f));
	point.SetAttenuation(0.1f, 0.01f, 0.0f);
	point.SetDiffuse(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	point.SetAmbient(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	point.SetRange(500.0f);
	point.SetEnable(true);

	cb->UploadEntityLights();
}

void CPythonGraphic::SetViewport(float fx, float fy, float fWidth, float fHeight)
{
	if (!ms_lpd3d11Context)
		return;

	UINT n = 1;
	ms_lpd3d11Context->RSGetViewports(&n, &m_backupViewport);

	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = fx;
	vp.TopLeftY = fy;
	vp.Width = fWidth;
	vp.Height = fHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	ms_lpd3d11Context->RSSetViewports(1, &vp);
}

void CPythonGraphic::RestoreViewport()
{
	if (ms_lpd3d11Context)
		ms_lpd3d11Context->RSSetViewports(1, &m_backupViewport);
}

void CPythonGraphic::SetGamma(float fGammaFactor)
{
	// TODO: D3D11 has no device-level gamma ramps. Implement via DXGI Output gamma control
	// or via a post-processing shader. For now this is a no-op.
	(void)fGammaFactor;
}

bool CPythonGraphic::SaveScreenShot()
{
	if (!ms_lpd3d11Device || !ms_lpd3d11Context || !ms_lpd3d11SwapChain)
		return false;

	ID3D11Texture2D* backBuffer = nullptr;

	HRESULT hr = ms_lpd3d11SwapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(&backBuffer));

	if (FAILED(hr) || !backBuffer)
	{
		TraceError("CPythonGraphic::SaveScreenShot - GetBuffer failed");
		return false;
	}

	DirectX::ScratchImage image;

	hr = DirectX::CaptureTexture(
		ms_lpd3d11Device,
		ms_lpd3d11Context,
		backBuffer,
		image);

	backBuffer->Release();

	if (FAILED(hr))
	{
		TraceError("CPythonGraphic::SaveScreenShot - CaptureTexture failed");
		return false;
	}

	if (!CreateDirectoryA("screenshot", nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
		return false;

	SYSTEMTIME time;
	GetLocalTime(&time);

	wchar_t fileName[MAX_PATH] = {};

	swprintf_s(
		fileName,
		L"screenshot/%04d%02d%02d_%02d%02d%02d.png",
		time.wYear,
		time.wMonth,
		time.wDay,
		time.wHour,
		time.wMinute,
		time.wSecond);

	hr = DirectX::SaveToWICFile(
		image.GetImages(),
		image.GetImageCount(),
		DirectX::WIC_FLAGS_NONE,
		DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
		fileName);

	if (FAILED(hr))
	{
		TraceError("CPythonGraphic::SaveScreenShot - SaveToWICFile failed");
		return false;
	}

	return true;
}

void CPythonGraphic::PushState()
{
	TState curState;

	curState.matProj = ms_matProj;
	curState.matView = ms_matView;

	m_stateStack.push(curState);
}

void CPythonGraphic::PopState()
{
	if (m_stateStack.empty())
	{
		assert(!"PythonGraphic::PopState StateStack is EMPTY");
		return;
	}
	
	TState & rState = m_stateStack.top();

	//STATEMANAGER.RestoreTransform(World);
	ms_matProj = rState.matProj;
	ms_matView = rState.matView;
	
	UpdatePipeLineMatrix();

	m_stateStack.pop();
	//CCamera::Instance().PopParams();
}

void CPythonGraphic::RenderImage(CGraphicImageInstance* pImageInstance, float x, float y)
{
	assert(pImageInstance != NULL);

	//SetColorRenderState();
	const CGraphicTexture * c_pTexture = pImageInstance->GetTexturePointer();

	float width = (float) pImageInstance->GetWidth();
	float height = (float) pImageInstance->GetHeight();

	c_pTexture->SetTextureStage(0);

	RenderTextureBox(x,
					 y,
					 x + width,
					 y + height,
					 0.0f,
					 0.5f / width, 
					 0.5f / height, 
					 (width + 0.5f) / width, 
					 (height + 0.5f) / height);
}

void CPythonGraphic::RenderAlphaImage(CGraphicImageInstance* pImageInstance, float x, float y, float aLeft, float aRight)
{
	assert(pImageInstance != NULL);

	XMFLOAT4 DiffuseColor1 = XMFLOAT4(1.0f, 1.0f, 1.0f, aLeft);
	XMFLOAT4 DiffuseColor2 = XMFLOAT4(1.0f, 1.0f, 1.0f, aRight);

	const CGraphicTexture * c_pTexture = pImageInstance->GetTexturePointer();

	float width = (float) pImageInstance->GetWidth();
	float height = (float) pImageInstance->GetHeight();

	c_pTexture->SetTextureStage(0);

	float sx = x;
	float sy = y;
	float ex = x + width;
	float ey = y + height;
	float z = 0.0f;

	float su = 0.0f;
	float sv = 0.0f;
	float eu = 1.0f;
	float ev = 1.0f;

	TPDTVertex vertices[4];
	vertices[0].position = TPosition(sx, sy, z);
	vertices[0].diffuse = ColorToUint(DiffuseColor1);
	vertices[0].texCoord = TTextureCoordinate(su, sv);

	vertices[1].position = TPosition(ex, sy, z);
	vertices[1].diffuse = ColorToUint(DiffuseColor2);
	vertices[1].texCoord = TTextureCoordinate(eu, sv);

	vertices[2].position = TPosition(sx, ey, z);
	vertices[2].diffuse = ColorToUint(DiffuseColor1);
	vertices[2].texCoord = TTextureCoordinate(su, ev);

	vertices[3].position = TPosition(ex, ey, z);
	vertices[3].diffuse = ColorToUint(DiffuseColor2);
	vertices[3].texCoord = TTextureCoordinate(eu, ev);

	_mgr->SetShader(VF_MESH);
	// 2004.11.18.myevan.DrawIndexPrimitiveUP -> DynamicVertexBuffer
	CGraphicBase::SetDefaultIndexBuffer(DEFAULT_IB_FILL_RECT);
	if (CGraphicBase::SetPDTStream(vertices, 4))
		STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 0, 2);
}

void CPythonGraphic::RenderCoolTimeBox(float fxCenter, float fyCenter, float fRadius, float fTime)
{
	if (fTime >= 1.0f)
		return;

	fTime = std::max(0.0f, fTime);

	static XMFLOAT4 color = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	static XMFLOAT2 s_v2BoxPos[8] =
	{
		XMFLOAT2( -1.0f, -1.0f ),
		XMFLOAT2( -1.0f,  0.0f ),
		XMFLOAT2( -1.0f, +1.0f ),
		XMFLOAT2(  0.0f, +1.0f ),
		XMFLOAT2( +1.0f, +1.0f ),
		XMFLOAT2( +1.0f,  0.0f ),
		XMFLOAT2( +1.0f, -1.0f ),
		XMFLOAT2(  0.0f, -1.0f ),
	};

	int iTriCount = int(8 - 8.0f * fTime);
	float fLastPercentage = (8 - 8.0f * fTime) - iTriCount;

	// Build fan vertex positions (center + edge points)
	std::vector<XMFLOAT2> fanPos;
	fanPos.push_back(XMFLOAT2(fxCenter, fyCenter));                      // v0 = center
	fanPos.push_back(XMFLOAT2(fxCenter, fyCenter - fRadius));            // v1 = top-center

	for (int j = 0; j < iTriCount; ++j)
		fanPos.push_back(XMFLOAT2(fxCenter + s_v2BoxPos[j].x * fRadius, fyCenter + s_v2BoxPos[j].y * fRadius));

	if (fLastPercentage > 0.0f)
	{
		XMFLOAT2 * pv2LastPos = &s_v2BoxPos[(iTriCount-1+8)%8];
		XMFLOAT2 * pv2Pos = &s_v2BoxPos[(iTriCount+8)%8];
		fanPos.push_back(XMFLOAT2(
			fxCenter + ((pv2Pos->x-pv2LastPos->x) * fLastPercentage + pv2LastPos->x) * fRadius,
			fyCenter + ((pv2Pos->y-pv2LastPos->y) * fLastPercentage + pv2LastPos->y) * fRadius));
		++iTriCount;
	}

	if (iTriCount < 1)
		return;

	// Expand fan into triangle list: for each tri, emit (center, v[i+1], v[i+2])
	std::vector<TPDTVertex> vertices;
	vertices.reserve(iTriCount * 3);
	TPDTVertex vtx;
	vtx.position.z = 0.0f;
	vtx.diffuse = ColorToUint(color);
	vtx.texCoord.x = 0.0f;
	vtx.texCoord.y = 0.0f;

	for (int i = 0; i < iTriCount; ++i)
	{
		vtx.position.x = fanPos[0].x; vtx.position.y = fanPos[0].y;
		vertices.push_back(vtx);
		vtx.position.x = fanPos[i+1].x; vtx.position.y = fanPos[i+1].y;
		vertices.push_back(vtx);
		vtx.position.x = fanPos[i+2].x; vtx.position.y = fanPos[i+2].y;
		vertices.push_back(vtx);
	}

	if (SetPDTStream(&vertices[0], vertices.size()))
	{
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, iTriCount, 0);
	}
}

long CPythonGraphic::GenerateColor(float r, float g, float b, float a)
{
	return GetColor(r, g, b, a);
}

void CPythonGraphic::RenderDownButton(float sx, float sy, float ex, float ey)
{
	RenderBox2d(sx, sy, ex, ey);

	SetDiffuseColor(m_darkColor);
	RenderLine2d(sx, sy, ex, sy);
	RenderLine2d(sx, sy, sx, ey);

	SetDiffuseColor(m_lightColor);
	RenderLine2d(sx, ey, ex, ey);
	RenderLine2d(ex, sy, ex, ey);
}

void CPythonGraphic::RenderUpButton(float sx, float sy, float ex, float ey)
{
	RenderBox2d(sx, sy, ex, ey);

	SetDiffuseColor(m_lightColor);
	RenderLine2d(sx, sy, ex, sy);
	RenderLine2d(sx, sy, sx, ey);

	SetDiffuseColor(m_darkColor);
	RenderLine2d(sx, ey, ex, ey);
	RenderLine2d(ex, sy, ex, ey);
}

DWORD CPythonGraphic::GetAvailableMemory()
{
	// D3D11: query DXGI adapter for video memory
	if (!ms_lpd3d11Device)
		return 0;

	IDXGIDevice* pDxgiDevice = NULL;
	if (FAILED(ms_lpd3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDxgiDevice)))
		return 0;

	IDXGIAdapter* pDxgiAdapter = NULL;
	if (FAILED(pDxgiDevice->GetAdapter(&pDxgiAdapter)))
	{
		pDxgiDevice->Release();
		return 0;
	}

	DXGI_ADAPTER_DESC desc;
	HRESULT hr = pDxgiAdapter->GetDesc(&desc);
	pDxgiAdapter->Release();
	pDxgiDevice->Release();

	if (FAILED(hr))
		return 0;

	return (DWORD)desc.DedicatedVideoMemory;
}

CPythonGraphic::CPythonGraphic()
{
	m_lightColor = GetColor(1.0f, 1.0f, 1.0f);
	m_darkColor = GetColor(0.0f, 0.0f, 0.0f);
	
	memset(&m_backupViewport, 0, sizeof(D3D11_VIEWPORT));

	m_fOrthoDepth = 1000.0f;
}

CPythonGraphic::~CPythonGraphic()
{
	Tracef("Python Graphic Clear\n");
}
