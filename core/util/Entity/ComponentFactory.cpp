#include "pch.h"
#include "Entity/ComponentFactory.h"

ff::ComponentFactory::ComponentFactory(
	std::unique_ptr<ff::IBytePoolAllocator>&& allocator,
	std::function<void(void*)>&& constructor,
	std::function<void(void*, const void*)>&& copyConstructor,
	std::function<void(void*)>&& destructor)
	: _allocator(std::move(allocator))
	, _constructor(std::move(constructor))
	, _copyConstructor(std::move(copyConstructor))
	, _destructor(std::move(destructor))
{
}

ff::ComponentFactory::~ComponentFactory()
{
	assert(_entityToComponent.IsEmpty());

	// just in case the component list isn't empty, clean them up
	for (auto kvp : _entityToComponent)
	{
		_destructor(kvp.GetEditableValue());
	}
}

void* ff::ComponentFactory::New(Entity entity, bool* usedExisting)
{
	void* component = Lookup(entity);

	if (usedExisting != nullptr)
	{
		*usedExisting = (component != nullptr);
	}

	if (component == nullptr)
	{
		component = _allocator->NewBytes();
		_entityToComponent.SetKey(entity, component);
		_constructor(component);
	}

	return component;
}

void* ff::ComponentFactory::Clone(Entity entity, Entity sourceEntity)
{
	void* component = Lookup(entity);
	const void* sourceComponent = Lookup(sourceEntity);
	assert(component == nullptr);

	if (component == nullptr && sourceComponent != nullptr)
	{
		component = _allocator->NewBytes();
		_entityToComponent.SetKey(entity, component);
		_copyConstructor(component, sourceComponent);
	}

	return component;
}

void* ff::ComponentFactory::Lookup(Entity entity) const
{
	auto iter = _entityToComponent.GetKey(entity);
	return iter ? iter->GetEditableValue() : nullptr;
}

bool ff::ComponentFactory::Delete(Entity entity)
{
	auto iter = _entityToComponent.GetKey(entity);
	if (iter)
	{
		void* component = iter->GetEditableValue();
		_entityToComponent.DeleteKey(*iter);
		_destructor(component);
		_allocator->DeleteBytes(component);

		return true;
	}

	return false;
}
