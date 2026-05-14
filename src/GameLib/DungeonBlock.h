#pragma once

#include "EterLib/ResourceManager.h"
#include "EterLib/GrpObjectInstance.h"
#include "EterGrnLib/ModelInstance.h"
#include "EterGrnLib/Thing.h"

class CDungeonModelInstance;

class CDungeonBlock : public CGraphicObjectInstance
{
	public:
		typedef std::vector<CDungeonModelInstance *> TModelInstanceContainer;
		enum
		{
			ID = THING_OBJECT
		};
		int GetType() const { return ID; }

	public:
		CDungeonBlock();
		virtual ~CDungeonBlock();

		void Destroy();

		void BuildBoundingSphere();
		bool Load(const char * c_szFileName);

		bool Intersect(float * pfu, float * pfv, float * pft);
		void GetBoundBox(XMFLOAT3 * pv3Min, XMFLOAT3 * pv3Max);

		void Update();
		void Render(const RenderContext& ctx);

		bool GetBoundingSphere(XMFLOAT3 & v3Center, float & fRadius);
		void OnUpdateCollisionData(const CStaticCollisionDataVector * pscdVector);
		void OnUpdateHeighInstance(CAttributeInstance * pAttributeInstance);
		bool OnGetObjectHeight(float fX, float fY, float * pfHeight);

		void OnRender(const RenderContext& ctx) {}
		void OnBlendRender(const RenderContext& ctx) {}
		void OnRenderToShadowMap(const RenderContext& ctx) {}
		void OnRenderShadow(const RenderContext& ctx);
		void OnRenderPCBlocker(const RenderContext& ctx) {}

	protected:
		void __Initialize();

	protected:
		XMFLOAT3 m_v3Center;
		float m_fRadius;

		CGraphicThing * m_pThing;
		TModelInstanceContainer m_ModelInstanceContainer;
		VBufferPtr	m_kDeformableVertexBuffer;
};
