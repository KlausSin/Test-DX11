#pragma once
#include "Core.h"
#include "EterLib/D3DXMathCompat.h"

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

// b2: Lighting emulation
struct CBLighting
{
	float lightPosition[4];
	float lightDir[4];
	float lightDiffuse[4];
	float lightAmbient[4];

	float matDiffuse[4];
	float matAmbient[4];
	float matEmissive[4];

	float lightAttenuation[4];
	float lightSpot[4];

	int lightingEnable;
	int pad0;
	int pad1;
	int pad2;
	float specularColor[4];
	float pad[12];
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

	// Lighting → constant buffer b2
	void SetLightingEnable(BOOL bEnable);
	void SetLight(DWORD index, const D3DLIGHT11* pLight);
	void SetMaterial(const D3DMATERIAL11* pMaterial);
	void SetAmbient(DWORD dwColor);
	void FlushLighting();

	DWORD GetAlphaRef() const { return DWORD(m_cbMaterial.alphaRef); }
	bool GetUseTexture0() const { return m_cbMaterial.useTexture0 != 0; }
	bool GetAlphaTestEnable() const { return m_cbMaterial.alphaTestEnable != 0; }
	bool GetFogEnable() const { return m_cbFog.fogEnable != 0; }
	bool GetLightingEnable() const { return m_cbLighting.lightingEnable != 0; }
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
	void SetSpecularPower(float power, const XMFLOAT4& color);

	void FlushAllState();
	void SetAllBuffers();


private:
	CBufferPtr				m_pCBMatrix;
	CBufferPtr				m_pCBMaterial;
	CBufferPtr				m_pCBLighting;
	CBufferPtr				m_pCBFog;
	CBufferPtr				m_pCBScreenSize;
	CBufferPtr				m_pCBBonePalette;
	CBufferPtr				m_pCBSpeedTree;

	CBMatrix				m_cbMatrix = {};
	CBMaterial				m_cbMaterial = {};
	CBLighting				m_cbLighting = {};
	CBFog					m_cbFog = {};
	CBScreenSize			m_cbScreenSize = {};
	CBSpeedTree				m_cbSpeedTree = {};

	bool					m_bMatrixDirty = true;
	bool					m_bMaterialDirty = true;
	bool					m_bLightingDirty = true;
	bool					m_bFogDirty = true;
	bool					m_bSpeedTreeDirty = true;

	DxManager* m_manager;

	friend class CD3D11Renderer;
};
