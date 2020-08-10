#include "pch.h"
#include "Entity/EntityDomain.h"

ff::EntityDomain::EntityDomain()
	: _lastEntityHash(0)
{
}

ff::EntityDomain::~EntityDomain()
{
	DeleteEntities();
}

ff::EntityDomain* ff::EntityDomain::Get(Entity entity)
{
	return EntityEntry::FromEntity(entity)->_domain;
}

ff::hash_t ff::EntityDomain::GetHash(Entity entity)
{
	return EntityEntry::FromEntity(entity)->_hash;
}

ff::Entity ff::EntityDomain::CreateEntity()
{
	EntityEntry& entityEntry = _entities.Push();
	entityEntry._domain = this;
	entityEntry._componentBits = 0;
	entityEntry._hash = ++_lastEntityHash;
	entityEntry._active = false;

	return &entityEntry;
}

ff::Entity ff::EntityDomain::CloneEntity(Entity sourceEntity)
{
	EntityEntry* sourceEntityEntry = EntityEntry::FromEntity(sourceEntity);
	Entity newEntity = CreateEntity();

	for (const ComponentFactoryEntry* factoryEntry : sourceEntityEntry->_components)
	{
		factoryEntry->_factory->Clone(newEntity, sourceEntity);
	}

	EntityEntry* newEntityEntry = EntityEntry::FromEntity(newEntity);
	newEntityEntry->_componentBits = sourceEntityEntry->_componentBits;
	newEntityEntry->_components = sourceEntityEntry->_components;
	newEntityEntry->_buckets = sourceEntityEntry->_buckets;

	return newEntity;
}

void ff::EntityDomain::ActivateEntity(Entity entity)
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	if (!entityEntry->_active)
	{
		entityEntry->_active = true;
		RegisterActivatedEntity(entityEntry);
		TriggerEvent(ENTITY_EVENT_ACTIVATED, entity);
	}
}

void ff::EntityDomain::DeactivateEntity(Entity entity)
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	if (entityEntry->_active)
	{
		entityEntry->_active = false;
		UnregisterDeactivatedEntity(entityEntry);
		TriggerEvent(ENTITY_EVENT_DEACTIVATED, entity);
	}
}

void ff::EntityDomain::DeleteEntity(Entity entity)
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	DeactivateEntity(entity);
	TriggerEvent(ENTITY_EVENT_DELETED, entity);

	for (const ComponentFactoryEntry* factoryEntry : entityEntry->_components)
	{
		factoryEntry->_factory->Delete(entityEntry);
	}

	_entities.Delete(*entityEntry);
}

void ff::EntityDomain::DeleteEntities()
{
	while (_entities.Size())
	{
		DeleteEntity(_entities.GetLast());
	}
}

bool ff::EntityDomain::IsEntityActive(Entity entity)
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	return entityEntry && entityEntry->_active;
}

ff::EntityDomain::ComponentFactoryEntry* ff::EntityDomain::AddComponentFactory(std::type_index componentType, CreateComponentFactoryFunc factoryFunc)
{
	auto iter = _componentFactories.GetKey(componentType);
	if (!iter)
	{
		ComponentFactoryEntry factoryEntry;
		factoryEntry._factory = factoryFunc();
		factoryEntry._bit = (uint64_t)1 << (_componentFactories.Size() % 64);
		iter = _componentFactories.SetKey(std::move(componentType), std::move(factoryEntry));
	}

	return &iter->GetEditableValue();
}

ff::EntityDomain::ComponentFactoryEntry* ff::EntityDomain::GetComponentFactory(std::type_index componentType)
{
	auto i = _componentFactories.GetKey(componentType);
	return i ? &i->GetEditableValue() : nullptr;
}

void ff::EntityDomain::SetComponent(Entity entity, ComponentFactoryEntry* factoryEntry, void* component, bool usedExisting)
{
	if (!usedExisting)
	{
		EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
		assert(entityEntry->_components.Find(factoryEntry) == INVALID_SIZE);

		entityEntry->_componentBits |= factoryEntry->_bit;
		entityEntry->_components.Push(factoryEntry);

		RegisterEntityWithBuckets(entityEntry, factoryEntry, component);
	}
}

void* ff::EntityDomain::GetComponent(Entity entity, ComponentFactoryEntry* factoryEntry)
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	return ((entityEntry->_componentBits & factoryEntry->_bit) != 0)
		? factoryEntry->_factory->Lookup(entity)
		: nullptr;
}

bool ff::EntityDomain::DeleteComponent(Entity entity, ComponentFactoryEntry* factoryEntry)
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	size_t i = (entityEntry->_componentBits & factoryEntry->_bit) != 0
		? entityEntry->_components.Find(factoryEntry)
		: INVALID_SIZE;

	if (i != INVALID_SIZE)
	{
		verify(factoryEntry->_factory->Delete(entity));

		entityEntry->_components.Delete(i);
		entityEntry->_componentBits = 0;

		for (ComponentFactoryEntry* factoryEntry : entityEntry->_components)
		{
			entityEntry->_componentBits |= factoryEntry->_bit;
		}

		UnregisterEntityWithBuckets(entityEntry, factoryEntry);

		return true;
	}

	return false;
}

ff::Vector<ff::EntityDomain::ComponentFactoryBucketEntry> ff::EntityDomain::FindComponentEntries(const BucketEntryBase::ComponentEntry* componentEntries)
{
	ff::Vector<ComponentFactoryBucketEntry> entries;
	for (const BucketEntryBase::ComponentEntry* componentEntry = componentEntries; componentEntry; componentEntry = componentEntry->_next)
	{
		ComponentFactoryBucketEntry bucketEntry;
		bucketEntry._factory = AddComponentFactory(componentEntry->_type, componentEntry->_factory);
		bucketEntry._offset = componentEntry->_offset;
		bucketEntry._required = componentEntry->_required;
		entries.Push(bucketEntry);
	}

	return entries;
}

void ff::EntityDomain::InitBucket(BucketBase* bucket, const BucketEntryBase::ComponentEntry* componentEntries)
{
	bucket->_components = FindComponentEntries(componentEntries);
	bucket->_requiredComponentBits = 0;

	for (const ComponentFactoryBucketEntry& factoryEntry : bucket->_components)
	{
		if (factoryEntry._required)
		{
			bucket->_requiredComponentBits |= factoryEntry._factory->_bit;
		}

		BucketComponentFactoryEntry bucketComponentEntry;
		bucketComponentEntry._bucket = bucket;
		bucketComponentEntry._offset = factoryEntry._offset;
		bucketComponentEntry._required = factoryEntry._required;

		factoryEntry._factory->_buckets.Push(bucketComponentEntry);
	}

	for (EntityEntry& entityEntry : _entities)
	{
		TryRegisterEntityWithBucket(&entityEntry, bucket);
	}
}

void ff::EntityDomain::RegisterActivatedEntity(EntityEntry* entityEntry)
{
	assert(entityEntry->_active);

	for (BucketBase* bucket : entityEntry->_buckets)
	{
		CreateBucketEntry(entityEntry, bucket);
	}
}

void ff::EntityDomain::UnregisterDeactivatedEntity(EntityEntry* entityEntry)
{
	assert(!entityEntry->_active);

	for (BucketBase* bucket : entityEntry->_buckets)
	{
		DeleteBucketEntry(entityEntry, bucket);
	}
}

static void*& GetComponentFromBucket(ff::BucketEntryBase* bucketEntry, size_t offset)
{
	return *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(bucketEntry) + offset);
}

// Called when a new component is added to an entity
void ff::EntityDomain::RegisterEntityWithBuckets(EntityEntry* entityEntry, ComponentFactoryEntry* factoryEntry, void* component)
{
	uint64_t entityBits = entityEntry->_componentBits;

	for (const BucketComponentFactoryEntry& bucketComponentEntry : factoryEntry->_buckets)
	{
		uint64_t reqBits = bucketComponentEntry._bucket->_requiredComponentBits;
		if ((reqBits & entityBits) != reqBits)
		{
			continue;
		}

		size_t bucketIndex = !bucketComponentEntry._required
			? entityEntry->_buckets.Find(bucketComponentEntry._bucket)
			: INVALID_SIZE;

		if (bucketIndex == INVALID_SIZE)
		{
			TryRegisterEntityWithBucket(entityEntry, bucketComponentEntry._bucket);
		}
		else if (entityEntry->_active)
		{
			auto iter = bucketComponentEntry._bucket->_entityToEntry.GetKey(entityEntry);
			if (!iter)
			{
				assert(false);
				continue;
			}

			// Update an existing entry with the new optional component
			BucketEntryBase* bucketEntry = iter->GetValue();
			::GetComponentFromBucket(bucketEntry, bucketComponentEntry._offset) = component;
		}
	}
}

// Called after a component is removed
void ff::EntityDomain::UnregisterEntityWithBuckets(EntityEntry* entityEntry, ComponentFactoryEntry* factoryEntry)
{
	uint64_t entityBits = entityEntry->_componentBits | factoryEntry->_bit;

	for (const BucketComponentFactoryEntry& bucketComponentEntry : factoryEntry->_buckets)
	{
		uint64_t reqBits = bucketComponentEntry._bucket->_requiredComponentBits;
		if ((reqBits & entityBits) != reqBits)
		{
			continue;
		}

		size_t bucketIndex = entityEntry->_buckets.Find(bucketComponentEntry._bucket);
		if (bucketIndex == INVALID_SIZE)
		{
			continue;
		}

		bool deleteEntity = false;

		if (bucketComponentEntry._required)
		{
			deleteEntity = true;
		}
		else if (entityEntry->_active)
		{
			// Update an existing entry
			auto iter = bucketComponentEntry._bucket->_entityToEntry.GetKey(entityEntry);
			if (!iter)
			{
				assert(false);
				continue;
			}

			BucketEntryBase* bucketEntry = iter->GetValue();
			::GetComponentFromBucket(bucketEntry, bucketComponentEntry._offset) = nullptr;

			if (entityEntry->_active && bucketComponentEntry._bucket->_requiredComponentBits == 0)
			{
				deleteEntity = true;
				for (const ComponentFactoryBucketEntry& componentBucketEntry : bucketComponentEntry._bucket->_components)
				{
					if (::GetComponentFromBucket(bucketEntry, componentBucketEntry._offset))
					{
						deleteEntity = false;
						break;
					}
				}
			}
		}

		if (deleteEntity)
		{
			if (entityEntry->_active)
			{
				DeleteBucketEntry(entityEntry, bucketComponentEntry._bucket);
			}

			entityEntry->_buckets.Delete(bucketIndex);
		}
	}
}

void ff::EntityDomain::TryRegisterEntityWithBucket(EntityEntry* entityEntry, BucketBase* bucket)
{
	uint64_t reqBits = bucket->_requiredComponentBits;
	uint64_t entityBits = entityEntry->_componentBits;

	if ((reqBits & entityBits) == reqBits && !bucket->_components.IsEmpty())
	{
		bool allFound = true;

		for (const ComponentFactoryBucketEntry& factoryEntry : bucket->_components)
		{
			if ((entityBits & factoryEntry._factory->_bit) == 0 ||
				entityEntry->_components.Find(factoryEntry._factory) == INVALID_SIZE)
			{
				if (factoryEntry._required)
				{
					allFound = false;
					break;
				}
			}
		}

		if (allFound)
		{
			entityEntry->_buckets.Push(bucket);

			if (entityEntry->_active)
			{
				CreateBucketEntry(entityEntry, bucket);
			}
		}
	}
}

void ff::EntityDomain::CreateBucketEntry(EntityEntry* entityEntry, BucketBase* bucket)
{
	assert(entityEntry->_active);

	BucketEntryBase* bucketEntry = bucket->_newEntryFunc(entityEntry);

	for (const ComponentFactoryBucketEntry& factoryEntry : bucket->_components)
	{
		void* component = GetComponent(bucketEntry->GetEntity(), factoryEntry._factory);
		assert(component != nullptr || !factoryEntry._required);
		::GetComponentFromBucket(bucketEntry, factoryEntry._offset) = component;
	}

	bucket->_entityToEntry.SetKey(bucketEntry->GetEntity(), bucketEntry);
	bucket->_notifyAdd(bucketEntry);
}

void ff::EntityDomain::DeleteBucketEntry(EntityEntry* entityEntry, BucketBase* bucket)
{
	bucket->_deleteEntryFunc(entityEntry);
}

bool ff::EntityDomain::AddEventHandler(hash_t eventId, IEntityEventHandler* handler)
{
	return AddEventHandler(GetEventEntry(eventId), nullptr, handler);
}

bool ff::EntityDomain::RemoveEventHandler(hash_t eventId, IEntityEventHandler* handler)
{
	return RemoveEventHandler(GetEventEntry(eventId), nullptr, handler);
}

bool ff::EntityDomain::AddEventHandler(hash_t eventId, Entity entity, IEntityEventHandler* handler)
{
	return AddEventHandler(GetEventEntry(eventId), entity, handler);
}

bool ff::EntityDomain::RemoveEventHandler(hash_t eventId, Entity entity, IEntityEventHandler* handler)
{
	return RemoveEventHandler(GetEventEntry(eventId), entity, handler);
}

bool ff::EntityDomain::AddEventHandler(EventHandlerEntry* eventEntry, Entity entity, IEntityEventHandler* handler)
{
	assertRetVal(eventEntry && handler, false);

	Vector<EventHandler>& eventHandlers = entity
		? EntityEntry::FromEntity(entity)->_eventHandlers
		: eventEntry->_eventHandlers;

	EventHandler eventHandler;
	eventHandler._eventId = eventEntry->_eventId;
	eventHandler._handler = handler;
	eventHandlers.Push(eventHandler);

	return true;
}

bool ff::EntityDomain::RemoveEventHandler(EventHandlerEntry* eventEntry, Entity entity, IEntityEventHandler* handler)
{
	assertRetVal(eventEntry && handler, false);

	Vector<EventHandler>& eventHandlers = entity
		? EntityEntry::FromEntity(entity)->_eventHandlers
		: eventEntry->_eventHandlers;

	EventHandler eventHandler;
	eventHandler._eventId = eventEntry->_eventId;
	eventHandler._handler = handler;
	assertRetVal(eventHandlers.DeleteItem(eventHandler), false);

	return true;
}

ff::EntityDomain::EventHandlerEntry* ff::EntityDomain::GetEventEntry(hash_t eventId)
{
	auto iter = _events.GetKey(eventId);
	if (!iter)
	{
		EventHandlerEntry eventEntry;
		eventEntry._eventId = eventId;
		iter = _events.SetKey(eventId, std::move(eventEntry));
	}

	return &iter->GetEditableValue();
}

void ff::EntityDomain::TriggerEvent(hash_t eventId, Entity entity, void* args)
{
	TriggerEvent(GetEventEntry(eventId), entity, args);
}

void ff::EntityDomain::TriggerEvent(hash_t eventId, void* args)
{
	TriggerEvent(GetEventEntry(eventId), nullptr, args);
}

void ff::EntityDomain::TriggerEvent(EventHandlerEntry* eventEntry, Entity entity, void* args)
{
	assertRet(eventEntry && eventEntry->_eventId != ENTITY_EVENT_ANY);

	// Call listeners for the specific entity first
	if (entity)
	{
		const Vector<EventHandler>& eventHandlers = EntityEntry::FromEntity(entity)->_eventHandlers;

		for (size_t i = eventHandlers.Size(); i; i--)
		{
			const EventHandler& eventHandler = eventHandlers[i - 1];
			if (eventHandler._eventId == eventEntry->_eventId || eventHandler._eventId == ENTITY_EVENT_ANY)
			{
				eventHandler._handler->OnEntityEvent(entity, eventEntry->_eventId, args);
			}
		}

		if (eventEntry->_eventId == ENTITY_EVENT_DELETED)
		{
			for (size_t i = eventHandlers.Size(); i; i--)
			{
				eventHandlers[i - 1]._handler->OnEntityDeleted(entity);
			}
		}
	}

	// Call global listeners for a specific event
	{
		const Vector<EventHandler>& eventHandlers = eventEntry->_eventHandlers;
		for (size_t i = eventHandlers.Size(); i; i--)
		{
			eventHandlers[i - 1]._handler->OnEntityEvent(entity, eventEntry->_eventId, args);
		}
	}

	// Call global listeners for any event
	{
		const Vector<EventHandler>& eventHandlers = GetEventEntry(ENTITY_EVENT_ANY)->_eventHandlers;
		for (size_t i = eventHandlers.Size(); i; i--)
		{
			eventHandlers[i - 1]._handler->OnEntityEvent(entity, eventEntry->_eventId, args);
		}
	}
}
