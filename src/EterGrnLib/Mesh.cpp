#include "StdAfx.h"
#include "Mesh.h"
#include "Model.h"
#include "Material.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <utility>

namespace
{
    bool IsValidMaterialIndex(int index, int count) noexcept
    {
        return index >= 0 && index < count;
    }
}

granny_data_type_definition GrannyPNT3322VertexType[5] =
{
    {GrannyReal32Member, GrannyVertexPositionName, 0, 3},
    {GrannyReal32Member, GrannyVertexNormalName, 0, 3},
    {GrannyReal32Member, GrannyVertexTextureCoordinatesName"0", 0, 2},
    {GrannyReal32Member, GrannyVertexTextureCoordinatesName"1", 0, 2},
    {GrannyEndMember}
};

granny_data_type_definition GrannySkinnedPNTVertexType[6] =
{
    {GrannyReal32Member, GrannyVertexPositionName, 0, 3},
    {GrannyReal32Member, GrannyVertexNormalName, 0, 3},
    {GrannyReal32Member, GrannyVertexBoneWeightsName, 0, 4},
    {GrannyUInt32Member, GrannyVertexBoneIndicesName, 0, 4},
    {GrannyReal32Member, GrannyVertexTextureCoordinatesName"0", 0, 2},
    {GrannyEndMember}
};

granny_data_type_definition GrannySkinnedPNT2VertexType[7] =
{
    {GrannyReal32Member, GrannyVertexPositionName, 0, 3},
    {GrannyReal32Member, GrannyVertexNormalName, 0, 3},
    {GrannyReal32Member, GrannyVertexBoneWeightsName, 0, 4},
    {GrannyUInt32Member, GrannyVertexBoneIndicesName, 0, 4},
    {GrannyReal32Member, GrannyVertexTextureCoordinatesName"0", 0, 2},
    {GrannyReal32Member, GrannyVertexTextureCoordinatesName"1", 0, 2},
    {GrannyEndMember}
};

CGrannyMesh::CGrannyMesh()
{
    Initialize();
}

CGrannyMesh::~CGrannyMesh()
{
    Destroy();
}


CGrannyMesh::CGrannyMesh(CGrannyMesh&& other) noexcept
{
    *this = std::move(other);
}

CGrannyMesh& CGrannyMesh::operator=(CGrannyMesh&& other) noexcept
{
    if (this == &other)
        return *this;

    Destroy();

    m_pgrnMeshType = other.m_pgrnMeshType;
    m_pgrnMesh = other.m_pgrnMesh;
    m_pgrnMeshBindingTemp = other.m_pgrnMeshBindingTemp;
    m_mtrlIndexVector = std::move(other.m_mtrlIndexVector);
    m_triGroupNodes = std::move(other.m_triGroupNodes);
    m_triGroupNodeLists = other.m_triGroupNodeLists;
    m_vtxBasePos = other.m_vtxBasePos;
    m_idxBasePos = other.m_idxBasePos;
    m_hasSkinnedVertex = other.m_hasSkinnedVertex;
    m_isTwoSide = other.m_isTwoSide;
    m_bHaveBlendThing = other.m_bHaveBlendThing;

    other.m_pgrnMesh = nullptr;
    other.m_pgrnMeshBindingTemp = nullptr;
    other.m_triGroupNodeLists.fill(nullptr);
    other.m_triGroupNodes.clear();
    other.m_mtrlIndexVector.clear();
    other.Initialize();
    return *this;
}

void CGrannyMesh::Initialize() noexcept
{
    m_triGroupNodeLists.fill(nullptr);
    m_pgrnMeshType = GrannyPNT332VertexType;
    m_pgrnMesh = nullptr;
    m_pgrnMeshBindingTemp = nullptr;
    m_vtxBasePos = 0;
    m_idxBasePos = 0;
    m_hasSkinnedVertex = false;
    m_isTwoSide = false;
    m_bHaveBlendThing = false;
}

void CGrannyMesh::Destroy()
{
    m_triGroupNodes.clear();
    m_mtrlIndexVector.clear();

    if (m_pgrnMeshBindingTemp)
        GrannyFreeMeshBinding(m_pgrnMeshBindingTemp);

    Initialize();
}

bool CGrannyMesh::IsEmpty() const noexcept
{
    return m_pgrnMesh == nullptr;
}

bool CGrannyMesh::CreateFromGrannyMeshPointer(granny_skeleton* pgrnSkeleton, granny_mesh* pgrnMesh, int vtxBasePos, int idxBasePos, CGrannyMaterialPalette& rkMtrlPal)
{
    assert(IsEmpty());

    if (!pgrnMesh)
        return false;

    m_pgrnMesh = pgrnMesh;
    m_vtxBasePos = vtxBasePos;
    m_idxBasePos = idxBasePos;

    if (m_pgrnMesh->BoneBindingCount < 0)
        return true;

    m_pgrnMeshBindingTemp = GrannyNewMeshBinding(m_pgrnMesh, pgrnSkeleton, pgrnSkeleton);
    m_hasSkinnedVertex = !GrannyMeshIsRigid(m_pgrnMesh);
    m_isTwoSide = m_pgrnMesh->Name && std::strncmp(m_pgrnMesh->Name, "2x", 2) == 0;

    return LoadMaterials(rkMtrlPal) && LoadTriGroupNodeList(rkMtrlPal);
}

void CGrannyMesh::LoadIndices(void* dstBaseIndices) const
{
    if (!dstBaseIndices || !m_pgrnMesh)
        return;

    auto* dstIndices = static_cast<TIndex*>(dstBaseIndices) + m_idxBasePos;
    GrannyCopyMeshIndices(m_pgrnMesh, sizeof(TIndex), dstIndices);
}

void CGrannyMesh::LoadVertices(void* dstBaseVertices) const
{
    if (!dstBaseVertices || !m_pgrnMesh || !GrannyMeshIsRigid(m_pgrnMesh))
        return;

    if (IsPNT2())
    {
        auto* dstVertices = static_cast<TPNT2Vertex*>(dstBaseVertices) + m_vtxBasePos;
        GrannyCopyMeshVertices(m_pgrnMesh, GrannyPNT3322VertexType, dstVertices);
        return;
    }

    auto* dstVertices = static_cast<TPNTVertex*>(dstBaseVertices) + m_vtxBasePos;
    GrannyCopyMeshVertices(m_pgrnMesh, m_pgrnMeshType, dstVertices);
}

void CGrannyMesh::LoadSkinnedVertices(void* dstBaseVertices, bool pnt2) const
{
    if (!dstBaseVertices || !m_pgrnMesh || GrannyMeshIsRigid(m_pgrnMesh))
        return;

    if (pnt2)
    {
        auto* dstVertices = static_cast<TGrannySkinnedPNT2Vertex*>(dstBaseVertices) + m_vtxBasePos;
        GrannyCopyMeshVertices(m_pgrnMesh, GrannySkinnedPNT2VertexType, dstVertices);
        return;
    }

    auto* dstVertices = static_cast<TGrannySkinnedPNTVertex*>(dstBaseVertices) + m_vtxBasePos;
    GrannyCopyMeshVertices(m_pgrnMesh, GrannySkinnedPNTVertexType, dstVertices);
}

bool CGrannyMesh::LoadMaterials(CGrannyMaterialPalette& rkMtrlPal)
{
    assert(m_pgrnMesh != nullptr);

    const int materialCount = m_pgrnMesh->MaterialBindingCount;
    if (materialCount <= 0)
        return true;

    m_mtrlIndexVector.reserve(materialCount);
    m_bHaveBlendThing = false;

    for (int i = 0; i < materialCount; ++i)
    {
        granny_material* material = m_pgrnMesh->MaterialBindings[i].Material;
        const auto materialIndex = rkMtrlPal.RegisterMaterial(material);
        m_mtrlIndexVector.push_back(materialIndex);
        m_bHaveBlendThing |= rkMtrlPal.GetMaterialRef(materialIndex).GetType() == CGrannyMaterial::TYPE_BLEND_PNT;
    }

    return true;
}

bool CGrannyMesh::LoadTriGroupNodeList(CGrannyMaterialPalette& rkMtrlPal)
{
    assert(m_pgrnMesh != nullptr);
    assert(m_triGroupNodes.empty());

    const int materialCount = m_pgrnMesh->MaterialBindingCount;
    const int groupCount = GrannyGetMeshTriangleGroupCount(m_pgrnMesh);

    if (materialCount <= 0 || groupCount <= 0)
        return true;

    m_triGroupNodes.resize(groupCount);
    const granny_tri_material_group* groups = GrannyGetMeshTriangleGroups(m_pgrnMesh);

    for (int i = 0; i < groupCount; ++i)
    {
        const auto& src = groups[i];
        auto& dst = m_triGroupNodes[i];

        dst.idxPos = m_idxBasePos + src.TriFirst * 3;
        dst.triCount = src.TriCount;
        dst.mtrlIndex = IsValidMaterialIndex(src.MaterialIndex, materialCount) ? m_mtrlIndexVector[src.MaterialIndex] : 0u;

        const CGrannyMaterial& material = rkMtrlPal.GetMaterialRef(dst.mtrlIndex);
        dst.pNextTriGroupNode = m_triGroupNodeLists[material.GetType()];
        m_triGroupNodeLists[material.GetType()] = &dst;
    }

    return true;
}

void CGrannyMesh::RebuildTriGroupNodeList()
{
}

void CGrannyMesh::ReloadMaterials()
{
}

bool CGrannyMesh::IsPNT2() const noexcept
{
    return m_pgrnMeshType == GrannyPNT3322VertexType;
}

void CGrannyMesh::SetPNT2Mesh() noexcept
{
    m_pgrnMeshType = GrannyPNT3322VertexType;
}

bool CGrannyMesh::HasSkinnedVertices() const noexcept
{
    return m_hasSkinnedVertex;
}

bool CGrannyMesh::IsTwoSide() const noexcept
{
    return m_isTwoSide;
}

int CGrannyMesh::GetVertexCount() const
{
    assert(m_pgrnMesh != nullptr);
    return GrannyGetMeshVertexCount(m_pgrnMesh);
}

int CGrannyMesh::GetVertexBasePosition() const noexcept
{
    return m_vtxBasePos;
}

int CGrannyMesh::GetIndexBasePosition() const noexcept
{
    return m_idxBasePos;
}

int* CGrannyMesh::GetDefaultBoneIndices() const
{
    return (int*)GrannyGetMeshBindingToBoneIndices(m_pgrnMeshBindingTemp);
}

const granny_mesh* CGrannyMesh::GetGrannyMeshPointer() const noexcept
{
    return m_pgrnMesh;
}

const CGrannyMesh::TTriGroupNode* CGrannyMesh::GetTriGroupNodeList(CGrannyMaterial::EType eMtrlType) const noexcept
{
    return m_triGroupNodeLists[eMtrlType];
}
