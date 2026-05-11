#pragma once

#pragma warning(disable:4786)

#include <deque>
#include "Thing.h"
#include "ModelInstance.h"

class CGrannyLODController : public CGraphicBase
{
public:
	static void SetMinLODMode(bool isEnable);

public:
	struct FSetLocalTime
	{
		float fLocalTime;
		void operator() (CGrannyLODController* pController)
		{
			pController->SetLocalTime(fLocalTime);
		}
	};

	struct FUpdateTime
	{
		float fElapsedTime;

		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->UpdateTime(fElapsedTime);
		}
	};

	struct FUpdateLODLevel
	{
		float fDistanceFromCenter;
		float fDistanceFromCamera;

		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->UpdateLODLevel(fDistanceFromCenter, fDistanceFromCamera);
		}
	};

	struct FRenderWithOneTexture
	{
		const RenderFrameContext* pCtx = nullptr;

		FRenderWithOneTexture() = default;
		explicit FRenderWithOneTexture(const RenderFrameContext& ctx) : pCtx(&ctx) {}

		void operator() (CGrannyLODController* pController)
		{
			if (!pController->isModelInstance())
				return;

			pController->RenderWithOneTexture(*pCtx);
		}
	};

	struct FBlendRenderWithOneTexture
	{
		const RenderFrameContext* pCtx = nullptr;

		FBlendRenderWithOneTexture() = default;
		explicit FBlendRenderWithOneTexture(const RenderFrameContext& ctx) : pCtx(&ctx) {}

		void operator() (CGrannyLODController* pController)
		{
			if (!pController->isModelInstance())
				return;

			pController->BlendRenderWithOneTexture(*pCtx);
		}
	};

	struct FRenderWithTwoTexture
	{
		const RenderFrameContext* pCtx = nullptr;

		FRenderWithTwoTexture() = default;
		explicit FRenderWithTwoTexture(const RenderFrameContext& ctx) : pCtx(&ctx) {}

		void operator() (CGrannyLODController* pController)
		{
			if (!pController->isModelInstance())
				return;

			pController->RenderWithTwoTexture(*pCtx);
		}
	};

	struct FBlendRenderWithTwoTexture
	{
		const RenderFrameContext* pCtx = nullptr;

		FBlendRenderWithTwoTexture() = default;
		explicit FBlendRenderWithTwoTexture(const RenderFrameContext& ctx) : pCtx(&ctx) {}

		void operator() (CGrannyLODController* pController)
		{
			if (!pController->isModelInstance())
				return;

			pController->BlendRenderWithTwoTexture(*pCtx);
		}
	};

	struct FRenderToShadowMap
	{
		const RenderFrameContext* pCtx = nullptr;

		FRenderToShadowMap() = default;
		explicit FRenderToShadowMap(const RenderFrameContext& ctx) : pCtx(&ctx) {}

		void operator() (CGrannyLODController* pController)
		{
			if (!pController->isModelInstance())
				return;

			pController->RenderToShadowMap(*pCtx);
		}
	};

	struct FRenderShadow
	{
		const RenderFrameContext* pCtx = nullptr;

		FRenderShadow() = default;
		explicit FRenderShadow(const RenderFrameContext& ctx) : pCtx(&ctx) {}

		void operator() (CGrannyLODController* pController)
		{
			if (!pController->isModelInstance())
				return;

			pController->RenderShadow(*pCtx);
		}
	};

	struct FDeform
	{
		const D3DXMATRIX* mc_pWorldMatrix;

		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->Deform(mc_pWorldMatrix);
		}
	};
	struct FDeformNoSkin
	{
		const D3DXMATRIX* mc_pWorldMatrix;

		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->DeformNoSkin(mc_pWorldMatrix);
		}
	};
	struct FDeformAll
	{
		const D3DXMATRIX* mc_pWorldMatrix;

		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->DeformAll(mc_pWorldMatrix);
		}
	};

	struct FCreateDeviceObjects
	{
		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->CreateDeviceObjects();
		}
	};

	struct FDestroyDeviceObjects
	{
		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->DestroyDeviceObjects();
		}
	};

	struct FBoundBox
	{
		D3DXVECTOR3* m_vtMin;
		D3DXVECTOR3* m_vtMax;

		FBoundBox(D3DXVECTOR3* vtMin, D3DXVECTOR3* vtMax)
		{
			m_vtMin = vtMin;
			m_vtMax = vtMax;
		}

		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->GetBoundBox(m_vtMin, m_vtMax);
		}
	};

	struct FResetLocalTime
	{
		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->ResetLocalTime();
		}
	};

	struct FReloadTexture
	{
		void operator () (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->ReloadTexture();
		}
	};

	struct FSetMotionPointer
	{
		const CGrannyMotion* m_pMotion;
		float					m_speedRatio;
		float					m_blendTime;
		int						m_loopCount;

		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->SetMotionPointer(m_pMotion, m_blendTime, m_loopCount, m_speedRatio);
		}
	};

	struct FChangeMotionPointer
	{
		const CGrannyMotion* m_pMotion;
		float					m_speedRatio;
		int						m_loopCount;

		void operator() (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->ChangeMotionPointer(m_pMotion, m_loopCount, m_speedRatio);
		}
	};

	struct FEndStopMotionPointer
	{
		const CGrannyMotion* m_pMotion;

		void operator () (CGrannyLODController* pController)
		{
			if (pController->isModelInstance())
				pController->SetMotionAtEnd();
		}
	};

	CGrannyLODController();
	virtual ~CGrannyLODController();

	void	Clear();

	void	CreateDeviceObjects();
	void	DestroyDeviceObjects();

	void	AddModel(CGraphicThing* pThing, int iSrcModel, CGrannyLODController* pSkelLODController = nullptr);
	void	AttachModelInstance(CGrannyLODController* pSrcLODController, const char* c_szBoneName);
	void	DetachModelInstance(CGrannyLODController* pSrcLODController);
	void	SetLODLimits(float fNearLOD, float fFarLOD);
	void	SetLODLevel(BYTE bLODLevel);
	BYTE	GetLODLevel() const { return m_bLODLevel; }
	void	SetMaterialImagePointer(const char* c_szImageName, CGraphicImage* pImage);
	void	SetMaterialData(const char* c_szImageName, const SMaterialData& c_rkMaterialData);
	void	SetSpecularInfo(const char* c_szMtrlName, BOOL bEnable, float fPower);

	void	RenderWithOneTexture(const RenderFrameContext& ctx);
	void	RenderWithTwoTexture(const RenderFrameContext& ctx);
	void	BlendRenderWithOneTexture(const RenderFrameContext& ctx);
	void	BlendRenderWithTwoTexture(const RenderFrameContext& ctx);

	void	Update(float fElapsedTime, float fDistanceFromCenter, float fDistanceFromCamera);
	void	UpdateLODLevel(float fDistanceFromCenter, float fDistanceFromCamera);
	void	UpdateTime(float fElapsedTime);

	void	UpdateSkeleton(const D3DXMATRIX* c_pWorldMatrix, float fElapsedTime);
	void	Deform(const D3DXMATRIX* c_pWorldMatrix);
	void	DeformNoSkin(const D3DXMATRIX* c_pWorldMatrix);
	void	DeformAll(const D3DXMATRIX* c_pWorldMatrix);

	void	RenderToShadowMap(const RenderFrameContext& ctx);
	void	RenderShadow(const RenderFrameContext& ctx);
	void	ReloadTexture();

	void	GetBoundBox(D3DXVECTOR3* vtMin, D3DXVECTOR3* vtMax);
	bool	Intersect(const D3DXMATRIX* c_pMatrix, float* u, float* v, float* t);

	void	SetLocalTime(float fLocalTime);
	void	ResetLocalTime();

	void	SetMotionPointer(const CGrannyMotion* c_pMotion, float fBlendTime, int iLoopCount, float speedRatio);
	void	ChangeMotionPointer(const CGrannyMotion* c_pMotion, int iLoopCount, float speedRatio);
	void	SetMotionAtEnd();

	BOOL	isModelInstance();
	CGrannyModelInstance* GetModelInstance();
	bool	HaveBlendThing() const;

protected:
	void	SetCurrentModelInstance(CGrannyModelInstance* pgrnModelInstance);
	void	RefreshAttachedModelInstance();

protected:
	float								m_fLODDistance;
	DWORD								m_dwLODAniFPS;

	//// Attaching Link Data
	// Data of Parent Side
	struct TAttachingModelData
	{
		CGrannyLODController* pkLODController{};
		std::string strBoneName;
	};

	std::vector<TAttachingModelData>	m_AttachedModelDataVector;
	// Data of Child Side
	CGrannyLODController* m_pAttachedParentModel;

	BYTE								m_bLODLevel;
	CGrannyModelInstance* m_pCurrentModelInstance;

	std::deque<CGrannyModelInstance*>	m_que_pkModelInst;
};
