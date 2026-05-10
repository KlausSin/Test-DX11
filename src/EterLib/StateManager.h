#ifndef __CSTATEMANAGER_H
#define __CSTATEMANAGER_H

#include "D3DXMathCompat.h"

#include <vector>
#include <stack>

#include "EterBase/Singleton.h"
#include "qMin32Lib/All.h"

class CD3D11Renderer;

#define CHECK_D3DAPI(a)		\
{							\
	HRESULT hr = (a);		\
							\
	if (hr != S_OK)			\
		assert(!#a);		\
}

static const DWORD STATEMANAGER_MAX_STAGES = 8;
static const DWORD STATEMANAGER_MAX_VCONSTANTS = 96;
static const DWORD STATEMANAGER_MAX_PCONSTANTS = 8;
static const DWORD STATEMANAGER_MAX_STREAMS = 16;


enum ETransform
{
	World = 0,
	View = 1,
	Projection = 2,
	Texture0 = 3,
	Texture1 = 4,
	Texture2 = 5,
	Texture3 = 6,
	COUNT = 7,
};

static constexpr UINT SM_MAX_TRANSFORMS = (UINT)ETransform::COUNT;

enum ESamplerStateType11
{
	SS11_MINFILTER = 0,
	SS11_MAGFILTER,
	SS11_MIPFILTER,
	SS11_ADDRESSU,
	SS11_ADDRESSV,
	SS11_ADDRESSW,
	SS11_MAX
};

enum ETextureFilterType11
{
	TF11_NONE = 0,
	TF11_POINT,
	TF11_LINEAR,
	TF11_ANISOTROPIC
};

enum ERenderState11
{
	RS11_ALPHABLENDENABLE = 0,
	RS11_SRCBLEND,
	RS11_DESTBLEND,
	RS11_BLENDOP,

	RS11_ALPHATESTENABLE,
	RS11_ALPHAREF,

	RS11_ZENABLE,
	RS11_ZWRITEENABLE,
	RS11_ZFUNC,

	RS11_STENCILENABLE,

	RS11_CULLMODE,
	RS11_FILLMODE,
	RS11_SCISSORTESTENABLE,
	RS11_COLORWRITEENABLE,

	RS11_LIGHTING,
	RS11_AMBIENT,

	RS11_FOGENABLE,
	RS11_FOGCOLOR,
	RS11_FOGSTART,
	RS11_FOGEND,

	RS11_TEXTUREFACTOR,

	RS11_MAX
};
static DWORD gs_DefaultRenderStates[RS11_MAX];

static inline D3D11_FILTER MapSamplerFilter11(DWORD minFilter, DWORD magFilter, DWORD mipFilter)
{
	if (minFilter == TF11_ANISOTROPIC || magFilter == TF11_ANISOTROPIC)
		return D3D11_FILTER_ANISOTROPIC;

	const bool minLinear = (minFilter != TF11_POINT && minFilter != TF11_NONE);
	const bool magLinear = (magFilter != TF11_POINT && magFilter != TF11_NONE);
	const bool mipLinear = (mipFilter != TF11_POINT && mipFilter != TF11_NONE);

	if (minLinear && magLinear && mipLinear)   return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	if (minLinear && magLinear && !mipLinear)  return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	if (minLinear && !magLinear && mipLinear)  return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
	if (minLinear && !magLinear && !mipLinear) return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	if (!minLinear && magLinear && mipLinear)  return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
	if (!minLinear && magLinear && !mipLinear) return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
	if (!minLinear && !magLinear && mipLinear) return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	return D3D11_FILTER_MIN_MAG_MIP_POINT;
}

class CStreamData
{
public:
	CStreamData(ID3D11Buffer* pStreamData = NULL, UINT Stride = 0) : m_lpStreamData(pStreamData), m_Stride(Stride)
	{
	}

	bool operator == (const CStreamData& rhs) const
	{
		return ((m_lpStreamData == rhs.m_lpStreamData) && (m_Stride == rhs.m_Stride));
	}

	ID3D11Buffer* m_lpStreamData;
	UINT			m_Stride;
};

class CIndexData
{
public:
	CIndexData(ID3D11Buffer* pIndexData = NULL, UINT BaseVertexIndex = 0)
		: m_lpIndexData(pIndexData),
		m_BaseVertexIndex(BaseVertexIndex)
	{
	}

	bool operator == (const CIndexData& rhs) const
	{
		return ((m_lpIndexData == rhs.m_lpIndexData) && (m_BaseVertexIndex == rhs.m_BaseVertexIndex));
	}

	ID3D11Buffer* m_lpIndexData;
	UINT			m_BaseVertexIndex;
};

class CStateManagerState
{
public:
	CStateManagerState()
	{
	}

	void ResetState()
	{
		DWORD i, y;

		for (i = 0; i < RS11_MAX; i++)
			m_RenderStates[i] = gs_DefaultRenderStates[i];

		for (i = 0; i < STATEMANAGER_MAX_STAGES; i++)
		{
			m_SamplerStates[i][SS11_MINFILTER] = TF11_LINEAR;
			m_SamplerStates[i][SS11_MAGFILTER] = TF11_LINEAR;
			m_SamplerStates[i][SS11_MIPFILTER] = TF11_LINEAR;
			m_SamplerStates[i][SS11_ADDRESSU] = D3D11_TEXTURE_ADDRESS_WRAP;
			m_SamplerStates[i][SS11_ADDRESSV] = D3D11_TEXTURE_ADDRESS_WRAP;
			m_SamplerStates[i][SS11_ADDRESSW] = D3D11_TEXTURE_ADDRESS_WRAP;
		}

		for (i = 0; i < STATEMANAGER_MAX_STREAMS; i++)
			m_StreamData[i] = CStreamData();

		m_IndexData = CIndexData();

		for (i = 0; i < STATEMANAGER_MAX_STAGES; i++)
			m_Textures[i] = NULL;

		for (i = 0; i < SM_MAX_TRANSFORMS; i++)
			D3DXMatrixIdentity(&m_Matrices[i]);

		m_bVertexProcessing = FALSE;
	}

	DWORD					m_RenderStates[RS11_MAX];
	DWORD					m_SamplerStates[STATEMANAGER_MAX_STAGES][SS11_MAX];

	ID3D11ShaderResourceView* m_Textures[STATEMANAGER_MAX_STAGES];

	D3DXMATRIX				m_Matrices[SM_MAX_TRANSFORMS];

	D3DMATERIAL11			m_D3DMaterial;

	CStreamData				m_StreamData[STATEMANAGER_MAX_STREAMS];
	CIndexData				m_IndexData;

	BOOL					m_bVertexProcessing;
};

class CStateManager : public CSingleton<CStateManager>
{
public:
	CStateManager();
	virtual ~CStateManager();

	void	SetDefaultState();
	void	Restore();

	bool	BeginScene();
	void	EndScene();

	void	SaveMaterial();
	void	SaveMaterial(const D3DMATERIAL11* pMaterial);
	void	RestoreMaterial();
	void	SetMaterial(const D3DMATERIAL11* pMaterial);
	void	GetMaterial(D3DMATERIAL11* pMaterial);

	void	SetLight(DWORD index, CONST D3DLIGHT11* pLight);
	void	GetLight(DWORD index, D3DLIGHT11* pLight);

	void	SetScissorRect(const RECT& c_rRect);
	void	GetScissorRect(RECT* pRect);

	void	SaveRenderState(ERenderState11 Type, DWORD dwValue);
	void	RestoreRenderState(ERenderState11 Type);
	void	SetRenderState(ERenderState11 Type, DWORD Value);
	void	GetRenderState(ERenderState11 Type, DWORD* pdwValue);

	void	SaveTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV);
	void	RestoreTexture(DWORD dwStage);
	void	SetTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV);
	void	GetTexture(DWORD dwStage, ID3D11ShaderResourceView** ppSRV);

	void	SetBestFiltering(DWORD dwStage);

	void	SaveSamplerState(DWORD dwStage, ESamplerStateType11 Type, DWORD dwValue);
	void	RestoreSamplerState(DWORD dwStage, ESamplerStateType11 Type);
	void	SetSamplerState(DWORD dwStage, ESamplerStateType11 Type, DWORD dwValue);
	void	GetSamplerState(DWORD dwStage, ESamplerStateType11 Type, DWORD* pdwValue);


	void SaveTransform(ETransform slot, const D3DXMATRIX* pMatrix);
	void RestoreTransform(ETransform slot);
	void SetTransform(ETransform slot, const D3DXMATRIX* pMatrix);
	void GetTransform(ETransform slot, D3DXMATRIX* pOut);

	void SaveStreamSource(UINT StreamNumber, ID3D11Buffer* pStreamData, UINT Stride);
	void RestoreStreamSource(UINT StreamNumber);
	void SetStreamSource(UINT StreamNumber, ID3D11Buffer* pStreamData, UINT Stride);

	void SaveIndices(ID3D11Buffer* pIndexData, UINT BaseVertexIndex);
	void RestoreIndices();
	void SetIndices(ID3D11Buffer* pIndexData, UINT BaseVertexIndex);

	HRESULT DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY Topology, UINT PrimitiveCount, UINT StartVertex_VertexStride, const void* pVertexData = nullptr);
	HRESULT DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY Topology, INT BaseVertexIndex, UINT StartIndex, UINT PrimitiveCount);
	HRESULT DrawTriangleFan11(UINT PrimitiveCount, const void* pVertexData, UINT VertexStride);

	DWORD GetRenderState(ERenderState11 Type);

	void StateManager_Capture();
	void StateManager_Apply();

	void SetD3D11Renderer(CD3D11Renderer* pRenderer);
	CD3D11Renderer* GetD3D11Renderer() { return m_pD3D11Renderer; }

#ifdef _DEBUG
	void ResetDrawCallCounter();
	int GetDrawCallCount() const;
#endif

private:
	CStateManagerState	m_CurrentState;
	CStateManagerState	m_CurrentState_Copy;

	bool				m_bForce;
	bool				m_bScene;
	DWORD				m_dwBestMinFilter;
	DWORD				m_dwBestMagFilter;
	CD3D11Renderer* m_pD3D11Renderer;

	std::vector<DWORD>						m_RenderStateStack[RS11_MAX];
	std::vector<DWORD>						m_SamplerStateStack[STATEMANAGER_MAX_STAGES][SS11_MAX];
	std::vector<D3DXMATRIX>					m_TransformStack[SM_MAX_TRANSFORMS];
	std::vector<ID3D11ShaderResourceView*>	m_TextureStack[STATEMANAGER_MAX_STAGES];
	std::vector<D3DMATERIAL11>				m_MaterialStack;
	std::vector<BOOL>						m_VertexProcessingStack;
	std::vector<CStreamData>				m_StreamStack[STATEMANAGER_MAX_STREAMS];
	std::vector<CIndexData>					m_IndexStack;

#ifdef _DEBUG
	int					m_iDrawCallCount;
	int					m_iLastDrawCallCount;
#endif
};

#define STATEMANAGER (CStateManager::Instance())

#endif __CSTATEMANAGER_H