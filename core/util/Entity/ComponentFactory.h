#pragma once
#include "Entity/Entity.h"

namespace ff
{
	struct Component
	{
	};

	// Creates and deletes components of a certain type for any entity
	class ComponentFactory
	{
	public:
		template<typename T>
		static std::shared_ptr<ComponentFactory> Create();

		UTIL_API ~ComponentFactory();

		ff::Component& New(Entity entity, bool* usedExisting = nullptr);
		ff::Component* Clone(Entity entity, Entity sourceEntity);
		ff::Component* Lookup(Entity entity) const;
		bool Delete(Entity entity);
		void* CastToVoid(Component* component) const;

	private:
		UTIL_API ComponentFactory(
			std::unique_ptr<ff::IBytePoolAllocator>&& allocator,
			std::function<void(Component&)>&& constructor,
			std::function<void(Component&, const Component&)>&& copyConstructor,
			std::function<void(Component&)>&& destructor,
			std::function<void* (Component*)>&& castFromBase);

		std::function<void(Component&)> _constructor;
		std::function<void(Component&, const Component&)> _copyConstructor;
		std::function<void(Component&)> _destructor;
		std::function<void* (Component*)> _castFromBase;

		std::unique_ptr<ff::IBytePoolAllocator> _allocator;
		ff::Map<Entity, ff::Component*> _entityToComponent;
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
			[](Component& component)
			{
				T* myComponent = static_cast<T*>(&component);
				::new(myComponent) T();
			},
			// T::T(T) constructor
				[](Component& component, const Component& sourceComponent)
			{
				T* myComponent = static_cast<T*>(&component);
				const T* mySourceComponent = static_cast<const T*>(&sourceComponent);
				::new(myComponent) T(*mySourceComponent);
			},
				// T::~T destructor
				[](Component& component)
			{
				T* myComponent = static_cast<T*>(&component);
				myComponent->~T();
			},
				// castFromBase
				[](Component* component)
			{
				return static_cast<T*>(component);
			}));
}

// Helper macros

#define DECLARE_ENTRY_COMPONENTS() \
	static const ff::ComponentTypeToFactoryEntry *GetComponentTypes(); \
	static size_t GetComponentCount();

#define DECLARE_ENTRY_COMPONENTS2(className) \
	className(); \
	DECLARE_ENTRY_COMPONENTS()

#define DECLARE_ENTRY_COMPONENTS3(className) \
	className(); \
	~className(); \
	DECLARE_ENTRY_COMPONENTS()

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
