#include "pch.h"
#include "Entity/EntityDomain.h"

ff::EntityDomain::EntityDomain()
	: _lastEntityId(0)
{
}

ff::EntityDomain::~EntityDomain()
{
	for (EntityEntry& entityEntry : _entities)
	{
		if (entityEntry._valid)
		{
			DeleteEntity(&entityEntry);
		}
	}

	FlushDeletedEntities();
}

ff::EntityDomain* ff::EntityDomain::Get(Entity entity)
{
	EntityEntry* entry = EntityEntry::FromEntity(entity);
	return entry->_domain;
}

ff::EntityId ff::EntityDomain::GetId(Entity entity)
{
	EntityEntry* entry = EntityEntry::FromEntity(entity);
	return entry->_id;
}

ff::EntityDomain* ff::EntityDomain::TryGet(Entity entity)
{
	EntityEntry* entry = EntityEntry::FromEntity(entity);
	return entry ? entry->_domain : nullptr;
}

void ff::EntityDomain::Advance()
{
	FlushDeletedEntities();
}

ff::Entity ff::EntityDomain::CreateEntity(StringRef name)
{
	EntityEntry& entityEntry = _entities.Push();
	entityEntry._domain = this;
	entityEntry._name = name;
	entityEntry._componentBits = 0;
	entityEntry._id = ++_lastEntityId;
	entityEntry._valid = true;
	entityEntry._active = false;

	return &entityEntry;
}

ff::Entity ff::EntityDomain::CloneEntity(Entity sourceEntity, StringRef name)
{
	EntityEntry* sourceEntityEntry = EntityEntry::FromEntity(sourceEntity);
	assertRetVal(sourceEntityEntry->_valid, INVALID_ENTITY);

	Entity newEntity = CreateEntity(name);
	EntityEntry* newEntityEntry = EntityEntry::FromEntity(newEntity);

	for (const ComponentFactoryEntry* factoryEntry : sourceEntityEntry->_components)
	{
		factoryEntry->_factory->Clone(newEntity, sourceEntity);
	}

	newEntityEntry->_componentBits = sourceEntityEntry->_componentBits;
	newEntityEntry->_components = sourceEntityEntry->_components;
	newEntityEntry->_buckets = sourceEntityEntry->_buckets;

	return newEntity;
}

ff::Entity ff::EntityDomain::GetEntity(StringRef name) const
{
	auto i = _entitiesByName.GetKey(name);
	return i ? i->GetValue() : ff::INVALID_ENTITY;
}

ff::Entity ff::EntityDomain::GetEntity(EntityId id) const
{
	auto i = _entitiesById.GetKey(id);
	return i ? i->GetValue() : ff::INVALID_ENTITY;
}

ff::StringRef ff::EntityDomain::GetEntityName(Entity entity) const
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	return entityEntry->_name;
}

ff::EntityId ff::EntityDomain::GetEntityId(Entity entity) const
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	return entityEntry->_id;
}

void ff::EntityDomain::ActivateEntity(Entity entity)
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	assertRet(entityEntry->_valid);

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
	assertRet(entityEntry->_valid);

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
	noAssertRet(entityEntry->_valid);

	DeactivateEntity(entity);
	TriggerEvent(ENTITY_EVENT_DELETED, entity);

	entityEntry->_valid = false;
	_deletedEntities.Push(entityEntry);
}

void ff::EntityDomain::DeleteEntities()
{
	ff::Vector<EntityEntry*> entities = _entities.ToPointerVector();
	for (EntityEntry* entityEntry : entities)
	{
		DeleteEntity(entityEntry);
	}
}

bool ff::EntityDomain::IsEntityActive(Entity entity)
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	return entityEntry && entityEntry->_valid && entityEntry->_active;
}

bool ff::EntityDomain::IsEntityPendingDeletion(Entity entity)
{
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	return !entityEntry->_valid;
}

ff::EntityDomain::ComponentFactoryEntry* ff::EntityDomain::AddComponentFactory(
	std::type_index componentType, CreateComponentFactoryFunc factoryFunc)
{
	auto iter = _componentFactories.GetKey(componentType);
	if (!iter)
	{
		std::shared_ptr<ComponentFactory> factory = factoryFunc();
		assertRetVal(factory != nullptr, nullptr);

		ComponentFactoryEntry factoryEntry;
		factoryEntry._factory = factory;
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

ff::Component* ff::EntityDomain::AddComponent(Entity entity, ComponentFactoryEntry* factoryEntry)
{
	assertRetVal(factoryEntry, nullptr);

	bool usedExisting = false;
	Component* component = &factoryEntry->_factory->New(entity, &usedExisting);

	if (!usedExisting)
	{
		EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
		assert(entityEntry->_components.Find(factoryEntry) == INVALID_SIZE);

		entityEntry->_componentBits |= factoryEntry->_bit;
		entityEntry->_components.Push(factoryEntry);

		RegisterEntityWithBuckets(entityEntry, factoryEntry, component);
	}

	return component;
}

ff::Component* ff::EntityDomain::CloneComponent(Entity entity, Entity sourceEntity, ComponentFactoryEntry* factoryEntry)
{
	assertRetVal(factoryEntry, nullptr);
	Component* component = GetComponent(entity, factoryEntry);
	assertRetVal(component == nullptr, component);

	component = factoryEntry->_factory->Clone(entity, sourceEntity);
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);
	assert(entityEntry->_components.Find(factoryEntry) == INVALID_SIZE);

	entityEntry->_componentBits |= factoryEntry->_bit;
	entityEntry->_components.Push(factoryEntry);

	RegisterEntityWithBuckets(entityEntry, factoryEntry, component);

	return component;
}

ff::Component* ff::EntityDomain::GetComponent(Entity entity, ComponentFactoryEntry* factoryEntry)
{
	assertRetVal(factoryEntry, nullptr);
	EntityEntry* entityEntry = EntityEntry::FromEntity(entity);

	return ((entityEntry->_componentBits & factoryEntry->_bit) != 0)
		? factoryEntry->_factory->Lookup(entity)
		: nullptr;
}

bool ff::EntityDomain::DeleteComponent(Entity entity, ComponentFactoryEntry* factoryEntry)
{
	assertRetVal(factoryEntry, false);
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

ff::Vector<ff::EntityDomain::ComponentFactoryBucketEntry> ff::EntityDomain::FindComponentEntries(
	const ComponentTypeToFactoryEntry* componentTypes, size_t count)
{
	ff::Vector<ComponentFactoryBucketEntry> entries;
	entries.Reserve(count);

	for (const ComponentTypeToFactoryEntry* componentType = componentTypes;
		componentType != componentTypes + count;
		componentType++)
	{
		ComponentFactoryBucketEntry bucketEntry;
		bucketEntry._factory = AddComponentFactory(componentType->_type, componentType->_factory);
		bucketEntry._required = componentType->_required;

		assertRetVal(bucketEntry._factory != nullptr, ff::Vector<ComponentFactoryBucketEntry>());
		entries.Push(bucketEntry);
	}

	return entries;
}

void ff::EntityDomain::InitBucket(BucketBase* bucket, const ComponentTypeToFactoryEntry* componentTypes, size_t componentCount)
{
	bucket->_components = FindComponentEntries(componentTypes, componentCount);
	bucket->_requiredComponentBits = 0;

	for (const ComponentFactoryBucketEntry& factoryEntry : bucket->_components)
	{
		if (factoryEntry._required)
		{
			bucket->_requiredComponentBits |= factoryEntry._factory->_bit;
		}

		BucketComponentFactoryEntry bucketComponentEntry;
		bucketComponentEntry._bucket = bucket;
		bucketComponentEntry._bucketEntryIndex = &factoryEntry - bucket->_components.ConstData();
		bucketComponentEntry._required = factoryEntry._required;

		factoryEntry._factory->_buckets.Push(bucketComponentEntry);
	}

	for (EntityEntry& entityEntry : _entities)
	{
		TryRegisterEntityWithBucket(&entityEntry, bucket);
	}
}

void ff::EntityDomain::FlushDeletedEntities()
{
	while (_deletedEntities.Size())
	{
		Vector<EntityEntry*> entities = _deletedEntities;
		_deletedEntities.Clear();

		for (EntityEntry* entityEntry : entities)
		{
			assert(!entityEntry->_valid);

			for (const ComponentFactoryEntry* factoryEntry : entityEntry->_components)
			{
				factoryEntry->_factory->Delete(entityEntry);
			}

			_entities.Delete(*entityEntry);
		}
	}
}

void ff::EntityDomain::RegisterActivatedEntity(EntityEntry* entityEntry)
{
	assert(entityEntry->_valid && entityEntry->_active);

	if (!entityEntry->_name.empty())
	{
		_entitiesByName.InsertKey(entityEntry->_name, entityEntry);
	}

	_entitiesById.SetKey(entityEntry->_id, entityEntry);

	for (BucketBase* bucket : entityEntry->_buckets)
	{
		CreateBucketEntry(entityEntry, bucket);
	}
}

void ff::EntityDomain::UnregisterDeactivatedEntity(EntityEntry* entityEntry)
{
	assert(entityEntry->_valid && !entityEntry->_active);

	_entitiesById.UnsetKey(entityEntry->_id);

	if (!entityEntry->_name.empty())
	{
		for (auto i = _entitiesByName.GetKey(entityEntry->_name); i; i = _entitiesByName.GetNextDupeKey(*i))
		{
			if (i->GetValue() == entityEntry)
			{
				_entitiesByName.DeleteKey(*i);
				break;
			}
		}
	}

	for (BucketBase* bucket : entityEntry->_buckets)
	{
		DeleteBucketEntry(entityEntry, bucket);
	}
}

// Called when a new component is added to an entity
void ff::EntityDomain::RegisterEntityWithBuckets(EntityEntry* entityEntry, ComponentFactoryEntry* factoryEntry, Component* component)
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
			EntityBucketEntry* bucketEntry = iter->GetValue();
			void** derivedComponents = static_cast<FakeBucketEntry*>(bucketEntry)->_components;

			derivedComponents[bucketComponentEntry._bucketEntryIndex] = factoryEntry->_factory->CastToVoid(component);
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

			EntityBucketEntry* bucketEntry = iter->GetValue();
			void** derivedComponents = static_cast<FakeBucketEntry*>(bucketEntry)->_components;

			derivedComponents[bucketComponentEntry._bucketEntryIndex] = nullptr;

			if (entityEntry->_active && bucketComponentEntry._bucket->_requiredComponentBits == 0)
			{
				deleteEntity = true;
				for (size_t i = 0, componentCount = bucketComponentEntry._bucket->_components.Size();
					i < componentCount && deleteEntity; i++)
				{
					if (derivedComponents[i] != nullptr)
					{
						deleteEntity = false;
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
	assert(entityEntry->_valid && entityEntry->_active);

	EntityBucketEntry* bucketEntry = bucket->_newEntryFunc(entityEntry);

	void** derivedComponents = static_cast<FakeBucketEntry*>(bucketEntry)->_components;
	for (const ComponentFactoryBucketEntry& factoryEntry : bucket->_components)
	{
		Component* component = GetComponent(bucketEntry->_entity, factoryEntry._factory);
		assert(component != nullptr || !factoryEntry._required);

		*derivedComponents = (component != nullptr)
			? factoryEntry._factory->_factory->CastToVoid(component)
			: nullptr;

		derivedComponents++;
	}

	bucket->_entityToEntry.SetKey(bucketEntry->_entity, bucketEntry);
	bucket->_notifyAdd(bucketEntry);
}

void ff::EntityDomain::DeleteBucketEntry(EntityEntry* entityEntry, BucketBase* bucket)
{
	assert(entityEntry->_valid);
	bucket->_deleteEntryFunc(entityEntry);
}

bool ff::EntityDomain::AddEventHandler(hash_t eventId, IEntityEventHandler* handler)
{
	return AddEventHandler(GetEventEntry(eventId), INVALID_ENTITY, handler);
}

bool ff::EntityDomain::RemoveEventHandler(hash_t eventId, IEntityEventHandler* handler)
{
	return RemoveEventHandler(GetEventEntry(eventId), INVALID_ENTITY, handler);
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

	Vector<EventHandler>& eventHandlers = (entity != INVALID_ENTITY)
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

	Vector<EventHandler>& eventHandlers = (entity != INVALID_ENTITY)
		? EntityEntry::FromEntity(entity)->_eventHandlers
		: eventEntry->_eventHandlers;

	if (!eventHandlers.IsEmpty())
	{
		EventHandler eventHandler;
		eventHandler._eventId = eventEntry->_eventId;
		eventHandler._handler = handler;

		assertRetVal(eventHandlers.DeleteItem(eventHandler), false);

		return true;
	}

	return false;
}

ff::EntityDomain::EventHandlerEntry* ff::EntityDomain::GetEventEntry(hash_t eventId)
{
	auto iter = _events.GetKey(eventId);
	if (!iter)
	{
		EventHandlerEntry eventEntry;
		eventEntry._eventId = eventId;

		iter = _events.SetKey(eventId, eventEntry);
	}

	return iter ? &iter->GetEditableValue() : nullptr;
}

void ff::EntityDomain::TriggerEvent(hash_t eventId, Entity entity, EntityEventArgs* args)
{
	TriggerEvent(GetEventEntry(eventId), entity, args);
}

void ff::EntityDomain::TriggerEvent(hash_t eventId, EntityEventArgs* args)
{
	TriggerEvent(GetEventEntry(eventId), INVALID_ENTITY, args);
}

void ff::EntityDomain::TriggerEvent(EventHandlerEntry* eventEntry, Entity entity, EntityEventArgs* args)
{
	assertRet(eventEntry && eventEntry->_eventId != ENTITY_EVENT_ANY);

	// Call listeners for the specific entity first
	if (entity != INVALID_ENTITY)
	{
		bool deactivatedEvent = eventEntry->_eventId == ENTITY_EVENT_DEACTIVATED;
		bool deletedEvent = eventEntry->_eventId == ENTITY_EVENT_DELETED;

		EntityEntry* entry = EntityEntry::FromEntity(entity);
		noAssertRet(entry->_active || deactivatedEvent || deletedEvent);

		const Vector<EventHandler>& eventHandlers = entry->_eventHandlers;
		for (auto i = eventHandlers.crbegin(); i != eventHandlers.crend(); i++)
		{
			const EventHandler& eventHandler = *i;

			if (eventHandler._eventId == eventEntry->_eventId || eventHandler._eventId == ENTITY_EVENT_ANY)
			{
				eventHandler._handler->OnEntityEvent(entity, eventEntry->_eventId, args);
			}

			if (deletedEvent && !eventHandlers.IsEmpty() && &eventHandlers.GetLast() >= &eventHandler)
			{
				eventHandler._handler->OnEntityDeleted(entity);
			}
		}
	}

	// Call global listeners for a specific event
	{
		const Vector<EventHandler>& eventHandlers = eventEntry->_eventHandlers;
		for (auto i = eventHandlers.crbegin(); i != eventHandlers.crend(); i++)
		{
			const EventHandler& eventHandler = *i;
			eventHandler._handler->OnEntityEvent(entity, eventEntry->_eventId, args);
		}
	}

	// Call global listeners for any event
	{
		const Vector<EventHandler>& eventHandlers = GetEventEntry(ENTITY_EVENT_ANY)->_eventHandlers;
		for (auto i = eventHandlers.crbegin(); i != eventHandlers.crend(); i++)
		{
			const EventHandler& eventHandler = *i;
			eventHandler._handler->OnEntityEvent(entity, eventEntry->_eventId, args);
		}
	}
}
