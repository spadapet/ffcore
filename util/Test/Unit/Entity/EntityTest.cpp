#include "pch.h"
#include "Entity/EntityDomain.h"

struct TestComponent1
{
	TestComponent1()
		: _pos(0, 0)
		, _speed(1)
	{
	}

	ff::PointInt _pos;
	float _speed;
};

struct TestComponent2
{
	TestComponent2()
		: _bounds(0, 0, 1, 1)
		, _prevBounds(0, 0, 0, 0)
	{
	}

	ff::RectInt _bounds;
	ff::RectInt _prevBounds;
};

struct TestComponent3
{
	TestComponent3()
		: _val(0)
	{
	}

	int _val;
};

struct TestBucketEntry : public ff::BucketEntry<TestComponent1, TestComponent2>
{
	bool _test;
};

struct TestBucketEntry2 : public ff::BucketEntry<TestComponent1, ff::Optional<TestComponent2>>
{
};

struct TestBucketEntry3 : public ff::BucketEntry<ff::Optional<TestComponent1>, ff::Optional<TestComponent2>>
{
};

struct TestEventArgs
{
	TestEventArgs()
		: _arg1(1)
		, _arg2(2)
	{
	}

	int _arg1;
	int _arg2;
};

class TestEventHandler : public ff::IEntityEventHandler
{
public:
	TestEventHandler()
		: _count(0)
	{
	}

	virtual void OnEntityEvent(ff::Entity entity, ff::hash_t eventId, void* eventArgs) override;

	int _count;
};

void TestEventHandler::OnEntityEvent(ff::Entity entity, ff::hash_t eventId, void* eventArgs)
{
	if (!entity)
	{
		_count += 1000;
	}

	if (eventArgs)
	{
		TestEventArgs* realArgs = (TestEventArgs*)eventArgs;
		realArgs->_arg1++;
		_count += 100;
	}

	_count++;
}

static bool EntityTestGeneral()
{
	ff::EntityDomain domain;
	ff::IEntityBucket<TestBucketEntry>* bucket = domain.GetBucket<TestBucketEntry>();

	ff::Entity entity1 = domain.CreateEntity();
	TestComponent1* t1 = domain.SetComponent<TestComponent1>(entity1);
	TestComponent2* t2 = domain.SetComponent<TestComponent2>(entity1);
	TestComponent2* t3 = domain.SetComponent<TestComponent2>(entity1);
	assertRetVal(domain.GetComponent<TestComponent1>(entity1) == t1, false);
	assertRetVal(domain.GetComponent<TestComponent2>(entity1) == t2, false);
	assertRetVal(t2 == t3, false);

	ff::RectInt testBounds(2, 3, 4, 5);
	t2->_bounds = testBounds;

	assertRetVal(bucket->GetEntries().Size() == 0, false);

	domain.ActivateEntity(entity1);
	ff::Entity entity2 = domain.CloneEntity(entity1);
	assertRetVal(bucket->GetEntries().Size() == 1, false);

	domain.ActivateEntity(entity2);
	assertRetVal(bucket->GetEntries().Size() == 2, false);

	const TestBucketEntry* firstEntry = bucket->GetEntries().GetFirst();
	assertRetVal(firstEntry, false);
	assertRetVal(firstEntry->GetEntity() == entity1, false);
	assertRetVal(firstEntry->GetComponent<TestComponent1>() == t1, false);
	assertRetVal(firstEntry->GetComponent<TestComponent2>() == t2, false);
	assertRetVal(firstEntry->GetComponent<TestComponent2>()->_bounds == testBounds, false);
	assertRetVal(firstEntry->GetComponent<TestComponent3>() == nullptr, false);

	domain.DeactivateEntity(entity1);
	assertRetVal(bucket->GetEntries().Size() == 1, false);

	ff::String testEventName(L"TestEvent");
	const ff::hash_t testEventId = ff::HashFunc(testEventName);

	TestEventHandler handler;
	assertRetVal(domain.AddEventHandler(testEventId, entity1, &handler), false);
	assertRetVal(domain.AddEventHandler(testEventId, entity2, &handler), false);
	{
		ff::EntityEventConnection eventConnection;
		assertRetVal(eventConnection.Connect(testEventId, &domain, &handler), false);

		TestEventArgs args;
		domain.TriggerEvent(testEventId);
		domain.TriggerEvent(testEventId);
		domain.TriggerEvent(ff::HashFunc(L"TestEvent2"));
		domain.TriggerEvent(testEventId, &args);
		assertRetVal(handler._count == 3103 && args._arg1 == 2 && args._arg2 == 2, false);
	}
	assertRetVal(domain.RemoveEventHandler(testEventId, entity1, &handler), false);
	assertRetVal(domain.RemoveEventHandler(testEventId, entity2, &handler), false);

	domain.DeleteEntity(entity1);
	assertRetVal(bucket->GetEntries().Size() == 1, false);

	return true;
}

static bool EntityTestAddRemoveComponent()
{
	ff::EntityDomain domain;
	ff::IEntityBucket<TestBucketEntry>* bucket = domain.GetBucket<TestBucketEntry>();

	ff::Entity entity = domain.CreateEntity();
	domain.ActivateEntity(entity);

	assertRetVal(bucket->GetEntries().Size() == 0, false);

	TestComponent1* t1 = domain.SetComponent<TestComponent1>(entity);
	assertRetVal(bucket->GetEntries().Size() == 0, false);

	TestComponent2* t2 = domain.SetComponent<TestComponent2>(entity);
	assertRetVal(bucket->GetEntries().Size() == 1, false);

	TestComponent3* t3 = domain.SetComponent<TestComponent3>(entity);
	assertRetVal(bucket->GetEntries().Size() == 1, false);

	assertRetVal(domain.DeleteComponent<TestComponent1>(entity), false);
	assertRetVal(bucket->GetEntries().Size() == 0, false);

	assertRetVal(domain.DeleteComponent<TestComponent2>(entity), false);
	assertRetVal(bucket->GetEntries().Size() == 0, false);

	assertRetVal(domain.DeleteComponent<TestComponent3>(entity), false);
	assertRetVal(bucket->GetEntries().Size() == 0, false);

	return true;
}

static bool EntityTestAddRemoveOneOptionalComponent1()
{
	ff::EntityDomain domain;
	ff::IEntityBucket<TestBucketEntry2>* bucket = domain.GetBucket<TestBucketEntry2>();

	ff::Entity entity = domain.CreateEntity();
	domain.ActivateEntity(entity);

	assertRetVal(bucket->GetEntries().Size() == 0, false);

	TestComponent1* t1 = domain.SetComponent<TestComponent1>(entity);
	assertRetVal(bucket->GetEntries().Size() == 1, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent1>() != nullptr, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent2>() == nullptr, false);

	TestComponent2* t2 = domain.SetComponent<TestComponent2>(entity);
	assertRetVal(bucket->GetEntries().Size() == 1, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent1>() != nullptr, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent2>() != nullptr, false);

	TestComponent3* t3 = domain.SetComponent<TestComponent3>(entity);
	assertRetVal(bucket->GetEntries().Size() == 1, false);

	assertRetVal(domain.DeleteComponent<TestComponent2>(entity), false);
	assertRetVal(bucket->GetEntries().Size() == 1, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent1>() != nullptr, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent2>() == nullptr, false);

	assertRetVal(domain.DeleteComponent<TestComponent1>(entity), false);
	assertRetVal(bucket->GetEntries().Size() == 0, false);

	return true;
}

static bool EntityTestAddRemoveOneOptionalComponent2()
{
	ff::EntityDomain domain;
	ff::IEntityBucket<TestBucketEntry2>* bucket = domain.GetBucket<TestBucketEntry2>();

	ff::Entity entity = domain.CreateEntity();
	domain.ActivateEntity(entity);

	assertRetVal(bucket->GetEntries().Size() == 0, false);

	TestComponent2* t2 = domain.SetComponent<TestComponent2>(entity);
	assertRetVal(bucket->GetEntries().Size() == 0, false);

	TestComponent1* t1 = domain.SetComponent<TestComponent1>(entity);
	assertRetVal(bucket->GetEntries().Size() == 1, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent1>() != nullptr, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent2>() != nullptr, false);

	assertRetVal(domain.DeleteComponent<TestComponent2>(entity), false);
	assertRetVal(bucket->GetEntries().Size() == 1, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent1>() != nullptr, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent2>() == nullptr, false);

	assertRetVal(domain.DeleteComponent<TestComponent1>(entity), false);
	assertRetVal(bucket->GetEntries().Size() == 0, false);

	return true;
}

static bool EntityTestAddRemoveAllOptionalComponents()
{
	ff::EntityDomain domain;
	ff::IEntityBucket<TestBucketEntry3>* bucket = domain.GetBucket<TestBucketEntry3>();

	ff::Entity entity = domain.CreateEntity();
	domain.ActivateEntity(entity);

	assertRetVal(bucket->GetEntries().Size() == 0, false);

	TestComponent2* t2 = domain.SetComponent<TestComponent2>(entity);
	assertRetVal(bucket->GetEntries().Size() == 1, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent1>() == nullptr, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent2>() != nullptr, false);

	TestComponent1* t1 = domain.SetComponent<TestComponent1>(entity);
	assertRetVal(bucket->GetEntries().Size() == 1, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent1>() != nullptr, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent2>() != nullptr, false);

	TestComponent3* t3 = domain.SetComponent<TestComponent3>(entity);
	assertRetVal(bucket->GetEntries().Size() == 1, false);

	assertRetVal(domain.DeleteComponent<TestComponent2>(entity), false);
	assertRetVal(bucket->GetEntries().Size() == 1, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent1>() != nullptr, false);
	assertRetVal(bucket->GetEntries().GetFirst()->GetComponent<TestComponent2>() == nullptr, false);

	assertRetVal(domain.DeleteComponent<TestComponent1>(entity), false);
	assertRetVal(bucket->GetEntries().Size() == 0, false);

	return true;
}

bool EntityTest()
{
	assertRetVal(EntityTestGeneral(), false);
	assertRetVal(EntityTestAddRemoveComponent(), false);
	assertRetVal(EntityTestAddRemoveOneOptionalComponent1(), false);
	assertRetVal(EntityTestAddRemoveOneOptionalComponent2(), false);
	assertRetVal(EntityTestAddRemoveAllOptionalComponents(), false);

	return true;
}
