#pragma once

#include "GrpImageInstance.h"

class CGraphicExpandedImageInstance : public CGraphicImageInstance
{
public:
	static DWORD Type();

	static void DeleteExpandedImageInstance(CGraphicExpandedImageInstance* instance)
	{
		Delete(instance);
	}

	enum ERenderingMode
	{
		RENDERING_MODE_NORMAL,
		RENDERING_MODE_SCREEN,
		RENDERING_MODE_COLOR_DODGE,
		RENDERING_MODE_MODULATE,
	};

public:
	CGraphicExpandedImageInstance();
	virtual ~CGraphicExpandedImageInstance();

	void Destroy();

	void SetDepth(float depth);
	void SetOrigin();
	void SetOrigin(float x, float y);
	void SetRotation(float rotation);
	void SetScale(float x, float y);
	void SetRenderingRect(float left, float top, float right, float bottom);
	void SetRenderingMode(int mode);

protected:
	void Initialize();

	void OnRender() override;
	void OnSetImagePointer() override;

	BOOL OnIsType(DWORD type) override;

protected:
	float m_fDepth;
	D3DXVECTOR2 m_v2Origin;
	D3DXVECTOR2 m_v2Scale;
	float m_fRotation;
	RECT m_RenderingRect;
	int m_iRenderingMode;

public:
	static void CreateSystem(UINT capacity);
	static void DestroySystem();

	static CGraphicExpandedImageInstance* New();
	static void Delete(CGraphicExpandedImageInstance* instance);

	static CDynamicPool<CGraphicExpandedImageInstance> ms_kPool;
};
