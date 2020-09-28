#pragma once

class WeakEntityListener;

namespace ff
{
	class EntityDomain;
	class IEntityEventHandler;

	struct EntityBase;
	typedef EntityBase* Entity;

	struct EntityBase
	{
		UTIL_API EntityDomain* GetDomain();

		template<typename T, typename... Args> T* SetComponent(Args&&... args);
		template<typename T> T* CloneComponent(Entity sourceEntity);
		template<typename T> T* GetComponent();
		template<typename T> bool DeleteComponent();

		UTIL_API Entity Clone();
		UTIL_API ff::hash_t GetHash();
		UTIL_API void Activate();
		UTIL_API void Deactivate();
		UTIL_API void Delete();
		UTIL_API bool IsActive();

		UTIL_API void TriggerEvent(hash_t eventId, void* args = nullptr);
		UTIL_API bool AddEventHandler(hash_t eventId, IEntityEventHandler* handler);
		UTIL_API bool RemoveEventHandler(hash_t eventId, IEntityEventHandler* handler);
	};

	class WeakEntity
	{
	public:
		UTIL_API WeakEntity();
		UTIL_API WeakEntity(Entity entity);
		UTIL_API WeakEntity(const WeakEntity& rhs);
		UTIL_API WeakEntity(WeakEntity&& rhs);
		UTIL_API ~WeakEntity();

		UTIL_API void Connect(Entity entity);
		UTIL_API void Disconnect();
		UTIL_API void Delete();

		UTIL_API bool IsValid() const;
		UTIL_API Entity GetEntity() const;
		UTIL_API EntityDomain* GetDomain() const;

		UTIL_API WeakEntity& operator=(Entity entity);
		UTIL_API WeakEntity& operator=(const WeakEntity& rhs);
		UTIL_API WeakEntity& operator=(WeakEntity&& rhs);
		UTIL_API Entity operator->() const;
		UTIL_API operator Entity() const;
		UTIL_API operator bool() const;

	private:
		WeakEntityListener* _listener;
	};

	template<>
	inline hash_t HashFunc<Entity>(const Entity& val)
	{
		return val->GetHash();
	}
};

template<typename T, typename... Args>
T* ff::EntityBase::SetComponent(Args&&... args)
{
	return GetDomain()->SetComponent<T, Args...>(this, std::forward<Args>(args)...);
}

template<typename T>
T* ff::EntityBase::CloneComponent(Entity sourceEntity)
{
	return GetDomain()->CloneComponent<T>(this, sourceEntity);
}

template<typename T>
T* ff::EntityBase::GetComponent()
{
	return GetDomain()->GetComponent<T>(this);
}

template<typename T>
bool ff::EntityBase::DeleteComponent()
{
	return GetDomain()->DeleteComponent<T>(this);
}
