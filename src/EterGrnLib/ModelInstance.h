#pragma once

//#define CACHE_DEFORMED_VERTEX
#include "Eterlib/GrpImage.h"
#include "Eterlib/GrpCollisionObject.h"

#include "Model.h"
#include "Motion.h"
#include "qMin32Lib/Core.h"
#include <vector>

class CGrannyModelInstance : public CGraphicCollisionObject
{
public:
	enum
	{
		ANIFPS_MIN = 30,
		ANIFPS_MAX = 120,
	};
public:
	static void DestroySystem();

	static CGrannyModelInstance* New();
	static void Delete(CGrannyModelInstance* pkInst);

	static CDynamicPool<CGrannyModelInstance>		ms_kPool;

public:
	struct FCreateDeviceObjects
	{
		void operator() (CGrannyModelInstance* pModelInstance)
		{
			pModelInstance->CreateDeviceObjects();
		}
	};

	struct FDestroyDeviceObjects
	{
		void operator() (CGrannyModelInstance* pModelInstance)
		{
			pModelInstance->DestroyDeviceObjects();
		}
	};

public:
	CGrannyModelInstance();
	virtual ~CGrannyModelInstance();

	bool	IsEmpty();
	void	Clear();

	bool	CreateDeviceObjects();
	void	DestroyDeviceObjects();

	// Update & Render
	void	Update(DWORD dwAniFPS);
	void	UpdateLocalTime(float fElapsedTime);
	void	UpdateTransform(XMFLOAT4X4* pMatrix, float fSecondsElapsed);

	void	UpdateSkeleton(const XMFLOAT4X4* c_pWorldMatrix, float fLocalTime);
	void	DeformNoSkin(const XMFLOAT4X4* c_pWorldMatrix);
	void	Deform(const XMFLOAT4X4* c_pWorldMatrix);

	// FIXME : 현재는 하드웨어의 한계로 2장의 텍스춰로 제한이 되어있는 상태이기에 이런
	//         불안정한 아키텍춰가 가능하지만, 궁극적인 방향은 (모델 텍스춰 전부) + (효과용 텍스춰)
	//         이런식의 자동 셋팅이 이뤄져야 되지 않나 생각합니다. - [levites]
	// NOTE : 내부에 if문을 포함 시키기 보다는 조금은 번거롭지만 이렇게 함수 콜 자체를 분리
	//        시키는 것이 퍼포먼스 적인 측면에서는 더 나은 것 같습니다. - [levites]
	// NOTE : 건물은 무조건 OneTexture. 캐릭터는 경우에 따라 TwoTexture.
	void	RenderWithOneTexture(const RenderContext& ctx);
	void	RenderWithTwoTexture(const RenderContext& ctx);
	void	BlendRenderWithOneTexture(const RenderContext& ctx);
	void	BlendRenderWithTwoTexture(const RenderContext& ctx);
	void	RenderWithoutTexture(const RenderContext& ctx);
	void	RenderProjectedShadow(const RenderContext& ctx);

	// Model
	CGrannyModel* GetModel();
	void	SetMaterialImagePointer(const char* c_szImageName, CGraphicImage* pImage);
	void	SetMaterialData(const char* c_szImageName, const SMaterialData& c_rkMaterialData);
	void	SetSpecularInfo(const char* c_szMtrlName, BOOL bEnable, float fPower);

	void	SetMainModelPointer(CGrannyModel* pkModel);
	void	SetLinkedModelPointer(CGrannyModel* pkModel, CGrannyModelInstance** ppkSkeletonInst);

	// Motion
	void	SetMotionPointer(const CGrannyMotion* pMotion, float blendTime = 0.0f, int loopCount = 0, float speedRatio = 1.0f);
	void	ChangeMotionPointer(const CGrannyMotion* pMotion, int loopCount = 0, float speedRatio = 1.0f);
	void	SetMotionAtEnd();
	bool	IsMotionPlaying();

	void	CopyMotion(CGrannyModelInstance* pModelInstance, bool bIsFreeSourceControl = false);

	// Time
	void	SetLocalTime(float fLocalTime);
	int		ResetLocalTime();
	float	GetLocalTime();

	// WORK
	DWORD	GetDeformableVertexCount();
	DWORD	GetVertexCount();

	// END_OF_WORK

	// Bone & Attaching
	const float* GetBoneMatrixPointer(int iBone) const;
	const float* GetCompositeBoneMatrixPointer(int iBone) const;
	bool			GetMeshMatrixPointer(int iMesh, const XMFLOAT4X4** c_ppMatrix) const;
	bool			GetBoneIndexByName(const char* c_szBoneName, int* pBoneIndex) const;
	void			SetParentModelInstance(const CGrannyModelInstance* c_pParentModelInstance, const char* c_szBoneName);
	void			SetParentModelInstance(const CGrannyModelInstance* c_pParentModelInstance, int iBone);

	// Collision Detection
	bool	Intersect(const XMFLOAT4X4* c_pMatrix, float* pu, float* pv, float* pt);
	void	MakeBoundBox(TBoundBox* pBoundBox, const float* mat, const float* OBBMin, const float* OBBMax, XMFLOAT3* vtMin, XMFLOAT3* vtMax);
	void	GetBoundBox(XMFLOAT3* vtMin, XMFLOAT3* vtMax);

	// Reload Texture
	void	ReloadTexture();


protected:
	void	__Initialize();

	void	__DestroyModelInstance();
	void	__DestroyMeshMatrices();


	void	__CreateModelInstance();
	void	__CreateMeshMatrices();

	// WORK		
	void	__DestroyWorldPose();
	void	__CreateWorldPose(CGrannyModelInstance* pkSrcModelInst);

	bool	__CreateMeshBindingVector(CGrannyModelInstance* pkDstModelInst);
	void	__DestroyMeshBindingVector();

	int* __GetMeshBoneIndices(unsigned int iMeshBinding) const;


	granny_world_pose* __GetWorldPosePtr() const;
	// END_OF_WORK


	void	UpdateWorldPose();
	void	UpdateWorldMatrices(const XMFLOAT4X4* c_pWorldMatrix);


	enum class TextureMode { None, One, Two };
	void RenderMeshNodeList(const RenderContext& ctx, TextureMode textureMode, CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType);
	void RenderMeshNodeListWithOneTexture(const RenderContext& ctx, CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType);
	void RenderMeshNodeListWithTwoTexture(const RenderContext& ctx, CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType);
	void RenderMeshNodeListWithoutTexture(const RenderContext& ctx, CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType);
	bool	UploadMeshBonePaletteToShader(int iMesh);

protected:
	CGrannyModel* m_pModel;

	granny_model_instance* m_pgrnModelInstance;


	granny_control* m_pgrnCtrl;
	granny_animation* m_pgrnAni;

	std::vector<XMFLOAT4X4> m_meshMatrices;


	const CGrannyModelInstance* mc_pParentInstance;
	int								m_iParentBoneIndex;

	float							m_fLocalTime;
	float							m_fSecondsElapsed;

	DWORD							m_dwOldUpdateFrame;

	CGrannyMaterialPalette			m_kMtrlPal;

	granny_world_pose* m_pgrnWorldPoseReal;
	std::vector<granny_mesh_binding*>	m_vct_pgrnMeshBinding;

	CGrannyModelInstance** m_ppkSkeletonInst;
	SMaterialData material_data_;
#ifdef _TEST
	D3DXMATRIX TEST_matWorld;
#endif
public:
	bool							HaveBlendThing() { return m_pModel->HaveBlendThing(); }
};
