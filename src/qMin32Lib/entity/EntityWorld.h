#pragma once

#include "EntityTypes.h"
#include "EntityComponent.h"

#include <entt.hpp>
#include <type_traits>
#include <utility>

class CEntityWorld
{
public:
	using Registry = entt::registry;

public:
	CEntityWorld();
	~CEntityWorld();

	EntityHandle CreateEntity();

	void DestroyEntity(EntityHandle entity);
	void DestroyEntityComponents(EntityHandle entity);

	bool IsValid(EntityHandle entity) const;

	Registry& GetRegistry();
	const Registry& GetRegistry() const;

	template <typename T, typename... Args>
	T& AddComponent(EntityHandle entity, Args&&... args)
	{
		return m_registry.emplace_or_replace<T>(
			entity,
			std::forward<Args>(args)...
		);
	}

	template <typename T>
	bool HasComponent(EntityHandle entity) const
	{
		return m_registry.valid(entity) &&
			m_registry.all_of<T>(entity);
	}

	template <typename T>
	T& GetComponent(EntityHandle entity)
	{
		return m_registry.get<T>(entity);
	}

	template <typename T>
	const T& GetComponent(EntityHandle entity) const
	{
		return m_registry.get<T>(entity);
	}

	template <typename T>
	T* TryGetComponent(EntityHandle entity)
	{
		return m_registry.valid(entity)
			? m_registry.try_get<T>(entity)
			: nullptr;
	}

	template <typename T>
	const T* TryGetComponent(EntityHandle entity) const
	{
		return m_registry.valid(entity)
			? m_registry.try_get<T>(entity)
			: nullptr;
	}

	template <typename T>
	void RemoveComponent(EntityHandle entity)
	{
		if (!HasComponent<T>(entity))
			return;

		if constexpr (std::is_base_of_v<CEntityComponent, T>)
			GetComponent<T>(entity).Destroy();

		m_registry.remove<T>(entity);
	}

private:
	Registry m_registry;
};
