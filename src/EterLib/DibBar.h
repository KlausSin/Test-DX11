#pragma once

#include "GrpDib.h"

class CBlockTexture;

class CDibBar
{
public:
	CDibBar();
	virtual ~CDibBar();

	bool Create(DWORD width, DWORD height);
	void Destroy();

	void Invalidate();
	void SetClipRect(const RECT& rect);
	void ClearBar();
	void Render(int x, int y);

protected:
	DWORD GetNearTextureSize(DWORD size) const;
	void BuildTextureBlockList(DWORD width, DWORD height, DWORD maxSize = 256);
	CBlockTexture* BuildTextureBlock(DWORD x, DWORD y, DWORD imageWidth, DWORD imageHeight, DWORD textureWidth, DWORD textureHeight);

	virtual void OnCreate() {}

protected:
	CGraphicDib m_dib;
	std::vector<CBlockTexture*> m_kVec_pkBlockTexture;

	DWORD m_dwWidth;
	DWORD m_dwHeight;
};
