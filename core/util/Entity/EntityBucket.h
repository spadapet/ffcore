#pragma once
#include "Entity/Entity.h"

namespace ff
{
	struct EntityBucketEntry
	{
		Entity _entity;
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
