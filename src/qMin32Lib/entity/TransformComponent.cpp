#include "pch.h"
#include "TransformComponent.h"

CTransformComponent::CTransformComponent()
	: m_position(0.0f, 0.0f, 0.0f),
	m_scale(1.0f, 1.0f, 1.0f),
	m_scalePosition(0.0f, 0.0f, 0.0f),
	m_yaw(0.0f),
	m_pitch(0.0f),
	m_roll(0.0f),
	m_dirty(true)
{
	XMStoreFloat4x4(&m_rotation, XMMatrixIdentity());
	XMStoreFloat4x4(&m_world, XMMatrixIdentity());

#ifdef ENABLE_OBJ_SCALLING
	XMStoreFloat4x4(&m_scaleMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_positionMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_transformMatrix, XMMatrixIdentity());
#endif
}

CTransformComponent::~CTransformComponent()
{
}

void CTransformComponent::OnCreateInternal()
{
	m_position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	m_scalePosition = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_yaw = 0.0f;
	m_pitch = 0.0f;
	m_roll = 0.0f;

	XMStoreFloat4x4(&m_rotation, XMMatrixIdentity());
	XMStoreFloat4x4(&m_world, XMMatrixIdentity());

#ifdef ENABLE_OBJ_SCALLING
	XMStoreFloat4x4(&m_scaleMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_positionMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_transformMatrix, XMMatrixIdentity());
#endif

	m_dirty = true;
}

void CTransformComponent::OnDestroyInternal()
{
}

const XMFLOAT3& CTransformComponent::GetPosition() const
{ 
	return m_position;
}

const XMFLOAT3& CTransformComponent::GetScale() const 
{ 
	return m_scale; 
}

const XMFLOAT3& CTransformComponent::GetScalePosition() const 
{
	return m_scalePosition;
}

float CTransformComponent::GetYaw() const 
{ 
	return m_yaw; 
}

float CTransformComponent::GetPitch() const
{ 
	return m_pitch; 
}

float CTransformComponent::GetRoll() const 
{ 
	return m_roll;
}

float CTransformComponent::GetRotation() const 
{ 
	return m_roll;
}

XMFLOAT4X4& CTransformComponent::GetWorldMatrix()
{
	UpdateMatrix();
	return m_world;
}

const XMFLOAT4X4& CTransformComponent::GetWorldMatrix() const
{
	return m_world;
}

XMFLOAT4X4& CTransformComponent::GetRotationMatrix()
{ 
	return m_rotation; 
}

const XMFLOAT4X4& CTransformComponent::GetRotationMatrix() const
{ 
	return m_rotation; 
}

#ifdef ENABLE_OBJ_SCALLING
XMFLOAT4X4& CTransformComponent::GetScaleMatrix()
{ 
	return m_scaleMatrix; 
}

const XMFLOAT4X4& CTransformComponent::GetScaleMatrix() const
{ 
	return m_scaleMatrix;
}

XMFLOAT4X4& CTransformComponent::GetPositionMatrix()
{ 
	return m_positionMatrix;
}

const XMFLOAT4X4& CTransformComponent::GetPositionMatrix() const 
{ 
	return m_positionMatrix;
}

XMFLOAT4X4& CTransformComponent::GetTransformMatrix()
{
	UpdateMatrix();
	return m_transformMatrix;
}

const XMFLOAT4X4& CTransformComponent::GetTransformMatrix() const
{
	return m_transformMatrix;
}
#endif

bool CTransformComponent::IsDirty() const
{ 
	return m_dirty;
}

void CTransformComponent::MarkDirty() 
{ 
	m_dirty = true;
}

void CTransformComponent::ClearDirty()
{ 
	m_dirty = false; 
}

void CTransformComponent::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3(x, y, z);
	m_dirty = true;
}

void CTransformComponent::SetPosition(const XMFLOAT3& pos)
{
	m_position = pos;
	m_dirty = true;
}

void CTransformComponent::AddPosition(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
	m_dirty = true;
}

void CTransformComponent::AddPosition(const XMFLOAT3& delta)
{
	AddPosition(delta.x, delta.y, delta.z);
}

void CTransformComponent::SetX(float x)
{
	m_position.x = x;
	m_dirty = true;
}

void CTransformComponent::SetY(float y)
{
	m_position.y = y;
	m_dirty = true;
}

void CTransformComponent::SetZ(float z)
{
	m_position.z = z;
	m_dirty = true;
}

void CTransformComponent::SetScale(float x, float y, float z)
{
	m_scale = XMFLOAT3(x, y, z);

#ifdef ENABLE_OBJ_SCALLING
	XMStoreFloat4x4(&m_scaleMatrix, XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z));
#endif

	m_dirty = true;
}

void CTransformComponent::SetScale(const XMFLOAT3& s)
{
	SetScale(s.x, s.y, s.z);
}

void CTransformComponent::SetScaleX(float x)
{
	SetScale(x, m_scale.y, m_scale.z);
}

void CTransformComponent::SetScaleY(float y)
{
	SetScale(m_scale.x, y, m_scale.z);
}

void CTransformComponent::SetScaleZ(float z)
{
	SetScale(m_scale.x, m_scale.y, z);
}

void CTransformComponent::SetScalePosition(float x, float y, float z)
{
	m_scalePosition = XMFLOAT3(x, y, z);

#ifdef ENABLE_OBJ_SCALLING
	XMStoreFloat4x4(&m_positionMatrix, XMMatrixTranslation(m_scalePosition.x, m_scalePosition.y, m_scalePosition.z));
#endif

	m_dirty = true;
}

void CTransformComponent::SetScalePosition(const XMFLOAT3& pos)
{
	SetScalePosition(pos.x, pos.y, pos.z);
}

void CTransformComponent::SetRotation(float r)
{
	m_yaw = 0.0f;
	m_pitch = 0.0f;
	m_roll = r;

	XMStoreFloat4x4(&m_rotation, XMMatrixRotationZ(XMConvertToRadians(r)));
	m_dirty = true;
}

void CTransformComponent::SetRotation(float y, float p, float r)
{
	m_yaw = y;
	m_pitch = p;
	m_roll = r;

	XMStoreFloat4x4(&m_rotation, XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_pitch), XMConvertToRadians(m_yaw), XMConvertToRadians(m_roll)));
	m_dirty = true;
}

void CTransformComponent::SetYaw(float y)
{
	SetRotation(y, m_pitch, m_roll);
}

void CTransformComponent::SetPitch(float p)
{
	SetRotation(m_yaw, p, m_roll);
}

void CTransformComponent::SetRoll(float r)
{
	SetRotation(m_yaw, m_pitch, r);
}

void CTransformComponent::AddRotation(float y, float p, float r)
{
	SetRotation(m_yaw + y, m_pitch + p, m_roll + r);
}

void CTransformComponent::SetRotationQuaternion(const XMFLOAT4& q)
{
	XMStoreFloat4x4(&m_rotation, XMMatrixRotationQuaternion(XMLoadFloat4(&q)));
	m_dirty = true;
}

void CTransformComponent::SetRotationMatrix(const XMFLOAT4X4& mat)
{
	m_rotation = mat;
	m_dirty = true;
}

void CTransformComponent::SetWorldMatrix(const XMFLOAT4X4& mat)
{
	m_world = mat;
	m_dirty = false;
}

#ifdef ENABLE_OBJ_SCALLING
void CTransformComponent::SetScaleMatrix(const XMFLOAT4X4& mat)
{
	m_scaleMatrix = mat;
	m_dirty = true;
}

void CTransformComponent::SetPositionMatrix(const XMFLOAT4X4& mat)
{
	m_positionMatrix = mat;
	m_dirty = true;
}

void CTransformComponent::SetTransformMatrix(const XMFLOAT4X4& mat)
{
	m_transformMatrix = mat;
	m_dirty = false;
}
#endif

void CTransformComponent::UpdateMatrix()
{
	if (!m_dirty)
		return;

#ifndef ENABLE_OBJ_SCALLING

	m_world = m_rotation;

	m_world._41 += m_position.x;
	m_world._42 += m_position.y;
	m_world._43 += m_position.z;

#else

	XMFLOAT4X4 tmp1;

	XMStoreFloat4x4(&tmp1, XMMatrixMultiply(XMLoadFloat4x4(&m_positionMatrix), XMLoadFloat4x4(&m_rotation)));

	m_world = tmp1;

	m_world._41 += m_position.x;
	m_world._42 += m_position.y;
	m_world._43 += m_position.z;

	XMStoreFloat4x4(&tmp1, XMMatrixMultiply(XMLoadFloat4x4(&m_positionMatrix), XMLoadFloat4x4(&m_scaleMatrix)));

	XMFLOAT4X4 tmp2;

	XMStoreFloat4x4(&tmp2, XMMatrixMultiply(XMLoadFloat4x4(&tmp1), XMLoadFloat4x4(&m_rotation)));

	m_transformMatrix = tmp2;

	m_transformMatrix._41 = m_scalePosition.x + m_position.x + m_transformMatrix._41;
	m_transformMatrix._42 = m_scalePosition.y + m_position.y + m_transformMatrix._42;
	m_transformMatrix._43 = m_scalePosition.z + m_position.z + m_transformMatrix._43;

#endif

	m_dirty = false;
}
