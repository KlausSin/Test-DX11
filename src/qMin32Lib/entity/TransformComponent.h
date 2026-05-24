#pragma once

#include "EntityComponent.h"
#include <DirectXMath.h>

using namespace DirectX;

class CTransformComponent : public CEntityComponent
{
public:
	CTransformComponent();
	virtual ~CTransformComponent();

	void OnCreateInternal() override;
	void OnDestroyInternal() override;

	const XMFLOAT3& GetPosition() const;
	const XMFLOAT3& GetScale() const;
	const XMFLOAT3& GetScalePosition() const;

	float GetYaw() const;
	float GetPitch() const;
	float GetRoll() const;
	float GetRotation() const;

	XMFLOAT4X4& GetWorldMatrix();
	const XMFLOAT4X4& GetWorldMatrix() const;

	XMFLOAT4X4& GetRotationMatrix();
	const XMFLOAT4X4& GetRotationMatrix() const;

#ifdef ENABLE_OBJ_SCALLING
	XMFLOAT4X4& GetScaleMatrix();
	const XMFLOAT4X4& GetScaleMatrix() const;
	XMFLOAT4X4& GetPositionMatrix();
	const XMFLOAT4X4& GetPositionMatrix() const;
	XMFLOAT4X4& GetTransformMatrix();
	const XMFLOAT4X4& GetTransformMatrix() const;
#endif

	bool IsDirty() const;
	void MarkDirty();
	void ClearDirty();

	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& pos);
	void AddPosition(float x, float y, float z);
	void AddPosition(const XMFLOAT3& delta);

	void SetX(float x);
	void SetY(float y);
	void SetZ(float z);

	void SetScale(float x, float y, float z);
	void SetScale(const XMFLOAT3& s);
	void SetScaleX(float x);
	void SetScaleY(float y);
	void SetScaleZ(float z);

	void SetScalePosition(float x, float y, float z);
	void SetScalePosition(const XMFLOAT3& pos);

	void SetRotation(float r);
	void SetRotation(float y, float p, float r);
	void SetYaw(float y);
	void SetPitch(float p);
	void SetRoll(float r);
	void AddRotation(float y, float p, float r);

	void SetRotationQuaternion(const XMFLOAT4& q);
	void SetRotationMatrix(const XMFLOAT4X4& mat);
	void SetWorldMatrix(const XMFLOAT4X4& mat);

#ifdef ENABLE_OBJ_SCALLING
	void SetScaleMatrix(const XMFLOAT4X4& mat);
	void SetPositionMatrix(const XMFLOAT4X4& mat);
	void SetTransformMatrix(const XMFLOAT4X4& mat);
#endif

	void UpdateMatrix();

private:
	XMFLOAT3 m_position;
	XMFLOAT3 m_scale;
	XMFLOAT3 m_scalePosition;

	float m_yaw;
	float m_pitch;
	float m_roll;

	XMFLOAT4X4 m_rotation;
	XMFLOAT4X4 m_world;

#ifdef ENABLE_OBJ_SCALLING
	XMFLOAT4X4 m_scaleMatrix;
	XMFLOAT4X4 m_positionMatrix;
	XMFLOAT4X4 m_transformMatrix;
#endif

	bool m_dirty;
};
