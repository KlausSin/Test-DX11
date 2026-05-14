#include "StdAfx.h"
#include "DibBar.h"
#include "BlockTexture.h"

CDibBar::CDibBar()
	: m_dwWidth(0),
	m_dwHeight(0)
{
}

CDibBar::~CDibBar()
{
	Destroy();
}

void CDibBar::Destroy()
{
	for (CBlockTexture* texture : m_kVec_pkBlockTexture)
		delete texture;

	m_kVec_pkBlockTexture.clear();

	m_dib.Destroy();

	m_dwWidth = 0;
	m_dwHeight = 0;
}

bool CDibBar::Create(DWORD width, DWORD height)
{
	Destroy();

	if (width == 0 || height == 0)
		return false;

	if (!m_dib.Create(width, height))
	{
		Tracef("Failed to create CDibBar\n");
		return false;
	}

	m_dwWidth = width;
	m_dwHeight = height;

	BuildTextureBlockList(width, height);
	ClearBar();

	OnCreate();

	return true;
}

void CDibBar::Invalidate()
{
	if (m_kVec_pkBlockTexture.empty())
		return;

	const RECT rect =
	{
		0,
		0,
		static_cast<LONG>(m_dwWidth),
		static_cast<LONG>(m_dwHeight)
	};

	for (CBlockTexture* texture : m_kVec_pkBlockTexture)
	{
		if (texture)
			texture->InvalidateRect(rect);
	}
}

void CDibBar::SetClipRect(const RECT& rect)
{
	for (CBlockTexture* texture : m_kVec_pkBlockTexture)
	{
		if (texture)
			texture->SetClipRect(rect);
	}
}

void CDibBar::ClearBar()
{
	DWORD* pixels = static_cast<DWORD*>(m_dib.GetPointer());

	if (!pixels)
		return;

	const DWORD width = m_dib.GetWidth();
	const DWORD height = m_dib.GetHeight();

	if (width == 0 || height == 0)
		return;

	memset(pixels, 0, width * height * sizeof(DWORD));
	Invalidate();
}

void CDibBar::Render(int x, int y)
{
	for (CBlockTexture* texture : m_kVec_pkBlockTexture)
	{
		if (texture)
			texture->Render(x, y);
	}
}

DWORD CDibBar::GetNearTextureSize(DWORD size) const
{
	if (size == 0)
		return 0;

	if ((size & (size - 1)) == 0)
		return size;

	DWORD result = 1;

	while (result < size)
		result <<= 1;

	return result;
}

CBlockTexture* CDibBar::BuildTextureBlock(
	DWORD x,
	DWORD y,
	DWORD imageWidth,
	DWORD imageHeight,
	DWORD textureWidth,
	DWORD textureHeight)
{
	if (imageWidth == 0 || imageHeight == 0 || textureWidth == 0 || textureHeight == 0)
		return nullptr;

	RECT rect =
	{
		static_cast<LONG>(x),
		static_cast<LONG>(y),
		static_cast<LONG>(x + imageWidth),
		static_cast<LONG>(y + imageHeight)
	};

	CBlockTexture* texture = new CBlockTexture;

	if (!texture->Create(&m_dib, rect, textureWidth, textureHeight))
	{
		delete texture;
		return nullptr;
	}

	return texture;
}

void CDibBar::BuildTextureBlockList(DWORD width, DWORD height, DWORD maxSize)
{
	if (width == 0 || height == 0 || maxSize == 0)
		return;

	const DWORD fullXCount = width / maxSize;
	const DWORD fullYCount = height / maxSize;

	const DWORD restX = width % maxSize;
	const DWORD restY = height % maxSize;

	const DWORD restTextureX = GetNearTextureSize(restX);
	const DWORD restTextureY = GetNearTextureSize(restY);

	for (DWORD y = 0; y < fullYCount; ++y)
	{
		for (DWORD x = 0; x < fullXCount; ++x)
		{
			CBlockTexture* texture = BuildTextureBlock(
				x * maxSize,
				y * maxSize,
				maxSize,
				maxSize,
				maxSize,
				maxSize);

			if (texture)
				m_kVec_pkBlockTexture.push_back(texture);
		}

		if (restX > 0)
		{
			CBlockTexture* texture = BuildTextureBlock(
				fullXCount * maxSize,
				y * maxSize,
				restX,
				maxSize,
				restTextureX,
				maxSize);

			if (texture)
				m_kVec_pkBlockTexture.push_back(texture);
		}
	}

	if (restY > 0)
	{
		for (DWORD x = 0; x < fullXCount; ++x)
		{
			CBlockTexture* texture = BuildTextureBlock(
				x * maxSize,
				fullYCount * maxSize,
				maxSize,
				restY,
				maxSize,
				restTextureY);

			if (texture)
				m_kVec_pkBlockTexture.push_back(texture);
		}

		if (restX > 0)
		{
			CBlockTexture* texture = BuildTextureBlock(
				fullXCount * maxSize,
				fullYCount * maxSize,
				restX,
				restY,
				restTextureX,
				restTextureY);

			if (texture)
				m_kVec_pkBlockTexture.push_back(texture);
		}
	}
}
