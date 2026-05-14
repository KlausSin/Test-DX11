#include "StdAfx.h"
#include "PythonApplication.h"

static bool bInitializedLogo = false;

int CPythonApplication::OnLogoOpen(char* szName)
{
	m_pLogoTex = new CGraphicImageTexture();
	m_bLogoError = true;
	m_bLogoPlay = false;

	m_nLeft = 0;
	m_nTop = 0;
	m_nRight = GetWidth();
	m_nBottom = GetHeight();

	if (!m_pLogoTex->CreateFromDiskFile(szName, DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_FILTER_MIN_MAG_MIP_LINEAR))
		return 0;

	m_bLogoError = false;
	bInitializedLogo = true;
	return 1;
}

int CPythonApplication::OnLogoUpdate()
{
	if (!bInitializedLogo || !m_pLogoTex || m_pLogoTex->IsEmpty())
		return 0;

	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		return 0;

	return 1;
}

void CPythonApplication::OnLogoRender()
{
	if (!m_pLogoTex || m_pLogoTex->IsEmpty() || m_bLogoError || !bInitializedLogo)
		return;

	STATEMANAGER.GetSampler().Push(0);
	STATEMANAGER.GetSampler().SetFilter(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR);

	m_pLogoTex->SetTextureStage(0);

	CPythonGraphic::instance().RenderTextureBox(
		m_nLeft,
		m_nTop,
		m_nRight,
		m_nBottom,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f);

	STATEMANAGER.GetSampler().Restore(0);
}

void CPythonApplication::OnLogoClose()
{
	if (!bInitializedLogo)
		return;

	if (m_pLogoTex)
	{
		m_pLogoTex->Destroy();
		delete m_pLogoTex;
		m_pLogoTex = nullptr;
	}

	m_bLogoError = true;
	m_bLogoPlay = false;
	bInitializedLogo = false;
}