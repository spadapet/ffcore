#pragma once
#include "Entity/ComponentFactory.h"
#include "Entity/EntityBucket.h"
#include "Entity/EntityEvents.h"

namespace ff
{
	class EntityDomain
	{
	public:
		UTIL_API EntityDomain();
		UTIL_API ~EntityDomain();

		UTIL_API static EntityDomain* Get(Entity entity);
		UTIL_API static EntityId GetId(Entity entity);
		UTIL_API static EntityDomain* TryGet(Entity entity);

		// Must call once per frame
		UTIL_API void Advance();

		// Bucket methods (T must derive from EntityBucketEntry)
		template<typename T> IEntityBucket<T>* GetBucket();

		// Component methods (T must derive from Component)
		template<typename T> T* AddComponent(Entity entity);
		template<typename T> T* CloneComponent(Entity entity, Entity sourceEntity);
		template<typename T> T* GetComponent(Entity entity);
		template<typename T> bool DeleteComponent(Entity entity);

		// Entity methods (not activated by default)
		UTIL_API Entity CreateEntity(StringRef name = String());
		UTIL_API Entity CloneEntity(Entity sourceEntity, StringRef name = String());
		UTIL_API Entity GetEntity(StringRef name) const;
		UTIL_API Entity GetEntity(EntityId id) const;
		UTIL_API StringRef GetEntityName(Entity entity) const;
		UTIL_API EntityId GetEntityId(Entity entity) const;
		UTIL_API void ActivateEntity(Entity entity);
		UTIL_API void DeactivateEntity(Entity entity);
		UTIL_API void DeleteEntity(Entity entity);
		UTIL_API void DeleteEntities();
		UTIL_API bool IsEntityActive(Entity entity);
		UTIL_API bool IsEntityPendingDeletion(Entity entity);

		// Events can be entity-specific or global
		UTIL_API void TriggerEvent(hash_t eventId, Entity entity, EntityEventArgs* args = nullptr);
		UTIL_API void TriggerEvent(hash_t eventId, EntityEventArgs* args = nullptr);

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
				return _factory == rhs._factory && _required == rhs._required;
			}

			ComponentFactoryEntry* _factory;
			bool _required;
		};

		// For a component factory, remember which buckets care about it
		struct BucketComponentFactoryEntry
		{
			bool operator==(const BucketComponentFactoryEntry& rhs) const
			{
				return _bucket == rhs._bucket && _bucketEntryIndex == rhs._bucketEntryIndex && _required == rhs._required;
			}

			BucketBase* _bucket;
			size_t _bucketEntryIndex;
			bool _required;
		};

		// Allows access into the components of EntityBucketEntry
		struct FakeBucketEntry : public EntityBucketEntry
		{
			void* _components[1];
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
			std::shared_ptr<ComponentFactory> _factory;
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
			String _name;
			Vector<EventHandler> _eventHandlers;
			Vector<BucketBase*> _buckets;
			Vector<ComponentFactoryEntry*> _components;
			uint64_t _componentBits;
			EntityId _id;
			bool _valid;
			bool _active;
		};

		struct BucketBase
		{
			virtual ~BucketBase() { }

			uint64_t _requiredComponentBits;
			Vector<ComponentFactoryBucketEntry> _components;
			Map<Entity, EntityBucketEntry*> _entityToEntry;
			std::function<EntityBucketEntry * (Entity)> _newEntryFunc;
			std::function<void(Entity)> _deleteEntryFunc;
			std::function<void(EntityBucketEntry*)> _notifyAdd;
		};

		template<typename TEntry>
		struct Bucket : public BucketBase, public IEntityBucket<TEntry>
		{
			Bucket()
			{
				_newEntryFunc = [this](Entity entity) -> EntityBucketEntry*
				{
					EntityBucketEntry* entry = &_entries.Push();
					_entityToEntry.SetKey(entity, entry);
					entry->_entity = entity;
					return entry;
				};

				_deleteEntryFunc = [this](Entity entity) -> void
				{
					auto iter = _entityToEntry.GetKey(entity);
					assertRet(iter);
					TEntry* entry = static_cast<TEntry*>(iter->GetValue());

					for (auto i = _listeners.crbegin(); i != _listeners.crend(); i++)
					{
						(*i)->OnEntryRemoving(*entry);
					}

					_entityToEntry.DeleteKey(*iter);
					_entries.Delete(*entry);
				};

				_notifyAdd = [this](EntityBucketEntry* entry) -> void
				{
					TEntry& tentry = *static_cast<TEntry*>(entry);
					for (auto i = _listeners.crbegin(); i != _listeners.crend(); i++)
					{
						(*i)->OnEntryAdded(tentry);
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
				noAssertRetVal(iter, nullptr);
				return static_cast<TEntry*>(iter->GetValue());
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
			Vector<IEntityBucketListener<TEntry>*, 8> _listeners;
		};

		// Component methods
		UTIL_API Component* AddComponent(Entity entity, ComponentFactoryEntry* factoryEntry);
		UTIL_API Component* CloneComponent(Entity entity, Entity sourceEntity, ComponentFactoryEntry* factoryEntry);
		UTIL_API Component* GetComponent(Entity entity, ComponentFactoryEntry* factoryEntry);
		UTIL_API bool DeleteComponent(Entity entity, ComponentFactoryEntry* factoryEntry);
		UTIL_API ComponentFactoryEntry* AddComponentFactory(std::type_index componentType, CreateComponentFactoryFunc factoryFunc);
		UTIL_API ComponentFactoryEntry* GetComponentFactory(std::type_index componentType);
		template<typename T> ComponentFactoryEntry* GetComponentFactory();
		Vector<ComponentFactoryBucketEntry> FindComponentEntries(const ComponentTypeToFactoryEntry* componentTypes, size_t count);

		// Bucket methods
		UTIL_API void InitBucket(BucketBase* bucket, const ComponentTypeToFactoryEntry* componentTypes, size_t componentCount);

		// Entity methods
		void FlushDeletedEntities();
		void RegisterActivatedEntity(EntityEntry* entityEntry);
		void UnregisterDeactivatedEntity(EntityEntry* entityEntry);
		void RegisterEntityWithBuckets(EntityEntry* entityEntry, ComponentFactoryEntry* newFactory, Component* component);
		void UnregisterEntityWithBuckets(EntityEntry* entityEntry, ComponentFactoryEntry* deletingFactory);
		void TryRegisterEntityWithBucket(EntityEntry* entityEntry, BucketBase* bucket);
		void CreateBucketEntry(EntityEntry* entityEntry, BucketBase* bucket);
		void DeleteBucketEntry(EntityEntry* entityEntry, BucketBase* bucket);

		// Event methods
		void TriggerEvent(EventHandlerEntry* eventEntry, Entity entity, EntityEventArgs* args);
		bool AddEventHandler(EventHandlerEntry* eventEntry, Entity entity, IEntityEventHandler* handler);
		bool RemoveEventHandler(EventHandlerEntry* eventEntry, Entity entity, IEntityEventHandler* handler);
		EventHandlerEntry* GetEventEntry(hash_t eventId);

		// Data
		EntityId _lastEntityId;
		List<EntityEntry> _entities;
		Vector<EntityEntry*> _deletedEntities;
		Map<String, EntityEntry*> _entitiesByName;
		Map<EntityId, EntityEntry*, NonHasher<EntityId>> _entitiesById;
		Map<hash_t, EventHandlerEntry, NonHasher<hash_t>> _events;
		Map<std::type_index, std::shared_ptr<BucketBase>> _buckets;
		Map<std::type_index, ComponentFactoryEntry> _componentFactories;
	};
}

template<typename T>
T* ff::EntityDomain::AddComponent(Entity entity)
{
	Component* component = AddComponent(entity, GetComponentFactory<T>());
	return static_cast<T*>(component);
}

template<typename T>
T* ff::EntityDomain::CloneComponent(Entity entity, Entity sourceEntity)
{
	Component* component = CloneComponent(entity, sourceEntity, GetComponentFactory<T>());
	return static_cast<T*>(component);
}

template<typename T>
T* ff::EntityDomain::GetComponent(Entity entity)
{
	Component* component = GetComponent(entity, GetComponentFactory<T>());
	return static_cast<T*>(component);
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

	if (factory == nullptr)
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
		std::shared_ptr<BucketBase> bucketBase = std::make_shared<Bucket<T>>();
		InitBucket(bucketBase.get(), T::GetComponentTypes(), T::GetComponentCount());
		iter = _buckets.SetKey(std::move(type), std::move(bucketBase));
	}

	return static_cast<Bucket<T>*>(iter->GetValue().get());
}
