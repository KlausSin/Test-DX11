#include "StdAfx.h"
#include "TextBar.h"

#include <utf8.h>

CTextBar::CTextBar(int fontSize, bool isBold)
{
	m_hFont = nullptr;
	m_textColor = 0x00FFFFFF;
	m_fontSize = fontSize;
	m_isBold = isBold;
	m_lineHeight = fontSize > 0 ? fontSize : 12;
}

CTextBar::~CTextBar()
{
	DestroyFont();
}

void CTextBar::DestroyFont()
{
	if (m_hFont)
	{
		DeleteObject(m_hFont);
		m_hFont = nullptr;
	}
}

void CTextBar::__SetFont(int fontSize, bool isBold)
{
	DestroyFont();

	m_fontSize = fontSize > 0 ? fontSize : 12;
	m_isBold = isBold;

	m_hFont = CreateFontW(
		-m_fontSize,
		0,
		0,
		0,
		m_isBold ? FW_BOLD : FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		L"Tahoma");

	HDC dc = CreateCompatibleDC(nullptr);

	if (dc)
	{
		HGDIOBJ oldFont = nullptr;

		if (m_hFont)
			oldFont = SelectObject(dc, m_hFont);

		TEXTMETRICW tm = {};
		GetTextMetricsW(dc, &tm);

		m_lineHeight = tm.tmHeight > 0 ? tm.tmHeight : m_fontSize;

		if (oldFont)
			SelectObject(dc, oldFont);

		DeleteDC(dc);
	}
}

void CTextBar::SetTextColor(int r, int g, int b)
{
	m_textColor =
		(static_cast<DWORD>(r) << 16) |
		(static_cast<DWORD>(g) << 8) |
		static_cast<DWORD>(b);
}

void CTextBar::GetTextExtent(const char* text, SIZE* size)
{
	if (!size)
		return;

	size->cx = 0;
	size->cy = 0;

	if (!text || !*text || !m_hFont)
		return;

	std::wstring wideText = Utf8ToWide(text);

	HDC dc = CreateCompatibleDC(nullptr);

	if (!dc)
		return;

	HGDIOBJ oldFont = SelectObject(dc, m_hFont);

	GetTextExtentPoint32W(
		dc,
		wideText.c_str(),
		static_cast<int>(wideText.length()),
		size);

	size->cy = m_lineHeight;

	SelectObject(dc, oldFont);
	DeleteDC(dc);
}

void CTextBar::TextOut(int x, int y, const char* text)
{
	if (!text || !*text || !m_hFont)
		return;

	DWORD* buffer = static_cast<DWORD*>(m_dib.GetPointer());

	if (!buffer)
		return;

	const int width = m_dib.GetWidth();
	const int height = m_dib.GetHeight();

	if (width <= 0 || height <= 0)
		return;

	std::wstring wideText = Utf8ToWide(text);

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* bits = nullptr;

	HDC dc = CreateCompatibleDC(nullptr);

	if (!dc)
		return;

	HBITMAP bitmap = CreateDIBSection(
		dc,
		&bmi,
		DIB_RGB_COLORS,
		&bits,
		nullptr,
		0);

	if (!bitmap || !bits)
	{
		if (bitmap)
			DeleteObject(bitmap);

		DeleteDC(dc);
		return;
	}

	memcpy(bits, buffer, width * height * sizeof(DWORD));

	HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
	HGDIOBJ oldFont = SelectObject(dc, m_hFont);

	SetBkMode(dc, TRANSPARENT);

	const BYTE r = static_cast<BYTE>((m_textColor >> 16) & 0xff);
	const BYTE g = static_cast<BYTE>((m_textColor >> 8) & 0xff);
	const BYTE b = static_cast<BYTE>(m_textColor & 0xff);

	::SetTextColor(dc, RGB(r, g, b));

	ExtTextOutW(
		dc,
		x,
		y,
		0,
		nullptr,
		wideText.c_str(),
		static_cast<UINT>(wideText.length()),
		nullptr);

	memcpy(buffer, bits, width * height * sizeof(DWORD));

	SelectObject(dc, oldFont);
	SelectObject(dc, oldBitmap);

	DeleteObject(bitmap);
	DeleteDC(dc);

	Invalidate();
}

void CTextBar::OnCreate()
{
	__SetFont(m_fontSize, m_isBold);
}
