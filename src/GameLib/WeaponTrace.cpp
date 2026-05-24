#include "StdAfx.h"
#include "EterLib/ResourceManager.h"
#include "EterLib/StateManager.h"

#include "WeaponTrace.h"

CDynamicPool<CWeaponTrace> CWeaponTrace::ms_kPool;

void CWeaponTrace::DestroySystem()
{
	ms_kPool.Destroy();
}

void CWeaponTrace::Delete(CWeaponTrace* pkWTDel)
{
	assert(pkWTDel!=NULL && "CWeaponTrace::Delete");

	pkWTDel->Clear();
	ms_kPool.Free(pkWTDel);
}

CWeaponTrace* CWeaponTrace::New()
{
	return ms_kPool.Alloc();
}

void CWeaponTrace::Update(float fReachScale)
{
	float fElapsedTime = CTimer::Instance().GetCurrentSecond() - m_fLastUpdate;
	m_fLastUpdate = CTimer::Instance().GetCurrentSecond();

	if (!m_pInstance)
		return;

	{
		TTimePointList::iterator it;

		for (it = m_ShortTimePointList.begin(); it != m_ShortTimePointList.end(); ++it)
		{
			it->first += fElapsedTime;
			if (it->first > m_fLifeTime) { it++; break; }
		}
		if (it != m_ShortTimePointList.end())
			m_ShortTimePointList.erase(it, m_ShortTimePointList.end());

		for (it = m_LongTimePointList.begin(); it != m_LongTimePointList.end(); ++it)
		{
			it->first += fElapsedTime;
			if (it->first > m_fLifeTime) { it++; break; }
		}
		if (it != m_LongTimePointList.end())
			m_LongTimePointList.erase(it, m_LongTimePointList.end());
	}

	if (m_isPlaying && m_fz >= 0.0001f)
	{
		XMFLOAT4X4* mat = nullptr;

		if (m_pInstance->GetCompositeBoneMatrix(m_dwModelInstanceIndex, m_iBoneIndex, &mat))
		{
			XMFLOAT4X4* bone = nullptr;;
			m_pInstance->GetBoneMatrix(m_dwModelInstanceIndex, m_iBoneIndex, &bone);

			mat->_41 = bone->_41;
			mat->_42 = bone->_42;
			mat->_43 = bone->_43;

			XMMATRIX m = XMLoadFloat4x4(mat);

			XMFLOAT4X4 matRotation;
			XMStoreFloat4x4(&matRotation, XMMatrixRotationZ(XMConvertToRadians(m_fRotation)));

			XMFLOAT4X4 matTranslation;
			XMStoreFloat4x4(&matTranslation,
				XMMatrixTranslation(0.0f, 0.0f, m_fLength * fReachScale));

			XMMATRIX point = m * XMLoadFloat4x4(&matRotation);

			XMFLOAT3 p;

			XMStoreFloat3(&p,
				XMVectorSet(
					m_fx + XMVectorGetX(point.r[3]),
					m_fy + XMVectorGetY(point.r[3]),
					m_fz + XMVectorGetZ(point.r[3]),
					0.0f));

			m_ShortTimePointList.push_front(TTimePoint(0.0f, p));

			point = XMLoadFloat4x4(&matTranslation) * point;

			XMStoreFloat3(&p,
				XMVectorSet(
					m_fx + XMVectorGetX(point.r[3]),
					m_fy + XMVectorGetY(point.r[3]),
					m_fz + XMVectorGetZ(point.r[3]),
					0.0f));

			m_LongTimePointList.push_front(TTimePoint(0.0f, p));
		}
	}
}

bool CWeaponTrace::BuildVertex()
{
	const int max_size = 300;

	float h[max_size];
	float stk[max_size];
	int sp = 0;
	XMFLOAT3 r[max_size];

	if (m_LongTimePointList.size() <= 1)
		return false;

	std::vector<TPDTVertex> shortV, longV;

	float length = std::min(m_fLifeTime, m_LongTimePointList.back().first);
	int n = (int)m_LongTimePointList.size() - 1;
	assert(n < max_size - 1);

	for (int loop = 0; loop <= 1; ++loop)
	{
		auto& Input = (loop ? m_LongTimePointList : m_ShortTimePointList);
		auto& Output = (loop ? longV : shortV);

		for (int i = 0; i < n; ++i)
		{
			h[i] = Input[i + 1].first - Input[i].first;
			XMVECTOR p0 = XMLoadFloat3(&Input[i].second);
			XMVECTOR p1 = XMLoadFloat3(&Input[i + 1].second);

			XMVECTOR diff = XMVectorScale(XMVectorSubtract(p1, p0), 3.0f / h[i]);

			XMStoreFloat3(&r[i], diff);
		}

		r[n] = XMFLOAT3(0, 0, 0);

		for (int i = n; i > 0; --i)
			r[i].x += r[i - 1].x, r[i].y += r[i - 1].y, r[i].z += r[i - 1].z;

		float rate = 0.5f;
		r[0].x *= 0.5f; r[0].y *= 0.5f; r[0].z *= 0.5f;

		stk[sp++] = rate;

		for (int i = 1; i < n; ++i)
		{
			r[i].x -= r[i - 1].x;
			r[i].y -= r[i - 1].y;
			r[i].z -= r[i - 1].z;

			rate = 1.0f / (4.0f - rate);

			r[i].x *= rate;
			r[i].y *= rate;
			r[i].z *= rate;

			stk[sp++] = rate;
		}

		r[n].x -= r[n - 1].x;
		r[n].y -= r[n - 1].y;
		r[n].z -= r[n - 1].z;

		rate = 1.0f / (2.0f - rate);

		r[n].x *= rate;
		r[n].y *= rate;
		r[n].z *= rate;

		for (int i = n - 1; i >= 0; --i)
		{
			float k = stk[--sp];
			r[i].x -= k * r[i + 1].x;
			r[i].y -= k * r[i + 1].y;
			r[i].z -= k * r[i + 1].z;
		}

		int base = 0;

		XMFLOAT3 a, b, c, d;

		XMFLOAT3 tmp;
		tmp.x = Input[1].second.x - Input[0].second.x;
		tmp.y = Input[1].second.y - Input[0].second.y;
		tmp.z = Input[1].second.z - Input[0].second.z;

		float timebase = 0.0f;
		float timenext = h[0];
		float dt = m_fSamplingTime;

		a = Input[0].second;
		b = r[0];

		c = XMFLOAT3(
			(3 * tmp.x - r[1].x * h[0] - 2 * h[0] * r[0].x) / (h[0] * h[0]),
			(3 * tmp.y - r[1].y * h[0] - 2 * h[0] * r[0].y) / (h[0] * h[0]),
			(3 * tmp.z - r[1].z * h[0] - 2 * h[0] * r[0].z) / (h[0] * h[0])
		);

		d = XMFLOAT3(
			(-2 * tmp.x + (r[1].x + r[0].x) * h[0]) / (h[0] * h[0] * h[0]),
			(-2 * tmp.y + (r[1].y + r[0].y) * h[0]) / (h[0] * h[0] * h[0]),
			(-2 * tmp.z + (r[1].z + r[0].z) * h[0]) / (h[0] * h[0] * h[0])
		);

		for (float t = 0; t <= length; t += dt)
		{
			while (t > timenext)
			{
				timebase = timenext;
				base++;

				if (base >= n) break;

				tmp.x = Input[base + 1].second.x - Input[base].second.x;
				tmp.y = Input[base + 1].second.y - Input[base].second.y;
				tmp.z = Input[base + 1].second.z - Input[base].second.z;

				a = Input[base].second;
				b = r[base];

				c = XMFLOAT3(
					(3 * tmp.x - r[base + 1].x * h[base] - 2 * h[base] * r[base].x) / (h[base] * h[base]),
					(3 * tmp.y - r[base + 1].y * h[base] - 2 * h[base] * r[base].y) / (h[base] * h[base]),
					(3 * tmp.z - r[base + 1].z * h[base] - 2 * h[base] * r[base].z) / (h[base] * h[base])
				);

				d = XMFLOAT3(
					(-2 * tmp.x + (r[base + 1].x + r[base].x) * h[base]) / (h[base] * h[base] * h[base]),
					(-2 * tmp.y + (r[base + 1].y + r[base].y) * h[base]) / (h[base] * h[base] * h[base]),
					(-2 * tmp.z + (r[base + 1].z + r[base].z) * h[base]) / (h[base] * h[base] * h[base])
				);

				timenext += h[base];
			}

			if (base > n) break;

			float cc = t - timebase;

			TPDTVertex v;

			float ttt = std::clamp((t + Input[0].first) / m_fLifeTime, 0.0f, 1.0f);

			v.diffuse = ColorToUint(XMFLOAT4(0.3f, 0.8f, 1.0f,
				loop ? std::clamp((1.0f - ttt) * (1.0f - ttt) / 2.5f - 0.1f, 0.0f, 1.0f) : 0.0f));

			v.position.x = a.x + cc * (b.x + cc * (c.x + cc * d.x));
			v.position.y = a.y + cc * (b.y + cc * (c.y + cc * d.y));
			v.position.z = a.z + cc * (b.z + cc * (c.z + cc * d.z));

			v.texCoord.x = t / m_fLifeTime;
			v.texCoord.y = loop ? 0.0f : 1.0f;

			Output.push_back(v);
		}
	}

	m_PDTVertexVector.clear();

	for (size_t i = 0; i < longV.size() && i < shortV.size(); ++i)
	{
		m_PDTVertexVector.push_back(longV[i]);
		m_PDTVertexVector.push_back(shortV[i]);
	}

	return true;
}

void CWeaponTrace::Render()
{
	if (!BuildVertex())
		return;

	if (m_PDTVertexVector.size() < 4)
		return;

	XMFLOAT4X4 matWorld;
	XMStoreFloat4x4(&matWorld, XMMatrixIdentity());

	STATEMANAGER.GetStateCache().Push();
	STATEMANAGER.GetTransform().SetWorld(matWorld);

	STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);

	STATEMANAGER.GetBlend().SetBlendEnable(true);
	STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);

	_mgr->GetCbMgr()->SetAlphaTestEnable(false);
	_mgr->GetCbMgr()->SetAlphaRef(0x00000011);

	STATEMANAGER.GetDepthStencil().SetDepthEnable(true);
	STATEMANAGER.GetDepthStencil().SetDepthFunc(D3D11_COMPARISON_LESS_EQUAL);
	STATEMANAGER.GetDepthStencil().SetDepthWriteEnable(false);

	_mgr->GetCbMgr()->SetEntityLightingEnable(FALSE);

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);

	_mgr->SetShader(VF_PDT, m_bUseTexture ? BLEND_UI_TEX : BLEND_UI_DIFFUSE);

	STATEMANAGER.DrawPrimitive11(
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		(UINT)(m_PDTVertexVector.size() - 2),
		sizeof(TPDTVertex),
		m_PDTVertexVector.data());

	STATEMANAGER.GetStateCache().Restore();

	_mgr->GetCbMgr()->SetEntityLightingEnable(TRUE);
}

void CWeaponTrace::UseAlpha()
{
	m_bUseTexture = false;
}

void CWeaponTrace::UseTexture()
{
	m_bUseTexture = true;
}

void CWeaponTrace::SetTexture(const char * c_szFileName)
{
	CGraphicImage * pImage = CResourceManager::Instance().GetTyped<CGraphicImage>("lot_ade10-2.tga");
	m_ImageInstance.SetImagePointer(pImage);
}

bool CWeaponTrace::SetWeaponInstance(CGraphicThingInstance* pInstance, DWORD dwModelIndex, const char* c_szBoneName)
{
	pInstance->Update();
	pInstance->DeformNoSkin();

	XMFLOAT3 v3Min, v3Max;
	if (!pInstance->GetBoundBox(dwModelIndex, &v3Min, &v3Max))
		return false;

	m_iBoneIndex = 0;
	m_dwModelInstanceIndex = dwModelIndex;
	m_pInstance = pInstance;

	XMFLOAT4X4* pmat = nullptr;
	pInstance->GetBoneMatrix(dwModelIndex, 0, &pmat);

	XMFLOAT3 bone(pmat->_41, pmat->_42, pmat->_43);

	XMVECTOR vBone = XMLoadFloat3(&bone);
	XMVECTOR vMin = XMLoadFloat3(&v3Min);
	XMVECTOR vMax = XMLoadFloat3(&v3Max);

	XMVECTOR d1 = vBone - vMin;
	XMVECTOR d2 = vBone - vMax;

	float len1 = XMVectorGetX(XMVector3LengthSq(d1));
	float len2 = XMVectorGetX(XMVector3LengthSq(d2));

	m_fLength = sqrtf(std::max(len1, len2));

	return true;
}

void CWeaponTrace::SetPosition(float fx, float fy, float fz)
{
	m_fx = fx;
	m_fy = fy;
	m_fz = fz;
}

void CWeaponTrace::SetRotation(float fRotation)
{
	m_fRotation = fRotation;
}

void CWeaponTrace::SetLifeTime(float fLifeTime)
{
	m_fLifeTime = fLifeTime;
}

void CWeaponTrace::SetSamplingTime(float fSamplingTime)
{
	m_fSamplingTime = fSamplingTime;
}

void CWeaponTrace::TurnOn()
{
	m_isPlaying = TRUE;
}
void CWeaponTrace::TurnOff()
{
	m_isPlaying = FALSE;
	//Clear();
}

void CWeaponTrace::Clear()
{
	//m_PDTVertexVector.clear();
	//m_CurvingTraceVector.clear();

	m_ShortTimePointList.clear();
	m_LongTimePointList.clear();
	Initialize();
}

void CWeaponTrace::Initialize()
{
	m_pInstance = NULL;
	m_dwModelInstanceIndex = 0;
	
	m_fx = 0.0f;
	m_fy = 0.0f;
	m_fz = 0.0f;
	m_fRotation = 0.0f;
	
	m_fLifeTime = 0.18f;
	//m_fLifeTime = 3.0f;
	m_fSamplingTime = 0.003f;
	//m_fLifeTime = 3.0f;
	//m_fSamplingTime = 0.003f;
	
	m_isPlaying = FALSE;
	
	m_bUseTexture = false;
	
	m_iBoneIndex = 0;
	
	m_fLastUpdate = CTimer::Instance().GetCurrentSecond();
	///////////////////////////////////////////////////////////////////////
	
	//const int c_iSplineCount = 8;
	//m_SplineValueVector.clear();
	//m_SplineValueVector.resize(c_iSplineCount);
	
	//for (int i = 0; i < c_iSplineCount; ++i)
	//{
	//	float fValue = float(i) / float(c_iSplineCount);
	//	m_SplineValueVector[i].fValue1 = fValue;
	//	m_SplineValueVector[i].fValue2 = fValue * fValue;
	//	m_SplineValueVector[i].fValue3 = fValue * fValue * fValue;
	//}

}

CWeaponTrace::CWeaponTrace()
{
	Initialize();
}
CWeaponTrace::~CWeaponTrace()
{
}
