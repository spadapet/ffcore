#pragma once
#include "Entity/EntityBucket.h"
#include "Entity/EntityEvents.h"

namespace ff
{
	class EntityDomain
	{
	public:
		UTIL_API EntityDomain();
		UTIL_API ~EntityDomain();

		// Bucket methods (T must be of type BucketEntry<Comp1, Comp2, ...>)
		template<typename T> IEntityBucket<T>* GetBucket();

		// Component methods
		template<typename T, typename... Args> T* SetComponent(Entity entity, Args&&... args);
		template<typename T> T* GetComponent(Entity entity);
		template<typename T> bool DeleteComponent(Entity entity);

		// Entity methods (not activated by default)
		UTIL_API Entity CreateEntity();
		UTIL_API Entity CloneEntity(Entity sourceEntity);
		UTIL_API void ActivateEntity(Entity entity);
		UTIL_API void DeactivateEntity(Entity entity);
		UTIL_API void DeleteEntity(Entity entity);
		UTIL_API void DeleteEntities();
		UTIL_API bool IsEntityActive(Entity entity);
		UTIL_API static EntityDomain* Get(Entity entity);
		UTIL_API static ff::hash_t GetHash(Entity entity);

		// Events can be entity-specific or global
		UTIL_API void TriggerEvent(hash_t eventId, Entity entity, void* args = nullptr);
		UTIL_API void TriggerEvent(hash_t eventId, void* args = nullptr);

		// Event registration can be entity-specific or global
		UTIL_API bool AddEventHandler(hash_t eventId, Entity entity, IEntityEventHandler* handler);
		UTIL_API bool RemoveEventHandler(hash_t eventId, Entity entity, IEntityEventHandler* handler);
		UTIL_API bool AddEventHandler(hash_t eventId, IEntityEventHandler* handler);
		UTIL_API bool RemoveEventHandler(hash_t eventId, IEntityEventHandler* handler);

	private:
		// Data structures

		struct ComponentFactoryEntry;
		struct EventHandlerEntry;
		struct BucketBase;

		// For a bucket, remembers which components it cares about
		struct ComponentFactoryBucketEntry
		{
			bool operator==(const ComponentFactoryBucketEntry& rhs) const
			{
				return _factory == rhs._factory && _offset == rhs._offset && _required == rhs._required;
			}

			ComponentFactoryEntry* _factory;
			size_t _offset;
			bool _required;
		};

		// For a component factory, remember which buckets care about it
		struct BucketComponentFactoryEntry
		{
			bool operator==(const BucketComponentFactoryEntry& rhs) const
			{
				return _bucket == rhs._bucket && _offset == rhs._offset && _required == rhs._required;
			}

			BucketBase* _bucket;
			size_t _offset;
			bool _required;
		};

		struct EventHandler
		{
			bool operator==(const EventHandler& rhs) const
			{
				return _eventId == rhs._eventId && _handler == rhs._handler;
			}

			hash_t _eventId;
			IEntityEventHandler* _handler;
		};

		struct EventHandlerEntry
		{
			hash_t _eventId;
			Vector<EventHandler> _eventHandlers;
		};

		struct ComponentFactoryEntry
		{
			std::unique_ptr<ComponentFactory> _factory;
			Vector<BucketComponentFactoryEntry> _buckets;
			uint64_t _bit;
		};

		struct EntityEntry : public EntityBase
		{
			static EntityEntry* FromEntity(Entity entity)
			{
				return static_cast<EntityEntry*>(entity);
			}

			EntityDomain* _domain;
			Vector<EventHandler> _eventHandlers;
			Vector<BucketBase*> _buckets;
			Vector<ComponentFactoryEntry*> _components;
			uint64_t _componentBits;
			ff::hash_t _hash;
			bool _active;
		};

		struct BucketBase
		{
			virtual ~BucketBase() { }

			uint64_t _requiredComponentBits;
			Vector<ComponentFactoryBucketEntry> _components;
			Map<Entity, BucketEntryBase*> _entityToEntry;
			std::function<BucketEntryBase* (Entity)> _newEntryFunc;
			std::function<void(Entity)> _deleteEntryFunc;
			std::function<void(BucketEntryBase*)> _notifyAdd;
		};

		template<typename TEntry>
		struct Bucket : public BucketBase, public IEntityBucket<TEntry>
		{
			Bucket()
			{
				_newEntryFunc = [this](Entity entity) -> BucketEntryBase*
				{
					BucketEntryBase* entry = &_entries.Push();
					*reinterpret_cast<Entity*>(entry) = entity;
					_entityToEntry.SetKey(entity, entry);
					return entry;
				};

				_deleteEntryFunc = [this](Entity entity) -> void
				{
					auto iter = _entityToEntry.GetKey(entity);
					assertRet(iter);
					TEntry* entry = static_cast<TEntry*>(iter->GetValue());

					for (size_t i = _listeners.Size(); i; i--)
					{
						_listeners[i - 1]->OnEntryRemoving(*entry);
					}

					_entityToEntry.DeleteKey(*iter);
					_entries.Delete(*entry);
				};

				_notifyAdd = [this](BucketEntryBase* entry) -> void
				{
					TEntry& tentry = *static_cast<TEntry*>(entry);
					for (size_t i = _listeners.Size(); i; i--)
					{
						_listeners[i - 1]->OnEntryAdded(tentry);
					}
				};
			}

			virtual const List<TEntry>& GetEntries() const override
			{
				return _entries;
			}

			virtual TEntry* GetEntry(Entity entity) const override
			{
				auto iter = _entityToEntry.GetKey(entity);
				return iter ? static_cast<TEntry*>(iter->GetValue()) : nullptr;
			}

			virtual void AddListener(IEntityBucketListener<TEntry>* listener) override
			{
				assertRet(listener && !_listeners.Contains(listener));
				_listeners.Push(listener);
			}

			virtual bool RemoveListener(IEntityBucketListener<TEntry>* listener) override
			{
				assertRetVal(_listeners.DeleteItem(listener), false);
				return true;
			}

			List<TEntry> _entries;
			Vector<IEntityBucketListener<TEntry>*> _listeners;
		};

		// Component methods
		UTIL_API void SetComponent(Entity entity, ComponentFactoryEntry* factoryEntry, void* component, bool usedExisting);
		UTIL_API void* GetComponent(Entity entity, ComponentFactoryEntry* factoryEntry);
		UTIL_API bool DeleteComponent(Entity entity, ComponentFactoryEntry* factoryEntry);
		UTIL_API ComponentFactoryEntry* AddComponentFactory(std::type_index componentType, CreateComponentFactoryFunc factoryFunc);
		UTIL_API ComponentFactoryEntry* GetComponentFactory(std::type_index componentType);
		template<typename T> ComponentFactoryEntry* GetComponentFactory();
		Vector<ComponentFactoryBucketEntry> FindComponentEntries(const BucketEntryBase::ComponentEntry* componentEntries);

		// Bucket methods
		UTIL_API void InitBucket(BucketBase* bucket, const BucketEntryBase::ComponentEntry* componentEntries);

		// Entity methods
		void RegisterActivatedEntity(EntityEntry* entityEntry);
		void UnregisterDeactivatedEntity(EntityEntry* entityEntry);
		void RegisterEntityWithBuckets(EntityEntry* entityEntry, ComponentFactoryEntry* newFactory, void* component);
		void UnregisterEntityWithBuckets(EntityEntry* entityEntry, ComponentFactoryEntry* deletingFactory);
		void TryRegisterEntityWithBucket(EntityEntry* entityEntry, BucketBase* bucket);
		void CreateBucketEntry(EntityEntry* entityEntry, BucketBase* bucket);
		void DeleteBucketEntry(EntityEntry* entityEntry, BucketBase* bucket);

		// Event methods
		void TriggerEvent(EventHandlerEntry* eventEntry, Entity entity, void* args);
		bool AddEventHandler(EventHandlerEntry* eventEntry, Entity entity, IEntityEventHandler* handler);
		bool RemoveEventHandler(EventHandlerEntry* eventEntry, Entity entity, IEntityEventHandler* handler);
		EventHandlerEntry* GetEventEntry(hash_t eventId);

		// Data
		ff::hash_t _lastEntityHash;
		List<EntityEntry> _entities;
		Map<hash_t, EventHandlerEntry, NonHasher<hash_t>> _events;
		Map<std::type_index, std::unique_ptr<BucketBase>> _buckets;
		Map<std::type_index, ComponentFactoryEntry> _componentFactories;
	};
}

template<typename T, typename... Args>
T* ff::EntityDomain::SetComponent(Entity entity, Args&&... args)
{
	bool usedExisting;
	ComponentFactoryEntry* factory = GetComponentFactory<T>();
	void* component = factory->_factory->New<T, Args...>(entity, usedExisting, std::forward<Args>(args)...);
	SetComponent(entity, factory, component, usedExisting);
	return reinterpret_cast<T*>(component);
}

template<typename T>
T* ff::EntityDomain::GetComponent(Entity entity)
{
	void* component = GetComponent(entity, GetComponentFactory<T>());
	return reinterpret_cast<T*>(component);
}

template<typename T>
bool ff::EntityDomain::DeleteComponent(Entity entity)
{
	return DeleteComponent(entity, GetComponentFactory<T>());
}

template<typename T>
ff::EntityDomain::ComponentFactoryEntry* ff::EntityDomain::GetComponentFactory()
{
	std::type_index type(typeid(T));
	ComponentFactoryEntry* factory = GetComponentFactory(type);
	if (!factory)
	{
		factory = AddComponentFactory(type, &ff::ComponentFactory::Create<T>);
	}

	return factory;
}

template<typename T>
ff::IEntityBucket<T>* ff::EntityDomain::GetBucket()
{
	std::type_index type(typeid(T));
	auto iter = _buckets.GetKey(type);
	if (!iter)
	{
		std::unique_ptr<BucketBase> bucketBase = std::make_unique<Bucket<T>>();
		InitBucket(bucketBase.get(), T::GetComponentEntries());
		iter = _buckets.SetKey(std::move(type), std::move(bucketBase));
	}

	return static_cast<Bucket<T>*>(iter->GetValue().get());
}
