#pragma once

#include "EterBase/Singleton.h"
#include "GrpBase.h"
#include "Util.h"
#include "Pool.h"
#include <deque>
#include <qMin32Lib/entity/LightComponent.h>

typedef DWORD TLightID;

enum ELightType
{
	LIGHT_TYPE_STATIC,
	LIGHT_TYPE_DYNAMIC,
};

class CLightBase
{
public:
	CLightBase() {}
	virtual ~CLightBase() {}

	void SetCurrentTime();

protected:
	static float ms_fCurTime;
};

class CLight : public CGraphicBase, public CLightBase
{
public:
	CLight();
	virtual ~CLight();

	void Initialize();
	void Clear();
	void Update();

	void SetParameter(TLightID id, const CLightComponent& light);

	void SetDistance(float fDistance);
	float GetDistance() const { return m_fDistance; }

	TLightID GetLightID() { return m_LightID; }

	BOOL isEdited() { return m_isEdited; }
	void SetDeviceLight(BOOL bActive);

	void SetDiffuseColor(float fr, float fg, float fb, float fa = 1.0f);
	void SetAmbientColor(float fr, float fg, float fb, float fa = 1.0f);
	void SetRange(float fRange);
	void SetPosition(float fx, float fy, float fz);

	const XMFLOAT3& GetPosition() const;

	void BlendDiffuseColor(const XMFLOAT4& c_rColor, float fBlendTime, float fDelayTime = 0.0f);
	void BlendAmbientColor(const XMFLOAT4& c_rColor, float fBlendTime, float fDelayTime = 0.0f);
	void BlendRange(float fRange, float fBlendTime, float fDelayTime = 0.0f);

	CLightComponent& LightComponent() { return m_d3dLight; }
	const CLightComponent& LightComponent() const { return m_d3dLight; }

private:
	TLightID m_LightID;
	CLightComponent m_d3dLight;

	BOOL m_isEdited;
	float m_fDistance;

	TTransitorColor m_DiffuseColorTransitor;
	TTransitorColor m_AmbientColorTransitor;
	TTransitorFloat m_RangeTransitor;
};

class CLightManager : public CGraphicBase, public CLightBase, public CSingleton<CLightManager>
{
public:
	enum
	{
		LIGHT_LIMIT_DEFAULT = 3,
	};

	typedef std::deque<TLightID> TLightIDDeque;
	typedef std::map<TLightID, CLight*> TLightMap;
	typedef std::vector<CLight*> TLightSortVector;

public:
	CLightManager();
	virtual ~CLightManager();

	void Destroy();
	void Initialize();

	void Update();
	void FlushLight();
	void RestoreLight();

	void RegisterLight(ELightType LightType, TLightID* poutLightID, const CLightComponent& LightData);
	CLight* GetLight(TLightID LightID);
	void DeleteLight(TLightID LightID);

	void SetCenterPosition(const XMFLOAT3& c_rv3Position);
	void SetLimitLightCount(DWORD dwLightCount);
	void SetSkipIndex(DWORD dwSkipIndex);

protected:
	TLightIDDeque m_NonUsingLightIDDeque;

	TLightMap m_LightMap;
	TLightSortVector m_LightSortVector;

	XMFLOAT3 m_v3CenterPosition;
	DWORD m_dwLimitLightCount;
	DWORD m_dwSkipIndex;

protected:
	TLightID NewLightID();
	void ReleaseLightID(TLightID LightID);

	CDynamicPool<CLight> m_LightPool;
};