#include "StdAfx.h"
#include "EterBase/Utils.h"
#include "AttributeInstance.h"
#include "GrpMath.h"

CDynamicPool<CAttributeInstance> CAttributeInstance::ms_kPool;

const float c_fStepSize = 50.0f;

bool CAttributeInstance::Picking(const XMFLOAT3& v, const XMFLOAT3& dir, float& out_x, float& out_y)
{
	if (IsEmpty())
		return false;

	bool bPicked = false;
	float nx = 0.0f;
	float ny = 0.0f;

	XMVECTOR rayOrigin = XMLoadFloat3(&v);
	XMVECTOR rayDir = XMLoadFloat3(&dir);

	for (DWORD i = 0; i < m_v3HeightDataVector.size(); ++i)
	{
		for (DWORD j = 0; j < m_v3HeightDataVector[i].size(); j += 3)
		{
			const XMFLOAT3& cv0 = m_v3HeightDataVector[i][j];
			const XMFLOAT3& cv2 = m_v3HeightDataVector[i][j + 1];
			const XMFLOAT3& cv1 = m_v3HeightDataVector[i][j + 2];

			XMVECTOR v0 = XMLoadFloat3(&cv0);
			XMVECTOR v1 = XMLoadFloat3(&cv1);
			XMVECTOR v2 = XMLoadFloat3(&cv2);

			XMVECTOR edge01 = v1 - v0;
			XMVECTOR edge02 = v2 - v0;
			XMVECTOR edge12 = v2 - v1;

			XMVECTOR n = XMVector3Cross(edge01, edge02);

			float denom = XMVectorGetX(XMVector3Dot(rayDir, n));
			if (fabsf(denom) < FLT_EPSILON)
				continue;

			float t = -XMVectorGetX(XMVector3Dot(rayOrigin - v0, n)) / denom;

			XMVECTOR x = rayOrigin + rayDir * t;

			XMVECTOR x0 = x - v0;
			XMVECTOR x1 = x - v1;
			XMVECTOR x2 = x - v2;

			XMVECTOR temp = XMVector3Cross(edge01, x0);
			if (XMVectorGetX(XMVector3Dot(temp, n)) < 0.0f)
				continue;

			temp = XMVector3Cross(edge12, x1);
			if (XMVectorGetX(XMVector3Dot(temp, n)) < 0.0f)
				continue;

			temp = XMVector3Cross(v0 - v2, x2);
			if (XMVectorGetX(XMVector3Dot(temp, n)) < 0.0f)
				continue;

			XMFLOAT3 hit;
			XMStoreFloat3(&hit, x);

			if (bPicked)
			{
				float newDist = (v.x - hit.x) * (v.x - hit.x) + (v.y - hit.y) * (v.y - hit.y);
				float oldDist = (v.x - nx) * (v.x - nx) + (v.y - ny) * (v.y - ny);

				if (newDist < oldDist)
				{
					nx = hit.x;
					ny = hit.y;
				}
			}
			else
			{
				nx = hit.x;
				ny = hit.y;
				bPicked = true;
			}
		}
	}

	if (bPicked)
	{
		out_x = nx;
		out_y = ny;
	}

	return bPicked;
}

BOOL CAttributeInstance::GetHeight(float fx, float fy, float* pfHeight)
{
	if (IsEmpty())
		return FALSE;

	fy *= -1.0f;

	if (!IsInHeight(fx, fy))
		return FALSE;

	BOOL bFlag = FALSE;

	for (DWORD i = 0; i < m_v3HeightDataVector.size(); ++i)
	{
		for (DWORD j = 0; j < m_v3HeightDataVector[i].size(); j += 3)
		{
			const XMFLOAT3& c_rv3Vertex0 = m_v3HeightDataVector[i][j];
			const XMFLOAT3& c_rv3Vertex1 = m_v3HeightDataVector[i][j + 1];
			const XMFLOAT3& c_rv3Vertex2 = m_v3HeightDataVector[i][j + 2];

			if (
				fx < c_rv3Vertex0.x && fx < c_rv3Vertex1.x && fx < c_rv3Vertex2.x ||
				fx > c_rv3Vertex0.x && fx > c_rv3Vertex1.x && fx > c_rv3Vertex2.x ||
				fy < c_rv3Vertex0.y && fy < c_rv3Vertex1.y && fy < c_rv3Vertex2.y ||
				fy > c_rv3Vertex0.y && fy > c_rv3Vertex1.y && fy > c_rv3Vertex2.y
				)
				continue;

			if (IsInTriangle2D(
				c_rv3Vertex0.x, c_rv3Vertex0.y,
				c_rv3Vertex1.x, c_rv3Vertex1.y,
				c_rv3Vertex2.x, c_rv3Vertex2.y,
				fx, fy))
			{
				XMFLOAT3 v3Line1 = {
					c_rv3Vertex1.x - c_rv3Vertex0.x,
					c_rv3Vertex1.y - c_rv3Vertex0.y,
					c_rv3Vertex1.z - c_rv3Vertex0.z
				};

				XMFLOAT3 v3Line2 = {
					c_rv3Vertex2.x - c_rv3Vertex0.x,
					c_rv3Vertex2.y - c_rv3Vertex0.y,
					c_rv3Vertex2.z - c_rv3Vertex0.z
				};

				XMFLOAT3 v3Cross;
				XMStoreFloat3(
					&v3Cross,
					XMVector3Normalize(
						XMVector3Cross(
							XMLoadFloat3(&v3Line1),
							XMLoadFloat3(&v3Line2)
						)
					)
				);

				if (v3Cross.z != 0.0f)
				{
					float fd = v3Cross.x * c_rv3Vertex0.x + v3Cross.y * c_rv3Vertex0.y + v3Cross.z * c_rv3Vertex0.z;
					float fm = v3Cross.x * fx + v3Cross.y * fy;

					*pfHeight = fMAX((fd - fm) / v3Cross.z, *pfHeight);
					bFlag = TRUE;
				}
			}
		}
	}

	return bFlag;
}

CAttributeData * CAttributeInstance::GetObjectPointer() const
{
	return m_roAttributeData.GetPointer();
}
BOOL CAttributeInstance::IsInHeight(float fx, float fy)
{
	float fdx = m_matGlobal._41 - fx;
	float fdy = m_matGlobal._42 - fy;
	if (sqrtf(fdx*fdx + fdy*fdy) > m_fHeightRadius)
		return FALSE;

	return TRUE;
}

void CAttributeInstance::SetObjectPointer(CAttributeData * pAttributeData)
{
	Clear();
	m_roAttributeData.SetPointer(pAttributeData);
}

void CAttributeInstance::RefreshObject(const XMFLOAT4X4& c_rmatGlobal)
{
	assert(!m_roAttributeData.IsNull());

	m_matGlobal = c_rmatGlobal;

	DWORD dwHeightDataCount = m_roAttributeData->GetHeightDataCount();

	m_v3HeightDataVector.clear();
	m_v3HeightDataVector.resize(dwHeightDataCount);

	m_fHeightRadius = 0.0f;

	for (DWORD i = 0; i < dwHeightDataCount; ++i)
	{
		const THeightData* c_pHeightData = nullptr;

		if (!m_roAttributeData->GetHeightDataPointer(i, &c_pHeightData) || c_pHeightData->v3VertexVector.empty())
			continue;

		DWORD dwVertexCount = c_pHeightData->v3VertexVector.size();

		m_v3HeightDataVector[i].resize(dwVertexCount);

		for (DWORD j = 0; j < dwVertexCount; ++j)
		{
			const XMFLOAT3& src = c_pHeightData->v3VertexVector[j];

			XMFLOAT4 v4Out;

			XMStoreFloat4(
				&v4Out,
				XMVector3Transform(
					XMLoadFloat3(&src),
					XMLoadFloat4x4(&m_matGlobal)
				)
			);

			m_v3HeightDataVector[i][j] = XMFLOAT3(v4Out.x, v4Out.y, v4Out.z);

			m_fHeightRadius = fMAX(m_fHeightRadius, fabsf(v4Out.x) + 200.0f);
			m_fHeightRadius = fMAX(m_fHeightRadius, fabsf(v4Out.y) + 200.0f);
			m_fHeightRadius = fMAX(m_fHeightRadius, fabsf(v4Out.z) + 200.0f);
		}
	}

	if (m_fHeightRadius < 1000.0f)
		m_fHeightRadius = 10000.0f;

	Tracef("RefreshObject() - Height blocks: %u | Radius: %.1f\n", dwHeightDataCount, m_fHeightRadius);
}

const char * CAttributeInstance::GetDataFileName() const
{
	return m_roAttributeData->GetFileName();
}

void CAttributeInstance::CreateSystem(UINT uCapacity)
{
	ms_kPool.Create(uCapacity);
}

void CAttributeInstance::DestroySystem()
{
	ms_kPool.Destroy();
}

CAttributeInstance* CAttributeInstance::New()
{
	return ms_kPool.Alloc();
}

void CAttributeInstance::Delete(CAttributeInstance* pkInst)
{
	ms_kPool.Free(pkInst);
}

BOOL CAttributeInstance::IsEmpty() const
{
	if (!m_v3HeightDataVector.empty())
		return FALSE;

	return TRUE;
}

void CAttributeInstance::Clear()
{
	m_fHeightRadius = 0.0f;
	m_fCollisionRadius = 0.0f;

	XMStoreFloat4x4(&m_matGlobal, XMMatrixIdentity());

	m_v3HeightDataVector.clear();

	m_roAttributeData.SetPointer(NULL);
}

CAttributeInstance::CAttributeInstance()
{
}
CAttributeInstance::~CAttributeInstance()
{
}
