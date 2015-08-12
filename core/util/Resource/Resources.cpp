#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Globals/ProcessGlobals.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Resource/Resources.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Value/Values.h"
#include "Windows/Handles.h"

class __declspec(uuid("676f438b-b858-4fa7-82f8-50dae9505bce"))
	Resources
	: public ff::ComBase
	, public ff::IResources
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(Resources);

	bool Init(ff::AppGlobals* globals, const ff::Dict& dict);

	// IResources
	virtual void SetResources(const ff::Dict& dict) override;
	virtual ff::Dict GetResources() const override;
	virtual void Clear() override;
	virtual bool IsLoading() const override;

	virtual bool HasResource(ff::StringRef name) override;
	virtual ff::SharedResourceValue GetResource(ff::StringRef name) override;
	virtual ff::SharedResourceValue FlushResource(ff::SharedResourceValue value) override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	typedef std::weak_ptr<ff::ResourceValue> WeakResourceValue;

	struct ValueInfo
	{
		WeakResourceValue _value;
		HANDLE _event;
	};

	typedef std::shared_ptr<ValueInfo> ValueInfoPtr;

	ff::AppGlobals* GetContext() const;
	ValueInfoPtr GetValueInfo(ff::StringRef name);
	ff::SharedResourceValue CreateNullResource(ff::StringRef name) const;

	ff::SharedResourceValue StartLoading(ff::StringRef name);
	void DoLoad(ValueInfoPtr info, ff::StringRef name, ff::ValuePtr value);
	void Invalidate(ff::StringRef name);
	void UpdateValueInfo(ValueInfoPtr info, ff::SharedResourceValue newValue);
	ff::ValuePtr CreateObjects(ff::ValuePtr value);

	ff::Mutex _mutex;
	ff::Dict _dict;
	ff::Map<ff::String, ValueInfoPtr> _values;
	ff::AppGlobals* _globals;
	std::atomic_long _loadingCount;
};

BEGIN_INTERFACES(Resources)
	HAS_INTERFACE(ff::IResources)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

#define RESOURCES_CLASS_NAME L"resources"

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<Resources>(RESOURCES_CLASS_NAME);
	});

bool ff::CreateResources(AppGlobals* globals, const Dict& dict, ff::IResources** obj)
{
	assertRetVal(obj, false);

	ComPtr<Resources, IResources> myObj;
	assertHrRetVal(ff::ComAllocator<Resources>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(globals, dict), false);

	*obj = myObj.Detach();
	return true;
}

Resources::Resources()
	: _loadingCount(0)
{
}

Resources::~Resources()
{
	Clear();
}

bool Resources::Init(ff::AppGlobals* globals, const ff::Dict& dict)
{
	_globals = globals;
	return LoadFromCache(dict);
}

void Resources::SetResources(const ff::Dict& dict)
{
	ff::LockMutex lock(_mutex);

	_dict.Add(dict);

	// Invalidate and reload existing resources that match the new names
	ff::Vector<ff::String> names = dict.GetAllNames();
	for (ff::StringRef name : names)
	{
		if (_values.KeyExists(name))
		{
			Invalidate(name);
			StartLoading(name);
		}
	}
}

ff::Dict Resources::GetResources() const
{
	ff::Dict dict;
	{
		ff::LockMutex lock(_mutex);
		dict = _dict;
	}

	return dict;
}

void Resources::Clear()
{
	ff::LockMutex lock(_mutex);

	for (const auto& i : _values)
	{
		Invalidate(i.GetKey());
	}

	_values.Clear();
	_dict.Clear();
}

bool Resources::IsLoading() const
{
	return _loadingCount != 0;
}

bool Resources::HasResource(ff::StringRef name)
{
	ff::LockMutex lock(_mutex);
	return _dict.GetValue(name) != nullptr;
}

ff::SharedResourceValue Resources::GetResource(ff::StringRef name)
{
	ff::LockMutex lock(_mutex);
	noAssertRetVal(_dict.GetValue(name), CreateNullResource(name));
	return StartLoading(name);
}

ff::SharedResourceValue Resources::FlushResource(ff::SharedResourceValue value)
{
	ff::ComPtr<ff::IResources> owner = value->GetLoadingOwner();
	noAssertRetVal(owner, value);
	assertRetVal(owner == this, value);

	ff::WinHandle loadEvent;
	{
		ff::LockMutex lock(_mutex);

		for (const auto& i : _values)
		{
			ValueInfo& info = *i.GetValue().get();
			if (info._event)
			{
				ff::SharedResourceValue curValue = info._value.lock();
				if (value == curValue)
				{
					loadEvent = ff::DuplicateHandle(info._event);
					break;
				}
			}
		}
	}

	if (loadEvent)
	{
		ff::WaitForHandle(loadEvent);
	}

	return value->IsValid() ? value : value->GetNewValue();
}

ff::AppGlobals* Resources::GetContext() const
{
	return _globals ? _globals : ff::AppGlobals::Get();
}

bool Resources::LoadFromSource(const ff::Dict& dict)
{
	ff::String basePath = dict.Get<ff::StringValue>(ff::RES_BASE);
	bool debug = dict.Get<ff::BoolValue>(ff::RES_DEBUG);

	ff::Vector<ff::String> errors;
	ff::Dict resDict = ff::LoadResourcesFromJsonDict(dict, basePath, debug, errors);

	return errors.IsEmpty() && LoadFromCache(resDict);
}

bool Resources::LoadFromCache(const ff::Dict& dict)
{
	SetResources(dict);
	return true;
}

bool Resources::SaveToCache(ff::Dict& dict)
{
	dict = GetResources();
	return true;
}

Resources::ValueInfoPtr Resources::GetValueInfo(ff::StringRef name)
{
	ff::LockMutex lock(_mutex);
	auto iter = _values.GetKey(name);

	if (!iter)
	{
		ValueInfoPtr info = std::make_shared<ValueInfo>();
		info->_value = CreateNullResource(name);
		info->_event = nullptr;
		iter = _values.SetKey(name, info);
	}

	return iter->GetValue();
}

ff::SharedResourceValue Resources::CreateNullResource(ff::StringRef name) const
{
	ff::ValuePtr nullValue = ff::Value::New<ff::NullValue>();
	return std::make_shared<ff::ResourceValue>(nullValue, name);
}

ff::SharedResourceValue Resources::StartLoading(ff::StringRef name)
{
	ff::LockMutex lock(_mutex);

	ValueInfoPtr infoPtr = GetValueInfo(name);
	ValueInfo& info = *infoPtr.get();
	ff::SharedResourceValue value = info._value.lock();

	if (value == nullptr)
	{
		value = CreateNullResource(name);
		value->StartedLoading(this);
		info._value = value;
	}

	noAssertRetVal(!info._event, value);
	noAssertRetVal(value->GetValue()->IsType<ff::NullValue>(), value);

	ff::ValuePtr dictValue = _dict.GetValue(name);
	assertRetVal(dictValue && !dictValue->IsType<ff::NullValue>(), value);

	info._event = ff::CreateEvent();
	_loadingCount.fetch_add(1);

	ff::ComPtr<Resources, IResources> keepAlive = this;
	ff::String keepName = name;

	ff::GetThreadPool()->AddTask([=]()
		{
			keepAlive->DoLoad(infoPtr, keepName, dictValue);
			keepAlive->_loadingCount.fetch_sub(1);
		});

	return value;
}

// background thread
void Resources::DoLoad(ValueInfoPtr info, ff::StringRef name, ff::ValuePtr value)
{
	value = CreateObjects(value);

	ff::SharedResourceValue newValue = std::make_shared<ff::ResourceValue>(value, name);
	UpdateValueInfo(info, newValue);
}

void Resources::Invalidate(ff::StringRef name)
{
	UpdateValueInfo(GetValueInfo(name), CreateNullResource(name));
}

void Resources::UpdateValueInfo(ValueInfoPtr infoPtr, ff::SharedResourceValue newValue)
{
	ff::LockMutex lock(_mutex);
	ValueInfo& info = *infoPtr.get();
	ff::SharedResourceValue oldResource = info._value.lock();

	if (oldResource != nullptr)
	{
		oldResource->Invalidate(newValue);
	}

	info._value = newValue;

	if (info._event)
	{
		HANDLE event = info._event;
		info._event = nullptr;

		::SetEvent(event);
		::CloseHandle(event);
	}
}

// background thread
ff::ValuePtr Resources::CreateObjects(ff::ValuePtr value)
{
	assertRetVal(value, nullptr);

	if (value->IsType<ff::SavedDataValue>())
	{
		// Uncompress all data
		ff::ValuePtr newValue = value->Convert<ff::DataValue>();
		assertRetVal(newValue, value);
		value = CreateObjects(newValue);
	}
	else if (value->IsType<ff::SavedDictValue>())
	{
		// Uncompress all dicts
		ff::ValuePtr newValue = value->Convert<ff::DictValue>();
		assertRetVal(newValue, value);
		value = CreateObjects(newValue);
	}
	else if (value->IsType<ff::StringValue>())
	{
		// Resolve references to other resources
		if (!std::wcsncmp(value->GetValue<ff::StringValue>().c_str(), ff::REF_PREFIX.GetString().c_str(), ff::REF_PREFIX.GetString().size()))
		{
			ff::String refName = value->GetValue<ff::StringValue>().substr(ff::REF_PREFIX.GetString().size());
			ff::SharedResourceValue refValue = GetResource(refName);
			ff::ValuePtr newValue = ff::Value::New<ff::SharedResourceWrapperValue>(refValue);
			value = CreateObjects(newValue);
		}
	}
	else if (value->IsType<ff::DictValue>())
	{
		// Convert dicts with a "res:type" value to a COM object
		ff::Dict dict = value->GetValue<ff::DictValue>();
		ff::String type = dict.Get<ff::StringValue>(ff::RES_TYPE);
		bool isNestedResources = (type == RESOURCES_CLASS_NAME);

		if (!isNestedResources)
		{
			ff::Vector<ff::String> names = dict.GetAllNames();
			for (ff::StringRef name : names)
			{
				ff::ValuePtr newValue = CreateObjects(dict.GetValue(name));
				dict.SetValue(name, newValue);
			}
		}

		if (!isNestedResources && type.size())
		{
			ff::ComPtr<IUnknown> obj = ff::ProcessGlobals::Get()->GetModules().CreateClass(type, GetContext());
			ff::ComPtr<ff::IResourcePersist> resObj;
			assertRetVal(resObj.QueryFrom(obj) && resObj->LoadFromCache(dict), value);

			ff::ValuePtr newValue = ff::Value::New<ff::ObjectValue>(resObj);
			value = CreateObjects(newValue);
		}
		else
		{
			value = ff::Value::New<ff::DictValue>(std::move(dict));
		}
	}
	else if (value->IsType<ff::ValueVectorValue>())
	{
		ff::Vector<ff::ValuePtr> vec = value->GetValue<ff::ValueVectorValue>();
		for (size_t i = 0; i < vec.Size(); i++)
		{
			vec[i] = CreateObjects(vec[i]);
		}

		value = ff::Value::New<ff::ValueVectorValue>(std::move(vec));
	}

	return value;
}
