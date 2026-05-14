#include "StdAfx.h"
#include "SnowParticle.h"

const float c_fSnowDistance = 70000.0f;

std::vector<CSnowParticle*> CSnowParticle::ms_kVct_SnowParticlePool;

void CSnowParticle::SetCameraVertex(const XMFLOAT3& rv3Up, const XMFLOAT3& rv3Cross)
{
	XMVECTOR up = XMLoadFloat3(&rv3Up);
	XMVECTOR cr = XMLoadFloat3(&rv3Cross);

	XMStoreFloat3(&m_v3Up, up * m_fHalfWidth);
	XMStoreFloat3(&m_v3Cross, cr * m_fHalfHeight);
}

bool CSnowParticle::IsActivate()
{
	return m_bActivate;
}

void CSnowParticle::Update(float fElapsedTime, const XMFLOAT3& c_rv3Pos)
{
	XMStoreFloat3(&m_v3Position,
		XMLoadFloat3(&m_v3Position) +
		XMLoadFloat3(&m_v3Velocity) * fElapsedTime);

	m_v3Position.x += m_v3Cross.x * sinf(m_fcurRadian) / 10.0f;
	m_v3Position.y += m_v3Cross.y * sinf(m_fcurRadian) / 10.0f;

	m_fcurRadian += m_fPeriod * fElapsedTime;

	if (m_v3Position.z < c_rv3Pos.z - 500.0f)
		m_bActivate = false;
	else if (fabs(m_v3Position.x - c_rv3Pos.x) > c_fSnowDistance)
		m_bActivate = false;
	else if (fabs(m_v3Position.y - c_rv3Pos.y) > c_fSnowDistance)
		m_bActivate = false;
}

void CSnowParticle::GetVerticies(SParticleVertex& v1, SParticleVertex& v2, SParticleVertex& v3, SParticleVertex& v4)
{
	XMVECTOR pos = XMLoadFloat3(&m_v3Position);
	XMVECTOR cr = XMLoadFloat3(&m_v3Cross);
	XMVECTOR up = XMLoadFloat3(&m_v3Up);

	XMFLOAT3 p;

	XMStoreFloat3(&p, pos - cr - up);
	v1.v3Pos = p; v1.u = 0.0f; v1.v = 0.0f;

	XMStoreFloat3(&p, pos + cr - up);
	v2.v3Pos = p; v2.u = 1.0f; v2.v = 0.0f;

	XMStoreFloat3(&p, pos - cr + up);
	v3.v3Pos = p; v3.u = 0.0f; v3.v = 1.0f;

	XMStoreFloat3(&p, pos + cr + up);
	v4.v3Pos = p; v4.u = 1.0f; v4.v = 1.0f;
}

void CSnowParticle::Init(const XMFLOAT3 & c_rv3Pos)
{
	float fRot = frandom(0.0f, 36000.0f) / 100.0f;
	float fDistance = frandom(0.0f, c_fSnowDistance) / 10.0f;

	m_v3Position.x = c_rv3Pos.x + fDistance*sin((double)XMConvertToRadians(fRot));
	m_v3Position.y = c_rv3Pos.y + fDistance*cos((double)XMConvertToRadians(fRot));
	m_v3Position.z = c_rv3Pos.z + frandom(1500.0f, 2000.0f);
	m_v3Velocity.x = 0.0f;
	m_v3Velocity.y = 0.0f;
	m_v3Velocity.z = frandom(-50.0f, -200.0f);
	m_fHalfWidth = frandom(2.0f, 7.0f);
	m_fHalfHeight = m_fHalfWidth;
	m_bActivate = true;
	m_bChangedSize = false;

	m_fPeriod = frandom(1.5f, 5.0f);
	m_fcurRadian = frandom(-1.6f, 1.6f);
	m_fAmplitude = frandom(1.0f, 3.0f);
}

CSnowParticle * CSnowParticle::New()
{
	if (ms_kVct_SnowParticlePool.empty())
	{
		return new CSnowParticle;
	}

	CSnowParticle * pParticle = ms_kVct_SnowParticlePool.back();
	ms_kVct_SnowParticlePool.pop_back();
	return pParticle;
}

void CSnowParticle::Delete(CSnowParticle * pSnowParticle)
{
	ms_kVct_SnowParticlePool.push_back(pSnowParticle);
}

void CSnowParticle::DestroyPool()
{
	stl_wipe(ms_kVct_SnowParticlePool);
}

CSnowParticle::CSnowParticle()
{
}
CSnowParticle::~CSnowParticle()
{
}
