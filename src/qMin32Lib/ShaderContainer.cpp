#include "pch.h"
#include "ShaderContainer.h"
#include "DxManager.h"
#include "Shaders.h"

static const std::vector<uint32_t> k_MeshPermutations = []()
	{
		const uint32_t blends[] = {
			SHADER_NONE,        // RenderWithOneTexture rigid
			IS_SKINNED,         // RenderWithOneTexture skinned
			HAS_TEX2,           // RenderWithTwoTexture rigid
			HAS_TEX2 | IS_SKINNED, // RenderWithTwoTexture skinned
			MESH_SPECULAR,
			MESH_SPECULAR | IS_SKINNED,
		};

		std::vector<uint32_t> out(std::begin(blends), std::end(blends));
		return out;
	}();

ShadersContainer::ShadersContainer(DxManager* manager)
	: m_manager(manager)
{
	if (!manager)
		return;

	m_desc[VF_PDT] = { "PDT_Vs",    "PDT_Ps" };
	m_desc[VF_MESH] = { "Mesh_Vs",   "Mesh_Ps" };
	m_desc[VF_PD] = { "PD_Vs",     "PD_Ps" };
	m_desc[VF_SCREEN] = { "SCREEN_Vs", "SCREEN_Ps" };
	m_desc[VF_PT] = { "PT_Vs",     "PT_Ps" };
	m_desc[VF_SKYBOX] = { "skybox_Vs", "skybox_Ps" };
	m_desc[VF_TERRAIN] = { "terrain_Vs", "terrain_Ps" };
	m_desc[VF_EFFECT] = { "effect_Vs", "effect_Ps" };
	m_desc[VF_BRANCH] = { "branch_Vs", "branch_Ps" };
	m_desc[VF_LEAF] = { "leaf_Vs", "leaf_Ps" };

	Precompile(VF_PDT, { SHADER_NONE, BLEND_MODULATE, BLEND_UI_TEX, BLEND_UI_DIFFUSE, BLEND_ADD,
		BLEND_MODULATE2X ,BLEND_SELECTARG2 ,BLEND_STAGE1_MODULATE ,BLEND_TEXFACTOR });
	Precompile(VF_MESH, k_MeshPermutations);
	Precompile(VF_PT, {MINIMAP_MARK,TERRAIN_SPLAT,TERRAIN_BASE });


	Precompile(VF_SKYBOX, { SKY_USE_TEXTURE, SKY_USE_DIFFUSE, SKY_CLOUD });
	Precompile(VF_TERRAIN, { TERRAIN_SPLAT, TERRAIN_SHADOW, TERRAIN_SHADOW_CHR, TERRAIN_MARKED, TERRAIN_BASE });
}

void ShadersContainer::Precompile(ED3D11VertexFormat format, const std::vector<uint32_t>& flagCombinations)
{
	const size_t index = static_cast<size_t>(format);
	if (index >= VF_COUNT)
		return;

	const auto& desc = m_desc[index];
	if (desc.vs.empty() || desc.ps.empty())
		return;

	for (uint32_t flags : flagCombinations)
	{
		const uint64_t key = MakeKey(format, flags);
		if (m_cache.count(key))
			continue;

		ShaderPtr shader;
		m_manager->CreateShader(shader, desc.vs, desc.ps, flags);
		if (shader)
			m_cache[key] = shader;
	}
}

uint64_t ShadersContainer::MakeKey(ED3D11VertexFormat format, uint32_t flags) const
{
	return (static_cast<uint64_t>(format) << 32) | flags;
}

ShaderPtr ShadersContainer::GetShader(ED3D11VertexFormat format, uint32_t flags)
{
	if (!m_manager)
		return nullptr;

	const size_t index = static_cast<size_t>(format);
	if (index >= VF_COUNT)
		return nullptr;

	const uint64_t key = MakeKey(format, flags);

	auto it = m_cache.find(key);
	if (it != m_cache.end())
		return it->second;

	const auto& desc = m_desc[index];
	if (desc.vs.empty() || desc.ps.empty())
		return nullptr;

	ShaderPtr shader;
	m_manager->CreateShader(shader, desc.vs, desc.ps, flags);
	if (!shader)
		return nullptr;

	m_cache[key] = shader;
	return shader;
}
