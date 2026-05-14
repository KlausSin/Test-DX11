#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <memory>

#include <wrl/client.h>
#include <EterLib/D3DXMathCompat.h>
using namespace Microsoft::WRL;

class ConstantBuffer;
using CBufferPtr = std::shared_ptr<ConstantBuffer>;

class IndexBuffer;
using IBufferPtr = std::shared_ptr<IndexBuffer>;

class VertexBuffer;
using VBufferPtr = std::shared_ptr<VertexBuffer>;

class CShaders;
using ShaderPtr = std::shared_ptr<CShaders>;

class ShadersContainer;
using ShadersContainerPtr = std::shared_ptr<ShadersContainer>;

class CBManager;
using CBManagerPtr = std::shared_ptr<CBManager>;

class DxManager;
template <typename T> using RefPtr = std::shared_ptr<T>;
template <typename T> using UniquePtr = std::unique_ptr<T>;

// Vertex format enum matching the game's vertex types
enum ED3D11VertexFormat
{
	VF_PDT,		// Position + Diffuse + TexCoord
	VF_MESH,	//Position + Normal + TexCoord / + TexCoord2
	VF_PD,		// Position + Diffuse
	VF_SCREEN,	// Pre-transformed: XYZRHW + Diffuse + Specular + Tex2
	VF_PT,		// Position + TexCoord (effects, snow, minimap)

	VF_SKYBOX,
	VF_TERRAIN,
	VF_EFFECT,

	//speedtree
	VF_BRANCH,
	VF_LEAF,

	VF_COUNT
};

static DWORD ColorToUint(const DirectX::XMFLOAT4& c)
{
	return
		(static_cast<DWORD>(c.w * 255.0f) << 24) |
		(static_cast<DWORD>(c.x * 255.0f) << 16) |
		(static_cast<DWORD>(c.y * 255.0f) << 8) | 
		(static_cast<DWORD>(c.z * 255.0f)); 
}

static XMFLOAT4 UintColor(DWORD c)
{
	return XMFLOAT4(
		((c >> 16) & 0xff) / 255.0f,
		((c >> 8) & 0xff) / 255.0f,
		((c >> 0) & 0xff) / 255.0f,
		((c >> 24) & 0xff) / 255.0f
	);
}

struct D3DMATERIAL11
{
	XMFLOAT4 Diffuse;
	XMFLOAT4 Ambient;
	XMFLOAT4 Specular;
	XMFLOAT4 Emissive;
	float Power;

	D3DMATERIAL11()
		: Diffuse(1.0f, 1.0f, 1.0f, 1.0f)
		, Ambient(1.0f, 1.0f, 1.0f, 1.0f)
		, Specular(0.0f, 0.0f, 0.0f, 1.0f)
		, Emissive(0.0f, 0.0f, 0.0f, 1.0f)
		, Power(0.0f)
	{
	}
};

enum D3DLIGHTTYPE11
{
	D3DLIGHT_POINT11 = 1,
	D3DLIGHT_SPOT11 = 2,
	D3DLIGHT_DIRECTIONAL11 = 3,
};

struct D3DLIGHT11
{
	D3DLIGHTTYPE11 Type;

	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular;
	XMFLOAT4 Ambient;

	XMFLOAT3 Position;
	float Range;

	XMFLOAT3 Direction;
	float Falloff;

	float Attenuation0;
	float Attenuation1;
	float Attenuation2;
	float Theta;

	float Phi;
	float Pad0;
	float Pad1;
	float Pad2;

	D3DLIGHT11()
		: Type(D3DLIGHT_DIRECTIONAL11)
		, Diffuse(1.0f, 1.0f, 1.0f, 1.0f)
		, Specular(1.0f, 1.0f, 1.0f, 1.0f)
		, Ambient(0.0f, 0.0f, 0.0f, 1.0f)
		, Position(0.0f, 0.0f, 0.0f)
		, Range(1000.0f)
		, Direction(0.0f, 0.0f, 1.0f)
		, Falloff(1.0f)
		, Attenuation0(1.0f)
		, Attenuation1(0.0f)
		, Attenuation2(0.0f)
		, Theta(0.0f)
		, Phi(0.0f)
		, Pad0(0.0f)
		, Pad1(0.0f)
		, Pad2(0.0f)
	{
	}
};

struct RenderFrameContext
{
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	XMFLOAT4X4 View{};
	XMFLOAT4X4 Projection{};
	XMFLOAT4X4 ViewProjection{};
	XMFLOAT4X4 ViewInverse{};

	XMFLOAT3 Eye{};
	XMFLOAT3 Target{};

	float DeltaTime = 0.0f;
	float Time = 0.0f;

	bool FogEnable = false;
	uint32_t FogColor = 0xffffffffu;
	float FogStart = 5000.0f;
	float FogEnd = 10000.0f;
	bool DensityFog = false;

	bool DrawShadow = false;
	bool DrawCharacterShadow = false;
	ID3D11ShaderResourceView* CharacterShadowTexture = nullptr;
	XMFLOAT4X4 DynamicShadowMatrix{};

	static RenderFrameContext Default()
	{
		RenderFrameContext ctx;
		XMStoreFloat4x4(&ctx.View, XMMatrixIdentity());
		XMStoreFloat4x4(&ctx.Projection, XMMatrixIdentity());
		XMStoreFloat4x4(&ctx.ViewProjection, XMMatrixIdentity());
		XMStoreFloat4x4(&ctx.ViewInverse, XMMatrixIdentity());
		XMStoreFloat4x4(&ctx.DynamicShadowMatrix, XMMatrixIdentity());
		return ctx;
	}
};

struct RenderObjectContext
{
	XMFLOAT4X4 World{};
	bool AlphaBlend = false;
	bool AlphaTest = true;
	bool TwoSided = false;
	bool Skinned = false;

	static RenderObjectContext Default()
	{
		RenderObjectContext ctx;
		XMStoreFloat4x4(&ctx.World, XMMatrixIdentity());
		return ctx;
	}
};

struct RenderContext
{
	RenderFrameContext Frame;
	RenderObjectContext Object;

	static RenderContext Default()
	{
		RenderContext ctx;
		ctx.Frame = RenderFrameContext::Default();
		ctx.Object = RenderObjectContext::Default();
		return ctx;
	}
};
