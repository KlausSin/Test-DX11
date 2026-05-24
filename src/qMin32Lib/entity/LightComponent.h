#pragma once

#include "EntityComponent.h"
#include <DirectXMath.h>

using namespace DirectX;

enum ELightTypeN : uint32_t
{
	LIGHT_TYPE_POINT = 1,
	LIGHT_TYPE_SPOT = 2,
	LIGHT_TYPE_DIRECTIONAL = 3,
};

class CLightComponent : public CEntityComponent
{
public:
	CLightComponent();

	void OnCreateInternal() override;
	void OnDestroyInternal() override;

	void SetType(ELightTypeN type);
	void SetEnable(bool enable);

	void SetDiffuse(const XMFLOAT4& color);
	void SetSpecular(const XMFLOAT4& color);
	void SetAmbient(const XMFLOAT4& color);

	void SetPosition(const XMFLOAT3& pos);
	void SetDirection(const XMFLOAT3& dir);

	void SetRange(float range);
	void SetFalloff(float falloff);

	void SetAttenuation(float a0, float a1, float a2);

	void SetTheta(float theta);
	void SetPhi(float phi);

	ELightTypeN GetType() const;

	bool IsEnable() const;

	const XMFLOAT4& GetDiffuse() const;
	const XMFLOAT4& GetSpecular() const;
	const XMFLOAT4& GetAmbient() const;

	const XMFLOAT3& GetPosition() const;
	const XMFLOAT3& GetDirection() const;

	float GetRange() const;
	float GetFalloff() const;

	float GetAttenuation0() const;
	float GetAttenuation1() const;
	float GetAttenuation2() const;

	float GetTheta() const;
	float GetPhi() const;

private:
	ELightTypeN m_type;

	bool m_enable;

	XMFLOAT4 m_diffuse;
	XMFLOAT4 m_specular;
	XMFLOAT4 m_ambient;

	XMFLOAT3 m_position;
	XMFLOAT3 m_direction;

	float m_range;
	float m_falloff;

	float m_attenuation0;
	float m_attenuation1;
	float m_attenuation2;

	float m_theta;
	float m_phi;
};