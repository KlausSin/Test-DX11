#include "pch.h"
#include "LightComponent.h"

CLightComponent::CLightComponent()
	: m_type(LIGHT_TYPE_DIRECTIONAL),
	m_enable(true),
	m_diffuse(1.0f, 1.0f, 1.0f, 1.0f),
	m_specular(1.0f, 1.0f, 1.0f, 1.0f),
	m_ambient(0.0f, 0.0f, 0.0f, 1.0f),
	m_position(0.0f, 0.0f, 0.0f),
	m_direction(0.0f, 0.0f, 1.0f),
	m_range(1000.0f),
	m_falloff(1.0f),
	m_attenuation0(1.0f),
	m_attenuation1(0.0f),
	m_attenuation2(0.0f),
	m_theta(0.0f),
	m_phi(0.0f)
{
}

void CLightComponent::OnCreateInternal()
{
}

void CLightComponent::OnDestroyInternal()
{
}

void CLightComponent::SetType(ELightTypeN type)
{ 
	m_type = type;
}

void CLightComponent::SetEnable(bool enable)
{ 
	m_enable = enable; 
}

void CLightComponent::SetDiffuse(const XMFLOAT4& color) 
{ 
	m_diffuse = color; 
}

void CLightComponent::SetSpecular(const XMFLOAT4& color)
{
	m_specular = color;
}

void CLightComponent::SetAmbient(const XMFLOAT4& color) 
{
	m_ambient = color;
}

void CLightComponent::SetPosition(const XMFLOAT3& pos) 
{ 
	m_position = pos; 
}

void CLightComponent::SetDirection(const XMFLOAT3& dir) 
{
	m_direction = dir; 
}

void CLightComponent::SetRange(float range)
{ 
	m_range = range; 
}

void CLightComponent::SetFalloff(float falloff) 
{ 
	m_falloff = falloff; 
}

void CLightComponent::SetAttenuation(float a0, float a1, float a2)
{
	m_attenuation0 = a0;
	m_attenuation1 = a1;
	m_attenuation2 = a2;
}

void CLightComponent::SetTheta(float theta) 
{
	m_theta = theta;
}

void CLightComponent::SetPhi(float phi) 
{ 
	m_phi = phi; 
}

ELightTypeN CLightComponent::GetType() const
{ 
	return m_type; 
}

bool CLightComponent::IsEnable() const 
{ 
	return m_enable;
}

const XMFLOAT4& CLightComponent::GetDiffuse() const
{ 
	return m_diffuse;
}

const XMFLOAT4& CLightComponent::GetSpecular() const
{ 
	return m_specular;
}

const XMFLOAT4& CLightComponent::GetAmbient() const 
{ 
	return m_ambient; 
}

const XMFLOAT3& CLightComponent::GetPosition() const 
{ 
	return m_position; 
}

const XMFLOAT3& CLightComponent::GetDirection() const 
{ 
	return m_direction; 
}

float CLightComponent::GetRange() const
{ 
	return m_range; 
}

float CLightComponent::GetFalloff() const
{ 
	return m_falloff;
}

float CLightComponent::GetAttenuation0() const 
{ 
	return m_attenuation0;
}

float CLightComponent::GetAttenuation1() const
{ 
	return m_attenuation1;
}

float CLightComponent::GetAttenuation2() const 
{ 
	return m_attenuation2;
}

float CLightComponent::GetTheta() const 
{ 
	return m_theta;
}

float CLightComponent::GetPhi() const 
{
	return m_phi; 
}
