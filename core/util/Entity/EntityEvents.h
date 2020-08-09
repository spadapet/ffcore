#pragma once
#include "Entity/Entity.h"

namespace ff
{
	class EntityDomain;

	const hash_t ENTITY_EVENT_ANY = 0;
	extern const hash_t ENTITY_EVENT_ACTIVATED;
	extern const hash_t ENTITY_EVENT_DEACTIVATED;
	extern const hash_t ENTITY_EVENT_DELETED;
	extern const hash_t ENTITY_EVENT_NULL;

	UTIL_API hash_t GetEntityEventActivated();
	UTIL_API hash_t GetEntityEventDeactivated();
	UTIL_API hash_t GetEntityEventDeleted();
	UTIL_API hash_t GetEntityEventNull();

	class IEntityEventHandler
	{
	public:
		UTIL_API virtual ~IEntityEventHandler();

		UTIL_API virtual void OnEntityDeleted(Entity entity);
		UTIL_API virtual void OnEntityEvent(Entity entity, hash_t eventId, void* eventArgs);
	};

	class EntityEventConnection
	{
	public:
		UTIL_API EntityEventConnection();
		UTIL_API EntityEventConnection(EntityEventConnection&& rhs);
		UTIL_API ~EntityEventConnection();

		UTIL_API bool Connect(hash_t eventId, EntityDomain* domain, IEntityEventHandler* handler);
		UTIL_API bool Connect(hash_t eventId, Entity entity, IEntityEventHandler* handler);
		UTIL_API bool Disconnect();
		UTIL_API Entity GetEntity() const;
		UTIL_API EntityDomain* GetDomain() const;

	private:
		void Init();
		bool Connect(hash_t eventId, EntityDomain* domain, Entity entity, IEntityEventHandler* handler);

		EntityDomain* _domain;
		hash_t _eventId;
		Entity _entity;
		IEntityEventHandler* _handler;
	};
}
