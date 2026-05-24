#include "pch.h"
#include "DxManager.h"
#include "ConstantBuffer.h"
#include "ConstantBufferManager.h"

CBManager::CBManager(DxManager* manager) : m_manager(manager)
{
	manager->CreateConstantBuffer(m_pCBMatrix, sizeof(CBMatrix));
	manager->CreateConstantBuffer(m_pCBMaterial, sizeof(CBMaterial));
	manager->CreateConstantBuffer(m_pCBFog, sizeof(CBFog));
	manager->CreateConstantBuffer(m_pCBScreenSize, sizeof(CBScreenSize));
	manager->CreateConstantBuffer(m_pCBBonePalette, sizeof(SGrannyBonePalette)); // b6
	manager->CreateConstantBuffer(m_pCBSpeedTree, sizeof(CBSpeedTree)); // b7
	manager->CreateConstantBuffer(m_pCBTerrain, sizeof(TerrainLayerCB)); // b8
	manager->CreateConstantBuffer(m_pCBEntityLighting, sizeof(CBEntityLighting));
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
void CBManager::SetScreenSize(float x, float y, float width, float height)
{
	m_cbScreenSize.screenWidth = x;
	m_cbScreenSize.screenHeight = y;
	m_cbScreenSize.pad0 = width;
	m_cbScreenSize.pad1 = height;

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

void CBManager::FlushAllState()
{
	FlushMatrix();
	FlushMaterial();
	FlushFog();
	FlushSpeedTree();
	FlushTerrain();
	FlushEntityLighting();
}

void CBManager::SetAllBuffers()
{
	m_manager->SetConstantBuffer(m_pCBMatrix, 0);
	m_manager->SetConstantBuffer(m_pCBMaterial, 1);
	m_manager->SetConstantBuffer(m_pCBFog, 4);
	m_manager->SetConstantBuffer(m_pCBScreenSize, 5);
	m_manager->SetConstantBuffer(m_pCBBonePalette, 6);
	m_manager->SetConstantBuffer(m_pCBSpeedTree, 7);
	m_manager->SetConstantBuffer(m_pCBTerrain, 8);
	m_manager->SetConstantBuffer(m_pCBEntityLighting, 9);
}

void CBManager::SetTerrain(const TerrainLayerCB& terrain)
{
	m_cbTerrain = terrain;
	m_bTerrainDirty = true;
}

void CBManager::FlushTerrain()
{
	if (!m_bTerrainDirty)
		return;
	m_pCBTerrain->Update(m_cbTerrain);
	m_bTerrainDirty = false;
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


void CBManager::SetEntityLightingEnable(BOOL bEnable)
{
	m_cbEntityLighting.settings.y = bEnable ? 1 : 0;
	m_bEntityLightingDirty = true;
}

void CBManager::ClearEntityLights()
{
	ZeroMemory(&m_cbEntityLighting, sizeof(m_cbEntityLighting));
	m_cbEntityLighting.globalAmbient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_cbEntityLighting.settings.x = 0;
	m_cbEntityLighting.settings.y = 1;
	m_bEntityLightingDirty = true;
}

void CBManager::SetEntityGlobalAmbient(DWORD color)
{
	m_cbEntityLighting.globalAmbient.x = ((color >> 16) & 0xFF) / 255.0f;
	m_cbEntityLighting.globalAmbient.y = ((color >> 8) & 0xFF) / 255.0f;
	m_cbEntityLighting.globalAmbient.z = (color & 0xFF) / 255.0f;
	m_cbEntityLighting.globalAmbient.w = ((color >> 24) & 0xFF) / 255.0f;
	m_bEntityLightingDirty = true;
}

void CBManager::SetEntityLight(uint32_t index, const CLightComponent& light)
{
	if (index >= MAX_ENTITY_LIGHTS)
		return;

	CBEntityLight& dst = m_cbEntityLighting.lights[index];

	const XMFLOAT3& pos = light.GetPosition();
	const XMFLOAT3& dir = light.GetDirection();

	const float range = light.GetRange() <= 0.0f ? 100000.0f : light.GetRange();
	const float falloff = light.GetFalloff() <= 0.0f ? 1.0f : light.GetFalloff();

	float att0 = light.GetAttenuation0();
	float att1 = light.GetAttenuation1();
	float att2 = light.GetAttenuation2();

	if (att0 == 0.0f && att1 == 0.0f && att2 == 0.0f)
		att0 = 1.0f;

	dst.position = XMFLOAT4(pos.x, pos.y, pos.z, 1.0f);
	dst.direction = XMFLOAT4(dir.x, dir.y, dir.z, 0.0f);
	dst.diffuse = light.GetDiffuse();
	dst.ambient = light.GetAmbient();
	dst.attenuation = XMFLOAT4(att0, att1, att2, range);
	dst.spot = XMFLOAT4(light.GetTheta(), light.GetPhi(), falloff, 0.0f);
	dst.params = XMINT4((int)light.GetType(), light.IsEnable() ? 1 : 0, 0, 0);

	if ((int)index + 1 > m_cbEntityLighting.settings.x)
		m_cbEntityLighting.settings.x = (int)index + 1;

	m_bEntityLightingDirty = true;
}

void CBManager::FlushEntityLighting()
{
	if (!m_bEntityLightingDirty)
		return;

	m_pCBEntityLighting->Update(m_cbEntityLighting);
	m_bEntityLightingDirty = false;
}

void CBManager::SetMaterialDiffuse(const XMFLOAT4& color)
{
	m_cbEntityLighting.material.diffuse = color;
	m_bEntityLightingDirty = true;
}

void CBManager::SetMaterialAmbient(const XMFLOAT4& color)
{
	m_cbEntityLighting.material.ambient = color;
	m_bEntityLightingDirty = true;
}

void CBManager::SetMaterialEmissive(const XMFLOAT4& color)
{
	m_cbEntityLighting.material.emissive = color;
	m_bEntityLightingDirty = true;
}

void CBManager::SetMaterialSpecular(const XMFLOAT4& color)
{
	m_cbEntityLighting.material.specular = color;
	m_bEntityLightingDirty = true;
}

void CBManager::SetMaterial(const CBMaterialX& material)
{
	m_cbEntityLighting.material = material;
	m_bEntityLightingDirty = true;
}

CLightComponent& CBManager::CreateEntityLight()
{
	auto light = std::make_unique<CLightComponent>();
	light->Create(nullptr);

	CLightComponent& ref = *light;
	m_entityLights.emplace_back(std::move(light));
	return ref;
}

CLightComponent* CBManager::GetEntityLight(uint32_t index)
{
	if (index >= m_entityLights.size())
		return nullptr;

	return m_entityLights[index].get();
}

void CBManager::UploadEntityLights()
{
	ZeroMemory(&m_cbEntityLighting, sizeof(m_cbEntityLighting));
	m_cbEntityLighting.globalAmbient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_cbEntityLighting.settings.y = 1;

	uint32_t index = 0;

	for (const std::unique_ptr<CLightComponent>& light : m_entityLights)
	{
		if (!light || !light->IsEnable())
			continue;

		SetEntityLight(index, *light);

		if (++index >= MAX_ENTITY_LIGHTS)
			break;
	}

	m_cbEntityLighting.settings.x = index;
}
