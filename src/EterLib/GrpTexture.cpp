#include "StdAfx.h"
#include "GrpTexture.h"

#include "StateManager.h"
#include <EterBase/Stl.h>

void CGraphicTexture::SetTextureStage(int stage) const
{
    STATEMANAGER.SetTexture(stage, m_lpSRV);
}

void CGraphicTexture::DestroyDeviceObjects()
{
    safe_release(m_lpSRV);
    m_bEmpty = true;
}

CGraphicTexture::CGraphicTexture()
{
    Initialize();
}

CGraphicTexture::~CGraphicTexture()
{
    Destroy();
}

void CGraphicTexture::Initialize()
{
    m_bEmpty = true;

    m_width = 0;
    m_height = 0;

    m_lpSRV = nullptr;
}

void CGraphicTexture::Destroy()
{
    if (m_lpSRV)
    {
        m_lpSRV->Release();
        m_lpSRV = nullptr;
    }

    Initialize();
}

bool CGraphicTexture::IsEmpty() const
{
    return m_bEmpty;
}

int CGraphicTexture::GetWidth() const
{
    return m_width;
}

int CGraphicTexture::GetHeight() const
{
    return m_height;
}

ID3D11ShaderResourceView* CGraphicTexture::GetSRV() const
{
    return m_lpSRV;
}
