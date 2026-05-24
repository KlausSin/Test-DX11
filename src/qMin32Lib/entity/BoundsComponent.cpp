#include "pch.h"
#include "BoundsComponent.h"

CBoundsComponent::CBoundsComponent()
	: m_dirty(true)
{
	Clear();
}

CBoundsComponent::~CBoundsComponent()
{
}

void CBoundsComponent::OnCreateInternal()
{
	Clear();
}

void CBoundsComponent::OnDestroyInternal()
{
}

void CBoundsComponent::Clear()
{
	for (uint32_t i = 0; i < 8; ++i)
		m_tbbox[i] = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	m_tbboxMin = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_tbboxMax = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_bboxMin = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_bboxMax = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_dirty = true;
}

void CBoundsComponent::SetBBox(const XMFLOAT3& min, const XMFLOAT3& max)
{
	m_bboxMin = min;
	m_bboxMax = max;

	m_dirty = true;
}

void CBoundsComponent::SetTBBox(const XMFLOAT3& min, const XMFLOAT3& max)
{
	m_tbboxMin = min;
	m_tbboxMax = max;

	m_dirty = true;
}

void CBoundsComponent::SetTBBoxVertex(uint32_t index, const XMFLOAT4& vertex)
{
	if (index >= 8)
		return;

	m_tbbox[index] = vertex;
	m_dirty = true;
}

XMFLOAT4& CBoundsComponent::GetTBBoxVertex(uint32_t index)
{
	return m_tbbox[index];
}

const XMFLOAT4& CBoundsComponent::GetTBBoxVertex(uint32_t index) const
{
	return m_tbbox[index];
}

XMFLOAT3& CBoundsComponent::GetTBBoxMin()
{
	return m_tbboxMin;
}

const XMFLOAT3& CBoundsComponent::GetTBBoxMin() const
{
	return m_tbboxMin;
}

XMFLOAT3& CBoundsComponent::GetTBBoxMax()
{
	return m_tbboxMax;
}

const XMFLOAT3& CBoundsComponent::GetTBBoxMax() const
{
	return m_tbboxMax;
}

XMFLOAT3& CBoundsComponent::GetBBoxMin()
{
	return m_bboxMin;
}

const XMFLOAT3& CBoundsComponent::GetBBoxMin() const
{
	return m_bboxMin;
}

XMFLOAT3& CBoundsComponent::GetBBoxMax()
{
	return m_bboxMax;
}

const XMFLOAT3& CBoundsComponent::GetBBoxMax() const
{
	return m_bboxMax;
}

void CBoundsComponent::CalculateTransformed(const XMFLOAT4X4& transform)
{
	m_tbbox[0] = XMFLOAT4(m_bboxMin.x, m_bboxMin.y, m_bboxMin.z, 1.0f);
	m_tbbox[1] = XMFLOAT4(m_bboxMin.x, m_bboxMax.y, m_bboxMin.z, 1.0f);
	m_tbbox[2] = XMFLOAT4(m_bboxMax.x, m_bboxMin.y, m_bboxMin.z, 1.0f);
	m_tbbox[3] = XMFLOAT4(m_bboxMax.x, m_bboxMax.y, m_bboxMin.z, 1.0f);

	m_tbbox[4] = XMFLOAT4(m_bboxMin.x, m_bboxMin.y, m_bboxMax.z, 1.0f);
	m_tbbox[5] = XMFLOAT4(m_bboxMin.x, m_bboxMax.y, m_bboxMax.z, 1.0f);
	m_tbbox[6] = XMFLOAT4(m_bboxMax.x, m_bboxMin.y, m_bboxMax.z, 1.0f);
	m_tbbox[7] = XMFLOAT4(m_bboxMax.x, m_bboxMax.y, m_bboxMax.z, 1.0f);

	for (uint32_t i = 0; i < 8; ++i)
	{
		XMStoreFloat4(&m_tbbox[i], XMVector4Transform(XMLoadFloat4(&m_tbbox[i]), XMLoadFloat4x4(&transform)));

		if (i == 0)
		{
			m_tbboxMin = XMFLOAT3(m_tbbox[i].x, m_tbbox[i].y, m_tbbox[i].z);
			m_tbboxMax = XMFLOAT3(m_tbbox[i].x, m_tbbox[i].y, m_tbbox[i].z);
		}
		else
		{
			m_tbboxMin.x = std::min(m_tbboxMin.x, m_tbbox[i].x);
			m_tbboxMin.y = std::min(m_tbboxMin.y, m_tbbox[i].y);
			m_tbboxMin.z = std::min(m_tbboxMin.z, m_tbbox[i].z);

			m_tbboxMax.x = std::max(m_tbboxMax.x, m_tbbox[i].x);
			m_tbboxMax.y = std::max(m_tbboxMax.y, m_tbbox[i].y);
			m_tbboxMax.z = std::max(m_tbboxMax.z, m_tbbox[i].z);
		}
	}

	m_dirty = false;
}
