#pragma once

#include "EterBase/FileLoader.h"
#include "DirectXMath.h"
#include <algorithm>

using namespace DirectX;

template<typename T>
class CTransitor
{
public:
	CTransitor() {}
	~CTransitor() {}

	void SetActive(BOOL bActive = TRUE)
	{
		m_bActivated = bActive;
	}

	BOOL isActive()
	{
		return m_bActivated;
	}

	BOOL isActiveTime(float fcurTime)
	{
		return (fcurTime < m_fEndTime);
	}

	DWORD GetID()
	{
		return m_dwID;
	}

	void SetID(DWORD dwID)
	{
		m_dwID = dwID;
	}

	void SetSourceValue(const T& src)
	{
		m_SourceValue = src;
	}

	void SetTransition(const T& src, const T& dst, float start, float blend)
	{
		m_SourceValue = src;
		m_TargetValue = dst;
		m_fStartTime = start;
		m_fEndTime = start + blend;
	}

	BOOL GetValue(float fcurTime, T* pValue)
	{
		if (fcurTime <= m_fStartTime)
			return FALSE;

		float t = (fcurTime - m_fStartTime) / (m_fEndTime - m_fStartTime);
		t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);

		*pValue = m_SourceValue + (m_TargetValue - m_SourceValue) * t;
		return TRUE;
	}

protected:
	DWORD m_dwID = 0;
	BOOL  m_bActivated = FALSE;
	float m_fStartTime = 0.0f;
	float m_fEndTime = 0.0f;

	T m_SourceValue{};
	T m_TargetValue{};
};

template<>
inline BOOL CTransitor<XMFLOAT3>::GetValue(float fcurTime, XMFLOAT3* pValue)
{
	if (fcurTime <= m_fStartTime)
		return FALSE;

	float t = (fcurTime - m_fStartTime) / (m_fEndTime - m_fStartTime);
	t = std::clamp(t, 0.0f, 1.0f);

	XMVECTOR s = XMLoadFloat3(&m_SourceValue);
	XMVECTOR d = XMLoadFloat3(&m_TargetValue);
	XMVECTOR r = s + (d - s) * t;

	XMStoreFloat3(pValue, r);
	return TRUE;
}

template<>
inline BOOL CTransitor<XMFLOAT4>::GetValue(float fcurTime, XMFLOAT4* pValue)
{
	if (fcurTime <= m_fStartTime)
		return FALSE;

	float t = (fcurTime - m_fStartTime) / (m_fEndTime - m_fStartTime);
	t = std::clamp(t, 0.0f, 1.0f);

	XMVECTOR s = XMLoadFloat4(&m_SourceValue);
	XMVECTOR d = XMLoadFloat4(&m_TargetValue);
	XMVECTOR r = s + (d - s) * t;

	XMStoreFloat4(pValue, r);
	return TRUE;
}

typedef CTransitor<float>     TTransitorFloat;
typedef CTransitor<XMFLOAT3>   TTransitorVector3;
typedef CTransitor<XMFLOAT4>   TTransitorColor;

///////////////////////////////////////////////////////////////////////////////////////////////////

void PrintfTabs(FILE * File, int iTabCount, const char * c_szString, ...);


//typedef CTokenVector TTokenVector;

extern bool	LoadTextData(const char * c_szFileName, CTokenMap & rstTokenMap);
extern bool	LoadMultipleTextData(const char * c_szFileName, CTokenVectorMap & rstTokenVectorMap);

extern XMFLOAT3 TokenToVector(CTokenVector & rVector);
extern XMFLOAT4 TokenToColor(CTokenVector & rVector);

#define GOTO_CHILD_NODE(TextFileLoader, Index) CTextFileLoader::CGotoChild Child(TextFileLoader, Index);

extern DWORD GetMaxTextureWidth();
extern DWORD GetMaxTextureHeight();