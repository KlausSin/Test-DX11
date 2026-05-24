#pragma once

#include "EntityComponent.h"
#include <DirectXMath.h>

using namespace DirectX;

class CBoundsComponent : public CEntityComponent
{
public:
	CBoundsComponent();
	virtual ~CBoundsComponent();

	void OnCreateInternal() override;
	void OnDestroyInternal() override;

	void Clear();

	void SetBBox(const XMFLOAT3& min,const XMFLOAT3& max);

	void SetTBBox(const XMFLOAT3& min, const XMFLOAT3& max);

	void SetTBBoxVertex(uint32_t index, const XMFLOAT4& vertex);

	XMFLOAT4& GetTBBoxVertex(uint32_t index);
	const XMFLOAT4& GetTBBoxVertex(uint32_t index) const;

	XMFLOAT3& GetTBBoxMin();
	const XMFLOAT3& GetTBBoxMin() const;

	XMFLOAT3& GetTBBoxMax();
	const XMFLOAT3& GetTBBoxMax() const;

	XMFLOAT3& GetBBoxMin();
	const XMFLOAT3& GetBBoxMin() const;

	XMFLOAT3& GetBBoxMax();
	const XMFLOAT3& GetBBoxMax() const;

	void CalculateTransformed(const XMFLOAT4X4& transform);

private:
	XMFLOAT4 m_tbbox[8];

	XMFLOAT3 m_tbboxMin;
	XMFLOAT3 m_tbboxMax;

	XMFLOAT3 m_bboxMin;
	XMFLOAT3 m_bboxMax;

	bool m_dirty;
};
