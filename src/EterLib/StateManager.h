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


struct SStreamData
{
	ID3D11Buffer* pBuffer = nullptr;
	UINT stride = 0;
};

struct SIndexData
{
	ID3D11Buffer* pBuffer = nullptr;
	UINT baseVertexIndex = 0;
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

		for (i = 0; i < STATEMANAGER_MAX_STREAMS; i++)
			m_StreamData[i] = {};

		m_IndexData = {};

		for (i = 0; i < STATEMANAGER_MAX_STAGES; i++)
			m_Textures[i] = NULL;
	}

	ID3D11ShaderResourceView* m_Textures[STATEMANAGER_MAX_STAGES];

	SStreamData m_StreamData[STATEMANAGER_MAX_STREAMS];
	SIndexData m_IndexData;
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

	void	SetScissorRect(const RECT& c_rRect);
	void	GetScissorRect(RECT* pRect);

	void	SaveTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV);
	void	RestoreTexture(DWORD dwStage);
	void	SetTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV);
	void	GetTexture(DWORD dwStage, ID3D11ShaderResourceView** ppSRV);

	void	SetBestFiltering(DWORD dwStage);

	void SaveStreamSource(UINT StreamNumber, ID3D11Buffer* pStreamData, UINT Stride);
	void RestoreStreamSource(UINT StreamNumber);
	void SetStreamSource(UINT StreamNumber, ID3D11Buffer* pStreamData, UINT Stride);

	void SaveIndices(ID3D11Buffer* pIndexData, UINT BaseVertexIndex);
	void RestoreIndices();
	void SetIndices(ID3D11Buffer* pIndexData, UINT BaseVertexIndex);

	HRESULT DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY Topology, UINT PrimitiveCount, UINT StartVertex_VertexStride, const void* pVertexData = nullptr);
	HRESULT DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY Topology, INT BaseVertexIndex, UINT StartIndex, UINT PrimitiveCount);
	HRESULT DrawTriangleFan11(UINT PrimitiveCount, const void* pVertexData, UINT VertexStride);

	void StateManager_Capture();
	void StateManager_Apply();

	void SetD3D11Renderer(CD3D11Renderer* pRenderer);
	CD3D11Renderer* GetD3D11Renderer() { return m_pD3D11Renderer; }

	CD3D11StateCacheSet& GetStateCache();

	CD3D11SamplerStateCache& GetSampler();
	CD3D11RasterStateCache& GetRaster();
	CD3D11DepthStencilStateCache& GetDepthStencil();
	CD3D11BlendStateCache& GetBlend();
	CD3D11TransformStateCache& GetTransform();

#ifdef _DEBUG
	void ResetDrawCallCounter();
	int GetDrawCallCount() const;
#endif

private:

	CStateManagerState	m_CurrentState;
	CStateManagerState	m_CurrentState_Copy;

	bool				m_bForce;
	bool				m_bScene;
	CD3D11Renderer* m_pD3D11Renderer;

	std::vector<ID3D11ShaderResourceView*>	m_TextureStack[STATEMANAGER_MAX_STAGES];
	std::vector<SStreamData>				m_StreamStack[STATEMANAGER_MAX_STREAMS];
	std::vector<SIndexData>					m_IndexStack;

#ifdef _DEBUG
	int					m_iDrawCallCount;
	int					m_iLastDrawCallCount;
#endif
};

#define STATEMANAGER (CStateManager::Instance())

#endif __CSTATEMANAGER_H