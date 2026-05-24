#pragma once

#include "EntityComponent.h"

class CRenderComponent : public CEntityComponent
{
public:
	CRenderComponent();
	virtual ~CRenderComponent();

	void OnCreateInternal() override;
	void OnDestroyInternal() override;

	void Show();
	void Hide();

	bool IsVisible() const;
	bool IsAlwaysHidden() const;
	bool IsBlockCamera() const;

	void SetVisible(bool visible);
	void SetAlwaysHidden(bool hidden);
	void SetBlockCamera(bool block);

	void SetRenderFlags(uint32_t flags);
	uint32_t GetRenderFlags() const;

	void AddRenderFlag(uint32_t flag);
	void RemoveRenderFlag(uint32_t flag);
	bool HasRenderFlag(uint32_t flag) const;

	void SetRenderMode(uint32_t mode);
	uint32_t GetRenderMode() const;

	void SetRenderObject(void* object);
	void* GetRenderObject();

	const void* GetRenderObject() const;

private:
	bool m_visible;
	bool m_alwaysHidden;
	bool m_blockCamera;

	uint32_t m_renderFlags;
	uint32_t m_renderMode;

	void* m_renderObject;
};
