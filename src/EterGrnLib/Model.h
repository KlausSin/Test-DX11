#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "qMin32Lib/Core.h"
#include "Mesh.h"

class CGrannyModel : public CReferenceObject
{
public:
    struct TMeshNode
    {
        int iMesh{};
        const CGrannyMesh* pMesh{};
        TMeshNode* pNextMeshNode{};
    };

public:
    CGrannyModel();
    ~CGrannyModel();

    CGrannyModel(const CGrannyModel&) = delete;
    CGrannyModel& operator=(const CGrannyModel&) = delete;

    [[nodiscard]] bool IsEmpty() const noexcept;
    bool CreateFromGrannyModelPointer(granny_model* pgrnModel);
    bool CreateDeviceObjects();
    void DestroyDeviceObjects();
    void Destroy();

    [[nodiscard]] int GetRigidVertexCount() const noexcept;
    [[nodiscard]] int GetDeformVertexCount() const noexcept;
    [[nodiscard]] int GetVertexCount() const noexcept;
    [[nodiscard]] bool HasSkinnedMesh() const noexcept;
    [[nodiscard]] int GetIdxCount() const noexcept;
    [[nodiscard]] int GetMeshCount() const noexcept;

    [[nodiscard]] CGrannyMesh* GetMeshPointer(int iMesh);
    [[nodiscard]] const CGrannyMesh* GetMeshPointer(int iMesh) const;
    [[nodiscard]] granny_model* GetGrannyModelPointer() noexcept;

    [[nodiscard]] VBufferPtr GetVertexBuffer() const noexcept;
    [[nodiscard]] VBufferPtr GetSkinnedVertexBuffer() const noexcept;
    [[nodiscard]] UINT GetSkinnedVertexStride() const noexcept;
    [[nodiscard]] IBufferPtr GetIndexBuffer() const noexcept;

    [[nodiscard]] const TMeshNode* GetMeshNodeList(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType) const noexcept;
    [[nodiscard]] const CGrannyMaterialPalette& GetMaterialPalette() const noexcept;
    [[nodiscard]] bool HaveBlendThing() const noexcept { return m_bHaveBlendThing; }

protected:
    bool LoadMeshes();
    bool LoadVertices();
    bool LoadSkinnedVertices();
    bool LoadIndices();
    void Initialize() noexcept;

    [[nodiscard]] BOOL CheckMeshIndex(int iIndex) const noexcept;
    void AppendMeshNode(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType, int iMesh);

private:
    using MeshNodeListArray = std::array<std::array<TMeshNode*, CGrannyMaterial::TYPE_MAX_NUM>, CGrannyMesh::TYPE_MAX_NUM>;

    granny_model* m_pgrnModel{};
    bool m_hasPNT2{};
    friend class CGrannyModelInstance;
    std::vector<CGrannyMesh> m_meshes;

    VBufferPtr m_pntVtxBuf;
    VBufferPtr m_skinnedVtxBuf;
    IBufferPtr m_idxBuf;

    std::vector<TMeshNode> m_meshNodes;
    MeshNodeListArray m_meshNodeLists{};

    int m_deformVtxCount{};
    int m_rigidVtxCount{};
    int m_vtxCount{};
    int m_idxCount{};
    int m_meshNodeSize{};
    int m_meshNodeCapacity{};

    bool m_hasSkinnedVertices{};
    bool m_bHaveBlendThing{};

    CGrannyMaterialPalette m_kMtrlPal;
    UINT m_stride{};
    UINT m_skinnedStride{};
};
