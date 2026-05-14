#include "StdAfx.h"
#include "ParticleInstance.h"
#include "ParticleProperty.h"

#include "EterBase/Random.h"
#include "EterLib/Camera.h"
#include "EterLib/StateManager.h"

CDynamicPool<CParticleInstance> CParticleInstance::ms_kPool;

void CParticleInstance::DestroySystem()
{
	ms_kPool.Destroy();
}

CParticleInstance* CParticleInstance::New()
{
	return ms_kPool.Alloc();
}

void CParticleInstance::DeleteThis()
{
	Destroy();
	
	ms_kPool.Free(this);	
}



float CParticleInstance::GetRadiusApproximation()
{
	return m_v2HalfSize.y*m_v2Scale.y + m_v2HalfSize.x*m_v2Scale.x;
}


BOOL CParticleInstance::Update(float fElapsedTime, float fAngle)
{
	using namespace DirectX;

	m_fLastLifeTime -= fElapsedTime;
	if (m_fLastLifeTime < 0.0f)
		return FALSE;

	float fLifePercentage = (m_fLifeTime - m_fLastLifeTime) / m_fLifeTime;

	UpdateRotation(fLifePercentage, fElapsedTime);
	UpdateTextureAnimation(fLifePercentage, fElapsedTime);
	UpdateScale(fLifePercentage, fElapsedTime);
	UpdateColor(fLifePercentage, fElapsedTime);
	UpdateGravity(fLifePercentage, fElapsedTime);
	UpdateAirResistance(fLifePercentage, fElapsedTime);

	m_v3LastPosition = m_v3Position;

	m_v3Position.x += m_v3Velocity.x * fElapsedTime;
	m_v3Position.y += m_v3Velocity.y * fElapsedTime;
	m_v3Position.z += m_v3Velocity.z * fElapsedTime;

	if (fAngle)
	{
		if (m_pParticleProperty->m_bAttachFlag)
		{
			float angle = XMConvertToRadians(fAngle);
			float fCos = cosf(angle);
			float fSin = sinf(angle);

			float rx = m_v3Position.x - m_v3StartPosition.x;
			float ry = m_v3Position.y - m_v3StartPosition.y;

			m_v3Position.x = fCos * rx + fSin * ry + m_v3StartPosition.x;
			m_v3Position.y = -fSin * rx + fCos * ry + m_v3StartPosition.y;
		}
		else
		{
			XMVECTOR q = XMQuaternionRotationAxis(XMLoadFloat3(&m_pParticleProperty->m_v3ZAxis), XMConvertToRadians(fAngle));
			XMVECTOR qc = XMQuaternionConjugate(q);

			XMVECTOR qr = XMVectorSet(
				m_v3Position.x - m_v3StartPosition.x,
				m_v3Position.y - m_v3StartPosition.y,
				m_v3Position.z - m_v3StartPosition.z,
				0.0f);

			qr = XMQuaternionMultiply(q, qr);
			qr = XMQuaternionMultiply(qr, qc);

			XMFLOAT3 rotated;
			XMStoreFloat3(&rotated, qr);

			m_v3Position.x = rotated.x + m_v3StartPosition.x;
			m_v3Position.y = rotated.y + m_v3StartPosition.y;
			m_v3Position.z = rotated.z + m_v3StartPosition.z;
		}
	}

	return TRUE;
}

void CParticleInstance::UpdateRotation(float time, float elapsedTime)
{
	if (m_rotationType == CParticleProperty::ROTATION_TYPE_NONE)
		return;

	if (m_rotationType == CParticleProperty::ROTATION_TYPE_TIME_EVENT)
		m_fRotationSpeed = GetTimeEventBlendValue(time, m_pParticleProperty->m_TimeEventRotation);

	m_fRotation += m_fRotationSpeed * elapsedTime;
}

void CParticleInstance::UpdateTextureAnimation(float time, float elapsedTime)
{
	if (m_byTextureAnimationType == CParticleProperty::TEXTURE_ANIMATION_TYPE_NONE)
		return;

	const float frameDelay = m_pParticleProperty->GetTextureAnimationFrameDelay();
	const DWORD frameCount = m_pParticleProperty->GetTextureAnimationFrameCount();

	m_fFrameTime += elapsedTime;

	const uint64_t elapsedFrames = static_cast<uint64_t>(m_fFrameTime / frameDelay);

	if (0 == elapsedFrames)
		return;

	m_fFrameTime -= elapsedFrames * frameDelay;

	switch (m_byTextureAnimationType)
	{
	case CParticleProperty::TEXTURE_ANIMATION_TYPE_CW:
		m_byFrameIndex += elapsedFrames;
		if (m_byFrameIndex >= frameCount)
			m_byFrameIndex = 0;
		break;

	case CParticleProperty::TEXTURE_ANIMATION_TYPE_CCW:
		m_byFrameIndex = std::min<uint8_t>(m_byFrameIndex - elapsedFrames, frameCount - 1);
		break;

	case CParticleProperty::TEXTURE_ANIMATION_TYPE_RANDOM_FRAME:
		if (frameCount != 0)
			m_byFrameIndex = random_range(0, frameCount - 1);
		break;

	default:
		break;
	}
}

void CParticleInstance::UpdateScale(float time, float elapsedTime)
{
	if (!m_pParticleProperty->m_TimeEventScaleX.empty())
		m_v2Scale.x = GetTimeEventBlendValue(time, m_pParticleProperty->m_TimeEventScaleX);

	if (!m_pParticleProperty->m_TimeEventScaleY.empty())
		m_v2Scale.y = GetTimeEventBlendValue(time, m_pParticleProperty->m_TimeEventScaleY);
}

void CParticleInstance::UpdateColor(float time, float elapsedTime)
{
	if (m_pParticleProperty->m_TimeEventColor.empty())
		return;

	m_Color = GetTimeEventBlendValue(time, m_pParticleProperty->m_TimeEventColor);
}

void CParticleInstance::UpdateGravity(float time, float elapsedTime)
{
	if (m_pParticleProperty->m_TimeEventGravity.empty())
		return;

	float fGravity;
	fGravity = GetTimeEventBlendValue(time, m_pParticleProperty->m_TimeEventGravity);

	m_v3Velocity.z -= fGravity * elapsedTime;
}

void CParticleInstance::UpdateAirResistance(float time, float elapsedTime)
{
	if (m_pParticleProperty->m_TimeEventAirResistance.empty())
		return;

	float fAirResistance = 1.0f - GetTimeEventBlendValue(time, m_pParticleProperty->m_TimeEventAirResistance);
	m_v3Velocity.x *= fAirResistance;
	m_v3Velocity.y *= fAirResistance;
	m_v3Velocity.z *= fAirResistance;
}

void CParticleInstance::Transform(const XMFLOAT4X4* c_matLocal)
{
	using namespace DirectX;

	_mgr->GetCbMgr()->SetTextureFactor(ColorToUint(m_Color));

	XMFLOAT3 v3Up;
	XMFLOAT3 v3Cross;

	if (!m_pParticleProperty->m_bStretchFlag)
	{
		CCamera* pCurrentCamera = CCameraManager::Instance().GetCurrentCamera();
		const XMFLOAT3& c_rv3Up = pCurrentCamera->GetUp();
		const XMFLOAT3& c_rv3Cross = pCurrentCamera->GetCross();

		switch (m_pParticleProperty->m_byBillboardType)
		{
		case BILLBOARD_TYPE_LIE:
		{
			float fCos = cosf(XMConvertToRadians(m_fRotation));
			float fSin = sinf(XMConvertToRadians(m_fRotation));

			v3Up = { fCos, -fSin, 0.0f };
			v3Cross = { fSin, fCos, 0.0f };
		}
		break;

		case BILLBOARD_TYPE_2FACE:
		case BILLBOARD_TYPE_3FACE:
		case BILLBOARD_TYPE_Y:
		{
			v3Up = { 0.0f, 0.0f, 1.0f };

			const XMFLOAT3& c_rv3View = pCurrentCamera->GetView();

			if (v3Up.x * c_rv3View.y - v3Up.y * c_rv3View.x < 0.0f)
			{
				v3Up.x *= -1.0f;
				v3Up.y *= -1.0f;
				v3Up.z *= -1.0f;
			}

			XMFLOAT3 viewXY = { c_rv3View.x, c_rv3View.y, 0.0f };
			XMStoreFloat3(&v3Cross, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&v3Up), XMLoadFloat3(&viewXY))));

			if (m_fRotation)
			{
				float fCos = -sinf(XMConvertToRadians(m_fRotation));
				float fSin = cosf(XMConvertToRadians(m_fRotation));

				XMFLOAT3 v3Temp = {
					v3Up.x * fCos - v3Cross.x * fSin,
					v3Up.y * fCos - v3Cross.y * fSin,
					v3Up.z * fCos - v3Cross.z * fSin
				};

				v3Cross = {
					v3Cross.x * fCos + v3Up.x * fSin,
					v3Cross.y * fCos + v3Up.y * fSin,
					v3Cross.z * fCos + v3Up.z * fSin
				};

				v3Up = v3Temp;
			}
		}
		break;

		case BILLBOARD_TYPE_ALL:
		default:
		{
			if (m_fRotation == 0.0f)
			{
				v3Up = { -c_rv3Cross.x, -c_rv3Cross.y, -c_rv3Cross.z };
				v3Cross = c_rv3Up;
			}
			else
			{
				const XMFLOAT3& c_rv3View = pCurrentCamera->GetView();

				XMVECTOR q = XMQuaternionRotationAxis(XMLoadFloat3(&c_rv3View), XMConvertToRadians(m_fRotation));
				XMVECTOR qc = XMQuaternionConjugate(q);

				XMVECTOR qrUp = XMVectorSet(-c_rv3Cross.x, -c_rv3Cross.y, -c_rv3Cross.z, 0.0f);
				qrUp = XMQuaternionMultiply(qc, qrUp);
				qrUp = XMQuaternionMultiply(qrUp, q);
				XMStoreFloat3(&v3Up, qrUp);

				XMVECTOR qrCross = XMVectorSet(c_rv3Up.x, c_rv3Up.y, c_rv3Up.z, 0.0f);
				qrCross = XMQuaternionMultiply(qc, qrCross);
				qrCross = XMQuaternionMultiply(qrCross, q);
				XMStoreFloat3(&v3Cross, qrCross);
			}
		}
		break;
		}
	}
	else
	{
		v3Up = {
			m_v3Position.x - m_v3LastPosition.x,
			m_v3Position.y - m_v3LastPosition.y,
			m_v3Position.z - m_v3LastPosition.z
		};

		if (c_matLocal)
			XMStoreFloat3(&v3Up, XMVector3TransformNormal(XMLoadFloat3(&v3Up), XMLoadFloat4x4(c_matLocal)));

		float length = XMVectorGetX(XMVector3Length(XMLoadFloat3(&v3Up)));

		if (length == 0.0f)
		{
			v3Up = { 0.0f, 0.0f, 1.0f };
		}
		else
		{
			float scale = (1.0f + logf(1.0f + length)) / length;
			v3Up.x *= scale;
			v3Up.y *= scale;
			v3Up.z *= scale;
		}

		CCamera* pCurrentCamera = CCameraManager::Instance().GetCurrentCamera();
		const XMFLOAT3& c_rv3View = pCurrentCamera->GetView();

		XMStoreFloat3(&v3Cross, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&v3Up), XMLoadFloat3(&c_rv3View))));
	}

	float crossScale = -(m_v2HalfSize.x * m_v2Scale.x);
	float upScale = m_v2HalfSize.y * m_v2Scale.y;

	v3Cross = { v3Cross.x * crossScale, v3Cross.y * crossScale, v3Cross.z * crossScale };
	v3Up = { v3Up.x * upScale, v3Up.y * upScale, v3Up.z * upScale };

	XMFLOAT3 v3Position = m_v3Position;

	if (c_matLocal && m_pParticleProperty->m_bAttachFlag)
		XMStoreFloat3(&v3Position, XMVector3TransformCoord(XMLoadFloat3(&m_v3Position), XMLoadFloat4x4(c_matLocal)));

	m_ParticleMesh[0].position = { v3Position.x - v3Up.x + v3Cross.x, v3Position.y - v3Up.y + v3Cross.y, v3Position.z - v3Up.z + v3Cross.z };
	m_ParticleMesh[1].position = { v3Position.x - v3Up.x - v3Cross.x, v3Position.y - v3Up.y - v3Cross.y, v3Position.z - v3Up.z - v3Cross.z };
	m_ParticleMesh[2].position = { v3Position.x + v3Up.x + v3Cross.x, v3Position.y + v3Up.y + v3Cross.y, v3Position.z + v3Up.z + v3Cross.z };
	m_ParticleMesh[3].position = { v3Position.x + v3Up.x - v3Cross.x, v3Position.y + v3Up.y - v3Cross.y, v3Position.z + v3Up.z - v3Cross.z };
}


void CParticleInstance::Transform(const XMFLOAT4X4* c_matLocal, const float c_fZRotation)
{
	using namespace DirectX;

	_mgr->GetCbMgr()->SetTextureFactor(ColorToUint(m_Color));

	XMFLOAT3 v3Up;
	XMFLOAT3 v3Cross;

	if (!m_pParticleProperty->m_bStretchFlag)
	{
		CCamera* pCurrentCamera = CCameraManager::Instance().GetCurrentCamera();
		const XMFLOAT3& c_rv3Up = pCurrentCamera->GetUp();
		const XMFLOAT3& c_rv3Cross = pCurrentCamera->GetCross();

		switch (m_pParticleProperty->m_byBillboardType)
		{
		case BILLBOARD_TYPE_LIE:
		{
			float fCos = cosf(XMConvertToRadians(m_fRotation));
			float fSin = sinf(XMConvertToRadians(m_fRotation));

			v3Up = { fCos, -fSin, 0.0f };
			v3Cross = { fSin, fCos, 0.0f };
		}
		break;

		case BILLBOARD_TYPE_2FACE:
		case BILLBOARD_TYPE_3FACE:
		case BILLBOARD_TYPE_Y:
		{
			v3Up = { 0.0f, 0.0f, 1.0f };

			const XMFLOAT3& c_rv3View = pCurrentCamera->GetView();

			if (v3Up.x * c_rv3View.y - v3Up.y * c_rv3View.x < 0.0f)
			{
				v3Up.x *= -1.0f;
				v3Up.y *= -1.0f;
				v3Up.z *= -1.0f;
			}

			XMVECTOR up = XMLoadFloat3(&v3Up);
			XMVECTOR view = XMVectorSet(c_rv3View.x, c_rv3View.y, 0.0f, 0.0f);
			XMVECTOR cross = XMVector3Normalize(XMVector3Cross(up, view));

			XMStoreFloat3(&v3Cross, cross);

			if (m_fRotation)
			{
				float fCos = -sinf(XMConvertToRadians(m_fRotation));
				float fSin = cosf(XMConvertToRadians(m_fRotation));

				XMFLOAT3 v3Temp = {
					v3Up.x * fCos - v3Cross.x * fSin,
					v3Up.y * fCos - v3Cross.y * fSin,
					v3Up.z * fCos - v3Cross.z * fSin
				};

				v3Cross = {
					v3Cross.x * fCos + v3Up.x * fSin,
					v3Cross.y * fCos + v3Up.y * fSin,
					v3Cross.z * fCos + v3Up.z * fSin
				};

				v3Up = v3Temp;
			}
		}
		break;

		case BILLBOARD_TYPE_ALL:
		default:
		{
			if (m_fRotation == 0.0f)
			{
				v3Up = { -c_rv3Cross.x, -c_rv3Cross.y, -c_rv3Cross.z };
				v3Cross = c_rv3Up;
			}
			else
			{
				const XMFLOAT3& c_rv3View = pCurrentCamera->GetView();

				XMVECTOR view = XMLoadFloat3(&c_rv3View);
				XMMATRIX matRotation = XMMatrixRotationAxis(view, XMConvertToRadians(m_fRotation));

				XMFLOAT3 negCross = { -c_rv3Cross.x, -c_rv3Cross.y, -c_rv3Cross.z };

				XMStoreFloat3(&v3Up, XMVector3TransformCoord(XMLoadFloat3(&negCross), matRotation));
				XMStoreFloat3(&v3Cross, XMVector3TransformCoord(XMLoadFloat3(&c_rv3Up), matRotation));
			}
		}
		break;
		}
	}
	else
	{
		v3Up = {
			m_v3Position.x - m_v3LastPosition.x,
			m_v3Position.y - m_v3LastPosition.y,
			m_v3Position.z - m_v3LastPosition.z
		};

		if (c_matLocal)
		{
			XMVECTOR up = XMLoadFloat3(&v3Up);
			XMMATRIX matLocal = XMLoadFloat4x4(c_matLocal);
			XMStoreFloat3(&v3Up, XMVector3TransformNormal(up, matLocal));
		}

		XMVECTOR up = XMLoadFloat3(&v3Up);
		float length = XMVectorGetX(XMVector3Length(up));

		if (length == 0.0f)
		{
			v3Up = { 0.0f, 0.0f, 1.0f };
		}
		else
		{
			float scale = (1.0f + logf(1.0f + length)) / length;
			v3Up.x *= scale;
			v3Up.y *= scale;
			v3Up.z *= scale;
		}

		CCamera* pCurrentCamera = CCameraManager::Instance().GetCurrentCamera();
		const XMFLOAT3& c_rv3View = pCurrentCamera->GetView();

		XMVECTOR cross = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&v3Up), XMLoadFloat3(&c_rv3View)));
		XMStoreFloat3(&v3Cross, cross);
	}

	if (c_fZRotation)
	{
		float x, y;
		float fCos = cosf(c_fZRotation);
		float fSin = sinf(c_fZRotation);

		x = v3Up.x;
		y = v3Up.y;
		v3Up.x = x * fCos - y * fSin;
		v3Up.y = y * fCos + x * fSin;

		x = v3Cross.x;
		y = v3Cross.y;
		v3Cross.x = x * fCos - y * fSin;
		v3Cross.y = y * fCos + x * fSin;
	}

	float crossScale = -(m_v2HalfSize.x * m_v2Scale.x);
	float upScale = m_v2HalfSize.y * m_v2Scale.y;

	v3Cross.x *= crossScale;
	v3Cross.y *= crossScale;
	v3Cross.z *= crossScale;

	v3Up.x *= upScale;
	v3Up.y *= upScale;
	v3Up.z *= upScale;

	XMFLOAT3 v3Position = m_v3Position;

	if (c_matLocal && m_pParticleProperty->m_bAttachFlag)
	{
		XMVECTOR pos = XMLoadFloat3(&m_v3Position);
		XMMATRIX matLocal = XMLoadFloat4x4(c_matLocal);
		XMStoreFloat3(&v3Position, XMVector3TransformCoord(pos, matLocal));
	}

	m_ParticleMesh[0].position = { v3Position.x - v3Up.x + v3Cross.x, v3Position.y - v3Up.y + v3Cross.y, v3Position.z - v3Up.z + v3Cross.z };
	m_ParticleMesh[1].position = { v3Position.x - v3Up.x - v3Cross.x, v3Position.y - v3Up.y - v3Cross.y, v3Position.z - v3Up.z - v3Cross.z };
	m_ParticleMesh[2].position = { v3Position.x + v3Up.x + v3Cross.x, v3Position.y + v3Up.y + v3Cross.y, v3Position.z + v3Up.z + v3Cross.z };
	m_ParticleMesh[3].position = { v3Position.x + v3Up.x - v3Cross.x, v3Position.y + v3Up.y - v3Cross.y, v3Position.z + v3Up.z - v3Cross.z };
}

void CParticleInstance::Destroy()
{
	__Initialize();
}

void CParticleInstance::__Initialize()
{
	m_v3Position = {0.0f, 0.0f, 0.0f};
	m_v3LastPosition = m_v3Position;
	m_v3Velocity = {0.0f, 0.0f, 0.0f};

	m_v2Scale = {1.0f, 1.0f};
	m_Color = {1.0f, 1.0f, 1.0f, 1.0f};

	m_byFrameIndex = 0;
	m_rotationType = CParticleProperty::ROTATION_TYPE_NONE;
	m_fFrameTime = 0;

	m_ParticleMesh[0].texCoord = {0.0f, 1.0f};
	m_ParticleMesh[1].texCoord = {0.0f, 0.0f};
	m_ParticleMesh[2].texCoord = {1.0f, 1.0f};
	m_ParticleMesh[3].texCoord = {1.0f, 0.0f};
}

CParticleInstance::CParticleInstance()
{
	__Initialize();
}

CParticleInstance::~CParticleInstance()
{
	Destroy();
}

TPTVertex * CParticleInstance::GetParticleMeshPointer()
{
	return m_ParticleMesh;
}
