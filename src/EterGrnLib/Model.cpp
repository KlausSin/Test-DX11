#include "StdAfx.h"
#include "Model.h"
#include "Mesh.h"
#include "qMin32Lib/All.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

namespace
{
    UINT GetMeshVertexStride(const granny_mesh* mesh)
    {
        if (!mesh || !mesh->PrimaryVertexData || !mesh->PrimaryVertexData->VertexType)
            return 0;

        UINT stride = 0;
        const granny_data_type_definition* type = mesh->PrimaryVertexData->VertexType;

        for (int i = 0; type[i].Type != GrannyEndMember; ++i)
        {
            const char* name = type[i].Name;
            if (!name || !name[0])
                continue;

            if (std::strcmp(name, GrannyVertexPositionName) == 0)
                stride += sizeof(float) * 3;
            else if (std::strcmp(name, GrannyVertexNormalName) == 0)
                stride += sizeof(float) * 3;
            else if (std::strcmp(name, GrannyVertexTextureCoordinatesName "0") == 0)
                stride += sizeof(float) * 2;
            else if (std::strcmp(name, GrannyVertexTextureCoordinatesName "1") == 0)
                stride += sizeof(float) * 2;
        }

        return stride;
    }
}

CGrannyModel::CGrannyModel()
{
    Initialize();
}

CGrannyModel::~CGrannyModel()
{
    Destroy();
}

void CGrannyModel::Initialize() noexcept
{
    for (auto& row : m_meshNodeLists)
        row.fill(nullptr);

    m_pgrnModel = nullptr;
    m_meshes.clear();
    m_meshNodes.clear();
    m_pntVtxBuf = nullptr;
    m_skinnedVtxBuf = nullptr;
    m_idxBuf = nullptr;

    m_meshNodeSize = 0;
    m_meshNodeCapacity = 0;
    m_rigidVtxCount = 0;
    m_deformVtxCount = 0;
    m_vtxCount = 0;
    m_idxCount = 0;
    m_hasSkinnedVertices = false;
    m_stride = 0;
    m_skinnedStride = 0;
    m_bHaveBlendThing = false;
    m_hasPNT2 = false;
}

void CGrannyModel::Destroy()
{
    m_kMtrlPal.Clear();
    m_meshNodes.clear();
    m_meshes.clear();
    Initialize();
}

bool CGrannyModel::IsEmpty() const noexcept
{
    return m_pgrnModel == nullptr;
}

bool CGrannyModel::CreateFromGrannyModelPointer(granny_model* pgrnModel)
{
    assert(IsEmpty());

    if (!pgrnModel)
        return false;

    m_pgrnModel = pgrnModel;

    if (!LoadMeshes() || !LoadVertices() || !LoadSkinnedVertices() || !LoadIndices())
        return false;

    AddReference();
    return true;
}

bool CGrannyModel::LoadMeshes()
{
    assert(m_pgrnModel != nullptr);

    const int meshCount = GetMeshCount();
    if (meshCount <= 0)
        return true;

    granny_skeleton* skeleton = m_pgrnModel->Skeleton;
    m_meshes.resize(meshCount);
    m_stride = 0;

    int rigidVertexBase = 0;
    int deformVertexBase = 0;
    int vertexBase = 0;
    int indexBase = 0;
    int diffuseNodeCount = 0;
    int blendNodeCount = 0;

    for (int i = 0; i < meshCount; ++i)
    {
        CGrannyMesh& mesh = m_meshes[i];
        granny_mesh* grannyMesh = m_pgrnModel->MeshBindings[i].Mesh;
        const int vertexCount = GrannyGetMeshVertexCount(grannyMesh);
        const int indexCount = GrannyGetMeshIndexCount(grannyMesh);

        if (GrannyMeshIsRigid(grannyMesh))
        {
            if (!mesh.CreateFromGrannyMeshPointer(skeleton, grannyMesh, rigidVertexBase, indexBase, m_kMtrlPal))
                return false;
            rigidVertexBase += vertexCount;
        }
        else
        {
            if (!mesh.CreateFromGrannyMeshPointer(skeleton, grannyMesh, deformVertexBase, indexBase, m_kMtrlPal))
                return false;
            deformVertexBase += vertexCount;
            m_hasSkinnedVertices |= mesh.HasSkinnedVertices();
        }

        m_bHaveBlendThing |= mesh.HaveBlendThing();
        const UINT meshStride = GetMeshVertexStride(grannyMesh);

        if (meshStride == sizeof(TPNT2Vertex))
        {
            mesh.SetPNT2Mesh();
            m_hasPNT2 = true;
        }

        vertexBase += vertexCount;
        indexBase += indexCount;

        diffuseNodeCount += mesh.GetTriGroupNodeList(CGrannyMaterial::TYPE_DIFFUSE_PNT) ? 1 : 0;
        blendNodeCount += mesh.GetTriGroupNodeList(CGrannyMaterial::TYPE_BLEND_PNT) ? 1 : 0;
    }

    m_meshNodeCapacity = diffuseNodeCount + blendNodeCount;
    m_meshNodes.resize(m_meshNodeCapacity);

    for (int i = 0; i < meshCount; ++i)
    {
        CGrannyMesh& mesh = m_meshes[i];
        granny_mesh* grannyMesh = m_pgrnModel->MeshBindings[i].Mesh;
        const auto meshType = GrannyMeshIsRigid(grannyMesh) ? CGrannyMesh::TYPE_RIGID : CGrannyMesh::TYPE_DEFORM;

        if (mesh.GetTriGroupNodeList(CGrannyMaterial::TYPE_DIFFUSE_PNT))
            AppendMeshNode(meshType, CGrannyMaterial::TYPE_DIFFUSE_PNT, i);

        if (mesh.GetTriGroupNodeList(CGrannyMaterial::TYPE_BLEND_PNT))
            AppendMeshNode(meshType, CGrannyMaterial::TYPE_BLEND_PNT, i);
    }

    m_rigidVtxCount = rigidVertexBase;
    m_deformVtxCount = deformVertexBase;
    m_vtxCount = vertexBase;
    m_idxCount = indexBase;
    return true;
}

bool CGrannyModel::LoadVertices()
{
    if (m_rigidVtxCount <= 0)
        return true;

    const bool hasPNT2 = std::any_of(m_meshes.begin(), m_meshes.end(), [](const CGrannyMesh& mesh) { return mesh.IsPNT2(); });

    if (hasPNT2)
    {
        m_stride = sizeof(TPNT2Vertex);
        std::vector<TPNT2Vertex> vertices(m_rigidVtxCount);
        for (const auto& mesh : m_meshes)
            mesh.LoadVertices(vertices.data());
        _mgr->CreateVertexBuffer(m_pntVtxBuf, vertices.data(), m_rigidVtxCount, m_stride);
        return true;
    }

    m_stride = sizeof(TPNTVertex);
    std::vector<TPNTVertex> vertices(m_rigidVtxCount);
    for (const auto& mesh : m_meshes)
        mesh.LoadVertices(vertices.data());
    _mgr->CreateVertexBuffer(m_pntVtxBuf, vertices.data(), m_rigidVtxCount, m_stride);
    return true;
}

bool CGrannyModel::LoadSkinnedVertices()
{
    if (m_deformVtxCount <= 0)
        return true;

    bool hasPNT2 = false;
    for (int i = 0; i < GetMeshCount(); ++i)
    {
        if (!GrannyMeshIsRigid(m_pgrnModel->MeshBindings[i].Mesh) && m_meshes[i].IsPNT2())
        {
            hasPNT2 = true;
            break;
        }
    }

    if (hasPNT2)
    {
        m_skinnedStride = sizeof(TGrannySkinnedPNT2Vertex);
        std::vector<TGrannySkinnedPNT2Vertex> vertices(m_deformVtxCount);
        for (const auto& mesh : m_meshes)
            mesh.LoadSkinnedVertices(vertices.data(), true);
        _mgr->CreateVertexBuffer(m_skinnedVtxBuf, vertices.data(), m_deformVtxCount, m_skinnedStride);
        return true;
    }

    m_skinnedStride = sizeof(TGrannySkinnedPNTVertex);
    std::vector<TGrannySkinnedPNTVertex> vertices(m_deformVtxCount);
    for (const auto& mesh : m_meshes)
        mesh.LoadSkinnedVertices(vertices.data(), false);
    _mgr->CreateVertexBuffer(m_skinnedVtxBuf, vertices.data(), m_deformVtxCount, m_skinnedStride);
    return true;
}

bool CGrannyModel::LoadIndices()
{
    if (m_idxCount <= 0)
        return true;

    std::vector<WORD> indices(m_idxCount);
    for (const auto& mesh : m_meshes)
        mesh.LoadIndices(indices.data());

    _mgr->CreateIndexBuffer(m_idxBuf, indices.data(), m_idxCount);
    return true;
}

void CGrannyModel::AppendMeshNode(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType, int iMesh)
{
    assert(m_meshNodeSize < m_meshNodeCapacity);

    auto& node = m_meshNodes[m_meshNodeSize++];
    node.iMesh = iMesh;
    node.pMesh = &m_meshes[iMesh];
    node.pNextMeshNode = m_meshNodeLists[eMeshType][eMtrlType];
    m_meshNodeLists[eMeshType][eMtrlType] = &node;
}

bool CGrannyModel::CreateDeviceObjects()
{
    for (auto& mesh : m_meshes)
        mesh.RebuildTriGroupNodeList();
    return true;
}

void CGrannyModel::DestroyDeviceObjects()
{
}

BOOL CGrannyModel::CheckMeshIndex(int iIndex) const noexcept
{
    return iIndex >= 0 && iIndex < GetMeshCount();
}

const CGrannyMaterialPalette& CGrannyModel::GetMaterialPalette() const noexcept
{
    return m_kMtrlPal;
}

const CGrannyModel::TMeshNode* CGrannyModel::GetMeshNodeList(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType) const noexcept
{
    return m_meshNodeLists[eMeshType][eMtrlType];
}

CGrannyMesh* CGrannyModel::GetMeshPointer(int iMesh)
{
    assert(CheckMeshIndex(iMesh));
    return &m_meshes[iMesh];
}

const CGrannyMesh* CGrannyModel::GetMeshPointer(int iMesh) const
{
    assert(CheckMeshIndex(iMesh));
    return &m_meshes[iMesh];
}

bool CGrannyModel::HasSkinnedMesh() const noexcept
{
    return m_deformVtxCount > 0 && m_skinnedVtxBuf != nullptr;
}

int CGrannyModel::GetRigidVertexCount() const noexcept { return m_rigidVtxCount; }
int CGrannyModel::GetDeformVertexCount() const noexcept { return m_deformVtxCount; }
int CGrannyModel::GetVertexCount() const noexcept { return m_vtxCount; }
int CGrannyModel::GetIdxCount() const noexcept { return m_idxCount; }
int CGrannyModel::GetMeshCount() const noexcept { return m_pgrnModel ? m_pgrnModel->MeshBindingCount : 0; }
granny_model* CGrannyModel::GetGrannyModelPointer() noexcept { return m_pgrnModel; }
IBufferPtr CGrannyModel::GetIndexBuffer() const noexcept { return m_idxBuf; }
VBufferPtr CGrannyModel::GetVertexBuffer() const noexcept { return m_pntVtxBuf; }
VBufferPtr CGrannyModel::GetSkinnedVertexBuffer() const noexcept { return m_skinnedVtxBuf; }
UINT CGrannyModel::GetSkinnedVertexStride() const noexcept { return m_skinnedStride; }
