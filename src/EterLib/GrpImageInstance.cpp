#include "StdAfx.h"
#include "GrpImageInstance.h"
#include "StateManager.h"
#include "EterBase/CRC32.h"

CDynamicPool<CGraphicImageInstance> CGraphicImageInstance::ms_kPool;

void CGraphicImageInstance::CreateSystem(UINT capacity)
{
	ms_kPool.Create(capacity);
}

void CGraphicImageInstance::DestroySystem()
{
	ms_kPool.Destroy();
}

CGraphicImageInstance* CGraphicImageInstance::New()
{
	return ms_kPool.Alloc();
}

void CGraphicImageInstance::Delete(CGraphicImageInstance* instance)
{
	if (!instance)
		return;

	instance->Destroy();
	ms_kPool.Free(instance);
}

CGraphicImageInstance::CGraphicImageInstance()
{
	Initialize();
}

CGraphicImageInstance::~CGraphicImageInstance()
{
	Destroy();
}

void CGraphicImageInstance::Initialize()
{
	m_DiffuseColor.x = 1.0f;
	m_DiffuseColor.y = 1.0f;
	m_DiffuseColor.z = 1.0f;
	m_DiffuseColor.w = 1.0f;

	m_v2Position.x = 0.0f;
	m_v2Position.y = 0.0f;
}

void CGraphicImageInstance::Destroy()
{
	m_roImage.SetPointer(nullptr);
	Initialize();
}

void CGraphicImageInstance::Render()
{
	if (IsEmpty())
		return;

	OnRender();
}

void CGraphicImageInstance::OnRender()
{
	CGraphicImage* image = m_roImage.GetPointer();

	if (!image)
		return;

	CGraphicTexture* texture = image->GetTexturePointer();

	if (!texture || !texture->GetSRV())
		return;

	const RECT& rect = image->GetRectReference();

	const float imageWidth = static_cast<float>(image->GetWidth());
	const float imageHeight = static_cast<float>(image->GetHeight());

	const float textureWidth = static_cast<float>(texture->GetWidth());
	const float textureHeight = static_cast<float>(texture->GetHeight());

	if (textureWidth <= 0.0f || textureHeight <= 0.0f)
		return;

	const float invTextureWidth = 1.0f / textureWidth;
	const float invTextureHeight = 1.0f / textureHeight;

	const float su = static_cast<float>(rect.left) * invTextureWidth;
	const float sv = static_cast<float>(rect.top) * invTextureHeight;
	const float eu = static_cast<float>(rect.right) * invTextureWidth;
	const float ev = static_cast<float>(rect.bottom) * invTextureHeight;

	TPDTVertex vertices[4];

	vertices[0].position = TPosition(m_v2Position.x, m_v2Position.y, 0.0f);
	vertices[1].position = TPosition(m_v2Position.x + imageWidth, m_v2Position.y, 0.0f);
	vertices[2].position = TPosition(m_v2Position.x, m_v2Position.y + imageHeight, 0.0f);
	vertices[3].position = TPosition(m_v2Position.x + imageWidth, m_v2Position.y + imageHeight, 0.0f);

	vertices[0].texCoord = TTextureCoordinate(su, sv);
	vertices[1].texCoord = TTextureCoordinate(eu, sv);
	vertices[2].texCoord = TTextureCoordinate(su, ev);
	vertices[3].texCoord = TTextureCoordinate(eu, ev);

	vertices[0].diffuse = ColorToUint(m_DiffuseColor);
	vertices[1].diffuse = ColorToUint(m_DiffuseColor);
	vertices[2].diffuse = ColorToUint(m_DiffuseColor);
	vertices[3].diffuse = ColorToUint(m_DiffuseColor);

	STATEMANAGER.GetStateCache().Push();

	STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);

	STATEMANAGER.GetDepthStencil().SetDepthEnable(false);
	STATEMANAGER.GetDepthStencil().SetDepthWriteEnable(false);

	STATEMANAGER.GetBlend().SetBlendEnable(true);
	STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);

	_mgr->GetCbMgr()->SetAlphaTestEnable(false);

	if (CGraphicBase::SetPDTStream(vertices, 4))
	{
		CGraphicBase::SetDefaultIndexBuffer(CGraphicBase::DEFAULT_IB_FILL_RECT);

		STATEMANAGER.SetTexture(0, texture->GetSRV());
		STATEMANAGER.SetTexture(1, nullptr);

		_mgr->SetShader(VF_PDT, BLEND_MODULATE);

		STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 0, 2);
	}

	STATEMANAGER.GetStateCache().Restore();
}

void CGraphicImageInstance::SetDiffuseColor(float r, float g, float b, float a)
{
	m_DiffuseColor.x = r;
	m_DiffuseColor.y = g;
	m_DiffuseColor.z = b;
	m_DiffuseColor.w = a;
}

void CGraphicImageInstance::SetPosition(float x, float y)
{
	m_v2Position.x = x;
	m_v2Position.y = y;
}

void CGraphicImageInstance::SetImagePointer(CGraphicImage* image)
{
	m_roImage.SetPointer(image);
	OnSetImagePointer();
}

void CGraphicImageInstance::ReloadImagePointer(CGraphicImage* image)
{
	if (m_roImage.IsNull())
	{
		SetImagePointer(image);
		return;
	}

	CGraphicImage* currentImage = m_roImage.GetPointer();

	if (currentImage)
		currentImage->Reload();
}

bool CGraphicImageInstance::IsEmpty() const
{
	return m_roImage.IsNull() || m_roImage->IsEmpty();
}

int CGraphicImageInstance::GetWidth() const
{
	if (IsEmpty())
		return 0;

	return m_roImage->GetWidth();
}

int CGraphicImageInstance::GetHeight() const
{
	if (IsEmpty())
		return 0;

	return m_roImage->GetHeight();
}

CGraphicTexture* CGraphicImageInstance::GetTexturePointer()
{
	CGraphicImage* image = m_roImage.GetPointer();

	if (!image)
		return nullptr;

	return image->GetTexturePointer();
}

const CGraphicTexture& CGraphicImageInstance::GetTextureReference() const
{
	return m_roImage->GetTextureReference();
}

CGraphicImage* CGraphicImageInstance::GetGraphicImagePointer()
{
	return m_roImage.GetPointer();
}

const CGraphicImage* CGraphicImageInstance::GetGraphicImagePointer() const
{
	return m_roImage.GetPointer();
}

bool CGraphicImageInstance::operator == (const CGraphicImageInstance& rhs) const
{
	return m_roImage.GetPointer() == rhs.m_roImage.GetPointer();
}

DWORD CGraphicImageInstance::Type()
{
	static DWORD type = GetCRC32("CGraphicImageInstance", strlen("CGraphicImageInstance"));

	return type;
}

BOOL CGraphicImageInstance::IsType(DWORD type)
{
	return OnIsType(type);
}

BOOL CGraphicImageInstance::OnIsType(DWORD type)
{
	if (CGraphicImageInstance::Type() == type)
		return TRUE;

	return FALSE;
}

void CGraphicImageInstance::OnSetImagePointer()
{
}
