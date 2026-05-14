#pragma once

#include "Resource.h"
#include "Ref.h"
#include "GrpImageTexture.h"

class CGraphicImage : public CResource
{
public:
	typedef CRef<CGraphicImage> TRef;

public:
	static TType Type();

public:
	explicit CGraphicImage(const char* fileName);
	~CGraphicImage() override;

	bool CreateDeviceObjects() override;
	void DestroyDeviceObjects() override;

	int GetWidth() const;
	int GetHeight() const;

	const RECT& GetRectReference() const;

	const CGraphicTexture& GetTextureReference() const;
	CGraphicTexture* GetTexturePointer();

protected:
	bool OnLoad(int size, const void* data) override;
	void OnClear() override;
	bool OnIsEmpty() const override;
	bool OnIsType(TType type) override;

protected:
	CGraphicImageTexture m_imageTexture;
	RECT m_rect;
};
