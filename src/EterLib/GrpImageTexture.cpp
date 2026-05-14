#include "StdAfx.h"
#include "GrpImageTexture.h"
#include "StateManager.h"
#include "PackLib/PackManager.h"
#include <EterBase/Stl.h>

using namespace DirectX;

CGraphicImageTexture::CGraphicImageTexture()
{
	Initialize();
}

CGraphicImageTexture::~CGraphicImageTexture()
{
	Destroy();
}

void CGraphicImageTexture::Initialize()
{
	CGraphicTexture::Initialize();

	m_d3dFmt = DXGI_FORMAT_UNKNOWN;
	m_dwFilter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	m_fileName.clear();

	m_lockPitch = 0;
}

void CGraphicImageTexture::Destroy()
{
	m_lockBuffer.clear();
	m_lockBuffer.shrink_to_fit();

	CGraphicTexture::Destroy();

	Initialize();
}

void CGraphicImageTexture::SetFileName(const char* fileName)
{
	m_fileName = fileName ? fileName : "";
}

bool CGraphicImageTexture::CreateFromScratchImage(const DirectX::ScratchImage& image)
{
	if (!ms_lpd3d11Device)
		return false;

	safe_release(m_lpSRV);

	const DirectX::TexMetadata& metadata = image.GetMetadata();

	const DirectX::ScratchImage* finalImage = &image;
	DirectX::ScratchImage convertedImage;

	if (metadata.format != DXGI_FORMAT_B8G8R8A8_UNORM &&
		metadata.format != DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		HRESULT hr = DirectX::Convert(image.GetImages(), image.GetImageCount(), metadata, DXGI_FORMAT_B8G8R8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, convertedImage);

		if (SUCCEEDED(hr))
			finalImage = &convertedImage;
	}

	const DirectX::TexMetadata& finalMetadata = finalImage->GetMetadata();

	HRESULT hr = DirectX::CreateShaderResourceView(ms_lpd3d11Device, finalImage->GetImages(), finalImage->GetImageCount(), finalMetadata, &m_lpSRV);

	if (FAILED(hr))
		return false;

	m_width = static_cast<int>(finalMetadata.width);
	m_height = static_cast<int>(finalMetadata.height);
	m_bEmpty = false;

	return true;
}

bool CGraphicImageTexture::LoadFromMemory(UINT size, const void* data, ScratchImage& image)
{
	if (!data || size == 0)
		return false;

	TexMetadata metadata;

	HRESULT hr = DirectX::LoadFromDDSMemory(reinterpret_cast<const uint8_t*>(data), static_cast<size_t>(size), DirectX::DDS_FLAGS_NONE, &metadata, image);

	if (SUCCEEDED(hr))
		return true;

	hr = DirectX::LoadFromTGAMemory(reinterpret_cast<const uint8_t*>(data), static_cast<size_t>(size), &metadata, image);

	if (SUCCEEDED(hr))
		return true;

	hr = DirectX::LoadFromWICMemory(reinterpret_cast<const uint8_t*>(data), static_cast<size_t>(size), DirectX::WIC_FLAGS_IGNORE_SRGB, &metadata, image);

	return SUCCEEDED(hr);
}

bool CGraphicImageTexture::LoadFromFile(const wchar_t* fileName, ScratchImage& image)
{
	if (!fileName || !fileName[0])
		return false;

	TexMetadata metadata;

	HRESULT hr = LoadFromDDSFile(fileName, DDS_FLAGS_NONE, &metadata, image);

	if (SUCCEEDED(hr))
		return true;

	hr = LoadFromTGAFile(fileName, &metadata, image);

	if (SUCCEEDED(hr))
		return true;

	hr = LoadFromWICFile(fileName, WIC_FLAGS_IGNORE_SRGB, &metadata, image);

	return SUCCEEDED(hr);
}

bool CGraphicImageTexture::CreateFromMemoryFile(UINT size, const void* data, DXGI_FORMAT format, DWORD filter)
{
	Destroy();

	m_d3dFmt = format;
	m_dwFilter = filter;

	ScratchImage image;

	if (!LoadFromMemory(size, data, image))
		return false;

	return CreateFromScratchImage(image);
}

bool CGraphicImageTexture::CreateFromDiskFile(const char* fileName, DXGI_FORMAT format, DWORD filter)
{
	Destroy();

	SetFileName(fileName);

	m_d3dFmt = format;
	m_dwFilter = filter;

	TPackFile file;

	if (CPackManager::Instance().GetFile(m_fileName.c_str(), file))
		return CreateFromMemoryFile(static_cast<UINT>(file.size()), file.data(), format, filter);

	wchar_t wideFileName[MAX_PATH] = {};

	MultiByteToWideChar(CP_ACP, 0, fileName, -1, wideFileName, MAX_PATH);

	ScratchImage image;

	if (!LoadFromFile(wideFileName, image))
		return false;

	return CreateFromScratchImage(image);
}

bool CGraphicImageTexture::CreateFromRawPixels(UINT width, UINT height, DXGI_FORMAT format, const void* pixels, UINT pitch)
{
	if (!ms_lpd3d11Device || !pixels || width == 0 || height == 0)
		return false;

	safe_release(m_lpSRV);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA subResource = {};
	subResource.pSysMem = pixels;
	subResource.SysMemPitch = pitch;

	ID3D11Texture2D* texture = nullptr;

	HRESULT hr = ms_lpd3d11Device->CreateTexture2D(&desc, &subResource, &texture);

	if (FAILED(hr))
		return false;

	hr = ms_lpd3d11Device->CreateShaderResourceView(texture, nullptr, &m_lpSRV);

	texture->Release();

	if (FAILED(hr))
		return false;

	m_width = static_cast<int>(width);
	m_height = static_cast<int>(height);
	m_bEmpty = false;

	return true;
}

bool CGraphicImageTexture::Create(UINT width, UINT height, DXGI_FORMAT format)
{
	Destroy();

	m_width = static_cast<int>(width);
	m_height = static_cast<int>(height);
	m_d3dFmt = format;

	m_lockPitch = width * 4;
	m_lockBuffer.resize(m_lockPitch * height);
	memset(m_lockBuffer.data(), 0, m_lockBuffer.size());

	return CreateFromRawPixels(width, height, format, m_lockBuffer.data(), m_lockPitch);
}

bool CGraphicImageTexture::CreateDeviceObjects()
{
	if (m_lockBuffer.empty())
		return m_lpSRV != nullptr;

	return CreateFromRawPixels(static_cast<UINT>(m_width), static_cast<UINT>(m_height), m_d3dFmt, m_lockBuffer.data(), m_lockPitch);
}

void CGraphicImageTexture::CreateFromTexturePointer(const CGraphicTexture* sourceTexture)
{
	Destroy();

	if (!sourceTexture)
		return;

	m_width = sourceTexture->GetWidth();
	m_height = sourceTexture->GetHeight();

	m_lpSRV = sourceTexture->GetSRV();

	if (m_lpSRV)
		m_lpSRV->AddRef();

	m_bEmpty = m_lpSRV == nullptr;
}

bool CGraphicImageTexture::Lock(int* pitch, void** pixels, int level)
{
	if (!pitch || !pixels || m_width <= 0 || m_height <= 0)
		return false;

	if (m_lockBuffer.empty())
	{
		m_lockPitch = static_cast<UINT>(m_width) * 4;
		m_lockBuffer.resize(m_lockPitch * static_cast<UINT>(m_height));
		memset(m_lockBuffer.data(), 0, m_lockBuffer.size());
	}

	*pitch = static_cast<int>(m_lockPitch);
	*pixels = m_lockBuffer.data();

	return true;
}

void CGraphicImageTexture::Unlock(int level)
{
	if (m_lockBuffer.empty())
		return;

	CreateFromRawPixels(static_cast<UINT>(m_width), static_cast<UINT>(m_height), m_d3dFmt == DXGI_FORMAT_UNKNOWN ? DXGI_FORMAT_B8G8R8A8_UNORM : m_d3dFmt, m_lockBuffer.data(), m_lockPitch);
}
