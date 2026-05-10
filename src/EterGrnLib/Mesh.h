#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "Material.h"

extern granny_data_type_definition GrannyPNT3322VertexType[5];
extern granny_data_type_definition GrannySkinnedPNTVertexType[6];
extern granny_data_type_definition GrannySkinnedPNT2VertexType[7];

struct granny_pnt3322_vertex
{
    granny_real32 Position[3]{};
    granny_real32 Normal[3]{};
    granny_real32 UV0[2]{};
    granny_real32 UV1[2]{};
};

struct TGrannySkinnedPNTVertex
{
    granny_real32 Position[3]{};
    granny_real32 Normal[3]{};
    granny_real32 BoneWeights[4]{};
    granny_uint32 BoneIndices[4]{};
    granny_real32 UV0[2]{};
};

struct TGrannySkinnedPNT2Vertex
{
    granny_real32 Position[3]{};
    granny_real32 Normal[3]{};
    granny_real32 BoneWeights[4]{};
    granny_uint32 BoneIndices[4]{};
    granny_real32 UV0[2]{};
    granny_real32 UV1[2]{};
};

class CGrannyMesh
{
public:
    enum EType : uint8_t
    {
        TYPE_RIGID,
        TYPE_DEFORM,
        TYPE_MAX_NUM
    };

    struct TTriGroupNode
    {
        TTriGroupNode* pNextTriGroupNode{};
        int idxPos{};
        int triCount{};
        uint32_t mtrlIndex{};
    };

public:
    CGrannyMesh();
    ~CGrannyMesh();

    CGrannyMesh(const CGrannyMesh&) = delete;
    CGrannyMesh& operator=(const CGrannyMesh&) = delete;
    CGrannyMesh(CGrannyMesh&& other) noexcept;
    CGrannyMesh& operator=(CGrannyMesh&& other) noexcept;

    [[nodiscard]] bool IsEmpty() const noexcept;
    bool CreateFromGrannyMeshPointer(granny_skeleton* pgrnSkeleton, granny_mesh* pgrnMesh, int vtxBasePos, int idxBasePos, CGrannyMaterialPalette& rkMtrlPal);

    void LoadIndices(void* dstBaseIndices) const;
    void LoadVertices(void* dstBaseVertices) const;
    void LoadSkinnedVertices(void* dstBaseVertices, bool pnt2) const;
    void Destroy();

    void SetPNT2Mesh() noexcept;

    [[nodiscard]] bool IsPNT2() const noexcept;
    [[nodiscard]] bool HasSkinnedVertices() const noexcept;
    [[nodiscard]] bool IsTwoSide() const noexcept;
    [[nodiscard]] bool HaveBlendThing() const noexcept { return m_bHaveBlendThing; }

    [[nodiscard]] int GetVertexCount() const;
    [[nodiscard]] int GetVertexBasePosition() const noexcept;
    [[nodiscard]] int GetIndexBasePosition() const noexcept;
    [[nodiscard]] int* GetDefaultBoneIndices() const;

    [[nodiscard]] const granny_mesh* GetGrannyMeshPointer() const noexcept;
    [[nodiscard]] const TTriGroupNode* GetTriGroupNodeList(CGrannyMaterial::EType eMtrlType) const noexcept;

    void RebuildTriGroupNodeList();
    void ReloadMaterials();

protected:
    void Initialize() noexcept;
    bool LoadMaterials(CGrannyMaterialPalette& rkMtrlPal);
    bool LoadTriGroupNodeList(CGrannyMaterialPalette& rkMtrlPal);

private:
    using TriGroupListArray = std::array<TTriGroupNode*, CGrannyMaterial::TYPE_MAX_NUM>;

    granny_data_type_definition* m_pgrnMeshType{};
    granny_mesh* m_pgrnMesh{};
    granny_mesh_binding* m_pgrnMeshBindingTemp{};

    std::vector<uint32_t> m_mtrlIndexVector;
    std::vector<TTriGroupNode> m_triGroupNodes;
    TriGroupListArray m_triGroupNodeLists{};

    int m_vtxBasePos{};
    int m_idxBasePos{};
    bool m_hasSkinnedVertex{};
    bool m_isTwoSide{};
    bool m_bHaveBlendThing{};
};
