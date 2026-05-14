#pragma once

#include "GrpImage.h"

class CGraphicSubImage : public CGraphicImage
{
public:
	typedef CRef<CGraphicImage> TRef;

public:
	static TType Type();
	static char m_SearchPath[256];

public:
	explicit CGraphicSubImage(const char* fileName);
	~CGraphicSubImage() override;

	bool CreateDeviceObjects() override;

	void SetImagePointer(CGraphicImage* image);
	bool SetImageFileName(const char* fileName);

	void SetRectPosition(int left, int top, int right, int bottom);
	void SetRectReference(const RECT& rect);

	static void SetSearchPath(const char* fileName);

protected:
	bool OnLoad(int size, const void* data) override;
	void OnClear() override;
	bool OnIsEmpty() const override;
	bool OnIsType(TType type) override;

protected:
	CGraphicImage::TRef m_roImage;
};
