#pragma once

#include "Module/Module.h"
#include "Resource/ResourcePersist.h"
#include "Resource/SharedResourceValue.h"
#include "Value/Value.h"

namespace ff
{
	class IResources;

	class ResourceValue
	{
	public:
		UTIL_API ResourceValue(IUnknown* obj, ff::StringRef name);
		UTIL_API ResourceValue(const ff::Value* value, ff::StringRef name);
		UTIL_API ResourceValue(ResourceValue&& rhs);

		UTIL_API ff::StringRef GetName() const;
		UTIL_API const ff::Value* GetValue() const;
		UTIL_API bool QueryObject(const GUID& iid, void** obj) const;

		UTIL_API bool IsValid() const;
		UTIL_API SharedResourceValue GetNewValue() const;
		UTIL_API void Invalidate(SharedResourceValue newValue);

		void StartedLoading(IResources* owner);
		ComPtr<IResources> GetLoadingOwner() const;

	private:
		ResourceValue(const ResourceValue& rhs);
		ResourceValue& operator=(const ResourceValue& rhs);

		ff::ValuePtr _value;
		SharedResourceValue _newValue;
		String _name;
		IResources* _owner;
	};

	class AutoResourceValue
	{
	public:
		UTIL_API AutoResourceValue();
		UTIL_API AutoResourceValue(IResources* resources, StringRef name);
		UTIL_API AutoResourceValue(const AutoResourceValue& rhs);
		UTIL_API AutoResourceValue(AutoResourceValue&& rhs);
		UTIL_API AutoResourceValue(SharedResourceValue value);
		UTIL_API AutoResourceValue& operator=(const AutoResourceValue& rhs);
		UTIL_API AutoResourceValue& operator=(AutoResourceValue&& rhs);

		UTIL_API void Init(IResources* resources, StringRef name);
		UTIL_API void Init(SharedResourceValue value);
		UTIL_API bool DidInit() const;
		UTIL_API const ff::Value* Flush();

		UTIL_API ff::StringRef GetName();
		UTIL_API const ff::Value* GetValue();
		UTIL_API SharedResourceValue GetResourceValue();
		UTIL_API bool QueryObject(const GUID& iid, void** obj);

	private:
		const SharedResourceValue& UpdateValue();

		SharedResourceValue _value;
	};

	template<typename T>
	class TypedResource
	{
	public:
		TypedResource();
		TypedResource(IResources* resources, StringRef name);
		TypedResource(StringRef name);
		TypedResource(const wchar_t* name);
		TypedResource(const TypedResource<T>& rhs);
		TypedResource(TypedResource<T>&& rhs);
		TypedResource& operator=(const TypedResource<T>& rhs);

		void Init(IResources* resources, StringRef name);
		void Init(StringRef name);
		void Init(const wchar_t* name);
		void Init(SharedResourceValue value);
		bool DidInit() const;

		bool HasObject();
		T* GetObject();
		T* Flush();
		bool HasCachedObject() const;
		T* GetCachedObject() const;
		T* operator->();
		SharedResourceValue GetResourceValue();

	private:
		AutoResourceValue _value;
		ValuePtr _objectValue;
		ComPtr<T> _object;
	};
}

template<typename T>
ff::TypedResource<T>::TypedResource()
{
}

template<typename T>
ff::TypedResource<T>::TypedResource(IResources* resources, StringRef name)
{
	Init(resources, name);
}

template<typename T>
ff::TypedResource<T>::TypedResource(StringRef name)
{
	Init(name);
}

template<typename T>
ff::TypedResource<T>::TypedResource(const wchar_t* name)
{
	Init(name);
}

template<typename T>
ff::TypedResource<T>::TypedResource(const TypedResource<T>& rhs)
	: _value(rhs._value)
	, _objectValue(rhs._objectValue)
	, _object(rhs._object)
{
}

template<typename T>
ff::TypedResource<T>::TypedResource(TypedResource<T>&& rhs)
	: _value(std::move(rhs._value))
	, _objectValue(std::move(rhs._objectValue))
	, _object(std::move(rhs._object))
{
}

template<typename T>
ff::TypedResource<T>& ff::TypedResource<T>::operator=(const TypedResource<T>& rhs)
{
	_value = rhs._value;
	_objectValue = rhs._objectValue;
	_object = rhs._object;
	return *this;
}

template<typename T>
void ff::TypedResource<T>::Init(IResources* resources, StringRef name)
{
	_object.Release();
	_objectValue.Release();
	return _value.Init(resources, name);
}

template<typename T>
void ff::TypedResource<T>::Init(StringRef name)
{
	Init(ff::GetThisModule().GetResources(), name);
}

template<typename T>
void ff::TypedResource<T>::Init(const wchar_t* name)
{
	Init(ff::String(name));
}

template<typename T>
void ff::TypedResource<T>::Init(SharedResourceValue value)
{
	_object.Release();
	_objectValue.Release();
	_value.Init(value);
}

template<typename T>
bool ff::TypedResource<T>::DidInit() const
{
	return _value.DidInit();
}

template<typename T>
bool ff::TypedResource<T>::HasObject()
{
	return GetObject() != nullptr;
}

template<typename T>
T* ff::TypedResource<T>::GetObject()
{
	const Value* newValue = _value.GetValue();
	if (_objectValue != newValue)
	{
		ComPtr<T> newObject;
		if (_value.QueryObject(__uuidof(T), (void**)&newObject))
		{
			ComPtr<IResourceGetClone> cloner;
			if (cloner.QueryFrom(newObject))
			{
				ComPtr<T> cloneObject;
				if (cloneObject.QueryFrom(cloner->GetResourceClone()))
				{
					newObject = cloneObject;
				}
			}

			_object = newObject;
		}
		else
		{
			_object.Release();
		}

		_objectValue = newValue;
	}

	return _object;
}

template<typename T>
T* ff::TypedResource<T>::Flush()
{
	_value.Flush();
	return GetObject();
}

template<typename T>
bool ff::TypedResource<T>::HasCachedObject() const
{
	return GetCachedObject() != nullptr;
}

template<typename T>
T* ff::TypedResource<T>::GetCachedObject() const
{
	return _object;
}

template<typename T>
T* ff::TypedResource<T>::operator->()
{
	return Flush();
}

template<typename T>
ff::SharedResourceValue ff::TypedResource<T>::GetResourceValue()
{
	return _value.GetResourceValue();
}
