#include "StdAfx.h"
#include "Material.h"
#include "Mesh.h"
#include "Eterbase/Filename.h"
#include "Eterlib/ResourceManager.h"
#include "Eterlib/StateManager.h"
#include "Eterlib/GrpScreen.h"
#include <memory>

CGraphicImageInstance CGrannyMaterial::ms_akSphereMapInstance[SPHEREMAP_NUM];

D3DXVECTOR3	CGrannyMaterial::ms_v3SpecularTrans(0.0f, 0.0f, 0.0f);
D3DXMATRIX	CGrannyMaterial::ms_matSpecular;

D3DXCOLOR g_fSpecularColor = D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f);

void CGrannyMaterial::TranslateSpecularMatrix(float fAddX, float fAddY, float fAddZ)
{
	static float SPECULAR_TRANSLATE_MAX = 1000000.0f;

	ms_v3SpecularTrans.x+=fAddX;
	ms_v3SpecularTrans.y+=fAddY;
	ms_v3SpecularTrans.z+=fAddZ;

	if (ms_v3SpecularTrans.x>=SPECULAR_TRANSLATE_MAX)
		ms_v3SpecularTrans.x=0.0f;

	if (ms_v3SpecularTrans.y>=SPECULAR_TRANSLATE_MAX)
		ms_v3SpecularTrans.y=0.0f;

	if (ms_v3SpecularTrans.z>=SPECULAR_TRANSLATE_MAX)
		ms_v3SpecularTrans.z=0.0f;

	D3DXMatrixTranslation(&ms_matSpecular, 
		ms_v3SpecularTrans.x, 
		ms_v3SpecularTrans.y, 
		ms_v3SpecularTrans.z
	);
}

void CGrannyMaterial::ApplyRenderState()
{
	assert(m_pfnApplyRenderState!=nullptr && "CGrannyMaterial::SaveRenderState");
	(this->*m_pfnApplyRenderState)();
}

void CGrannyMaterial::RestoreRenderState()
{
	assert(m_pfnRestoreRenderState!=nullptr && "CGrannyMaterial::RestoreRenderState");
	(this->*m_pfnRestoreRenderState)();
}

void CGrannyMaterial::Copy(CGrannyMaterial& rkMtrl)
{
	m_pgrnMaterial = rkMtrl.m_pgrnMaterial;
	m_roImage[0] =  rkMtrl.m_roImage[0];
	m_roImage[1] =  rkMtrl.m_roImage[1];
    m_eType = rkMtrl.m_eType;	
}

CGrannyMaterial::CGrannyMaterial()
{
	m_bTwoSideRender = false;
	m_dwLastCullRenderStateForTwoSideRendering = D3D11_CULL_FRONT;

	Initialize();
}

CGrannyMaterial::~CGrannyMaterial()
{
}

CGrannyMaterial::EType CGrannyMaterial::GetType() const
{
	return m_eType;
}

void CGrannyMaterial::SetImagePointer(int iStage, CGraphicImage* pImage)
{	
	assert(iStage<2 && "CGrannyMaterial::SetImagePointer");
	m_roImage[iStage]=pImage;
}

bool CGrannyMaterial::IsIn(const char* c_szImageName, int* piStage)
{
	std::string strImageName = c_szImageName;
	CFileNameHelper::StringPath(strImageName);

	granny_texture * pgrnDiffuseTexture = GrannyGetMaterialTextureByType(m_pgrnMaterial, GrannyDiffuseColorTexture);
	if (pgrnDiffuseTexture)
	{
		std::string strDiffuseFileName = pgrnDiffuseTexture->FromFileName;
		CFileNameHelper::StringPath(strDiffuseFileName);
		if (strDiffuseFileName == strImageName)
		{
			*piStage=0;
			return true;
		}
	}

    granny_texture * pgrnOpacityTexture = GrannyGetMaterialTextureByType(m_pgrnMaterial, GrannyOpacityTexture);
	if (pgrnOpacityTexture)
	{
		std::string strOpacityFileName = pgrnOpacityTexture->FromFileName;
		CFileNameHelper::StringPath(strOpacityFileName);
		if (strOpacityFileName == strImageName)
		{
			*piStage=1;
			return true;
		}
	}

	return false;
}

void CGrannyMaterial::SetSpecularInfo(BOOL bFlag, float fPower, BYTE uSphereMapIndex)
{
	m_fSpecularPower = fPower;
	m_bSphereMapIndex = uSphereMapIndex;
	m_bSpecularEnable = bFlag;	

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
	if (m_pgrnMaterial==pgrnMaterial)
		return true;

	return false;
}


ID3D11ShaderResourceView* CGrannyMaterial::GetSRV(int iStage) const
{
	const CGraphicImage::TRef & ratImage = m_roImage[iStage];

	if (ratImage.IsNull())
		return nullptr;

	CGraphicImage * pImage = ratImage.GetPointer();
	const CGraphicTexture * pTexture = pImage->GetTexturePointer();

	return pTexture->GetSRV();
}

CGraphicImage * CGrannyMaterial::GetImagePointer(int iStage) const
{
	const CGraphicImage::TRef & ratImage = m_roImage[iStage];

	if (ratImage.IsNull())
		return nullptr;

	CGraphicImage * pImage = ratImage.GetPointer();
	return pImage;
}

const CGraphicTexture* CGrannyMaterial::GetDiffuseTexture() const
{
	if (m_roImage[0].IsNull())
		return nullptr;

	return m_roImage[0].GetPointer()->GetTexturePointer();
}

const CGraphicTexture* CGrannyMaterial::GetOpacityTexture() const
{
	if (m_roImage[1].IsNull())
		return nullptr;

	return m_roImage[1].GetPointer()->GetTexturePointer();
}

BOOL CGrannyMaterial::__IsSpecularEnable() const
{
	return m_bSpecularEnable;
}

// MR-12: Fix specular isolation issue
float CGrannyMaterial::GetSpecularPower() const
{
	return m_fSpecularPower;
}
// MR-12: -- END OF -- Fix specular isolation issue

extern const std::string& GetModelLocalPath();

CGraphicImage* CGrannyMaterial::__GetImagePointer(const char* fileName)
{
	assert(*fileName != '\0');

	CResourceManager& rkResMgr = CResourceManager::Instance();

	// SUPPORT_LOCAL_TEXTURE
	int fileName_len = strlen(fileName);
	if (fileName_len > 2 && fileName[1] != ':')
	{
		char localFileName[256];		
		const std::string& modelLocalPath = GetModelLocalPath();

		int localFileName_len = modelLocalPath.length() + 1 + fileName_len;
		if (localFileName_len < sizeof(localFileName) - 1)
		{
			_snprintf(localFileName, sizeof(localFileName), "%s%s", GetModelLocalPath().c_str(), fileName);
			CResource* pResource = rkResMgr.GetResourcePointer(localFileName);
			return static_cast<CGraphicImage*>(pResource);
		}		
	}
	// END_OF_SUPPORT_LOCAL_TEXTURE
	

	CResource* pResource = rkResMgr.GetResourcePointer(fileName);
	return static_cast<CGraphicImage*>(pResource);
}

bool CGrannyMaterial::CreateFromGrannyMaterialPointer(granny_material * pgrnMaterial)
{
	m_pgrnMaterial = pgrnMaterial;

	granny_texture * pgrnDiffuseTexture = nullptr;
	granny_texture * pgrnOpacityTexture = nullptr;

	if (pgrnMaterial)
	{
		if (pgrnMaterial->MapCount > 1 && !_strnicmp(pgrnMaterial->Name, "Blend", 5))
		{
			pgrnDiffuseTexture = GrannyGetMaterialTextureByType(pgrnMaterial->Maps[0].Material, GrannyDiffuseColorTexture);
			pgrnOpacityTexture = GrannyGetMaterialTextureByType(pgrnMaterial->Maps[1].Material, GrannyDiffuseColorTexture);
		}
		else
		{
			pgrnDiffuseTexture = GrannyGetMaterialTextureByType(m_pgrnMaterial, GrannyDiffuseColorTexture);
			pgrnOpacityTexture = GrannyGetMaterialTextureByType(m_pgrnMaterial, GrannyOpacityTexture);
		}

		// Two-Side 렌더링이 필요한 지 검사
		{			
			granny_int32 twoSided = 0;
			granny_data_type_definition TwoSidedFieldType[] =
			{
				{GrannyInt32Member, "Two-sided"},
				{GrannyEndMember},
			};

			granny_variant twoSideResult;

			if (GrannyFindMatchingMember(pgrnMaterial->ExtendedData.Type, pgrnMaterial->ExtendedData.Object, "Two-sided", &twoSideResult)  && nullptr != twoSideResult.Type)
				GrannyConvertSingleObject(twoSideResult.Type, twoSideResult.Object, TwoSidedFieldType, &twoSided, nullptr);

			m_bTwoSideRender = 1 == twoSided;
		}
	}

	if (pgrnDiffuseTexture)
		m_roImage[0].SetPointer(__GetImagePointer(pgrnDiffuseTexture->FromFileName));

	if (pgrnOpacityTexture)
		m_roImage[1].SetPointer(__GetImagePointer(pgrnOpacityTexture->FromFileName));

	// 오퍼시티가 있으면 블렌딩 메쉬
	if (!m_roImage[1].IsNull())
		m_eType = TYPE_BLEND_PNT;
	else
		m_eType = TYPE_DIFFUSE_PNT;

	return true;
}

void CGrannyMaterial::Initialize()
{
	m_roImage[0] = nullptr;
	m_roImage[1] = nullptr;
	m_bIsSkinned = false;

	SetSpecularInfo(FALSE, 0.0f, 0);
}

void CGrannyMaterial::__ApplyDiffuseRenderState()
{
	STATEMANAGER.SetTexture(0, GetSRV(0));

	if (m_bTwoSideRender)
	{
		m_dwLastCullRenderStateForTwoSideRendering = STATEMANAGER.GetRaster().GetCullMode();
		STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);
	}
}

void CGrannyMaterial::__RestoreDiffuseRenderState()
{
	if (m_bTwoSideRender)
		STATEMANAGER.GetRaster().SetCullMode((D3D11_CULL_MODE)m_dwLastCullRenderStateForTwoSideRendering);
}

void CGrannyMaterial::__ApplySpecularRenderState()
{
	if (STATEMANAGER.GetBlend().GetBlendEnable())
	{
		__ApplyDiffuseRenderState();
		return;
	}

	CGraphicTexture* pkTexture = ms_akSphereMapInstance[m_bSphereMapIndex].GetTexturePointer();

	STATEMANAGER.SetTexture(0, GetSRV(0));
	STATEMANAGER.SetTexture(1, pkTexture ? pkTexture->GetSRV() : nullptr);

	_mgr->GetCbMgr()->SetSpecularPower(GetSpecularPower(), g_fSpecularColor);

	uint32_t flags = MESH_SPECULAR;
	if (m_bIsSkinned)
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
	CResourceManager& rkResMgr = CResourceManager::Instance();
	CGraphicImage * pImage = (CGraphicImage *)rkResMgr.GetResourcePointer(c_szSphereMapImageFileName);
	ms_akSphereMapInstance[uMapIndex].SetImagePointer(pImage);
}

void CGrannyMaterial::DestroySphereMap()
{
	for (UINT uMapIndex=0; uMapIndex<SPHEREMAP_NUM; ++uMapIndex)
		ms_akSphereMapInstance[uMapIndex].Destroy();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CGrannyMaterialPalette::CGrannyMaterialPalette()
{
}

CGrannyMaterialPalette::~CGrannyMaterialPalette()
{
	Clear();
}

void CGrannyMaterialPalette::Copy(const CGrannyMaterialPalette& rkMtrlPalSrc)
{
	m_mtrlVector=rkMtrlPalSrc.m_mtrlVector;
}

void CGrannyMaterialPalette::Clear()
{
	m_mtrlVector.clear();
}

CGrannyMaterial& CGrannyMaterialPalette::GetMaterialRef(DWORD mtrlIndex)
{
	assert(mtrlIndex<m_mtrlVector.size());
	return *m_mtrlVector[mtrlIndex].GetPointer();
}

void CGrannyMaterialPalette::SetMaterialImagePointer(const char* c_szImageName, CGraphicImage* pImage)
{
	DWORD size=m_mtrlVector.size();
	DWORD i;
	for (i=0; i<size; ++i)
	{
		CGrannyMaterial::TRef& roMtrl=m_mtrlVector[i];

		int iStage;
		if (roMtrl->IsIn(c_szImageName, &iStage))
		{
			auto pkNewMtrl = std::make_unique<CGrannyMaterial>();
			pkNewMtrl->Copy(*roMtrl.GetPointer());
			pkNewMtrl->SetImagePointer(iStage, pImage);
			roMtrl = pkNewMtrl.release();

			return;
		}
	}
}

void CGrannyMaterialPalette::SetMaterialData(const char* c_szMtrlName, const SMaterialData& c_rkMaterialData)
{
	if (c_szMtrlName)
	{
		std::vector<CGrannyMaterial::TRef>::iterator i;
		for (i=m_mtrlVector.begin(); i!=m_mtrlVector.end(); ++i)
		{
			CGrannyMaterial::TRef& roMtrl=*i;

			int iStage;
			if (roMtrl->IsIn(c_szMtrlName, &iStage))
			{
				auto pkNewMtrl = std::make_unique<CGrannyMaterial>();
				pkNewMtrl->Copy(*roMtrl.GetPointer());
				pkNewMtrl->SetImagePointer(iStage, c_rkMaterialData.pImage);
				pkNewMtrl->SetSpecularInfo(c_rkMaterialData.isSpecularEnable, c_rkMaterialData.fSpecularPower, c_rkMaterialData.bSphereMapIndex);
				roMtrl = pkNewMtrl.release();

				return;
			}
		}
	}
	else
	{
		std::vector<CGrannyMaterial::TRef>::iterator i;
		for (i = m_mtrlVector.begin(); i != m_mtrlVector.end(); ++i)
		{
			CGrannyMaterial::TRef& roMtrl = *i;

			if (roMtrl->GetType() != CGrannyMaterial::TYPE_DIFFUSE_PNT)
				continue;

			roMtrl->SetSpecularInfo(c_rkMaterialData.isSpecularEnable, c_rkMaterialData.fSpecularPower, c_rkMaterialData.bSphereMapIndex);
		}
	}
}

void CGrannyMaterialPalette::SetSpecularInfo(const char* c_szMtrlName, BOOL bEnable, float fPower)
{
	DWORD size=m_mtrlVector.size();
	DWORD i;
	if (c_szMtrlName)
	{
		for (i=0; i<size; ++i)
		{
			CGrannyMaterial::TRef& roMtrl=m_mtrlVector[i];

			int iStage;
			if (roMtrl->IsIn(c_szMtrlName, &iStage))
			{
				roMtrl->SetSpecularInfo(bEnable, fPower, 0);
				return;
			}
		}
	}
	else
	{
		for (i=0; i<size; ++i)
		{
			CGrannyMaterial::TRef& roMtrl=m_mtrlVector[i];
			roMtrl->SetSpecularInfo(bEnable, fPower, 0);
		}
	}
}

DWORD CGrannyMaterialPalette::RegisterMaterial(granny_material* pgrnMaterial)
{
	DWORD size=m_mtrlVector.size();
	DWORD i;
	for (i=0; i<size; ++i)
	{
		CGrannyMaterial::TRef& roMtrl=m_mtrlVector[i];
		if (roMtrl->IsEqual(pgrnMaterial))
			return i;
	}

	auto pkNewMtrl = std::make_unique<CGrannyMaterial>();
	pkNewMtrl->CreateFromGrannyMaterialPointer(pgrnMaterial);
	m_mtrlVector.push_back(pkNewMtrl.release());
	
	return size;
}

DWORD CGrannyMaterialPalette::GetMaterialCount() const
{
	return m_mtrlVector.size();
}

