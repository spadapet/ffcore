#pragma once
#include "Entity/Entity.h"

namespace ff
{
	// Creates and deletes components of a certain type for any entity
	class ComponentFactory
	{
	public:
		template<typename T>
		static std::shared_ptr<ComponentFactory> Create();

		UTIL_API ~ComponentFactory();

		void* New(Entity entity, bool* usedExisting = nullptr);
		void* Clone(Entity entity, Entity sourceEntity);
		void* Lookup(Entity entity) const;
		bool Delete(Entity entity);

	private:
		UTIL_API ComponentFactory(
			std::unique_ptr<ff::IBytePoolAllocator>&& allocator,
			std::function<void(void*)>&& constructor,
			std::function<void(void*, const void*)>&& copyConstructor,
			std::function<void(void*)>&& destructor);

		std::function<void(void*)> _constructor;
		std::function<void(void*, const void*)> _copyConstructor;
		std::function<void(void*)> _destructor;

		std::unique_ptr<ff::IBytePoolAllocator> _allocator;
		ff::Map<Entity, void*> _entityToComponent;
	};

	typedef std::shared_ptr<ComponentFactory>(*CreateComponentFactoryFunc)();

	struct ComponentTypeToFactoryEntry
	{
		std::type_index _type;
		CreateComponentFactoryFunc _factory;
		bool _required;
	};
}

template<typename T>
std::shared_ptr<ff::ComponentFactory> ff::ComponentFactory::Create()
{
	return std::shared_ptr<ComponentFactory>(
		new ComponentFactory(
			std::make_unique<ff::BytePoolAllocator<sizeof(T), alignof(T), false>>(),
			// T::T constructor
			[](void* component)
			{
				::new(component) T();
			},
			// T::T(T) constructor
				[](void* component, const void* sourceComponent)
			{
				::new(component) T(*reinterpret_cast<const T*>(sourceComponent));
			},
				// T::~T destructor
				[](void* component)
			{
				reinterpret_cast<T*>(component)->~T();
			}));
}

// Helper macros

#define DECLARE_ENTRY_COMPONENTS() \
	static const ff::ComponentTypeToFactoryEntry *GetComponentTypes(); \
	static size_t GetComponentCount();

#define BEGIN_ENTRY_COMPONENTS(className) \
	static const ff::ComponentTypeToFactoryEntry s_components_##className[] \
	{

#define HAS_COMPONENT(componentClass) \
	{ std::type_index(typeid(componentClass)), &ff::ComponentFactory::Create<componentClass>, true },

#define HAS_OPTIONAL_COMPONENT(componentClass) \
	{ std::type_index(typeid(componentClass)), &ff::ComponentFactory::Create<componentClass>, false },

#define END_ENTRY_COMPONENTS(className) \
	}; \
	const ff::ComponentTypeToFactoryEntry *className::GetComponentTypes() \
	{ \
		return s_components_##className; \
	} \
	size_t className::GetComponentCount() \
	{ \
		return _countof(s_components_##className); \
	}

#define EMPTY_ENTRY_COMPONENTS(className) \
	const ff::ComponentTypeToFactoryEntry *className::GetComponentTypes() \
	{ \
		return nullptr; \
	} \
	size_t className::GetComponentCount() \
	{ \
		return 0; \
	}
