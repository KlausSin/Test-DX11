#pragma once

#include "GrpTexture.h"

#include <DirectXTex/DirectXTex.h>
#include <string>

class CGraphicImageTexture : public CGraphicTexture
{
public:
	CGraphicImageTexture();
	virtual ~CGraphicImageTexture();

	void Destroy();

	bool Create(UINT width, UINT height, DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM);

	bool CreateDeviceObjects();

	void CreateFromTexturePointer(const CGraphicTexture* sourceTexture);
	bool CreateFromDiskFile(const char* fileName, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, DWORD filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	bool CreateFromMemoryFile(UINT size, const void* data, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, DWORD filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	bool CreateFromRawPixels(UINT width, UINT height, DXGI_FORMAT format, const void* pixels, UINT pitch);

	bool Lock(int* pitch, void** pixels, int level = 0);
	void Unlock(int level = 0);
	void SetFileName(const char* fileName);

private:
	void Initialize();

	bool CreateFromScratchImage(const DirectX::ScratchImage& image);
	bool LoadFromMemory(UINT size, const void* data, DirectX::ScratchImage& image);
	bool LoadFromFile(const wchar_t* fileName, DirectX::ScratchImage& image);

private:
	DXGI_FORMAT m_d3dFmt;
	DWORD m_dwFilter;

	std::string m_fileName;

	std::vector<uint8_t> m_lockBuffer;
	UINT m_lockPitch;
};
