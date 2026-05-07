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
	VF_PN,		// Position + Normal only (terrain HTP)
	VF_SCREEN,	// Pre-transformed: XYZRHW + Diffuse + Specular + Tex2
	VF_PT,		// Position + TexCoord (effects, snow, minimap)
	VF_PDT2,	// Position + Diffuse + TexCoord + TexCoord2 (SpeedTree branches/fronds)
	
	VF_SKYBOX,
	VF_TERRAIN,
	VF_EFFECT,

	//speedtree
	VF_BRANCH,
	VF_LEAF,

	VF_COUNT
};

struct D3DMATERIAL11
{
	D3DXCOLOR Diffuse;
	D3DXCOLOR Ambient;
	D3DXCOLOR Specular;
	D3DXCOLOR Emissive;
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

	D3DXCOLOR Diffuse;
	D3DXCOLOR Specular;
	D3DXCOLOR Ambient;

	D3DXVECTOR3 Position;
	float Range;

	D3DXVECTOR3 Direction;
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
