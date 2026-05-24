#pragma once
#include "Core.h"
#include "EterLib/D3DXMathCompat.h"
#include "entity/LightComponent.h"

#ifndef MAX_ENTITY_LIGHTS
#define MAX_ENTITY_LIGHTS 32
#endif

struct CBEntityLight
{
	XMFLOAT4 position;
	XMFLOAT4 direction;
	XMFLOAT4 diffuse;
	XMFLOAT4 ambient;
	XMFLOAT4 attenuation;
	XMFLOAT4 spot;
	XMINT4 params;
};

struct CBMaterialX
{
	XMFLOAT4 diffuse;
	XMFLOAT4 ambient;
	XMFLOAT4 emissive;
	XMFLOAT4 specular;
	XMFLOAT4 params;
};

struct CBEntityLighting
{
	CBEntityLight lights[MAX_ENTITY_LIGHTS];
	CBMaterialX material;
	XMFLOAT4 globalAmbient;
	XMINT4 settings;
};

struct CBPerFrame
{
	XMFLOAT4X4 matWorld;
	XMFLOAT4X4 matView;
	XMFLOAT4X4 matProj;
};

struct CBTexTransform
{
	XMFLOAT4X4 tex0;
	XMFLOAT4X4 tex1;
	XMFLOAT4X4 tex2;
	XMFLOAT4X4 tex3;
};

struct CBMatrix
{
	CBPerFrame frame;
	CBTexTransform texTransform;
};

// b1: Material / texture stage state emulation
struct CBMaterial
{
	float textureFactor[4];

	int useTexture0;
	int useTexture1;
	int alphaTestEnable;
	int alphaRef;
};

// b4: Fog
struct CBFog
{
	float fogColor[4];
	float fogStart;
	float fogEnd;
	int fogEnable;
	int fogPad;
};

// b5: Screen dimensions (for XYZRHW conversion)
struct CBScreenSize
{
	float screenWidth;
	float screenHeight;
	float pad0;
	float pad1;
};

#ifndef SPEEDTREE_MAX_LEAF_TABLE_FLOAT4
#define SPEEDTREE_MAX_LEAF_TABLE_FLOAT4 1024
#endif

struct CBSpeedTree
{
	XMFLOAT4X4 matCompound;
	float treePos[4];
	float fog[4];
	float lightDir[4];
	float lightAmbient[4];
	float lightDiffuse[4];
	float materialDiffuse[4];
	float materialAmbient[4];
	float leafLightingAdjustment[4];
	float leafTable[SPEEDTREE_MAX_LEAF_TABLE_FLOAT4][4];
};

#ifndef GRANNY_DX11_MAX_BONES
#define GRANNY_DX11_MAX_BONES 256
#endif

struct SGrannyBonePalette
{
	DirectX::XMFLOAT4X4 Bone[GRANNY_DX11_MAX_BONES];
};

struct TerrainLayerCB
{
	XMFLOAT4 brush;
	XMFLOAT4 flags;
	XMFLOAT4 eye;
	XMFLOAT4 layerTiling[8];
	XMFLOAT4 layerStrength[8];
	XMFLOAT4 layerEnabled[8];
};


class CBManager
{
public:
	CBManager(DxManager* manager);

	// Transforms → constant buffer b0
	void SetWorldMatrix(const XMFLOAT4X4& mat);
	void SetViewMatrix(const XMFLOAT4X4& mat);
	void SetProjMatrix(const XMFLOAT4X4& mat);
	void FlushMatrix();
	// Texture coordinate transform → constant buffer b3
	void SetTexTransform(DWORD dwStage, const XMFLOAT4X4& mat);

	DWORD GetAlphaRef() const { return DWORD(m_cbMaterial.alphaRef); }
	bool GetUseTexture0() const { return m_cbMaterial.useTexture0 != 0; }
	bool GetAlphaTestEnable() const { return m_cbMaterial.alphaTestEnable != 0; }
	bool GetFogEnable() const { return m_cbFog.fogEnable != 0; }

	// Fog → constant buffer b4
	void SetFogEnable(BOOL bEnable);
	void SetFogColor(DWORD dwColor);
	void SetFogStart(float fStart);
	void SetFogEnd(float fEnd);
	void FlushFog();

	// Texture stage states → constant buffer b1
	void SetTextureFactor(DWORD dwFactor);
	void SetAlphaTestEnable(BOOL bEnable);
	void SetAlphaRef(DWORD dwRef);
	void SetUseTexture0(bool enable);
	void SetUseTexture1(bool enable);
	void FlushMaterial();

	void SetScreenSize(float width, float height);
	void SetScreenSize(float x, float y, float width, float height);

	void SetSpeedTreeCompoundMatrix(const XMFLOAT4X4& mat);
	void SetSpeedTreeTreePosition(const XMFLOAT4& pos);
	void SetSpeedTreeFog(const float* fog);
	void SetSpeedTreeLight(const float* light);
	void SetSpeedTreeMaterialConstants(const float* material, float leafLightingAdjustment);
	void SetSpeedTreeLeafTables(const float* table, UINT float4Count);
	void FlushSpeedTree();

	//bone mesh
	bool UploadBonePalette(const DirectX::XMFLOAT4X4* bones, unsigned int count);

	void FlushAllState();
	void SetAllBuffers();
	void SetTerrain(const TerrainLayerCB& terrain );
	void					FlushTerrain();

private:
	CBufferPtr				m_pCBMatrix;
	CBufferPtr				m_pCBMaterial;

	CBufferPtr				m_pCBFog;
	CBufferPtr				m_pCBScreenSize;
	CBufferPtr				m_pCBBonePalette;
	CBufferPtr				m_pCBSpeedTree;

	CBMatrix				m_cbMatrix = {};
	CBMaterial				m_cbMaterial = {};

	CBFog					m_cbFog = {};
	CBScreenSize			m_cbScreenSize = {};
	CBSpeedTree				m_cbSpeedTree = {};

	CBufferPtr				m_pCBTerrain;
	TerrainLayerCB			m_cbTerrain = {};
	bool					m_bTerrainDirty = true;


	bool					m_bMatrixDirty = true;
	bool					m_bMaterialDirty = true;
	bool					m_bFogDirty = true;
	bool					m_bSpeedTreeDirty = true;

	DxManager* m_manager;

	friend class CD3D11Renderer;

public:
	void SetEntityLightingEnable(BOOL bEnable);
	void ClearEntityLights();
	void SetEntityGlobalAmbient(DWORD color);
	void SetEntityLight(uint32_t index, const CLightComponent& light);
	void FlushEntityLighting();

	void SetMaterialDiffuse(const XMFLOAT4& color);
	void SetMaterialAmbient(const XMFLOAT4& color);
	void SetMaterialEmissive(const XMFLOAT4& color);
	void SetMaterialSpecular(const XMFLOAT4& color);
	void SetMaterial(const CBMaterialX& material);
	CLightComponent& CreateEntityLight();
	CLightComponent* GetEntityLight(uint32_t index);
	void UploadEntityLights();

private:
	CBufferPtr m_pCBEntityLighting;
	CBEntityLighting m_cbEntityLighting = {};
	bool m_bEntityLightingDirty = true;
	std::vector<std::unique_ptr<CLightComponent>> m_entityLights;
};
