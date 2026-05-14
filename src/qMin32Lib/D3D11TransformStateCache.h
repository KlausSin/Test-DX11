#pragma once
#include "D3D11StateCommon.h"
#include "EterLib/D3DXMathCompat.h"

struct SD3D11TransformStateKey
{
	XMFLOAT4X4 world;
	XMFLOAT4X4 view;
	XMFLOAT4X4 projection;
	XMFLOAT4X4 texture[4];

	SD3D11TransformStateKey();
	bool operator==(const SD3D11TransformStateKey& rhs) const;
};

class CD3D11TransformStateCache
{
public:
	CD3D11TransformStateCache();

	void ResetDefault();
	void ForceDirty();
	bool IsDirty() const;
	bool CanRestore() const;

	void Push();
	bool Restore();
	void ClearStack();

	void SetWorld(const XMFLOAT4X4& value);
	void SetView(const XMFLOAT4X4& value);
	void SetProjection(const XMFLOAT4X4& value);
	void SetTexture0(const XMFLOAT4X4& value);
	void SetTexture1(const XMFLOAT4X4& value);
	void SetTexture2(const XMFLOAT4X4& value);
	void SetTexture3(const XMFLOAT4X4& value);
	void SetKey(const SD3D11TransformStateKey& key);

	const XMFLOAT4X4& GetWorld() const;
	const XMFLOAT4X4& GetView() const;
	const XMFLOAT4X4& GetProjection() const;
	const XMFLOAT4X4& GetTexture0() const;
	const XMFLOAT4X4& GetTexture1() const;
	const SD3D11TransformStateKey& GetKey() const;

private:
	void ApplyKey();

	SD3D11TransformStateKey m_key;
	std::vector<SD3D11TransformStateKey> m_stack;
	bool m_dirty;
};
