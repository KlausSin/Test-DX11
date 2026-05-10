#pragma once
#include "D3D11StateCommon.h"
#include "EterLib/D3DXMathCompat.h"
#include "Core.h"

struct SD3D11LightingStateKey
{
	bool lightingEnable;
	DWORD ambient;
	D3DMATERIAL11 material;
	D3DLIGHT11 lights[8];

	SD3D11LightingStateKey();
	bool operator==(const SD3D11LightingStateKey& rhs) const;
};

class CD3D11LightingStateCache
{
public:
	CD3D11LightingStateCache();

	void ResetDefault();
	void ForceDirty();
	bool IsDirty() const;
	bool CanRestore() const;

	void Push();
	bool Restore();
	void ClearStack();

	void SetLightingEnable(bool value);
	void SetAmbient(DWORD color);
	void SetMaterial(const D3DMATERIAL11& material);
	void SetLight(UINT index, const D3DLIGHT11& light);
	void SetKey(const SD3D11LightingStateKey& key);

	bool GetLightingEnable() const;
	DWORD GetAmbient() const;
	const D3DMATERIAL11& GetMaterial() const;
	const D3DLIGHT11& GetLight(UINT index) const;
	const SD3D11LightingStateKey& GetKey() const;

private:
	bool IsValidLight(UINT index) const;
	void ApplyKey();

	SD3D11LightingStateKey m_key;
	std::vector<SD3D11LightingStateKey> m_stack;
	bool m_dirty;
};
