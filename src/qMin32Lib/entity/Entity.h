#pragma once

#include "EntityTypes.h"
#include "EntityWorld.h"
#include "EntityComponent.h"

class CEntity
{
public:
	CEntity();
	virtual ~CEntity();

	void CreateEntity();
	void DestroyEntity();

	bool IsEntityValid() const;
	EntityHandle GetEntityHandle() const;

	static CEntityWorld& GetEntityWorld();

	template <typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		EnsureEntity();

		T& component = ms_world.AddComponent<T>(m_entity, std::forward<Args>(args)...);

		if constexpr (std::is_base_of_v<CEntityComponent, T>)
			component.Create(this);

		return component;
	}

	template <typename T>
	bool HasComponent() const
	{
		return ms_world.HasComponent<T>(m_entity);
	}

	template <typename T>
	T& GetComponent()
	{
		return ms_world.GetComponent<T>(m_entity);
	}

	template <typename T>
	const T& GetComponent() const
	{
		return ms_world.GetComponent<T>(m_entity);
	}

	template <typename T>
	T* TryGetComponent()
	{
		return ms_world.TryGetComponent<T>(m_entity);
	}

	template <typename T>
	const T* TryGetComponent() const
	{
		return ms_world.TryGetComponent<T>(m_entity);
	}

	template <typename T>
	void RemoveComponent()
	{
		if (!HasComponent<T>())
			return;

		if constexpr (std::is_base_of_v<CEntityComponent, T>)
			GetComponent<T>().Destroy();

		ms_world.RemoveComponent<T>(m_entity);
	}

protected:
	virtual void OnCreateInternal() {}
	virtual void OnDestroyInternal() {}

	void EnsureEntity();

protected:
	EntityHandle m_entity;

private:
	static CEntityWorld ms_world;
};
