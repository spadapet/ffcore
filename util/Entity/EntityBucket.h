#pragma once
#include "Entity/ComponentFactory.h"

namespace ff
{
	// For optional components in a BucketEntry template
	template<typename T>
	struct Optional { };

	class BucketEntryBase
	{
	public:
		BucketEntryBase()
			: _entity(nullptr)
		{
		}

		Entity GetEntity() const
		{
			return _entity;
		}

		template<typename T>
		T* GetComponent() const
		{
			return _entity->GetComponent<T>();
		}

		struct ComponentEntry
		{
			const ComponentEntry* _next;
			std::type_index _type;
			CreateComponentFactoryFunc _factory;
			size_t _offset;
			bool _required;
		};

		static const ComponentEntry* GetComponentEntries()
		{
			return nullptr;
		}

	protected:
		template<typename T>
		struct ComponentAccessor
		{
			using Type = typename T;
			static constexpr bool required = true;
		};

		template<typename T>
		struct ComponentAccessor<Optional<T>>
		{
			using Type = typename T;
			static constexpr bool required = false;
		};

	private:
		Entity _entity;
	};

	template<typename... Types>
	class BucketEntry
	{
		// only specializations are used
	};

	template<>
	class BucketEntry<> : public BucketEntryBase
	{
		// end of recursion
	};

	template<typename FirstType, typename... NextTypes>
	class BucketEntry<FirstType, NextTypes...> : public BucketEntry<NextTypes...>
	{
	private:
		using BaseType = typename BucketEntry<NextTypes...>;
		using FirstComponentType = typename BucketEntryBase::ComponentAccessor<FirstType>::Type;

	public:
		BucketEntry()
			: _component(nullptr)
		{
		}

		static const BucketEntryBase::ComponentEntry* GetComponentEntries()
		{
			using MyType = typename BucketEntry<FirstType, NextTypes...>;

			static const BucketEntryBase::ComponentEntry entry
			{
				BaseType::GetComponentEntries(),
				std::type_index(typeid(FirstComponentType)),
				&ff::ComponentFactory::Create<FirstComponentType>,
				reinterpret_cast<uint8_t*>(&reinterpret_cast<MyType*>(256)->_component) - reinterpret_cast<uint8_t*>(static_cast<BucketEntryBase*>(reinterpret_cast<MyType*>(256))),
				BucketEntryBase::ComponentAccessor<FirstType>::required,
			};

			return &entry;
		}

		template<typename T>
		T* GetComponent() const
		{
			return BaseType::GetComponent<T>();
		}

		template<>
		FirstComponentType* GetComponent<FirstComponentType>() const
		{
			return _component;
		}

	private:
		FirstComponentType* _component;
	};

	template<typename TEntry>
	class IEntityBucketListener
	{
	public:
		virtual void OnEntryAdded(TEntry& entry) = 0;
		virtual void OnEntryRemoving(TEntry& entry) = 0;
	};

	template<typename TEntry>
	struct IEntityBucket
	{
		virtual const List<TEntry>& GetEntries() const = 0;
		virtual TEntry* GetEntry(Entity entity) const = 0;
		virtual void AddListener(IEntityBucketListener<TEntry>* listener) = 0;
		virtual bool RemoveListener(IEntityBucketListener<TEntry>* listener) = 0;
	};
}
