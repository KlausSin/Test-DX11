#include "StdAfx.h"
#include "BlockTexture.h"
#include "GrpBase.h"
#include "GrpDib.h"
#include "Eterbase/Stl.h"
#include "Eterlib/StateManager.h"

void CBlockTexture::SetClipRect(const RECT & c_rRect)
{
	m_bClipEnable = TRUE;
	m_clipRect = c_rRect;
}

void CBlockTexture::Render(int ix, int iy)
{
	int isx = ix + m_rect.left;
	int isy = iy + m_rect.top;
	int iex = ix + m_rect.left + m_dwWidth;
	int iey = iy + m_rect.top + m_dwHeight;

	float su = 0.0f;
	float sv = 0.0f;
	float eu = 1.0f;
	float ev = 1.0f;

	if (m_bClipEnable)
	{
		if (isx > m_clipRect.right || iex < m_clipRect.left || isy > m_clipRect.bottom || iey < m_clipRect.top)
			return;

		if (m_clipRect.left > isx) { int d = m_clipRect.left - isx; isx += d; su += float(d) / m_dwWidth; }
		if (iex > m_clipRect.right) { int d = iex - m_clipRect.right; iex -= d; eu -= float(d) / m_dwWidth; }
		if (m_clipRect.top > isy) { int d = m_clipRect.top - isy; isy += d; sv += float(d) / m_dwHeight; }
		if (iey > m_clipRect.bottom) { int d = iey - m_clipRect.bottom; iey -= d; ev -= float(d) / m_dwHeight; }
	}

	TPDTVertex vertices[4] = {};
	vertices[0].position.x = (float)isx; vertices[0].position.y = (float)isy; vertices[0].position.z = 0.0f;
	vertices[0].texCoord = TTextureCoordinate(su, sv);
	vertices[0].diffuse = 0xFFFFFFFF;

	vertices[1].position.x = (float)iex; vertices[1].position.y = (float)isy; vertices[1].position.z = 0.0f;
	vertices[1].texCoord = TTextureCoordinate(eu, sv);
	vertices[1].diffuse = 0xFFFFFFFF;

	vertices[2].position.x = (float)isx; vertices[2].position.y = (float)iey; vertices[2].position.z = 0.0f;
	vertices[2].texCoord = TTextureCoordinate(su, ev);
	vertices[2].diffuse = 0xFFFFFFFF;

	vertices[3].position.x = (float)iex; vertices[3].position.y = (float)iey; vertices[3].position.z = 0.0f;
	vertices[3].texCoord = TTextureCoordinate(eu, ev);
	vertices[3].diffuse = 0xFFFFFFFF;

	if (CGraphicBase::SetPDTStream(vertices, 4))
	{
		CGraphicBase::SetDefaultIndexBuffer(CGraphicBase::DEFAULT_IB_FILL_RECT);

		STATEMANAGER.SetTexture(0, m_lpSRV);
		STATEMANAGER.SetTexture(1, NULL);

		_mgr->SetShader(VF_PDT, BLEND_UI_TEX);

		STATEMANAGER.GetBlend().Push();

		STATEMANAGER.GetBlend().SetBlendEnable(true);
		STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
		STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);

		STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 0, 2);

		STATEMANAGER.GetBlend().Restore();
	}
}

void CBlockTexture::InvalidateRect(const RECT& c_rsrcRect)
{
	if (!ms_lpd3d11Context || !m_lpTex || !m_pDIB)
		return;

	void* dibPtr = m_pDIB->GetPointer();
	if (!dibPtr)
		return;

	const DWORD dibWidth = m_pDIB->GetWidth();
	const DWORD dibHeight = m_pDIB->GetHeight();

	if (dibWidth == 0 || dibHeight == 0 || m_dwWidth == 0 || m_dwHeight == 0)
		return;

	if (m_rect.left < 0 || m_rect.top < 0)
		return;

	if ((DWORD)m_rect.left >= dibWidth || (DWORD)m_rect.top >= dibHeight)
		return;

	DWORD copyWidth = m_dwWidth;
	DWORD copyHeight = m_dwHeight;

	if ((DWORD)m_rect.left + copyWidth > dibWidth)
		copyWidth = dibWidth - (DWORD)m_rect.left;

	if ((DWORD)m_rect.top + copyHeight > dibHeight)
		copyHeight = dibHeight - (DWORD)m_rect.top;

	if (copyWidth == 0 || copyHeight == 0)
		return;

	D3D11_MAPPED_SUBRESOURCE mapped;
	if (FAILED(ms_lpd3d11Context->Map(m_lpTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		return;

	DWORD* pdwSrc = (DWORD*)dibPtr;
	pdwSrc += m_rect.left + m_rect.top * dibWidth;

	DWORD* pdwDst = (DWORD*)mapped.pData;
	DWORD dstPitch = mapped.RowPitch / 4;

	for (DWORD y = 0; y < copyHeight; ++y)
	{
		memcpy(pdwDst, pdwSrc, copyWidth * sizeof(DWORD));
		pdwDst += dstPitch;
		pdwSrc += dibWidth;
	}

	ms_lpd3d11Context->Unmap(m_lpTex, 0);
}

bool CBlockTexture::Create(CGraphicDib * pDIB, const RECT & c_rRect, DWORD dwWidth, DWORD dwHeight)
{
	if (!ms_lpd3d11Device)
		return false;

	D3D11_TEXTURE2D_DESC td = {};
	td.Width          = dwWidth;
	td.Height         = dwHeight;
	td.MipLevels      = 1;
	td.ArraySize      = 1;
	td.Format         = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage          = D3D11_USAGE_DYNAMIC;
	td.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&td, NULL, &m_lpTex)))
	{
		Tracef("Failed to create block texture %u x %u\n", dwWidth, dwHeight);
		return false;
	}

	if (FAILED(ms_lpd3d11Device->CreateShaderResourceView(m_lpTex, NULL, &m_lpSRV)))
	{
		safe_release(m_lpTex);
		return false;
	}

	// Optional: clear texture on creation
	D3D11_MAPPED_SUBRESOURCE mapped;
	if (SUCCEEDED(ms_lpd3d11Context->Map(m_lpTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		memset(mapped.pData, 0, mapped.RowPitch * dwHeight);
		ms_lpd3d11Context->Unmap(m_lpTex, 0);
	}

	m_pDIB       = pDIB;
	m_rect       = c_rRect;
	m_dwWidth    = dwWidth;
	m_dwHeight   = dwHeight;
	m_bClipEnable = FALSE;

	return true;
}

CBlockTexture::CBlockTexture()
{
	m_pDIB = NULL;
	m_lpTex = NULL;
	m_lpSRV = NULL;
}

CBlockTexture::~CBlockTexture()
{
	safe_release(m_lpSRV);
	safe_release(m_lpTex);
}