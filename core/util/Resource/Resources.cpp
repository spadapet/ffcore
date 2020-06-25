#include "pch.h"
#include "Dict/ValueTable.h"
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

	bool Init(ff::AppGlobals* globals, ff::IValueTable* values, const ff::Dict& dict);

	// IResourceAccess
	virtual ff::Vector<ff::String> GetResourceNames() const override;
	virtual ff::SharedResourceValue GetResource(ff::StringRef name) override;

	// IResources
	virtual ff::SharedResourceValue FlushResource(ff::SharedResourceValue value) override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	typedef std::weak_ptr<ff::ResourceValue> WeakResourceValue;

	struct ValueInfo;

	struct ValueLoadingInfo
	{
		HANDLE _event;
		ff::SharedResourceValue _originalValue;
		ff::SharedResourceValue _finalValue;
		ff::Vector<ValueInfo*> _childInfos;
		ff::Vector<ValueInfo*> _parentInfos;
	};

	struct ValueInfo
	{
		WeakResourceValue _value;
		ff::ValuePtr _dictValue;
		std::shared_ptr<ValueLoadingInfo> _loading;
	};

	void UpdateValueInfo(ValueInfo& info, ff::SharedResourceValue newValue);
	ff::ValuePtr CreateObjects(ValueInfo& info, ff::ValuePtr dictValue);

	ff::Mutex _mutex;
	ff::Map<ff::String, ValueInfo> _values;
	ff::AppGlobals* _globals;
	ff::ComPtr<ff::IValueTable> _valueTable;
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

bool ff::CreateResources(AppGlobals* globals, ff::IValueTable* values, const Dict& dict, ff::IResources** obj)
{
	assertRetVal(obj, false);

	ComPtr<Resources, IResources> myObj;
	assertHrRetVal(ff::ComAllocator<Resources>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(globals, values, dict), false);

	*obj = myObj.Detach();
	return true;
}

static ff::SharedResourceValue CreateNullResource(ff::StringRef name)
{
	ff::ValuePtr nullValue = ff::Value::New<ff::NullValue>();
	return std::make_shared<ff::ResourceValue>(nullValue, name);
}

Resources::Resources()
{
}

Resources::~Resources()
{
}

bool Resources::Init(ff::AppGlobals* globals, ff::IValueTable* values, const ff::Dict& dict)
{
	_globals = globals;
	_valueTable = values;
	return LoadFromCache(dict);
}

ff::Vector<ff::String> Resources::GetResourceNames() const
{
	ff::Vector<ff::String> names;
	names.Reserve(_values.Size());

	for (auto i : _values)
	{
		names.Push(i.GetKey());
	}

	return names;
}

ff::SharedResourceValue Resources::GetResource(ff::StringRef name)
{
	auto iter = _values.GetKey(name);
	noAssertRetVal(iter, ::CreateNullResource(name));
	ValueInfo& info = iter->GetEditableValue();

	ff::LockMutex lock(_mutex);
	ff::SharedResourceValue value = info._value.lock();

	if (!value)
	{
		value = ::CreateNullResource(name);
		value->SetLoadingOwner(this);

		info._value = value;
		info._loading = std::make_shared<ValueLoadingInfo>();
		info._loading->_originalValue = value;
		info._loading->_event = ff::CreateEvent();

		ff::ComPtr<Resources, IResources> keepAlive = this;
		ff::String keepName = name;
		ValueInfo* keepInfo = &info;

		ff::GetThreadPool()->AddTask([this, keepAlive, keepName, keepInfo]()
			{
				ff::ValuePtr newValue = CreateObjects(*keepInfo, keepInfo->_dictValue);
				UpdateValueInfo(*keepInfo, std::make_shared<ff::ResourceValue>(newValue, keepName));
			});
	}

	return value;
}

ff::SharedResourceValue Resources::FlushResource(ff::SharedResourceValue value)
{
	ff::ComPtr<ff::IResources> owner = value ? value->GetLoadingOwner() : nullptr;
	noAssertRetVal(owner, value);
	assertRetVal(owner == this, value);

	ff::WinHandle loadEvent;
	{
		ff::LockMutex lock(_mutex);

		auto i = _values.GetKey(value->GetName());
		if (i)
		{
			ValueInfo& info = i->GetEditableValue();
			if (info._loading)
			{
				loadEvent = ff::DuplicateHandle(info._loading->_event);
			}
		}
	}

	if (loadEvent)
	{
		ff::WaitForHandle(loadEvent);
	}

	return value->IsValid() ? value : value->GetNewValue();
}

bool Resources::LoadFromSource(const ff::Dict& dict)
{
	ff::String basePath = dict.Get<ff::StringValue>(ff::RES_BASE);
	bool debug = dict.Get<ff::BoolValue>(ff::RES_DEBUG);

	ff::Vector<ff::String> errors;
	ff::Dict resDict = ff::LoadResourcesFromJsonDict(dict, basePath, debug, errors);

	if (errors.Size())
	{
		ff::ComPtr<ff::IResourceLoadListener> loadListener;
		if (loadListener.QueryFrom(dict.Get<ff::ObjectValue>(ff::RES_LOAD_LISTENER)))
		{
			for (ff::StringRef error : errors)
			{
				loadListener->AddError(error);
			}
		}
	}

	return errors.IsEmpty() && LoadFromCache(resDict);
}

bool Resources::LoadFromCache(const ff::Dict& dict)
{
	for (ff::String name : dict.GetAllNames())
	{
		ValueInfo info;
		info._dictValue = dict.GetValue(name);
		_values.SetKey(name, info);
	}

	return true;
}

bool Resources::SaveToCache(ff::Dict& dict)
{
	for (auto i : _values)
	{
		dict.SetValue(i.GetKey(), i.GetValue()._dictValue);
	}

	return true;
}

// background thread
void Resources::UpdateValueInfo(ValueInfo& info, ff::SharedResourceValue newValue)
{
	ff::LockMutex lock(_mutex);
	ValueLoadingInfo& loadInfo = *info._loading;
	ff::Vector<ValueInfo*> parentInfos;

	loadInfo._finalValue = newValue;

	if (loadInfo._childInfos.IsEmpty())
	{
		ff::SharedResourceValue oldResource = info._value.lock();
		if (oldResource)
		{
			oldResource->Invalidate(newValue);
		}

		info._value = newValue;

		HANDLE event = loadInfo._event;
		parentInfos = std::move(loadInfo._parentInfos);
		info._loading = nullptr;

		::SetEvent(event);
		::CloseHandle(event);
	}

	for (ValueInfo* parentInfo : parentInfos)
	{
		ValueLoadingInfo& parentLoadingInfo = *parentInfo->_loading;
		verify(parentLoadingInfo._childInfos.DeleteItem(&info));

		if (parentLoadingInfo._childInfos.IsEmpty() && parentLoadingInfo._finalValue)
		{
			UpdateValueInfo(*parentInfo, parentLoadingInfo._finalValue);
		}
	}
}

// background thread
ff::ValuePtr Resources::CreateObjects(ValueInfo& info, ff::ValuePtr value)
{
	assertRetVal(value, nullptr);

	if (value->IsType<ff::SavedDataValue>())
	{
		value = CreateObjects(info, value->Convert<ff::DataValue>());
	}
	else if (value->IsType<ff::SavedDictValue>())
	{
		value = CreateObjects(info, value->Convert<ff::DictValue>());
	}
	else if (value->IsType<ff::StringValue>())
	{
		// Resolve references to other resources
		ff::StringRef str = value->GetValue<ff::StringValue>();
		ff::StringRef refPrefix = ff::REF_PREFIX.GetString();
		ff::StringRef locPrefix = ff::LOC_PREFIX.GetString();

		if (!std::wcsncmp(str.c_str(), refPrefix.c_str(), refPrefix.size()))
		{
			ff::String refName = str.substr(refPrefix.size());
			ff::SharedResourceValue refValue = GetResource(refName);

			if (refValue)
			{
				ff::LockMutex lock(_mutex);
				auto i = _values.GetKey(refName);
				if (i)
				{
					ValueInfo& refInfo = i->GetEditableValue();
					if (refInfo._loading)
					{
						info._loading->_childInfos.Push(&refInfo);
						refInfo._loading->_parentInfos.Push(&info);
					}
				}
			}

			ff::ValuePtr newValue = ff::Value::New<ff::SharedResourceWrapperValue>(refValue);
			value = CreateObjects(info, newValue);
		}
		else if (!std::wcsncmp(str.c_str(), locPrefix.c_str(), locPrefix.size()))
		{
			ff::String locName = str.substr(locPrefix.size());
			value = _valueTable ? _valueTable->GetValue(locName) : nullptr;
			assertSz(value, ff::String::format_new(L"Missing localized resource value: %s", locName.c_str()).c_str());
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
				ff::ValuePtr newValue = CreateObjects(info, dict.GetValue(name));
				dict.SetValue(name, newValue);
			}
		}

		if (!isNestedResources && type.size())
		{
			ff::AppGlobals* appGlobals = _globals ? _globals : ff::AppGlobals::Get();
			ff::ComPtr<IUnknown> obj = ff::ProcessGlobals::Get()->GetModules().CreateClass(type, appGlobals);
			ff::ComPtr<ff::IResourcePersist> resObj;
			assertRetVal(resObj.QueryFrom(obj) && resObj->LoadFromCache(dict), value);

			ff::ValuePtr newValue = ff::Value::New<ff::ObjectValue>(resObj);
			value = CreateObjects(info, newValue);
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
			vec[i] = CreateObjects(info, vec[i]);
		}

		value = ff::Value::New<ff::ValueVectorValue>(std::move(vec));
	}

	return value;
}
