#pragma once

#include "DibBar.h"

class CTextBar : public CDibBar
{
public:
	CTextBar(int fontSize, bool isBold);
	virtual ~CTextBar();

	void TextOut(int x, int y, const char* text);
	void SetTextColor(int r, int g, int b);
	void GetTextExtent(const char* text, SIZE* size);

protected:
	void __SetFont(int fontSize, bool isBold);
	void OnCreate();

private:
	void DestroyFont();

private:
	HFONT m_hFont;
	DWORD m_textColor;

	int m_fontSize;
	bool m_isBold;
	int m_lineHeight;
};
