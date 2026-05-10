#pragma once
#include "D3D11StateCommon.h"
#include "EterLib/D3DXMathCompat.h"

struct SD3D11TransformStateKey
{
	D3DXMATRIX world;
	D3DXMATRIX view;
	D3DXMATRIX projection;
	D3DXMATRIX texture[2];

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

	void SetWorld(const D3DXMATRIX& value);
	void SetView(const D3DXMATRIX& value);
	void SetProjection(const D3DXMATRIX& value);
	void SetTexture0(const D3DXMATRIX& value);
	void SetTexture1(const D3DXMATRIX& value);
	void SetKey(const SD3D11TransformStateKey& key);

	const D3DXMATRIX& GetWorld() const;
	const D3DXMATRIX& GetView() const;
	const D3DXMATRIX& GetProjection() const;
	const D3DXMATRIX& GetTexture0() const;
	const D3DXMATRIX& GetTexture1() const;
	const SD3D11TransformStateKey& GetKey() const;

private:
	void ApplyKey();

	SD3D11TransformStateKey m_key;
	std::vector<SD3D11TransformStateKey> m_stack;
	bool m_dirty;
};
