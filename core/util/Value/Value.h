#pragma once

namespace ff
{
	class IDataReader;
	class IDataWriter;
	class IValueType;
	class Value;

	typedef ff::SmartPtr<const Value> ValuePtr;

	template<typename T>
	class ValuePtrT
	{
	public:
		ValuePtrT();
		ValuePtrT(const ValuePtrT<T>& rhs);
		ValuePtrT(ValuePtrT<T>&& rhs);
		ValuePtrT(const ValuePtr& rhs);
		ValuePtrT(const Value* rhs);
		ValuePtrT(const T* rhs);

		ValuePtrT<T>& operator=(const ValuePtrT<T>& rhs);
		ValuePtrT<T>& operator=(ValuePtrT<T>&& rhs);
		ValuePtrT<T>& operator=(const ValuePtr& rhs);
		ValuePtrT<T>& operator=(const Value* rhs);

		bool operator==(const ValuePtrT<T>& rhs);
		bool operator==(const ValuePtr& rhs);
		bool operator==(const Value* rhs);

		operator const T* () const;
		operator const ValuePtr& () const;
		const T* operator->() const;
		bool operator!() const;

		void Attach(T* value);
		ValuePtr&& Move();

		std::type_index GetType() const;
		auto GetValue() const -> decltype(((T*)0)->GetValue());

	private:
		ff::ValuePtr _value;
	};

	class Value
	{
	public:
		template<typename T, typename TypeT> static void RegisterType();
		template<typename T, typename... Args> static ValuePtr New(Args&&... args);
		template<typename T, typename... Args> static ValuePtrT<T> NewT(Args&&... args);
		template<typename T> static ValuePtr NewDefault();
		template<typename T> static ValuePtrT<T> NewDefaultT();

		template<typename T> auto GetValue() const -> decltype(((T*)0)->GetValue());
		template<typename T> bool IsType() const;
		template<typename T> ValuePtr Convert() const;

		UTIL_API bool IsType(std::type_index type) const;
		UTIL_API bool IsSameType(const Value* rhs) const;
		UTIL_API ValuePtr Convert(std::type_index type) const;
		UTIL_API bool Compare(const Value* value) const;

		UTIL_API bool CanHaveNamedChildren() const;
		UTIL_API ValuePtr GetNamedChild(ff::StringRef name) const;
		UTIL_API ff::Vector<ff::String> GetChildNames(bool sorted = false) const;

		UTIL_API bool CanHaveIndexedChildren() const;
		UTIL_API ValuePtr GetIndexChild(size_t index) const;
		UTIL_API size_t GetIndexChildCount() const;

		UTIL_API ff::StringRef GetTypeName() const;
		UTIL_API DWORD GetTypeId() const;
		UTIL_API std::type_index GetTypeIndex() const;
		UTIL_API ff::ComPtr<IUnknown> GetComObject() const;
		UTIL_API ff::String Print() const;
		UTIL_API void DebugDump() const;
		UTIL_API bool Save(ff::IDataWriter* stream) const;
		UTIL_API static ValuePtr Load(DWORD typeId, ff::IDataReader* stream);

		UTIL_API void AddRef() const;
		UTIL_API void Release() const;

		static void UnregisterAllTypes();

	protected:
		UTIL_API Value();
		UTIL_API ~Value();

	private:
		struct TypeEntry
		{
			TypeEntry()
				: _typeIndex(typeid(std::nullptr_t))
			{
			}

			ff::String _typeName;
			DWORD _typeId;
			size_t _typeSize;
			std::type_index _typeIndex;
			std::unique_ptr<ff::IValueType> _valueType;
			ff::IBytePoolAllocator* _allocator;
			std::function<void(Value*)> _destructFunc;
		};

		static ff::Map<std::type_index, TypeEntry> s_typeToEntry;
		static ff::Map<DWORD, TypeEntry*, ff::NonHasher<DWORD>> s_idToEntry;

		UTIL_API static const TypeEntry* GetTypeEntry(std::type_index typeIndex);
		UTIL_API static const TypeEntry* GetTypeEntry(DWORD typeId);
		UTIL_API static void RegisterType(TypeEntry&& entry);

		const TypeEntry* _entry;
		mutable std::atomic_long _refs;
	};
}

template<typename T, typename TypeT>
void ff::Value::RegisterType()
{
	TypeEntry entry;
	entry._typeName = ff::String::from_acp(typeid(T).name());
	entry._typeId = (DWORD)(ff::HashFunc(entry._typeName) & 0xFFFFFFFF);
	entry._typeSize = sizeof(T);
	entry._typeIndex = typeid(T);
	entry._valueType = std::make_unique<TypeT>();
	entry._allocator = nullptr;
	entry._destructFunc = [](Value* value) { ((T*)value)->~T(); };

	RegisterType(std::move(entry));
}

template<typename T, typename... Args>
ff::ValuePtr ff::Value::New(Args&&... args)
{
	ff::ValuePtrT<T> value = NewT<T, Args &&...>(std::forward<Args>(args)...);
	return value.Move();
}

template<typename T, typename... Args>
ff::ValuePtrT<T> ff::Value::NewT(Args&&... args)
{
	const TypeEntry* entry = GetTypeEntry(typeid(T));
	assert(entry);

	T* value = static_cast<T*>(T::GetStaticValue(std::forward<Args>(args)...));
	if (!value)
	{
		value = static_cast<T*>((Value*)entry->_allocator->NewBytes());
		::new(value) T(std::forward<Args>(args)...);
		value->_refs = 1;
	}

	value->_entry = entry;

	ff::ValuePtrT<T> valuePtr;
	valuePtr.Attach(value);

	return valuePtr;
}

template<typename T>
ff::ValuePtr ff::Value::NewDefault()
{
	ff::ValuePtrT<T> value = NewDefaultT<T>();
	return value.Move();
}

template<typename T>
ff::ValuePtrT<T> ff::Value::NewDefaultT()
{
	const TypeEntry* entry = GetTypeEntry(typeid(T));
	assert(entry);

	ff::Value* staticValue = T::GetStaticDefaultValue();
	staticValue->_entry = entry;
	return staticValue;
}

template<typename T>
auto ff::Value::GetValue() const -> decltype(((T*)0)->GetValue())
{
	const Value* val = IsType<T>() ? this : T::GetStaticDefaultValue();
	return static_cast<const T*>(val)->GetValue();
}

template<typename T>
bool ff::Value::IsType() const
{
	return IsType(typeid(T));
}

template<typename T>
ff::ValuePtr ff::Value::Convert() const
{
	return Convert(typeid(T));
}

template<typename T>
ff::ValuePtrT<T>::ValuePtrT()
{
}

template<typename T>
ff::ValuePtrT<T>::ValuePtrT(const ValuePtrT<T>& rhs)
	: _value(rhs._value)
{
}

template<typename T>
ff::ValuePtrT<T>::ValuePtrT(ValuePtrT<T>&& rhs)
	: _value(std::move(rhs._value))
{
}

template<typename T>
ff::ValuePtrT<T>::ValuePtrT(const ValuePtr& rhs)
	: _value(rhs->Convert<T>())
{
}

template<typename T>
ff::ValuePtrT<T>::ValuePtrT(const Value* rhs)
	: _value(rhs->Convert<T>())
{
}

template<typename T>
ff::ValuePtrT<T>::ValuePtrT(const T* rhs)
	: _value(t)
{
}

template<typename T>
ff::ValuePtrT<T>& ff::ValuePtrT<T>::operator=(const ValuePtrT<T>& rhs)
{
	_value = rhs._value;
	return *this;
}

template<typename T>
ff::ValuePtrT<T>& ff::ValuePtrT<T>::operator=(ValuePtrT<T>&& rhs)
{
	_value = std::move(rhs._value);
	return *this;
}

template<typename T>
ff::ValuePtrT<T>& ff::ValuePtrT<T>::operator=(const ValuePtr& rhs)
{
	_value = rhs->Convert<T>();
	return *this;
}

template<typename T>
ff::ValuePtrT<T>& ff::ValuePtrT<T>::operator=(const Value* rhs)
{
	_value = rhs->Convert<T>();
	return *this;
}

template<typename T>
bool ff::ValuePtrT<T>::operator==(const ValuePtrT<T>& rhs)
{
	return _value == rhs;
}

template<typename T>
bool ff::ValuePtrT<T>::operator==(const ValuePtr& rhs)
{
	return _value == rhs;
}

template<typename T>
bool ff::ValuePtrT<T>::operator==(const Value* rhs)
{
	return _value == rhs;
}

template<typename T>
ff::ValuePtrT<T>::operator const T* () const
{
	return static_cast<const T*>(_value.Object());
}

template<typename T>
ff::ValuePtrT<T>::operator const ff::ValuePtr& () const
{
	return _value;
}

template<typename T>
const T* ff::ValuePtrT<T>::operator->() const
{
	return _value;
}

template<typename T>
bool ff::ValuePtrT<T>::operator!() const
{
	return !_value;
}

template<typename T>
void ff::ValuePtrT<T>::Attach(T* value)
{
	_value.Attach(value);
}

template<typename T>
ff::ValuePtr&& ff::ValuePtrT<T>::Move()
{
	return std::move(_value);
}

template<typename T>
std::type_index ff::ValuePtrT<T>::GetType() const
{
	return typeid(T);
}

template<typename T>
auto ff::ValuePtrT<T>::GetValue() const -> decltype(((T*)0)->GetValue())
{
	return _value->GetValue<T>();
}
