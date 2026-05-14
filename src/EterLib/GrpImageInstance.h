#pragma once

#include "GrpImage.h"
#include "Pool.h"

class CGraphicImageInstance
{
public:
	static DWORD Type();
	BOOL IsType(DWORD type);

public:
	CGraphicImageInstance();
	virtual ~CGraphicImageInstance();

	void Destroy();
	void Render();

	void SetDiffuseColor(float r, float g, float b, float a);
	void SetPosition(float x, float y);

	void SetImagePointer(CGraphicImage* image);
	void ReloadImagePointer(CGraphicImage* image);

	bool IsEmpty() const;

	int GetWidth() const;
	int GetHeight() const;

	CGraphicTexture* GetTexturePointer();
	const CGraphicTexture& GetTextureReference() const;

	CGraphicImage* GetGraphicImagePointer();
	const CGraphicImage* GetGraphicImagePointer() const;

	bool operator == (const CGraphicImageInstance& rhs) const;

protected:
	void Initialize();

	virtual void OnRender();
	virtual void OnSetImagePointer();

	virtual BOOL OnIsType(DWORD type);

protected:
	XMFLOAT4 m_DiffuseColor;
	XMFLOAT2 m_v2Position;

	CGraphicImage::TRef m_roImage;

public:
	static void CreateSystem(UINT capacity);
	static void DestroySystem();

	static CGraphicImageInstance* New();
	static void Delete(CGraphicImageInstance* instance);

	static CDynamicPool<CGraphicImageInstance> ms_kPool;
};
