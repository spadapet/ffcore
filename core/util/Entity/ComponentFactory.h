#pragma once
#include "Entity/Entity.h"

namespace ff
{
	// Creates and deletes components of a certain type for any entity
	class ComponentFactory
	{
	public:
		template<typename T>
		static std::unique_ptr<ComponentFactory> Create();

		UTIL_API ComponentFactory(std::unique_ptr<ff::IBytePoolAllocator>&& allocator, std::function<void(void*, const void*)>&& copyConstructor, std::function<void(void*)>&& destructor);
		UTIL_API ~ComponentFactory();

		template<typename T, typename... Args> void* New(Entity entity, bool& usedExisting, Args&&... args);
		void* Clone(Entity entity, Entity sourceEntity);
		void* Lookup(Entity entity) const;
		bool Delete(Entity entity);

	private:
		UTIL_API void* NewBytes(Entity entity, bool& usedExisting);

		std::function<void(void*, const void*)> _copyConstructor;
		std::function<void(void*)> _destructor;

		std::unique_ptr<ff::IBytePoolAllocator> _allocator;
		ff::Map<Entity, void*> _entityToComponent;
	};

	typedef std::unique_ptr<ComponentFactory>(*CreateComponentFactoryFunc)();
}

template<typename T, typename... Args>
void* ff::ComponentFactory::New(Entity entity, bool& usedExisting, Args&&... args)
{
	void* component = NewBytes(entity, usedExisting);
	if (usedExisting)
	{
		*reinterpret_cast<T*>(component) = T(std::forward<Args>(args)...);
	}
	else
	{
		::new(component) T(std::forward<Args>(args)...);
	}

	return component;
}

template<typename T>
std::unique_ptr<ff::ComponentFactory> ff::ComponentFactory::Create()
{
	return std::make_unique<ComponentFactory>(
		std::make_unique<ff::BytePoolAllocator<sizeof(T), alignof(T), false>>(),
		[](void* component, const void* sourceComponent)
		{
			::new(component) T(*reinterpret_cast<const T*>(sourceComponent));
		},
		[](void* component)
		{
			reinterpret_cast<T*>(component)->~T();
		});
}
