#pragma once
#include "Core.h"
#include "EterLib/D3DXMathCompat.h"

struct CBPerFrame
{
	D3DXMATRIX matWorld;
	D3DXMATRIX matView;
	D3DXMATRIX matProj;
};

// b1: Material / texture stage state emulation
struct CBMaterial
{
	float textureFactor[4];

	int useTexture0;
	int useTexture1;
	int alphaTestEnable;
	int alphaRef;

	float drawColor[4];
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

// b3: Texture coordinate transform
struct CBTexTransform
{
	D3DXMATRIX matTexTransform0;
	D3DXMATRIX matTexTransform1;
	D3DXMATRIX matTexTransform2;
	D3DXMATRIX matTexTransform3;
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
	D3DXMATRIX matCompound;
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
	void SetWorldMatrix(const D3DXMATRIX& mat);
	void SetViewMatrix(const D3DXMATRIX& mat);
	void SetProjMatrix(const D3DXMATRIX& mat);
	void FlushTransforms();
	// Texture coordinate transform → constant buffer b3
	void SetTexTransform(DWORD dwStage, const D3DXMATRIX& mat);

	// Lighting → constant buffer b2
	void SetLightingEnable(BOOL bEnable);
	void SetLight(DWORD index, const D3DLIGHT11* pLight);
	void SetMaterial(const D3DMATERIAL11* pMaterial);
	void SetAmbient(DWORD dwColor);
	void FlushLighting();

	DWORD GetAlphaRef() const { return DWORD(m_cbMaterial.alphaRef); }

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

	void FlushMaterial();

	void SetScreenSize(float width, float height);

	void SetSpeedTreeCompoundMatrix(const D3DXMATRIX& mat);
	void SetSpeedTreeTreePosition(const D3DXVECTOR4& pos);
	void SetSpeedTreeFog(const float* fog);
	void SetSpeedTreeLight(const float* light);
	void SetSpeedTreeMaterialConstants(const float* material, float leafLightingAdjustment);
	void SetSpeedTreeLeafTables(const float* table, UINT float4Count);
	void FlushSpeedTree();

	//bone mesh
	bool UploadBonePalette(const DirectX::XMFLOAT4X4* bones, unsigned int count);
	void SetSpecularPower(float power, const D3DXCOLOR& color);

	void FlushAllState();
	void SetAllBuffers();


private:
	CBufferPtr				m_pCBPerFrame;
	CBufferPtr				m_pCBMaterial;
	CBufferPtr				m_pCBLighting;
	CBufferPtr				m_pCBTexTransform;
	CBufferPtr				m_pCBFog;
	CBufferPtr				m_pCBScreenSize;
	CBufferPtr				m_pCBBonePalette;
	CBufferPtr				m_pCBSpeedTree;

	CBPerFrame				m_cbPerFrame = {};
	CBMaterial				m_cbMaterial = {};
	CBLighting				m_cbLighting = {};
	CBTexTransform			m_cbTexTransform = {};
	CBFog					m_cbFog = {};
	CBScreenSize			m_cbScreenSize = {};
	CBSpeedTree				m_cbSpeedTree = {};

	bool					m_bTransformDirty = true;
	bool					m_bMaterialDirty = true;
	bool					m_bLightingDirty = true;
	bool					m_bFogDirty = true;
	bool					m_bSpeedTreeDirty = true;
	bool m_bMaterialConstantsDirty = true;
	DxManager* m_manager;

	friend class CD3D11Renderer;
};
