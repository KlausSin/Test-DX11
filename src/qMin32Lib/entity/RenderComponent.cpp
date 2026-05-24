#include "pch.h"
#include "RenderComponent.h"

CRenderComponent::CRenderComponent()
	: m_visible(true),
	m_alwaysHidden(false),
	m_blockCamera(false),
	m_renderFlags(0),
	m_renderMode(0),
	m_renderObject(nullptr)
{
}

CRenderComponent::~CRenderComponent()
{
}

void CRenderComponent::OnCreateInternal()
{
	m_visible = true;
	m_alwaysHidden = false;
	m_blockCamera = false;

	m_renderFlags = 0;
	m_renderMode = 0;

	m_renderObject = nullptr;
}

void CRenderComponent::OnDestroyInternal()
{
	m_renderObject = nullptr;
}

void CRenderComponent::Show()
{
	m_visible = true;
}

void CRenderComponent::Hide()
{
	m_visible = false;
}

bool CRenderComponent::IsVisible() const
{
	return m_visible;
}

bool CRenderComponent::IsAlwaysHidden() const
{
	return m_alwaysHidden;
}

bool CRenderComponent::IsBlockCamera() const
{
	return m_blockCamera;
}

void CRenderComponent::SetVisible(bool visible)
{
	m_visible = visible;
}

void CRenderComponent::SetAlwaysHidden(bool hidden)
{
	m_alwaysHidden = hidden;
}

void CRenderComponent::SetBlockCamera(bool block)
{
	m_blockCamera = block;
}

void CRenderComponent::SetRenderFlags(uint32_t flags)
{
	m_renderFlags = flags;
}

uint32_t CRenderComponent::GetRenderFlags() const
{
	return m_renderFlags;
}

void CRenderComponent::AddRenderFlag(uint32_t flag)
{
	m_renderFlags |= flag;
}

void CRenderComponent::RemoveRenderFlag(uint32_t flag)
{
	m_renderFlags &= ~flag;
}

bool CRenderComponent::HasRenderFlag(uint32_t flag) const
{
	return (m_renderFlags & flag) != 0;
}

void CRenderComponent::SetRenderMode(uint32_t mode)
{
	m_renderMode = mode;
}

uint32_t CRenderComponent::GetRenderMode() const
{
	return m_renderMode;
}

void CRenderComponent::SetRenderObject(void* object)
{
	m_renderObject = object;
}

void* CRenderComponent::GetRenderObject()
{
	return m_renderObject;
}

const void* CRenderComponent::GetRenderObject() const
{
	return m_renderObject;
}
