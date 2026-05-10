
#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <vector>
#include <map>

class CStateManagerDx11
{
public:
	CStateManagerDx11(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	~CStateManagerDx11();

	void SetTexture(UINT stage, ID3D11ShaderResourceView* pSRV);

	void SetSamplerWrap(UINT stage);
	void SetSamplerClamp(UINT stage);
	void SetSamplerFilter(UINT stage, D3D11_FILTER filter);

	void SetVertexBuffer(UINT slot, ID3D11Buffer* pVB, UINT stride, UINT offset = 0);
	void SetIndexBuffer(ID3D11Buffer* pIB, DXGI_FORMAT format = DXGI_FORMAT_R16_UINT, UINT offset = 0);

	void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);

	void SetAlphaBlend(BOOL enable);
	void SetBlendFunc(D3D11_BLEND src, D3D11_BLEND dst);
	void SetBlendOp(D3D11_BLEND_OP op);

	void SetDepthEnable(BOOL enable);
	void SetDepthWrite(BOOL enable);
	void SetDepthFunc(D3D11_COMPARISON_FUNC func);

	void SetCullMode(D3D11_CULL_MODE mode);
	void SetFillMode(D3D11_FILL_MODE mode);
	D3D11_CULL_MODE GetCullMode() const { return m_rasterKey.cullMode; }

	void SetScissorEnable(BOOL enable);
	bool IsAlphaBlendEnabled() const { return m_blendKey.enable; }
	void SetViewport(float width, float height);

	void Draw(UINT vertexCount, UINT startVertex = 0);
	void DrawIndexed(UINT indexCount, UINT startIndex = 0, INT baseVertex = 0);

private:
	struct SSamplerKey
	{
		D3D11_FILTER filter;
		D3D11_TEXTURE_ADDRESS_MODE addressU;
		D3D11_TEXTURE_ADDRESS_MODE addressV;

		bool operator<(const SSamplerKey& o) const
		{
			if (filter != o.filter)
				return filter < o.filter;

			if (addressU != o.addressU)
				return addressU < o.addressU;

			return addressV < o.addressV;
		}
	};

	struct SBlendKey
	{
		BOOL enable;
		D3D11_BLEND src;
		D3D11_BLEND dst;
		D3D11_BLEND_OP op;

		bool operator<(const SBlendKey& o) const
		{
			return memcmp(this, &o, sizeof(*this)) < 0;
		}
	};

	struct SDepthKey
	{
		BOOL enable;
		BOOL write;
		D3D11_COMPARISON_FUNC func;

		bool operator<(const SDepthKey& o) const
		{
			return memcmp(this, &o, sizeof(*this)) < 0;
		}
	};

	struct SRasterKey
	{
		D3D11_FILL_MODE fillMode;
		D3D11_CULL_MODE cullMode;
		BOOL scissor;

		bool operator<(const SRasterKey& o) const
		{
			return memcmp(this, &o, sizeof(*this)) < 0;
		}
	};

private:
	ID3D11SamplerState* GetOrCreateSampler(const SSamplerKey& key);
	ID3D11BlendState* GetOrCreateBlend(const SBlendKey& key);
	ID3D11DepthStencilState* GetOrCreateDepth(const SDepthKey& key);
	ID3D11RasterizerState* GetOrCreateRaster(const SRasterKey& key);

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pContext;

	ID3D11ShaderResourceView* m_pTextures[8];

	ID3D11SamplerState* m_pSamplers[8];
	D3D11_FILTER m_samplerFilters[8];

	ID3D11Buffer* m_pVB[8];
	UINT m_vertexStride[8];
	UINT m_vertexOffset[8];

	ID3D11Buffer* m_pIB;
	DXGI_FORMAT m_ibFormat;
	UINT m_ibOffset;

	D3D11_PRIMITIVE_TOPOLOGY m_topology;

	SBlendKey m_blendKey;
	SDepthKey m_depthKey;
	SRasterKey m_rasterKey;

	std::map<SSamplerKey, ID3D11SamplerState*> m_samplerCache;
	std::map<SBlendKey, ID3D11BlendState*> m_blendCache;
	std::map<SDepthKey, ID3D11DepthStencilState*> m_depthCache;
	std::map<SRasterKey, ID3D11RasterizerState*> m_rasterCache;
};
