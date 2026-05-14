#include "StdAfx.h"
#include "GrpExpandedImageInstance.h"
#include "StateManager.h"
#include "EterBase/CRC32.h"

CDynamicPool<CGraphicExpandedImageInstance> CGraphicExpandedImageInstance::ms_kPool;

void CGraphicExpandedImageInstance::CreateSystem(UINT capacity)
{
	ms_kPool.Create(capacity);
}

void CGraphicExpandedImageInstance::DestroySystem()
{
	ms_kPool.Destroy();
}

CGraphicExpandedImageInstance* CGraphicExpandedImageInstance::New()
{
	return ms_kPool.Alloc();
}

void CGraphicExpandedImageInstance::Delete(CGraphicExpandedImageInstance* instance)
{
	if (!instance)
		return;

	instance->Destroy();
	ms_kPool.Free(instance);
}

CGraphicExpandedImageInstance::CGraphicExpandedImageInstance()
{
	Initialize();
}

CGraphicExpandedImageInstance::~CGraphicExpandedImageInstance()
{
	Destroy();
}

void CGraphicExpandedImageInstance::Initialize()
{
	m_fDepth = 0.0f;

	m_v2Origin.x = 0.0f;
	m_v2Origin.y = 0.0f;

	m_v2Scale.x = 1.0f;
	m_v2Scale.y = 1.0f;

	m_fRotation = 0.0f;

	ZeroMemory(&m_RenderingRect, sizeof(m_RenderingRect));

	m_iRenderingMode = RENDERING_MODE_NORMAL;
}

void CGraphicExpandedImageInstance::Destroy()
{
	CGraphicImageInstance::Destroy();
	Initialize();
}

void CGraphicExpandedImageInstance::OnRender()
{
	CGraphicImage* image = m_roImage.GetPointer();

	if (!image)
		return;

	CGraphicTexture* texture = image->GetTexturePointer();

	if (!texture || !texture->GetSRV())
		return;

	const RECT& rect = image->GetRectReference();

	const float textureWidth = static_cast<float>(texture->GetWidth());
	const float textureHeight = static_cast<float>(texture->GetHeight());

	if (textureWidth <= 0.0f || textureHeight <= 0.0f)
		return;

	const float invTextureWidth = 1.0f / textureWidth;
	const float invTextureHeight = 1.0f / textureHeight;

	const float imageWidth = static_cast<float>(image->GetWidth()) * m_v2Scale.x;
	const float imageHeight = static_cast<float>(image->GetHeight()) * m_v2Scale.y;

	const float su = static_cast<float>(rect.left - m_RenderingRect.left) * invTextureWidth;
	const float sv = static_cast<float>(rect.top - m_RenderingRect.top) * invTextureHeight;

	const float eu = static_cast<float>(
		rect.left + m_RenderingRect.right + image->GetWidth()) * invTextureWidth;

	const float ev = static_cast<float>(
		rect.top + m_RenderingRect.bottom + image->GetHeight()) * invTextureHeight;

	TPDTVertex vertices[4];

	vertices[0].position = TPosition(m_v2Position.x, m_v2Position.y, m_fDepth);
	vertices[1].position = TPosition(m_v2Position.x, m_v2Position.y, m_fDepth);
	vertices[2].position = TPosition(m_v2Position.x, m_v2Position.y, m_fDepth);
	vertices[3].position = TPosition(m_v2Position.x, m_v2Position.y, m_fDepth);

	vertices[0].texCoord = TTextureCoordinate(su, sv);
	vertices[1].texCoord = TTextureCoordinate(eu, sv);
	vertices[2].texCoord = TTextureCoordinate(su, ev);
	vertices[3].texCoord = TTextureCoordinate(eu, ev);

	vertices[0].diffuse = m_DiffuseColor;
	vertices[1].diffuse = m_DiffuseColor;
	vertices[2].diffuse = m_DiffuseColor;
	vertices[3].diffuse = m_DiffuseColor;

	if (m_fRotation == 0.0f)
	{
		vertices[0].position.x -= static_cast<float>(m_RenderingRect.left);
		vertices[0].position.y -= static_cast<float>(m_RenderingRect.top);

		vertices[1].position.x += imageWidth + static_cast<float>(m_RenderingRect.right);
		vertices[1].position.y -= static_cast<float>(m_RenderingRect.top);

		vertices[2].position.x -= static_cast<float>(m_RenderingRect.left);
		vertices[2].position.y += imageHeight + static_cast<float>(m_RenderingRect.bottom);

		vertices[3].position.x += imageWidth + static_cast<float>(m_RenderingRect.right);
		vertices[3].position.y += imageHeight + static_cast<float>(m_RenderingRect.bottom);
	}
	else
	{
		const float halfWidth = imageWidth * 0.5f;
		const float halfHeight = imageHeight * 0.5f;
		const float radian = D3DXToRadian(m_fRotation);

		const float cosValue = cosf(radian);
		const float sinValue = sinf(radian);

		const float localX[4] = { -halfWidth, halfWidth, -halfWidth, halfWidth };
		const float localY[4] = { -halfHeight, -halfHeight, halfHeight, halfHeight };

		for (int i = 0; i < 4; ++i)
		{
			vertices[i].position.x += m_v2Origin.x + localX[i] * cosValue - localY[i] * sinValue;
			vertices[i].position.y += m_v2Origin.y + localX[i] * sinValue + localY[i] * cosValue;
		}
	}

	switch (m_iRenderingMode)
	{
		case RENDERING_MODE_SCREEN:
		case RENDERING_MODE_COLOR_DODGE:
			STATEMANAGER.GetBlend().Push();
			STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_INV_DEST_COLOR);
			STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_ONE);
			break;

		case RENDERING_MODE_MODULATE:
			STATEMANAGER.GetBlend().Push();
			STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_ZERO);
			STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_SRC_COLOR);
			break;
	}

	if (CGraphicBase::SetPDTStream(vertices, 4))
	{
		CGraphicBase::SetDefaultIndexBuffer(CGraphicBase::DEFAULT_IB_FILL_RECT);

		STATEMANAGER.SetTexture(0, texture->GetSRV());
		STATEMANAGER.SetTexture(1, nullptr);

		_mgr->SetShader(VF_PDT, BLEND_UI_TEX);

		STATEMANAGER.DrawIndexedPrimitive11(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
			0,
			0,
			2);
	}

	switch (m_iRenderingMode)
	{
		case RENDERING_MODE_SCREEN:
		case RENDERING_MODE_COLOR_DODGE:
		case RENDERING_MODE_MODULATE:
			STATEMANAGER.GetBlend().Restore();
			break;
	}
}

void CGraphicExpandedImageInstance::SetDepth(float depth)
{
	m_fDepth = depth;
}

void CGraphicExpandedImageInstance::SetOrigin()
{
	SetOrigin(
		static_cast<float>(GetWidth()) * 0.5f,
		static_cast<float>(GetHeight()) * 0.5f);
}

void CGraphicExpandedImageInstance::SetOrigin(float x, float y)
{
	m_v2Origin.x = x;
	m_v2Origin.y = y;
}

void CGraphicExpandedImageInstance::SetRotation(float rotation)
{
	m_fRotation = rotation;
}

void CGraphicExpandedImageInstance::SetScale(float x, float y)
{
	m_v2Scale.x = x;
	m_v2Scale.y = y;
}

void CGraphicExpandedImageInstance::SetRenderingRect(
	float left,
	float top,
	float right,
	float bottom)
{
	if (IsEmpty())
		return;

	const float width = static_cast<float>(GetWidth());
	const float height = static_cast<float>(GetHeight());

	m_RenderingRect.left = static_cast<LONG>(width * left);
	m_RenderingRect.top = static_cast<LONG>(height * top);
	m_RenderingRect.right = static_cast<LONG>(width * right);
	m_RenderingRect.bottom = static_cast<LONG>(height * bottom);
}

void CGraphicExpandedImageInstance::SetRenderingMode(int mode)
{
	m_iRenderingMode = mode;
}

void CGraphicExpandedImageInstance::OnSetImagePointer()
{
	if (IsEmpty())
		return;

	SetOrigin();
}

BOOL CGraphicExpandedImageInstance::OnIsType(DWORD type)
{
	if (CGraphicExpandedImageInstance::Type() == type)
		return TRUE;

	return CGraphicImageInstance::OnIsType(type);
}

DWORD CGraphicExpandedImageInstance::Type()
{
	static DWORD type = GetCRC32(
		"CGraphicExpandedImageInstance",
		strlen("CGraphicExpandedImageInstance"));

	return type;
}
