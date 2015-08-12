#include "pch.h"
#include "Entity/ComponentFactory.h"

ff::ComponentFactory::ComponentFactory(
	std::unique_ptr<ff::IBytePoolAllocator>&& allocator,
	std::function<void(Component&)>&& constructor,
	std::function<void(Component&, const Component&)>&& copyConstructor,
	std::function<void(Component&)>&& destructor,
	std::function<void* (Component*)>&& castFromBase)
	: _allocator(std::move(allocator))
	, _constructor(std::move(constructor))
	, _copyConstructor(std::move(copyConstructor))
	, _destructor(std::move(destructor))
	, _castFromBase(std::move(castFromBase))
{
}

ff::ComponentFactory::~ComponentFactory()
{
	assert(_entityToComponent.IsEmpty());

	// just in case the component list isn't empty, clean them up
	for (auto kvp : _entityToComponent)
	{
		Component* component = kvp.GetEditableValue();
		_destructor(*component);
	}
}

ff::Component& ff::ComponentFactory::New(Entity entity, bool* usedExisting)
{
	Component* component = Lookup(entity);

	if (usedExisting != nullptr)
	{
		*usedExisting = (component != nullptr);
	}

	if (component == nullptr)
	{
		component = reinterpret_cast<Component*>(_allocator->NewBytes());
		_entityToComponent.SetKey(entity, component);
		_constructor(*component);
	}

	return *component;
}

ff::Component* ff::ComponentFactory::Clone(Entity entity, Entity sourceEntity)
{
	Component* component = Lookup(entity);
	Component* sourceComponent = Lookup(sourceEntity);
	assert(component == nullptr);

	if (component == nullptr && sourceComponent != nullptr)
	{
		component = reinterpret_cast<Component*>(_allocator->NewBytes());
		_entityToComponent.SetKey(entity, component);
		_copyConstructor(*component, *sourceComponent);
	}

	return component;
}

ff::Component* ff::ComponentFactory::Lookup(Entity entity) const
{
	auto iter = _entityToComponent.GetKey(entity);
	return iter ? iter->GetEditableValue() : nullptr;
}

bool ff::ComponentFactory::Delete(Entity entity)
{
	auto iter = _entityToComponent.GetKey(entity);
	if (iter)
	{
		Component* component = iter->GetEditableValue();
		_entityToComponent.DeleteKey(*iter);
		_destructor(*component);
		_allocator->DeleteBytes(component);

		return true;
	}

	return false;
}

void* ff::ComponentFactory::CastToVoid(Component* component) const
{
	return _castFromBase(component);
}
