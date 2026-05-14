#pragma once

#include "GrpImageTexture.h"

#include <map>
#include <vector>
#include <string>

class CGraphicFontTexture
{
public:
	typedef wchar_t TCharacterKey;

	struct TCharacterInfomation
	{
		short index;
		short width;
		short height;
		float left;
		float top;
		float right;
		float bottom;
		float advance;
		float bearingX;
	};

	typedef std::vector<TCharacterInfomation*> TPCharacterInfomationVector;

public:
	CGraphicFontTexture();
	~CGraphicFontTexture();

	void Destroy();

	bool Create(const char* fontName, int fontSize, bool italic = false);
	bool CreateDeviceObjects();
	void DestroyDeviceObjects();

	bool IsEmpty() const;

	bool CheckTextureIndex(DWORD textureIndex) const;
	void SelectTexture(DWORD textureIndex);

	bool UpdateTexture();

	TCharacterInfomation* GetCharacterInfomation(wchar_t keyValue);
	TCharacterInfomation* UpdateCharacterInfomation(TCharacterKey keyValue);

	float GetKerning(wchar_t prev, wchar_t cur) const;

	ID3D11ShaderResourceView* GetSRV() const;

private:
	void Initialize();

	bool AppendTexture();
	bool RenderGlyphToAtlas(TCharacterKey keyValue, TCharacterInfomation& outInfo);
	void MoveToNextRow(int rowHeight);

private:
	typedef std::vector<CGraphicImageTexture*> TGraphicImageTexturePointerVector;
	typedef std::map<TCharacterKey, TCharacterInfomation> TCharacterInfomationMap;

private:
	TGraphicImageTexturePointerVector m_pFontTextureVector;
	TCharacterInfomationMap m_charInfoMap;

	std::vector<DWORD> m_atlasBuffer;

	std::string m_fontName;

	HFONT m_hFont;
	HDC m_hDC;
	HBITMAP m_hBitmap;
	void* m_bitmapBits;

	int m_fontSize;
	bool m_italic;

	int m_atlasWidth;
	int m_atlasHeight;

	int m_x;
	int m_y;
	int m_step;

	bool m_isDirty;
	short m_selectedTexture;

	int m_lineHeight;
	int m_ascent;
};
