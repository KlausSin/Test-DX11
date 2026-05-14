#include "StdAfx.h"
#include "GrpImage.h"

CGraphicImage::CGraphicImage(const char* fileName)
	: CResource(fileName)
{
	ZeroMemory(&m_rect, sizeof(m_rect));
}

CGraphicImage::~CGraphicImage()
{
}

CGraphicImage::TType CGraphicImage::Type()
{
	static TType type = StringToType("CGraphicImage");
	return type;
}

bool CGraphicImage::CreateDeviceObjects()
{
	return m_imageTexture.CreateDeviceObjects();
}

void CGraphicImage::DestroyDeviceObjects()
{
	m_imageTexture.DestroyDeviceObjects();
}

int CGraphicImage::GetWidth() const
{
	return m_rect.right - m_rect.left;
}

int CGraphicImage::GetHeight() const
{
	return m_rect.bottom - m_rect.top;
}

const RECT& CGraphicImage::GetRectReference() const
{
	return m_rect;
}

const CGraphicTexture& CGraphicImage::GetTextureReference() const
{
	return m_imageTexture;
}

CGraphicTexture* CGraphicImage::GetTexturePointer()
{
	return &m_imageTexture;
}

bool CGraphicImage::OnLoad(int size, const void* data)
{
	if (!data || size <= 0)
		return false;

	if (!m_imageTexture.CreateFromMemoryFile(size, data))
		return false;

	m_rect.left = 0;
	m_rect.top = 0;
	m_rect.right = m_imageTexture.GetWidth();
	m_rect.bottom = m_imageTexture.GetHeight();

	return true;
}

void CGraphicImage::OnClear()
{
	m_imageTexture.Destroy();
	ZeroMemory(&m_rect, sizeof(m_rect));
}

bool CGraphicImage::OnIsEmpty() const
{
	return m_imageTexture.IsEmpty();
}

bool CGraphicImage::OnIsType(TType type)
{
	if (type == CGraphicImage::Type())
		return true;

	return CResource::OnIsType(type);
}
