#include "StdAfx.h"
#include "Material.h"
#include "Mesh.h"
#include "Eterbase/Filename.h"
#include "Eterlib/ResourceManager.h"
#include "Eterlib/StateManager.h"
#include "Eterlib/GrpScreen.h"

CGraphicImageInstance CGrannyMaterial::ms_akSphereMapInstance[SPHEREMAP_NUM];

XMFLOAT3 CGrannyMaterial::ms_v3SpecularTrans(0.0f, 0.0f, 0.0f);
XMFLOAT4X4 CGrannyMaterial::ms_matSpecular;

XMFLOAT4 g_fSpecularColor(0.0f, 0.0f, 0.0f, 0.0f);

extern const std::string& GetModelLocalPath();

void CGrannyMaterial::TranslateSpecularMatrix(float fAddX, float fAddY, float fAddZ)
{
    static float SPECULAR_TRANSLATE_MAX = 1000000.0f;

    ms_v3SpecularTrans.x += fAddX;
    ms_v3SpecularTrans.y += fAddY;
    ms_v3SpecularTrans.z += fAddZ;

    if (ms_v3SpecularTrans.x >= SPECULAR_TRANSLATE_MAX)
        ms_v3SpecularTrans.x = 0.0f;

    if (ms_v3SpecularTrans.y >= SPECULAR_TRANSLATE_MAX)
        ms_v3SpecularTrans.y = 0.0f;

    if (ms_v3SpecularTrans.z >= SPECULAR_TRANSLATE_MAX)
        ms_v3SpecularTrans.z = 0.0f;

    XMStoreFloat4x4(
        &ms_matSpecular,
        XMMatrixTranslation(
            ms_v3SpecularTrans.x,
            ms_v3SpecularTrans.y,
            ms_v3SpecularTrans.z
        )
    );
}
CGrannyMaterial::CGrannyMaterial()
{
    Initialize();
}

CGrannyMaterial::~CGrannyMaterial()
{
}

void CGrannyMaterial::Initialize()
{
    m_state = TMaterialState{};
    m_state.images[0] = nullptr;
    m_state.images[1] = nullptr;

    m_dwLastCullRenderStateForTwoSideRendering = D3D11_CULL_FRONT;
    SetSpecularInfo(FALSE, 0.0f, 0);
}

void CGrannyMaterial::Destroy()
{
    Initialize();
}

void CGrannyMaterial::Copy(const CGrannyMaterial& rkMtrl)
{
    m_state = rkMtrl.m_state;
    m_dwLastCullRenderStateForTwoSideRendering = rkMtrl.m_dwLastCullRenderStateForTwoSideRendering;
    SetSpecularInfo(m_state.specularEnable, m_state.specularPower, m_state.sphereMapIndex);
}

CGrannyMaterial::TRef CGrannyMaterial::CloneWithImage(int iStage, CGraphicImage* pImage) const
{
    if (iStage < 0 || iStage >= 2)
        return nullptr;

    CGrannyMaterial* material = new CGrannyMaterial();
    material->Copy(*this);
    material->SetImagePointer(iStage, pImage);
    return CGrannyMaterial::TRef(material);
}

CGrannyMaterial::TRef CGrannyMaterial::CloneWithMaterialData(int iStage, const SMaterialData& c_rkMaterialData) const
{
    if (iStage < 0 || iStage >= 2)
        return nullptr;

    CGrannyMaterial* material = new CGrannyMaterial();
    material->Copy(*this);
    material->SetImagePointer(iStage, c_rkMaterialData.pImage);
    material->SetSpecularInfo(c_rkMaterialData.isSpecularEnable, c_rkMaterialData.fSpecularPower, c_rkMaterialData.bSphereMapIndex);
    return CGrannyMaterial::TRef(material);
}

void CGrannyMaterial::ApplyRenderState()
{
    assert(m_pfnApplyRenderState != nullptr && "CGrannyMaterial::ApplyRenderState");
    (this->*m_pfnApplyRenderState)();
}

void CGrannyMaterial::RestoreRenderState()
{
    assert(m_pfnRestoreRenderState != nullptr && "CGrannyMaterial::RestoreRenderState");
    (this->*m_pfnRestoreRenderState)();
}

CGrannyMaterial::EType CGrannyMaterial::GetType() const
{
    return m_state.type;
}

void CGrannyMaterial::SetSkinned(bool bSkinned)
{
    m_state.isSkinned = bSkinned;
}

void CGrannyMaterial::SetImagePointer(int iStage, CGraphicImage* pImage)
{
    assert(iStage >= 0 && iStage < 2 && "CGrannyMaterial::SetImagePointer");

    if (iStage < 0 || iStage >= 2)
        return;

    m_state.images[iStage] = pImage;

    if (!m_state.images[1].IsNull())
        m_state.type = TYPE_BLEND_PNT;
    else
        m_state.type = TYPE_DIFFUSE_PNT;
}

bool CGrannyMaterial::IsIn(const char* c_szImageName, int* piStage)
{
    if (!c_szImageName || !piStage || !m_state.pgrnMaterial)
        return false;

    std::string strImageName = c_szImageName;
    CFileNameHelper::StringPath(strImageName);

    granny_texture* pgrnDiffuseTexture = GrannyGetMaterialTextureByType(m_state.pgrnMaterial, GrannyDiffuseColorTexture);
    if (pgrnDiffuseTexture)
    {
        std::string strDiffuseFileName = pgrnDiffuseTexture->FromFileName;
        CFileNameHelper::StringPath(strDiffuseFileName);

        if (strDiffuseFileName == strImageName)
        {
            *piStage = 0;
            return true;
        }
    }

    granny_texture* pgrnOpacityTexture = GrannyGetMaterialTextureByType(m_state.pgrnMaterial, GrannyOpacityTexture);
    if (pgrnOpacityTexture)
    {
        std::string strOpacityFileName = pgrnOpacityTexture->FromFileName;
        CFileNameHelper::StringPath(strOpacityFileName);

        if (strOpacityFileName == strImageName)
        {
            *piStage = 1;
            return true;
        }
    }

    return false;
}

void CGrannyMaterial::SetSpecularInfo(BOOL bFlag, float fPower, BYTE uSphereMapIndex)
{
    m_state.specularPower = fPower;
    m_state.sphereMapIndex = uSphereMapIndex;
    m_state.specularEnable = bFlag;

    if (bFlag)
    {
        m_pfnApplyRenderState = &CGrannyMaterial::__ApplySpecularRenderState;
        m_pfnRestoreRenderState = &CGrannyMaterial::__RestoreSpecularRenderState;
    }
    else
    {
        m_pfnApplyRenderState = &CGrannyMaterial::__ApplyDiffuseRenderState;
        m_pfnRestoreRenderState = &CGrannyMaterial::__RestoreDiffuseRenderState;
    }
}

bool CGrannyMaterial::IsEqual(granny_material* pgrnMaterial) const
{
    return m_state.pgrnMaterial == pgrnMaterial;
}

ID3D11ShaderResourceView* CGrannyMaterial::GetSRV(int iStage) const
{
    if (iStage < 0 || iStage >= 2)
        return nullptr;

    const CGraphicImage::TRef& imageRef = m_state.images[iStage];

    if (imageRef.IsNull())
        return nullptr;

    CGraphicImage* image = imageRef.GetPointer();
    if (!image || !image->IsData())
        return nullptr;

    CGraphicTexture* texture = image->GetTexturePointer();
    return texture ? texture->GetSRV() : nullptr;
}

CGraphicImage* CGrannyMaterial::GetImagePointer(int iStage) const
{
    if (iStage < 0 || iStage >= 2)
        return nullptr;

    if (m_state.images[iStage].IsNull())
        return nullptr;

    return m_state.images[iStage].GetPointer();
}

const CGraphicTexture* CGrannyMaterial::GetDiffuseTexture() const
{
    CGraphicImage* image = GetImagePointer(0);
    return image && image->IsData() ? image->GetTexturePointer() : nullptr;
}

const CGraphicTexture* CGrannyMaterial::GetOpacityTexture() const
{
    CGraphicImage* image = GetImagePointer(1);
    return image && image->IsData() ? image->GetTexturePointer() : nullptr;
}

BOOL CGrannyMaterial::__IsSpecularEnable() const
{
    return m_state.specularEnable;
}

float CGrannyMaterial::GetSpecularPower() const
{
    return m_state.specularPower;
}

bool CGrannyMaterial::IsSpecularEnabled() const
{
    return m_state.specularEnable != FALSE;
}

BYTE CGrannyMaterial::GetSphereMapIndex() const
{
    return m_state.sphereMapIndex;
}

bool CGrannyMaterial::IsTwoSided() const
{
    return m_state.twoSideRender;
}

CGraphicImage* CGrannyMaterial::__GetImagePointer(const char* fileName)
{
    if (!fileName || !*fileName)
        return nullptr;

    CResourceManager& resMgr = CResourceManager::Instance();

    const int fileNameLen = static_cast<int>(strlen(fileName));

    if (fileNameLen > 2 && fileName[1] != ':')
    {
        char localFileName[256] = {};
        const std::string& modelLocalPath = GetModelLocalPath();

        if (modelLocalPath.length() + fileNameLen < sizeof(localFileName) - 1)
        {
            _snprintf(localFileName, sizeof(localFileName), "%s%s", modelLocalPath.c_str(), fileName);

            if (CGraphicImage* image = resMgr.GetTyped<CGraphicImage>(localFileName))
                return image;
        }
    }

    return resMgr.GetTyped<CGraphicImage>(fileName);
}

bool CGrannyMaterial::CreateFromGrannyMaterialPointer(granny_material* pgrnMaterial)
{
    m_state.pgrnMaterial = pgrnMaterial;

    granny_texture* pgrnDiffuseTexture = nullptr;
    granny_texture* pgrnOpacityTexture = nullptr;

    if (pgrnMaterial)
    {
        if (pgrnMaterial->MapCount > 1 && !_strnicmp(pgrnMaterial->Name, "Blend", 5))
        {
            pgrnDiffuseTexture = GrannyGetMaterialTextureByType(pgrnMaterial->Maps[0].Material, GrannyDiffuseColorTexture);
            pgrnOpacityTexture = GrannyGetMaterialTextureByType(pgrnMaterial->Maps[1].Material, GrannyDiffuseColorTexture);
        }
        else
        {
            pgrnDiffuseTexture = GrannyGetMaterialTextureByType(pgrnMaterial, GrannyDiffuseColorTexture);
            pgrnOpacityTexture = GrannyGetMaterialTextureByType(pgrnMaterial, GrannyOpacityTexture);
        }

        granny_int32 twoSided = 0;
        granny_data_type_definition TwoSidedFieldType[] =
        {
            { GrannyInt32Member, "Two-sided" },
            { GrannyEndMember },
        };

        granny_variant twoSideResult;

        if (GrannyFindMatchingMember(pgrnMaterial->ExtendedData.Type, pgrnMaterial->ExtendedData.Object, "Two-sided", &twoSideResult) && nullptr != twoSideResult.Type)
            GrannyConvertSingleObject(twoSideResult.Type, twoSideResult.Object, TwoSidedFieldType, &twoSided, nullptr);

        m_state.twoSideRender = twoSided == 1;
    }

    if (pgrnDiffuseTexture)
        m_state.images[0].SetPointer(__GetImagePointer(pgrnDiffuseTexture->FromFileName));

    if (pgrnOpacityTexture)
        m_state.images[1].SetPointer(__GetImagePointer(pgrnOpacityTexture->FromFileName));

    m_state.type = !m_state.images[1].IsNull() ? TYPE_BLEND_PNT : TYPE_DIFFUSE_PNT;

    return true;
}

void CGrannyMaterial::__ApplyDiffuseRenderState()
{
    STATEMANAGER.SetTexture(0, GetSRV(0));

    if (m_state.twoSideRender)
    {
        m_dwLastCullRenderStateForTwoSideRendering = STATEMANAGER.GetRaster().GetCullMode();
        STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);
    }
}

void CGrannyMaterial::__RestoreDiffuseRenderState()
{
    if (m_state.twoSideRender)
        STATEMANAGER.GetRaster().SetCullMode((D3D11_CULL_MODE)m_dwLastCullRenderStateForTwoSideRendering);
}

void CGrannyMaterial::__ApplySpecularRenderState()
{
    if (STATEMANAGER.GetBlend().GetBlendEnable())
    {
        __ApplyDiffuseRenderState();
        return;
    }

    CGraphicTexture* pkTexture = ms_akSphereMapInstance[m_state.sphereMapIndex].GetTexturePointer();

    STATEMANAGER.SetTexture(0, GetSRV(0));
    STATEMANAGER.SetTexture(1, pkTexture ? pkTexture->GetSRV() : nullptr);

    uint32_t flags = MESH_SPECULAR;

    if (m_state.isSkinned)
        flags |= IS_SKINNED;

    _mgr->SetShader(VF_MESH, flags);
}

void CGrannyMaterial::__RestoreSpecularRenderState()
{
    if (STATEMANAGER.GetBlend().GetBlendEnable())
    {
        __RestoreDiffuseRenderState();
        return;
    }
}

void CGrannyMaterial::CreateSphereMap(UINT uMapIndex, const char* c_szSphereMapImageFileName)
{
    if (uMapIndex >= SPHEREMAP_NUM)
        return;

    CGraphicImage* pImage = CResourceManager::Instance().GetTyped<CGraphicImage>(c_szSphereMapImageFileName);
    ms_akSphereMapInstance[uMapIndex].SetImagePointer(pImage);
}

void CGrannyMaterial::DestroySphereMap()
{
    for (UINT uMapIndex = 0; uMapIndex < SPHEREMAP_NUM; ++uMapIndex)
        ms_akSphereMapInstance[uMapIndex].Destroy();
}

CGrannyMaterialPalette::CGrannyMaterialPalette()
{
}

CGrannyMaterialPalette::~CGrannyMaterialPalette()
{
    Clear();
}

void CGrannyMaterialPalette::Copy(const CGrannyMaterialPalette& rkMtrlPalSrc)
{
    m_mtrlVector = rkMtrlPalSrc.m_mtrlVector;
}

void CGrannyMaterialPalette::Clear()
{
    m_mtrlVector.clear();
}

CGrannyMaterial& CGrannyMaterialPalette::GetMaterialRef(DWORD mtrlIndex)
{
    assert(mtrlIndex < m_mtrlVector.size());
    return *m_mtrlVector[mtrlIndex].GetPointer();
}

void CGrannyMaterialPalette::SetMaterialImagePointer(const char* c_szImageName, CGraphicImage* pImage)
{
    for (auto& roMtrl : m_mtrlVector)
    {
        int iStage = 0;

        if (roMtrl->IsIn(c_szImageName, &iStage))
        {
            roMtrl = roMtrl->CloneWithImage(iStage, pImage);
            return;
        }
    }
}

void CGrannyMaterialPalette::SetMaterialData(const char* c_szMtrlName, const SMaterialData& c_rkMaterialData)
{
    if (c_szMtrlName)
    {
        for (auto& roMtrl : m_mtrlVector)
        {
            int iStage = 0;

            if (roMtrl->IsIn(c_szMtrlName, &iStage))
            {
                roMtrl = roMtrl->CloneWithMaterialData(iStage, c_rkMaterialData);
                return;
            }
        }

        return;
    }

    for (auto& roMtrl : m_mtrlVector)
    {
        if (roMtrl->GetType() != CGrannyMaterial::TYPE_DIFFUSE_PNT)
            continue;

        roMtrl->SetSpecularInfo(c_rkMaterialData.isSpecularEnable, c_rkMaterialData.fSpecularPower, c_rkMaterialData.bSphereMapIndex);
    }
}

void CGrannyMaterialPalette::SetSpecularInfo(const char* c_szMtrlName, BOOL bEnable, float fPower)
{
    if (c_szMtrlName)
    {
        for (auto& roMtrl : m_mtrlVector)
        {
            int iStage = 0;

            if (roMtrl->IsIn(c_szMtrlName, &iStage))
            {
                roMtrl->SetSpecularInfo(bEnable, fPower, 0);
                return;
            }
        }

        return;
    }

    for (auto& roMtrl : m_mtrlVector)
        roMtrl->SetSpecularInfo(bEnable, fPower, 0);
}

DWORD CGrannyMaterialPalette::RegisterMaterial(granny_material* pgrnMaterial)
{
    const DWORD size = static_cast<DWORD>(m_mtrlVector.size());

    for (DWORD i = 0; i < size; ++i)
    {
        if (m_mtrlVector[i]->IsEqual(pgrnMaterial))
            return i;
    }

    CGrannyMaterial* material = new CGrannyMaterial();

    if (!material->CreateFromGrannyMaterialPointer(pgrnMaterial))
    {
        delete material;
        return 0;
    }

    m_mtrlVector.emplace_back(material);
    return size;
}

DWORD CGrannyMaterialPalette::GetMaterialCount() const
{
    return static_cast<DWORD>(m_mtrlVector.size());
}
