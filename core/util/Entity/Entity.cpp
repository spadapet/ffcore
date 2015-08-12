#include "pch.h"
#include "Entity/EntityDomain.h"

ff::EntityDomain* ff::EntityBase::GetDomain()
{
	return ff::EntityDomain::Get(this);
}

ff::Entity ff::EntityBase::Clone(StringRef name)
{
	return GetDomain()->CloneEntity(this, name);
}

ff::StringRef ff::EntityBase::GetName()
{
	return GetDomain()->GetEntityName(this);
}

ff::EntityId ff::EntityBase::GetId()
{
	return EntityDomain::GetId(this);
}

void ff::EntityBase::Activate()
{
	GetDomain()->ActivateEntity(this);
}

void ff::EntityBase::Deactivate()
{
	GetDomain()->DeactivateEntity(this);
}

void ff::EntityBase::Delete()
{
	GetDomain()->DeleteEntity(this);
}

bool ff::EntityBase::IsActive()
{
	return GetDomain()->IsEntityActive(this);
}

bool ff::EntityBase::IsPendingDeletion()
{
	return GetDomain()->IsEntityPendingDeletion(this);
}

void ff::EntityBase::TriggerEvent(hash_t eventId, EntityEventArgs* args)
{
	GetDomain()->TriggerEvent(eventId, this, args);
}

bool ff::EntityBase::AddEventHandler(hash_t eventId, IEntityEventHandler* handler)
{
	return GetDomain()->AddEventHandler(eventId, this, handler);
}

bool ff::EntityBase::RemoveEventHandler(hash_t eventId, IEntityEventHandler* handler)
{
	return GetDomain()->RemoveEventHandler(eventId, this, handler);
}

class WeakEntityListener : private ff::IEntityEventHandler
{
public:
	WeakEntityListener();
	~WeakEntityListener();

	void Connect(ff::Entity entity);
	void Disconnect();
	ff::Entity GetEntity() const;

	// IEntityEventHandler
	virtual void OnEntityDeleted(ff::Entity entity) override;

private:
	ff::Entity _entity;
};

WeakEntityListener::WeakEntityListener()
	: _entity(ff::INVALID_ENTITY)
{
}

WeakEntityListener::~WeakEntityListener()
{
	Disconnect();
}

void WeakEntityListener::Connect(ff::Entity entity)
{
	if (entity != _entity)
	{
		Disconnect();

		_entity = entity;

		if (_entity != ff::INVALID_ENTITY)
		{
			_entity->GetDomain()->AddEventHandler(ff::ENTITY_EVENT_NULL, _entity, this);
		}
	}
}

void WeakEntityListener::Disconnect()
{
	if (_entity != ff::INVALID_ENTITY)
	{
		_entity->GetDomain()->RemoveEventHandler(ff::ENTITY_EVENT_NULL, _entity, this);
		_entity = ff::INVALID_ENTITY;
	}
}

ff::Entity WeakEntityListener::GetEntity() const
{
	return _entity;
}

void WeakEntityListener::OnEntityDeleted(ff::Entity entity)
{
	Disconnect();
}

static ff::PoolAllocator<WeakEntityListener> s_weakEntityListenerPool;

ff::WeakEntity::WeakEntity()
	: _listener(s_weakEntityListenerPool.New())
{
}

ff::WeakEntity::WeakEntity(Entity entity)
	: WeakEntity()
{
	Connect(entity);
}

ff::WeakEntity::WeakEntity(const WeakEntity& rhs)
	: WeakEntity()
{
	Connect(rhs.GetEntity());
}

ff::WeakEntity::WeakEntity(WeakEntity&& rhs)
	: _listener(rhs._listener)
{
	rhs._listener = nullptr;
}

ff::WeakEntity::~WeakEntity()
{
	s_weakEntityListenerPool.Delete(_listener);
}

void ff::WeakEntity::Connect(Entity entity)
{
	_listener->Connect(entity);
}

void ff::WeakEntity::Disconnect()
{
	_listener->Disconnect();
}

void ff::WeakEntity::Delete()
{
	ff::EntityDomain* domain = GetDomain();
	if (domain)
	{
		domain->DeleteEntity(GetEntity());
	}

	assert(!IsValid());
}

bool ff::WeakEntity::IsValid() const
{
	return GetEntity() != ff::INVALID_ENTITY;
}

ff::Entity ff::WeakEntity::GetEntity() const
{
	return _listener->GetEntity();
}

ff::EntityDomain* ff::WeakEntity::GetDomain() const
{
	return ff::EntityDomain::TryGet(GetEntity());
}

ff::WeakEntity& ff::WeakEntity::operator=(Entity entity)
{
	Connect(entity);
	return *this;
}

ff::WeakEntity& ff::WeakEntity::operator=(const WeakEntity& rhs)
{
	Connect(rhs.GetEntity());
	return *this;
}

ff::WeakEntity& ff::WeakEntity::operator=(WeakEntity&& rhs)
{
	_listener = rhs._listener;
	rhs._listener = nullptr;
	return *this;
}

ff::Entity ff::WeakEntity::operator->() const
{
	Entity entity = GetEntity();
	assert(entity);
	return entity;
}

ff::WeakEntity::operator ff::Entity() const
{
	return GetEntity();
}

ff::WeakEntity::operator bool() const
{
	return IsValid();
}
