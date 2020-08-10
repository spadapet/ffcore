#include "pch.h"
#include "Entity/EntityDomain.h"
#include "Entity/EntityEvents.h"

const ff::hash_t ff::ENTITY_EVENT_ACTIVATED = ff::HashStaticString(L"_Activated");
const ff::hash_t ff::ENTITY_EVENT_DEACTIVATED = ff::HashStaticString(L"_Deactivated");
const ff::hash_t ff::ENTITY_EVENT_DELETED = ff::HashStaticString(L"_Deleted");
const ff::hash_t ff::ENTITY_EVENT_NULL = ff::HashStaticString(L"_Null");

ff::hash_t ff::GetEntityEventActivated()
{
	return ff::ENTITY_EVENT_ACTIVATED;
}

ff::hash_t ff::GetEntityEventDeactivated()
{
	return ff::ENTITY_EVENT_DEACTIVATED;
}

ff::hash_t ff::GetEntityEventDeleted()
{
	return ff::ENTITY_EVENT_DELETED;
}

ff::hash_t ff::GetEntityEventNull()
{
	return ff::ENTITY_EVENT_NULL;
}

ff::IEntityEventHandler::~IEntityEventHandler()
{
}

void ff::IEntityEventHandler::OnEntityEvent(Entity entity, hash_t eventId, void* eventArgs)
{
}

void ff::IEntityEventHandler::OnEntityDeleted(Entity entity)
{
}

ff::EntityEventConnection::EntityEventConnection()
{
	Init();
}

ff::EntityEventConnection::EntityEventConnection(EntityEventConnection&& rhs)
{
	_domain = rhs._domain;
	_entity = rhs._entity;
	_eventId = rhs._eventId;
	_handler = rhs._handler;

	rhs.Init();
}

ff::EntityEventConnection::~EntityEventConnection()
{
	Disconnect();
}

bool ff::EntityEventConnection::Connect(hash_t eventId, EntityDomain* domain, IEntityEventHandler* handler)
{
	return Connect(eventId, domain, nullptr, handler);
}

bool ff::EntityEventConnection::Connect(hash_t eventId, Entity entity, IEntityEventHandler* handler)
{
	return Connect(eventId, entity ? entity->GetDomain() : nullptr, entity, handler);
}

bool ff::EntityEventConnection::Connect(hash_t eventId, EntityDomain* domain, Entity entity, IEntityEventHandler* handler)
{
	Disconnect();

	assertRetVal(domain && domain->AddEventHandler(eventId, entity, handler), false);

	_domain = domain;
	_eventId = eventId;
	_entity = entity;
	_handler = handler;

	return true;
}

bool ff::EntityEventConnection::Disconnect()
{
	bool removed = false;

	if (_domain)
	{
		removed = _domain->RemoveEventHandler(_eventId, _entity, _handler);
		assert(removed);
		Init();
	}

	return removed;
}

ff::Entity ff::EntityEventConnection::GetEntity() const
{
	return _entity;
}

ff::EntityDomain* ff::EntityEventConnection::GetDomain() const
{
	return _domain;
}

void ff::EntityEventConnection::Init()
{
	_domain = nullptr;
	_eventId = 0;
	_entity = nullptr;
	_handler = nullptr;
}
