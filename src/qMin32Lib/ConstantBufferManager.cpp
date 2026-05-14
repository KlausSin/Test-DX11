#include "pch.h"
#include "DxManager.h"
#include "ConstantBuffer.h"
#include "ConstantBufferManager.h"

CBManager::CBManager(DxManager* manager) : m_manager(manager)
{
	manager->CreateConstantBuffer(m_pCBMatrix, sizeof(CBMatrix));
	manager->CreateConstantBuffer(m_pCBMaterial, sizeof(CBMaterial));
	manager->CreateConstantBuffer(m_pCBLighting, sizeof(CBLighting));
	manager->CreateConstantBuffer(m_pCBFog, sizeof(CBFog));
	manager->CreateConstantBuffer(m_pCBScreenSize, sizeof(CBScreenSize));
	manager->CreateConstantBuffer(m_pCBBonePalette, sizeof(SGrannyBonePalette)); // b6
	manager->CreateConstantBuffer(m_pCBSpeedTree, sizeof(CBSpeedTree)); // b7
}

void CBManager::SetWorldMatrix(const XMFLOAT4X4& mat)
{
	m_cbMatrix.frame.matWorld = mat;
	m_bMatrixDirty = true;
}

void CBManager::SetViewMatrix(const XMFLOAT4X4& mat)
{
	m_cbMatrix.frame.matView = mat;
	m_bMatrixDirty = true;
}

void CBManager::SetProjMatrix(const XMFLOAT4X4& mat)
{
	m_cbMatrix.frame.matProj = mat;
	m_bMatrixDirty = true;
}

void CBManager::SetTexTransform(DWORD dwStage, const XMFLOAT4X4& mat)
{
	if (dwStage == 0)
		m_cbMatrix.texTransform.tex0 = mat;
	else if (dwStage == 1)
		m_cbMatrix.texTransform.tex1 = mat;
	else if (dwStage == 2)
		m_cbMatrix.texTransform.tex2 = mat;
	else if (dwStage == 3)
		m_cbMatrix.texTransform.tex3 = mat;
	else
		return;

	m_bMatrixDirty = true;
}

void CBManager::FlushMatrix()
{
	if (!m_bMatrixDirty)
		return;

	m_pCBMatrix->Update(m_cbMatrix);
	m_bMatrixDirty = false;
}

void CBManager::SetLightingEnable(BOOL bEnable)
{
	m_cbLighting.lightingEnable = bEnable ? 1 : 0;
	m_bLightingDirty = true;
}

void CBManager::SetLight(DWORD index, const D3DLIGHT11* pLight)
{
	if (index > 0 || !pLight)
		return;

	D3DLIGHT11 light = *pLight;

	if (light.Type != D3DLIGHT_POINT11 &&
		light.Type != D3DLIGHT_SPOT11 &&
		light.Type != D3DLIGHT_DIRECTIONAL11)
		light.Type = D3DLIGHT_DIRECTIONAL11;

	if (light.Range <= 0.0f)
		light.Range = 100000.0f;

	if (light.Attenuation0 == 0.0f &&
		light.Attenuation1 == 0.0f &&
		light.Attenuation2 == 0.0f)
		light.Attenuation0 = 1.0f;

	m_cbLighting.lightPosition[0] = light.Position.x;
	m_cbLighting.lightPosition[1] = light.Position.y;
	m_cbLighting.lightPosition[2] = light.Position.z;
	m_cbLighting.lightPosition[3] = 1.0f;

	m_cbLighting.lightDir[0] = light.Direction.x;
	m_cbLighting.lightDir[1] = light.Direction.y;
	m_cbLighting.lightDir[2] = light.Direction.z;
	m_cbLighting.lightDir[3] = 0.0f;

	m_cbLighting.lightDiffuse[0] = light.Diffuse.x;
	m_cbLighting.lightDiffuse[1] = light.Diffuse.y;
	m_cbLighting.lightDiffuse[2] = light.Diffuse.z;
	m_cbLighting.lightDiffuse[3] = light.Diffuse.w;

	m_cbLighting.lightAmbient[0] = light.Ambient.x;
	m_cbLighting.lightAmbient[1] = light.Ambient.y;
	m_cbLighting.lightAmbient[2] = light.Ambient.z;
	m_cbLighting.lightAmbient[3] = light.Ambient.w;

	m_cbLighting.lightAttenuation[0] = light.Attenuation0;
	m_cbLighting.lightAttenuation[1] = light.Attenuation1;
	m_cbLighting.lightAttenuation[2] = light.Attenuation2;
	m_cbLighting.lightAttenuation[3] = light.Range;

	m_cbLighting.lightSpot[0] = light.Theta;
	m_cbLighting.lightSpot[1] = light.Phi;
	m_cbLighting.lightSpot[2] = light.Falloff <= 0.0f ? 1.0f : light.Falloff;
	m_cbLighting.lightSpot[3] = float(light.Type);

	m_bLightingDirty = true;
}

void CBManager::SetMaterial(const D3DMATERIAL11* pMaterial)
{
	m_cbLighting.matDiffuse[0] = pMaterial->Diffuse.x;
	m_cbLighting.matDiffuse[1] = pMaterial->Diffuse.y;
	m_cbLighting.matDiffuse[2] = pMaterial->Diffuse.z;
	m_cbLighting.matDiffuse[3] = pMaterial->Diffuse.w;

	m_cbLighting.matAmbient[0] = pMaterial->Ambient.x;
	m_cbLighting.matAmbient[1] = pMaterial->Ambient.y;
	m_cbLighting.matAmbient[2] = pMaterial->Ambient.z;
	m_cbLighting.matAmbient[3] = pMaterial->Ambient.w;

	m_cbLighting.matEmissive[0] = pMaterial->Emissive.x;
	m_cbLighting.matEmissive[1] = pMaterial->Emissive.y;
	m_cbLighting.matEmissive[2] = pMaterial->Emissive.z;
	m_cbLighting.matEmissive[3] = pMaterial->Emissive.w;

	m_bLightingDirty = true;
}

void CBManager::SetAmbient(DWORD dwColor)
{
	m_cbLighting.lightAmbient[0] = ((dwColor >> 16) & 0xFF) / 255.0f;
	m_cbLighting.lightAmbient[1] = ((dwColor >> 8) & 0xFF) / 255.0f;
	m_cbLighting.lightAmbient[2] = (dwColor & 0xFF) / 255.0f;
	m_cbLighting.lightAmbient[3] = ((dwColor >> 24) & 0xFF) / 255.0f;
	m_bLightingDirty = true;
}

void CBManager::FlushLighting()
{
	if (!m_bLightingDirty) return;

	m_pCBLighting->Update(m_cbLighting);

	m_bLightingDirty = false;
}

void CBManager::SetFogEnable(BOOL bEnable)
{
	m_cbFog.fogEnable = bEnable ? 1 : 0;
	m_bFogDirty = true;
}
void CBManager::SetFogColor(DWORD dwColor)
{
	m_cbFog.fogColor[0] = ((dwColor >> 16) & 0xFF) / 255.0f;	// R
	m_cbFog.fogColor[1] = ((dwColor >> 8) & 0xFF) / 255.0f;	// G
	m_cbFog.fogColor[2] = (dwColor & 0xFF) / 255.0f;	// B
	m_cbFog.fogColor[3] = 1.0f;
	m_bFogDirty = true;
}
void CBManager::SetFogStart(float fStart)
{
	m_cbFog.fogStart = fStart;
	m_bFogDirty = true;
}
void CBManager::SetFogEnd(float fEnd)
{
	m_cbFog.fogEnd = fEnd;
	m_bFogDirty = true;
}

void CBManager::FlushFog()
{
	if (!m_bFogDirty) return;
	m_pCBFog->Update(m_cbFog);

	m_bFogDirty = false;
}

void CBManager::SetTextureFactor(DWORD dwFactor)
{
	// D3D9 DWORD color format is 0xAARRGGBB
	m_cbMaterial.textureFactor[0] = ((dwFactor >> 16) & 0xFF) / 255.0f;	// R
	m_cbMaterial.textureFactor[1] = ((dwFactor >> 8) & 0xFF) / 255.0f;	// G
	m_cbMaterial.textureFactor[2] = (dwFactor & 0xFF) / 255.0f;	// B
	m_cbMaterial.textureFactor[3] = ((dwFactor >> 24) & 0xFF) / 255.0f;	// A
	m_bMaterialDirty = true;
}

void CBManager::SetAlphaTestEnable(BOOL bEnable)
{
	m_cbMaterial.alphaTestEnable = bEnable ? 1 : 0;
	m_bMaterialDirty = true;
}

void CBManager::SetAlphaRef(DWORD dwRef)
{
	m_cbMaterial.alphaRef = (int)(dwRef & 0xFF);
	m_bMaterialDirty = true;
}

void CBManager::FlushMaterial()
{
	if (!m_bMaterialDirty) return;

	m_pCBMaterial->Update(m_cbMaterial);

	m_bMaterialDirty = false;
}

void CBManager::SetScreenSize(float width, float height)
{
	m_cbScreenSize.screenWidth = width;
	m_cbScreenSize.screenHeight = height;

	m_pCBScreenSize->Update(m_cbScreenSize);
}

bool CBManager::UploadBonePalette(const DirectX::XMFLOAT4X4* bones, unsigned int count)
{
	if (!bones || !m_pCBBonePalette)
		return false;

	if (count > GRANNY_DX11_MAX_BONES)
		count = GRANNY_DX11_MAX_BONES;

	SGrannyBonePalette palette = {};
	memcpy(palette.Bone, bones, sizeof(DirectX::XMFLOAT4X4) * count);

	if (!m_pCBBonePalette->Update(palette))
		return false;

	return true;
}

void CBManager::SetSpecularPower(float power, const XMFLOAT4& color)
{
	m_cbLighting.specularColor[0] = color.x;
	m_cbLighting.specularColor[1] = color.y;
	m_cbLighting.specularColor[2] = color.z;
	m_cbLighting.specularColor[3] = power;
	m_bLightingDirty = true;
}

void CBManager::FlushAllState()
{
	FlushMatrix();
	FlushMaterial();
	FlushLighting();
	FlushFog();
	FlushSpeedTree();
}

void CBManager::SetAllBuffers()
{
	m_manager->SetConstantBuffer(m_pCBMatrix, 0);
	m_manager->SetConstantBuffer(m_pCBMaterial, 1);
	m_manager->SetConstantBuffer(m_pCBLighting, 2);
	m_manager->SetConstantBuffer(m_pCBFog, 4);
	m_manager->SetConstantBuffer(m_pCBScreenSize, 5);
	m_manager->SetConstantBuffer(m_pCBBonePalette, 6);
	m_manager->SetConstantBuffer(m_pCBSpeedTree, 7);
}

void CBManager::SetSpeedTreeCompoundMatrix(const XMFLOAT4X4& mat)
{
	m_cbSpeedTree.matCompound = mat;
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeTreePosition(const XMFLOAT4& pos)
{
	m_cbSpeedTree.treePos[0] = pos.x;
	m_cbSpeedTree.treePos[1] = pos.y;
	m_cbSpeedTree.treePos[2] = pos.z;
	m_cbSpeedTree.treePos[3] = pos.w;
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeFog(const float* fog)
{
	if (!fog)
		return;
	memcpy(m_cbSpeedTree.fog, fog, sizeof(float) * 4);
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeLight(const float* light)
{
	if (!light)
		return;
	memcpy(m_cbSpeedTree.lightDir, light + 0, sizeof(float) * 4);
	memcpy(m_cbSpeedTree.lightAmbient, light + 4, sizeof(float) * 4);
	memcpy(m_cbSpeedTree.lightDiffuse, light + 8, sizeof(float) * 4);
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeMaterialConstants(const float* material, float leafLightingAdjustment)
{
	if (!material)
		return;
	m_cbSpeedTree.materialDiffuse[0] = material[0];
	m_cbSpeedTree.materialDiffuse[1] = material[1];
	m_cbSpeedTree.materialDiffuse[2] = material[2];
	m_cbSpeedTree.materialDiffuse[3] = 1.0f;
	m_cbSpeedTree.materialAmbient[0] = material[3];
	m_cbSpeedTree.materialAmbient[1] = material[4];
	m_cbSpeedTree.materialAmbient[2] = material[5];
	m_cbSpeedTree.materialAmbient[3] = 1.0f;
	m_cbSpeedTree.leafLightingAdjustment[0] = leafLightingAdjustment;
	m_cbSpeedTree.leafLightingAdjustment[1] = 0.0f;
	m_cbSpeedTree.leafLightingAdjustment[2] = 0.0f;
	m_cbSpeedTree.leafLightingAdjustment[3] = 0.0f;
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeLeafTables(const float* table, UINT float4Count)
{
	if (!table || float4Count == 0)
		return;
	if (float4Count > SPEEDTREE_MAX_LEAF_TABLE_FLOAT4)
		float4Count = SPEEDTREE_MAX_LEAF_TABLE_FLOAT4;
	memcpy(m_cbSpeedTree.leafTable, table, float4Count * sizeof(float) * 4);
	m_bSpeedTreeDirty = true;
}

void CBManager::FlushSpeedTree()
{
	if (!m_bSpeedTreeDirty)
		return;
	m_pCBSpeedTree->Update(m_cbSpeedTree);
	m_bSpeedTreeDirty = false;
}

void CBManager::SetUseTexture0(bool enable)
{
	m_cbMaterial.useTexture0 = enable ? 1 : 0;
	m_bMaterialDirty = true;
}
void CBManager::SetUseTexture1(bool enable)
{
	m_cbMaterial.useTexture1 = enable ? 1 : 0;
	m_bMaterialDirty = true;
}
