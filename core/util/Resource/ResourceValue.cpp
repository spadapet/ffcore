#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "Resource/Resources.h"
#include "Resource/ResourceValue.h"
#include "Value/Values.h"

static ff::Mutex& GetStaticMutex()
{
	static ff::Mutex s_mutex;
	return s_mutex;
}

ff::ResourceValue::ResourceValue(IUnknown* obj, ff::StringRef name)
	: _name(name)
	, _owner(nullptr)
{
	_value = obj
		? ff::Value::New<ff::ObjectValue>(obj)
		: ff::Value::New<ff::NullValue>();
}

ff::ResourceValue::ResourceValue(const ff::Value* value, StringRef name)
	: _value(value)
	, _name(name)
	, _owner(nullptr)
{
	if (!_value)
	{
		_value = ff::Value::New<ff::NullValue>();
	}
}

ff::ResourceValue::ResourceValue(ResourceValue&& rhs)
	: _value(std::move(rhs._value))
	, _newValue(std::move(rhs._newValue))
	, _name(std::move(rhs._name))
	, _owner(rhs._owner)
{
	rhs._owner = nullptr;
}

const ff::Value* ff::ResourceValue::GetValue() const
{
	return _value;
}

ff::StringRef ff::ResourceValue::GetName() const
{
	return _name;
}

bool ff::ResourceValue::QueryObject(const GUID& iid, void** obj) const
{
	noAssertRetVal(_value != nullptr && _value->IsType<ff::ObjectValue>(), false);
	return SUCCEEDED(_value->GetValue<ff::ObjectValue>()->QueryInterface(iid, obj));
}

bool ff::ResourceValue::IsValid() const
{
	return _newValue == nullptr;
}

void ff::ResourceValue::StartedLoading(IResources* owner)
{
	ff::LockMutex lock(::GetStaticMutex());
	assert(!_owner && owner);
	_owner = owner;
}

void ff::ResourceValue::Invalidate(SharedResourceValue newValue)
{
	assertRet(newValue != nullptr);

	ff::LockMutex lock(::GetStaticMutex());
	_newValue = newValue;
	_owner = nullptr;
}

ff::ComPtr<ff::IResources> ff::ResourceValue::GetLoadingOwner() const
{
	ff::LockMutex lock(::GetStaticMutex());
	return _owner;
}

ff::SharedResourceValue ff::ResourceValue::GetNewValue() const
{
	SharedResourceValue newValue;

	if (!IsValid())
	{
		ff::LockMutex lock(::GetStaticMutex());
		if (!IsValid())
		{
			newValue = _newValue;

			while (!newValue->IsValid())
			{
				newValue = newValue->GetNewValue();
			}
		}
	}

	return newValue;
}

ff::AutoResourceValue::AutoResourceValue()
{
}

ff::AutoResourceValue::AutoResourceValue(IResources* resources, StringRef name)
{
	Init(resources, name);
}

ff::AutoResourceValue::AutoResourceValue(const AutoResourceValue& rhs)
	: _value(rhs._value)
{
}

ff::AutoResourceValue::AutoResourceValue(AutoResourceValue&& rhs)
	: _value(std::move(rhs._value))
{
}

ff::AutoResourceValue::AutoResourceValue(SharedResourceValue value)
	: _value(value)
{
}

ff::AutoResourceValue& ff::AutoResourceValue::operator=(const AutoResourceValue& rhs)
{
	_value = rhs._value;
	return *this;
}

ff::AutoResourceValue& ff::AutoResourceValue::operator=(AutoResourceValue&& rhs)
{
	_value = std::move(rhs._value);
	return *this;
}

void ff::AutoResourceValue::Init(IResources* resources, StringRef name)
{
	if (resources)
	{
		_value = resources->GetResource(name);
	}
	else
	{
		ff::ValuePtr nullValue = ff::Value::New<ff::NullValue>();
		_value = std::make_shared<ResourceValue>(nullValue, name);
	}
}

void ff::AutoResourceValue::Init(SharedResourceValue value)
{
	assert(value != nullptr);
	_value = value;
}

bool ff::AutoResourceValue::DidInit() const
{
	return _value != nullptr;
}

const ff::Value* ff::AutoResourceValue::Flush()
{
	if (DidInit())
	{
		ff::ComPtr<ff::IResources> owner = _value->GetLoadingOwner();
		if (owner)
		{
			_value = owner->FlushResource(_value);
		}

		return GetValue();
	}

	return nullptr;
}

ff::StringRef ff::AutoResourceValue::GetName()
{
	return UpdateValue()->GetName();
}

const ff::Value* ff::AutoResourceValue::GetValue()
{
	return UpdateValue()->GetValue();
}

ff::SharedResourceValue ff::AutoResourceValue::GetResourceValue()
{
	return UpdateValue();
}

bool ff::AutoResourceValue::QueryObject(const GUID& iid, void** obj)
{
	return UpdateValue()->QueryObject(iid, obj);
}

const ff::SharedResourceValue& ff::AutoResourceValue::UpdateValue()
{
	if (!_value)
	{
		static ff::SharedResourceValue s_null;
		if (!s_null)
		{
			ff::LockMutex lock(::GetStaticMutex());
			if (!s_null)
			{
				ff::ValuePtr nullValue = ff::Value::New<ff::NullValue>();
				s_null = std::make_shared<ff::ResourceValue>(nullValue, ff::GetEmptyString());

				ff::SharedResourceValue* p_null = &s_null;
				ff::AtProgramShutdown([p_null]()
					{
						p_null->reset();
					});
			}
		}

		return s_null;
	}

	if (!_value->IsValid())
	{
		_value = _value->GetNewValue();
	}

	return _value;
}
