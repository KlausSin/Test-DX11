#pragma once

#include <granny.h>
#include <windows.h>
#include <vector>

#include "Eterlib/ReferenceObject.h"
#include "Eterlib/Ref.h"
#include "Eterlib/GrpImageInstance.h"
#include "Util.h"

class CGrannyMaterial : public CReferenceObject
{
public:
    typedef CRef<CGrannyMaterial> TRef;

    enum EType
    {
        TYPE_DIFFUSE_PNT,
        TYPE_BLEND_PNT,
        TYPE_MAX_NUM
    };

    struct TMaterialState
    {
        granny_material* pgrnMaterial = nullptr;
        CGraphicImage::TRef images[2];
        EType type = TYPE_DIFFUSE_PNT;
        float specularPower = 0.0f;
        BOOL specularEnable = FALSE;
        bool twoSideRender = false;
        bool isSkinned = false;
        BYTE sphereMapIndex = 0;
    };

public:
    static void CreateSphereMap(UINT uMapIndex, const char* c_szSphereMapImageFileName);
    static void DestroySphereMap();
    static void TranslateSpecularMatrix(float fAddX, float fAddY, float fAddZ);

public:
    CGrannyMaterial();
    virtual ~CGrannyMaterial();

    void Destroy();
    void Copy(const CGrannyMaterial& rkMtrl);

    bool IsEqual(granny_material* pgrnMaterial) const;
    bool IsIn(const char* c_szImageName, int* iStage);

    void SetSpecularInfo(BOOL bFlag, float fPower, BYTE uSphereMapIndex);
    void SetImagePointer(int iStage, CGraphicImage* pImage);
    void SetSkinned(bool bSkinned);

    bool CreateFromGrannyMaterialPointer(granny_material* pgrnMaterial);

    void ApplyRenderState();
    void RestoreRenderState();

    CGrannyMaterial::TRef CloneWithImage(int iStage, CGraphicImage* pImage) const;
    CGrannyMaterial::TRef CloneWithMaterialData(int iStage, const SMaterialData& c_rkMaterialData) const;

    CGrannyMaterial::EType GetType() const;
    CGraphicImage* GetImagePointer(int iStage) const;

    const CGraphicTexture* GetDiffuseTexture() const;
    const CGraphicTexture* GetOpacityTexture() const;

    ID3D11ShaderResourceView* GetSRV(int iStage) const;

    float GetSpecularPower() const;
    bool IsSpecularEnabled() const;
    BYTE GetSphereMapIndex() const;
    bool IsTwoSided() const;

protected:
    void Initialize();

    CGraphicImage* __GetImagePointer(const char* c_szFileName);

    BOOL __IsSpecularEnable() const;

    void __ApplyDiffuseRenderState();
    void __RestoreDiffuseRenderState();
    void __ApplySpecularRenderState();
    void __RestoreSpecularRenderState();

protected:
    TMaterialState m_state;
    DWORD m_dwLastCullRenderStateForTwoSideRendering = D3D11_CULL_FRONT;

    void (CGrannyMaterial::* m_pfnApplyRenderState)() = nullptr;
    void (CGrannyMaterial::* m_pfnRestoreRenderState)() = nullptr;

private:
    enum
    {
        SPHEREMAP_NUM = 10,
    };

    static XMFLOAT4X4 ms_matSpecular;
    static XMFLOAT3 ms_v3SpecularTrans;
    static CGraphicImageInstance ms_akSphereMapInstance[SPHEREMAP_NUM];
};

class CGrannyMaterialPalette
{
public:
    CGrannyMaterialPalette();
    virtual ~CGrannyMaterialPalette();

    void Clear();
    void Copy(const CGrannyMaterialPalette& rkMtrlPalSrc);

    DWORD RegisterMaterial(granny_material* pgrnMaterial);

    void SetMaterialImagePointer(const char* c_szMtrlName, CGraphicImage* pImage);
    void SetMaterialData(const char* c_szMtrlName, const SMaterialData& c_rkMaterialData);
    void SetSpecularInfo(const char* c_szMtrlName, BOOL bEnable, float fPower);

    CGrannyMaterial& GetMaterialRef(DWORD mtrlIndex);

    DWORD GetMaterialCount() const;

protected:
    std::vector<CGrannyMaterial::TRef> m_mtrlVector;
};
