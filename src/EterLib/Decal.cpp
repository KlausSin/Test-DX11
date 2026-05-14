#include "stdafx.h"
#include "Decal.h"
#include "StateManager.h"

using namespace DirectX;

CDecal::CDecal() : m_cfDecalEpsilon(0.25f)
{
	Clear();
}

CDecal::~CDecal()
{
	Clear();
}

void CDecal::Clear()
{
	m_v3Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_v3Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_v4LeftPlane = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_v4RightPlane = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_v4TopPlane = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_v4BottomPlane = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_v4FrontPlane = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_v4BackPlane = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

	m_dwVertexCount = 0;
	m_dwPrimitiveCount = 0;

	m_TriangleFanStructVector.clear();

	memset(m_Vertices, 0, sizeof(m_Vertices));
	memset(m_Indices, 0, sizeof(m_Indices));
}

void CDecal::ClipMesh(DWORD dwPrimitiveCount, const XMFLOAT3* c_pv3Vertex, const XMFLOAT3* c_pv3Normal)
{
	XMFLOAT3 v3NewVertex[9];
	XMFLOAT3 v3NewNormal[9];

	for (DWORD dwi = 0; dwi < dwPrimitiveCount; ++dwi)
	{
		const XMFLOAT3& v3_1 = c_pv3Vertex[3 * dwi];
		const XMFLOAT3& v3_2 = c_pv3Vertex[3 * dwi + 1];
		const XMFLOAT3& v3_3 = c_pv3Vertex[3 * dwi + 2];

		const XMVECTOR vv_ = XMLoadFloat3(&v3_2) - XMLoadFloat3(&v3_1);
		const XMVECTOR vv_2 = XMLoadFloat3(&v3_3) - XMLoadFloat3(&v3_1);
		const XMVECTOR v3Cross = XMVector3Cross(vv_, vv_2);

		if (XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_v3Normal), v3Cross)) > m_cfDecalEpsilon * XMVectorGetX(XMVector3Length(v3Cross)))
		{
			v3NewVertex[0] = v3_1;
			v3NewVertex[1] = v3_2;
			v3NewVertex[2] = v3_3;

			v3NewNormal[0] = c_pv3Normal[3 * dwi];
			v3NewNormal[1] = c_pv3Normal[3 * dwi + 1];
			v3NewNormal[2] = c_pv3Normal[3 * dwi + 2];

			uint32_t dwCount = ClipPolygon(3, v3NewVertex, v3NewNormal, v3NewVertex, v3NewNormal);
			if ((dwCount != 0) && (!AddPolygon(dwCount, v3NewVertex, v3NewNormal)))
				break;
		}
	}
}

bool CDecal::AddPolygon(DWORD dwAddCount, const XMFLOAT3* c_pv3Vertex, const XMFLOAT3* /*c_pv3Normal*/)
{
	if (m_dwVertexCount + dwAddCount >= MAX_DECAL_VERTICES)
		return false;

	TTRIANGLEFANSTRUCT aTriangleFanStruct;
	aTriangleFanStruct.m_wMinIndex = m_dwVertexCount;
	aTriangleFanStruct.m_dwVertexCount = dwAddCount;
	aTriangleFanStruct.m_dwPrimitiveCount = dwAddCount - 2;
	aTriangleFanStruct.m_dwVBOffset = m_dwVertexCount;

	m_TriangleFanStructVector.push_back(aTriangleFanStruct);

	DWORD dwCount = m_dwVertexCount;
	WORD* wIndex = m_Indices + dwCount;

	m_dwPrimitiveCount += dwAddCount - 2;

	for (DWORD dwVertexNum = 0; dwVertexNum < dwAddCount; ++dwVertexNum)
	{
		*wIndex++ = (WORD)dwCount;
		m_Vertices[dwCount].position = c_pv3Vertex[dwVertexNum];
		m_Vertices[dwCount].diffuse = 0xFFFFFFFF;
		++dwCount;
	}

	m_dwVertexCount = dwCount;
	return true;
}

DWORD CDecal::ClipPolygon(DWORD dwVertexCount, const XMFLOAT3* c_pv3Vertex, const XMFLOAT3* c_pv3Normal, XMFLOAT3* c_pv3NewVertex, XMFLOAT3* c_pv3NewNormal) const
{
	XMFLOAT3 v3TempVertex[9];
	XMFLOAT3 v3TempNormal[9];

	uint32_t dwCount = ClipPolygonAgainstPlane(m_v4LeftPlane, dwVertexCount, c_pv3Vertex, c_pv3Normal, v3TempVertex, v3TempNormal);
	if (dwCount != 0)
	{
		dwCount = ClipPolygonAgainstPlane(m_v4RightPlane, dwCount, v3TempVertex, v3TempNormal, c_pv3NewVertex, c_pv3NewNormal);
		if (dwCount != 0)
		{
			dwCount = ClipPolygonAgainstPlane(m_v4BottomPlane, dwCount, c_pv3NewVertex, c_pv3NewNormal, v3TempVertex, v3TempNormal);
			if (dwCount != 0)
			{
				dwCount = ClipPolygonAgainstPlane(m_v4TopPlane, dwCount, v3TempVertex, v3TempNormal, c_pv3NewVertex, c_pv3NewNormal);
				if (dwCount != 0)
				{
					dwCount = ClipPolygonAgainstPlane(m_v4BackPlane, dwCount, c_pv3NewVertex, c_pv3NewNormal, v3TempVertex, v3TempNormal);
					if (dwCount != 0)
						dwCount = ClipPolygonAgainstPlane(m_v4FrontPlane, dwCount, v3TempVertex, v3TempNormal, c_pv3NewVertex, c_pv3NewNormal);
				}
			}
		}
	}

	return dwCount;
}

DWORD CDecal::ClipPolygonAgainstPlane(const XMFLOAT4& c_rv4Plane, DWORD dwVertexCount, const XMFLOAT3* c_pv3Vertex, const XMFLOAT3* c_pv3Normal, XMFLOAT3* c_pv3NewVertex, XMFLOAT3* c_pv3NewNormal)
{
	bool bNegative[10];

	DWORD dwNegativeCount = 0;
	for (DWORD dwi = 0; dwi < dwVertexCount; ++dwi)
	{
		bool bNeg = XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&c_rv4Plane), XMLoadFloat3(&c_pv3Vertex[dwi]))) < 0.0f;
		bNegative[dwi] = bNeg;
		dwNegativeCount += bNeg;
	}

	if (dwNegativeCount == dwVertexCount)
		return 0;

	DWORD dwCount = 0;
	for (DWORD dwCurIndex = 0; dwCurIndex < dwVertexCount; ++dwCurIndex)
	{
		DWORD dwPrevIndex = (dwCurIndex != 0) ? dwCurIndex - 1 : dwVertexCount - 1;

		if (bNegative[dwCurIndex])
		{
			if (!bNegative[dwPrevIndex])
			{
				const XMFLOAT3& v3_1 = c_pv3Vertex[dwPrevIndex];
				const XMFLOAT3& v3_2 = c_pv3Vertex[dwCurIndex];

				float ft = XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&c_rv4Plane), XMLoadFloat3(&v3_1))) /
					(c_rv4Plane.x * (v3_1.x - v3_2.x) + c_rv4Plane.y * (v3_1.y - v3_2.y) + c_rv4Plane.z * (v3_1.z - v3_2.z));

				XMStoreFloat3(&c_pv3NewVertex[dwCount], XMLoadFloat3(&v3_1) * (1.0f - ft) + XMLoadFloat3(&v3_2) * ft);

				const XMFLOAT3& v3_n1 = c_pv3Normal[dwPrevIndex];
				const XMFLOAT3& v3_n2 = c_pv3Normal[dwCurIndex];

				XMStoreFloat3(&c_pv3NewNormal[dwCount], XMLoadFloat3(&v3_n1) * (1.0f - ft) + XMLoadFloat3(&v3_n2) * ft);
				++dwCount;
			}
		}
		else
		{
			if (bNegative[dwPrevIndex])
			{
				const XMFLOAT3& v3_1 = c_pv3Vertex[dwCurIndex];
				const XMFLOAT3& v3_2 = c_pv3Vertex[dwPrevIndex];

				float ft = XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&c_rv4Plane), XMLoadFloat3(&v3_1))) /
					(c_rv4Plane.x * (v3_1.x - v3_2.x) + c_rv4Plane.y * (v3_1.y - v3_2.y) + c_rv4Plane.z * (v3_1.z - v3_2.z));

				XMStoreFloat3(&c_pv3NewVertex[dwCount], XMLoadFloat3(&v3_1) * (1.0f - ft) + XMLoadFloat3(&v3_2) * ft);

				const XMFLOAT3& v3_n1 = c_pv3Normal[dwCurIndex];
				const XMFLOAT3& v3_n2 = c_pv3Normal[dwPrevIndex];

				XMStoreFloat3(&c_pv3NewNormal[dwCount], XMLoadFloat3(&v3_n1) * (1.0f - ft) + XMLoadFloat3(&v3_n2) * ft);
				++dwCount;
			}

			c_pv3NewVertex[dwCount] = c_pv3Vertex[dwCurIndex];
			c_pv3NewNormal[dwCount] = c_pv3Normal[dwCurIndex];
			++dwCount;
		}
	}

	return dwCount;
}

void CDecal::Render()
{
	XMFLOAT4X4 matWorld;
	XMStoreFloat4x4(&matWorld, XMMatrixIdentity());

	STATEMANAGER.GetTransform().SetWorld(matWorld);

	_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);

	for (uint32_t dwi = 0; dwi < m_TriangleFanStructVector.size(); ++dwi)
	{
		STATEMANAGER.DrawTriangleFan11(
			m_TriangleFanStructVector[dwi].m_dwPrimitiveCount,
			m_Vertices + m_TriangleFanStructVector[dwi].m_wMinIndex,
			sizeof(TPDTVertex)
		);
	}
}