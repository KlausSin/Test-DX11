#include "StdAfx.h"
#include "GrpFontTexture.h"

#include "EterBase/Stl.h"

CGraphicFontTexture::CGraphicFontTexture()
{
	Initialize();
}

CGraphicFontTexture::~CGraphicFontTexture()
{
	Destroy();
}

void CGraphicFontTexture::Initialize()
{
	m_hFont = nullptr;
	m_hDC = nullptr;
	m_hBitmap = nullptr;
	m_bitmapBits = nullptr;

	m_fontSize = 12;
	m_italic = false;

	m_atlasWidth = 512;
	m_atlasHeight = 512;

	m_x = 0;
	m_y = 0;
	m_step = 0;

	m_isDirty = false;
	m_selectedTexture = 0;

	m_lineHeight = 12;
	m_ascent = 10;

	m_fontName.clear();
}

void CGraphicFontTexture::Destroy()
{
	for (CGraphicImageTexture* texture : m_pFontTextureVector)
		delete texture;

	m_pFontTextureVector.clear();
	m_charInfoMap.clear();
	m_atlasBuffer.clear();

	if (m_hBitmap)
	{
		DeleteObject(m_hBitmap);
		m_hBitmap = nullptr;
	}

	if (m_hFont)
	{
		DeleteObject(m_hFont);
		m_hFont = nullptr;
	}

	if (m_hDC)
	{
		DeleteDC(m_hDC);
		m_hDC = nullptr;
	}

	Initialize();
}

bool CGraphicFontTexture::IsEmpty() const
{
	return m_hFont == nullptr;
}

bool CGraphicFontTexture::Create(const char* fontName, int fontSize, bool italic)
{
	Destroy();

	m_fontName = fontName ? fontName : "Tahoma";
	m_fontSize = fontSize > 0 ? fontSize : 8;
	m_italic = italic;

	m_atlasWidth = 256;
	m_atlasHeight = 256;

	m_atlasBuffer.resize(m_atlasWidth * m_atlasHeight);
	memset(m_atlasBuffer.data(), 0, m_atlasBuffer.size() * sizeof(DWORD));

	m_hDC = CreateCompatibleDC(nullptr);

	if (!m_hDC)
		return false;

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = m_atlasWidth;
	bmi.bmiHeader.biHeight = -m_atlasHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	m_hBitmap = CreateDIBSection(
		m_hDC,
		&bmi,
		DIB_RGB_COLORS,
		&m_bitmapBits,
		nullptr,
		0);

	if (!m_hBitmap || !m_bitmapBits)
		return false;

	SelectObject(m_hDC, m_hBitmap);

	wchar_t wideFontName[LF_FACESIZE] = {};
	MultiByteToWideChar(CP_ACP, 0, m_fontName.c_str(), -1, wideFontName, LF_FACESIZE);

	const int height = -m_fontSize;

	m_hFont = CreateFontW(
		height,
		0,
		0,
		0,
		FW_NORMAL,
		m_italic ? TRUE : FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		wideFontName);

	if (!m_hFont)
		return false;

	SelectObject(m_hDC, m_hFont);
	SetBkMode(m_hDC, TRANSPARENT);
	SetTextColor(m_hDC, RGB(255, 255, 255));

	TEXTMETRICW tm = {};
	GetTextMetricsW(m_hDC, &tm);

	m_lineHeight = tm.tmHeight;
	m_ascent = tm.tmAscent;

	if (!AppendTexture())
		return false;

	return true;
}

bool CGraphicFontTexture::CreateDeviceObjects()
{
	for (CGraphicImageTexture* texture : m_pFontTextureVector)
	{
		if (texture)
			texture->CreateDeviceObjects();
	}

	return true;
}

void CGraphicFontTexture::DestroyDeviceObjects()
{
	for (CGraphicImageTexture* texture : m_pFontTextureVector)
	{
		if (texture)
			texture->DestroyDeviceObjects();
	}
}

bool CGraphicFontTexture::AppendTexture()
{
	CGraphicImageTexture* texture = new CGraphicImageTexture;

	if (!texture->Create(
		static_cast<UINT>(m_atlasWidth),
		static_cast<UINT>(m_atlasHeight),
		DXGI_FORMAT_B8G8R8A8_UNORM))
	{
		delete texture;
		return false;
	}

	m_pFontTextureVector.push_back(texture);

	memset(m_atlasBuffer.data(), 0, m_atlasBuffer.size() * sizeof(DWORD));

	m_x = 0;
	m_y = 0;
	m_step = 0;
	m_isDirty = true;

	return true;
}

bool CGraphicFontTexture::CheckTextureIndex(DWORD textureIndex) const
{
	return textureIndex < m_pFontTextureVector.size();
}

void CGraphicFontTexture::SelectTexture(DWORD textureIndex)
{
	if (!CheckTextureIndex(textureIndex))
		return;

	m_selectedTexture = static_cast<short>(textureIndex);
}

ID3D11ShaderResourceView* CGraphicFontTexture::GetSRV() const
{
	if (!CheckTextureIndex(m_selectedTexture))
		return nullptr;

	return m_pFontTextureVector[m_selectedTexture]->GetSRV();
}

bool CGraphicFontTexture::UpdateTexture()
{
	if (!m_isDirty)
		return true;

	if (m_pFontTextureVector.empty())
		return false;

	CGraphicImageTexture* texture = m_pFontTextureVector.back();

	if (!texture)
		return false;

	int pitch = 0;
	void* pixels = nullptr;

	if (!texture->Lock(&pitch, &pixels))
		return false;

	const int dstPitch = pitch / 4;

	DWORD* dst = static_cast<DWORD*>(pixels);
	const DWORD* src = m_atlasBuffer.data();

	for (int y = 0; y < m_atlasHeight; ++y)
		memcpy(dst + y * dstPitch, src + y * m_atlasWidth, m_atlasWidth * sizeof(DWORD));

	texture->Unlock();

	m_isDirty = false;
	return true;
}

void CGraphicFontTexture::MoveToNextRow(int rowHeight)
{
	m_y += rowHeight + 1;
	m_x = 0;
	m_step = 0;
}

CGraphicFontTexture::TCharacterInfomation* CGraphicFontTexture::GetCharacterInfomation(wchar_t keyValue)
{
	TCharacterInfomationMap::iterator it = m_charInfoMap.find(keyValue);

	if (it != m_charInfoMap.end())
		return &it->second;

	return UpdateCharacterInfomation(keyValue);
}

CGraphicFontTexture::TCharacterInfomation* CGraphicFontTexture::UpdateCharacterInfomation(TCharacterKey keyValue)
{
	if (!m_hDC || !m_hFont)
		return nullptr;

	if (keyValue == 0x08)
		keyValue = L' ';

	TCharacterInfomation info = {};

	if (!RenderGlyphToAtlas(keyValue, info))
		return nullptr;

	TCharacterInfomation& storedInfo = m_charInfoMap[keyValue];
	storedInfo = info;

	return &storedInfo;
}

bool CGraphicFontTexture::RenderGlyphToAtlas(TCharacterKey keyValue, TCharacterInfomation& outInfo)
{
	SelectObject(m_hDC, m_hFont);

	SIZE size = {};
	wchar_t text[2] = { keyValue, 0 };

	GetTextExtentPoint32W(m_hDC, text, 1, &size);

	ABC abc = {};
	int advance = size.cx;

	if (GetCharABCWidthsW(m_hDC, keyValue, keyValue, &abc))
		advance = abc.abcA + abc.abcB + abc.abcC;

	const int glyphWidth = std::max((int)size.cx + 2, 1);
	const int glyphHeight = std::max(m_lineHeight, 1);

	if (m_x + glyphWidth >= m_atlasWidth)
		MoveToNextRow(m_step);

	if (m_y + glyphHeight >= m_atlasHeight)
	{
		if (!UpdateTexture())
			return false;

		if (!AppendTexture())
			return false;
	}

	RECT drawRect = {};
	drawRect.left = m_x;
	drawRect.top = m_y;
	drawRect.right = m_x + glyphWidth;
	drawRect.bottom = m_y + glyphHeight;

	FillRect(m_hDC, &drawRect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

	ExtTextOutW(
		m_hDC,
		m_x,
		m_y,
		ETO_CLIPPED,
		&drawRect,
		text,
		1,
		nullptr);

	DWORD* dib = static_cast<DWORD*>(m_bitmapBits);

	for (int y = 0; y < glyphHeight; ++y)
	{
		for (int x = 0; x < glyphWidth; ++x)
		{
			const int srcIndex = (m_y + y) * m_atlasWidth + (m_x + x);
			const DWORD src = dib[srcIndex];

			const BYTE b = static_cast<BYTE>((src >> 0) & 0xff);
			const BYTE g = static_cast<BYTE>((src >> 8) & 0xff);
			const BYTE r = static_cast<BYTE>((src >> 16) & 0xff);
			const BYTE a = std::max(r, std::max(g, b));

			m_atlasBuffer[srcIndex] =
				(static_cast<DWORD>(a) << 24) |
				(static_cast<DWORD>(r) << 16) |
				(static_cast<DWORD>(g) << 8) |
				static_cast<DWORD>(b);
		}
	}

	const float invWidth = 1.0f / static_cast<float>(m_atlasWidth);
	const float invHeight = 1.0f / static_cast<float>(m_atlasHeight);

	outInfo.index = static_cast<short>(m_pFontTextureVector.size() - 1);
	outInfo.width = static_cast<short>(glyphWidth);
	outInfo.height = static_cast<short>(glyphHeight);
	outInfo.left = static_cast<float>(m_x) * invWidth;
	outInfo.top = static_cast<float>(m_y) * invHeight;
	outInfo.right = static_cast<float>(m_x + glyphWidth) * invWidth;
	outInfo.bottom = static_cast<float>(m_y + glyphHeight) * invHeight;
	outInfo.advance = static_cast<float>(advance > 0 ? advance : glyphWidth);
	outInfo.bearingX = 0.0f;

	m_x += glyphWidth + 1;

	if (m_step < glyphHeight)
		m_step = glyphHeight;

	m_isDirty = true;
	return true;
}

float CGraphicFontTexture::GetKerning(wchar_t prev, wchar_t cur) const
{
	if (!m_hDC || !prev || !cur)
		return 0.0f;

	wchar_t pair[3] = { prev, cur, 0 };
	wchar_t first[2] = { prev, 0 };
	wchar_t second[2] = { cur, 0 };

	SIZE pairSize = {};
	SIZE firstSize = {};
	SIZE secondSize = {};

	GetTextExtentPoint32W(m_hDC, pair, 2, &pairSize);
	GetTextExtentPoint32W(m_hDC, first, 1, &firstSize);
	GetTextExtentPoint32W(m_hDC, second, 1, &secondSize);

	return static_cast<float>(pairSize.cx - firstSize.cx - secondSize.cx);
}
