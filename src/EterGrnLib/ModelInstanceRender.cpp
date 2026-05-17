#include "StdAfx.h"
#include "Eterlib/StateManager.h"
#include "ModelInstance.h"
#include "Model.h"
#include "qMin32Lib/All.h"
#include <qMin32Lib/ConstantBufferManager.h>
#include <algorithm>

#ifdef _TEST

#include "Eterlib/GrpScreen.h"

void Granny_RenderBoxBones(const granny_skeleton* pkGrnSkeleton, const granny_world_pose* pkGrnWorldPose, const D3DXMATRIX& matBase)
{
	D3DXMATRIX matWorld;
	CScreen screen;

	for (int iBone = 0; iBone != pkGrnSkeleton->BoneCount; ++iBone)
	{
		const granny_bone& rkGrnBone = pkGrnSkeleton->Bones[iBone];
		const D3DXMATRIX* c_matBone = reinterpret_cast<const D3DXMATRIX*>(GrannyGetWorldPose4x4(pkGrnWorldPose, iBone));

		D3DXMatrixMultiply(&matWorld, c_matBone, &matBase);

		STATEMANAGER.SetTransform(World, &matWorld);
		screen.RenderBox3d(-5.0f, -5.0f, -5.0f, 5.0f, 5.0f, 5.0f);
	}
}

#endif

void CGrannyModelInstance::DeformNoSkin(const XMFLOAT4X4* c_pWorldMatrix)
{
	if (IsEmpty())
		return;

	UpdateWorldPose();
	UpdateWorldMatrices(c_pWorldMatrix);
}

namespace
{
	void ApplyRenderFrameContextToMeshCB(const RenderContext& ctx)
	{
		if (!ctx.Frame.DeviceContext)
			return;

		auto cb = _mgr->GetCbMgr();
		cb->SetFogEnable(ctx.Frame.FogEnable);
		cb->SetFogColor(ctx.Frame.FogColor);
		cb->SetFogStart(ctx.Frame.FogStart);
		cb->SetFogEnd(ctx.Frame.FogEnd);
	}

	void ApplyRenderObjectContextToState(const RenderContext& ctx)
	{
		auto cb = _mgr->GetCbMgr();
		cb->SetAlphaTestEnable(ctx.Object.AlphaTest);
	}
}

void CGrannyModelInstance::RenderWithOneTexture(const RenderContext& ctx)
{
	if (IsEmpty())
		return;

	ApplyRenderFrameContextToMeshCB(ctx);

#ifdef _TEST
	Granny_RenderBoxBones(GrannyGetSourceSkeleton(m_pgrnModelInstance), m_pgrnWorldPose, TEST_matWorld);
	if (GetAsyncKeyState('P'))
		Tracef("render %x", m_pgrnModelInstance);
	return;
#endif

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeList(ctx, TextureMode::One, CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNTVertex));
		RenderMeshNodeList(ctx, TextureMode::One, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
}

void CGrannyModelInstance::BlendRenderWithOneTexture(const RenderContext& ctx)
{
	if (IsEmpty())
		return;

	ApplyRenderFrameContextToMeshCB(ctx);

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeList(ctx, TextureMode::One, CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNTVertex));
		RenderMeshNodeList(ctx, TextureMode::One, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

void CGrannyModelInstance::RenderWithTwoTexture(const RenderContext& ctx)
{
	if (IsEmpty())
		return;

	ApplyRenderFrameContextToMeshCB(ctx);

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, HAS_TEX2 | IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeList(ctx, TextureMode::Two, CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH, HAS_TEX2);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNT2Vertex));
		RenderMeshNodeList(ctx, TextureMode::Two, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
}

void CGrannyModelInstance::BlendRenderWithTwoTexture(const RenderContext& ctx)
{
	if (IsEmpty())
		return;

	ApplyRenderFrameContextToMeshCB(ctx);

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();


	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, HAS_TEX2 | IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeList(ctx, TextureMode::Two, CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH, HAS_TEX2);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNT2Vertex));
		RenderMeshNodeList(ctx, TextureMode::Two, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

void CGrannyModelInstance::RenderWithoutTexture(const RenderContext& ctx)
{
	if (IsEmpty())
		return;

	ApplyRenderFrameContextToMeshCB(ctx);

	STATEMANAGER.SetTexture(0, nullptr);
	STATEMANAGER.SetTexture(1, nullptr);

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeList(ctx, TextureMode::None, CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
		RenderMeshNodeList(ctx, TextureMode::None, CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (rigidVB)
	{
		if (m_pModel->m_hasPNT2) // dacă ai funcție de genul
		{
			_mgr->SetShader(VF_MESH, HAS_TEX2);
			_mgr->SetVertexBuffer(rigidVB, sizeof(TPNT2Vertex));
		}
		else
		{
			_mgr->SetShader(VF_MESH);
			_mgr->SetVertexBuffer(rigidVB, sizeof(TPNTVertex));
		}

		RenderMeshNodeList(ctx, TextureMode::None, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
		RenderMeshNodeList(ctx, TextureMode::None, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNTVertex));
		RenderMeshNodeList(ctx, TextureMode::None, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
		RenderMeshNodeList(ctx, TextureMode::None, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

bool CGrannyModelInstance::UploadMeshBonePaletteToShader(int iMesh)
{
	if (!m_pModel || !__GetWorldPosePtr())
		return false;

	if (iMesh < 0 || iMesh >= static_cast<int>(m_vct_pgrnMeshBinding.size()))
		return false;

	int* boneIndices = __GetMeshBoneIndices(static_cast<unsigned int>(iMesh));
	if (!boneIndices)
		return false;

	const CGrannyMesh* pMesh = m_pModel->GetMeshPointer(iMesh);
	if (!pMesh || !pMesh->GetGrannyMeshPointer())
		return false;

	const int meshBoneCount = pMesh->GetGrannyMeshPointer()->BoneBindingCount;
	const XMFLOAT4X4* composite = reinterpret_cast<const XMFLOAT4X4*>(GrannyGetWorldPoseComposite4x4Array(__GetWorldPosePtr()));

	DirectX::XMFLOAT4X4 palette[GRANNY_DX11_MAX_BONES];
	const int count = std::min(meshBoneCount, GRANNY_DX11_MAX_BONES);

	for (int i = 0; i < count; ++i)
	{
		XMFLOAT4X4 m = composite[boneIndices[i]];
		memcpy(&palette[i], &m, sizeof(DirectX::XMFLOAT4X4));
	}

	return _mgr->GetCbMgr()->UploadBonePalette(palette, count);
}

namespace
{
	void ApplySpecularOverrideIfNeeded(CGrannyMaterial& material, const SMaterialData& data)
	{
		if (data.pImage)
			return;

		if (std::fabs(material.GetSpecularPower() - data.fSpecularPower) >= std::numeric_limits<float>::epsilon())
			material.SetSpecularInfo(data.isSpecularEnable, data.fSpecularPower, data.bSphereMapIndex);
	}
}

void CGrannyModelInstance::RenderMeshNodeList(const RenderContext& ctx, TextureMode textureMode, CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	assert(m_pModel != nullptr);

	const auto indexBuffer = m_pModel->GetIndexBuffer();
	assert(indexBuffer != nullptr);

	const CGrannyModel::TMeshNode* meshNode = m_pModel->GetMeshNodeList(eMeshType, eMtrlType);

	_mgr->SetIndexBuffer(indexBuffer);

	while (meshNode)
	{
		const CGrannyMesh* mesh = meshNode->pMesh;
		const int vertexBase = mesh->GetVertexBasePosition();

		RenderContext meshCtx = ctx;
		meshCtx.Object.World = m_meshMatrices[meshNode->iMesh];
		meshCtx.Object.Skinned = eMeshType == CGrannyMesh::TYPE_DEFORM;

		STATEMANAGER.GetTransform().SetWorld(meshCtx.Object.World);
		ApplyRenderObjectContextToState(meshCtx);

		if (meshCtx.Object.Skinned)
			UploadMeshBonePaletteToShader(meshNode->iMesh);

		const CGrannyMesh::TTriGroupNode* triGroup = mesh->GetTriGroupNodeList(eMtrlType);

		while (triGroup)
		{
			ms_faceCount += triGroup->triCount;

			if (textureMode == TextureMode::One)
			{
				CGrannyMaterial& material = m_kMtrlPal.GetMaterialRef(triGroup->mtrlIndex);

				ApplySpecularOverrideIfNeeded(material, material_data_);
				material.SetSkinned(meshCtx.Object.Skinned);
				meshCtx.Object.TwoSided = meshCtx.Object.TwoSided || material.IsTwoSided();

				material.ApplyRenderState();
				
				STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertexBase, triGroup->idxPos, triGroup->triCount);

				material.RestoreRenderState();
			}
			else if (textureMode == TextureMode::Two)
			{
				const CGrannyMaterial& material = m_kMtrlPal.GetMaterialRef(triGroup->mtrlIndex);

				STATEMANAGER.SetTexture(0, material.GetSRV(0));
				STATEMANAGER.SetTexture(1, material.GetSRV(1));

				STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertexBase, triGroup->idxPos, triGroup->triCount);
			}
			else
			{
				STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertexBase, triGroup->idxPos, triGroup->triCount);
			}

			triGroup = triGroup->pNextTriGroupNode;
		}

		meshNode = meshNode->pNextMeshNode;
	}
}

void CGrannyModelInstance::RenderMeshNodeListWithOneTexture(const RenderContext& ctx, CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	RenderMeshNodeList(ctx, TextureMode::One, eMeshType, eMtrlType);
}

void CGrannyModelInstance::RenderMeshNodeListWithTwoTexture(const RenderContext& ctx, CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	RenderMeshNodeList(ctx, TextureMode::Two, eMeshType, eMtrlType);
}

void CGrannyModelInstance::RenderMeshNodeListWithoutTexture(const RenderContext& ctx, CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	_mgr->GetCbMgr()->SetUseTexture0(false);
	_mgr->GetCbMgr()->SetUseTexture1(false);
	RenderMeshNodeList(ctx, TextureMode::None, eMeshType, eMtrlType);
}

void CGrannyModelInstance::RenderProjectedShadow(const RenderContext& ctx)
{
	if (IsEmpty())
		return;

	ApplyRenderFrameContextToMeshCB(ctx);

	_mgr->GetCbMgr()->SetUseTexture0(false);
	_mgr->GetCbMgr()->SetUseTexture1(false);

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, IS_SKINNED | MESH_PROJECTED_SHADOW);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeList(ctx, TextureMode::None, CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH, MESH_PROJECTED_SHADOW);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNTVertex));
		RenderMeshNodeList(ctx, TextureMode::None, CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
}
