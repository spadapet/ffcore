#include "pch.h"
#include "Dict/DictPersist.h"
#include "Globals/ProcessStartup.h"
#include "Value/NullValue.h"
#include "Value/Value.h"
#include "Value/ValuePersist.h"
#include "Value/ValueType.h"

ff::Map<std::type_index, ff::Value::TypeEntry> ff::Value::s_typeToEntry;
ff::Map<DWORD, ff::Value::TypeEntry*, ff::NonHasher<DWORD>> ff::Value::s_idToEntry;

static ff::IBytePoolAllocator* GetValuePool(size_t typeSize)
{
	static ff::BytePoolAllocator<sizeof(size_t) * 2, alignof(size_t)> pool2;
	static ff::BytePoolAllocator<sizeof(size_t) * 3, alignof(size_t)> pool3;
	static ff::BytePoolAllocator<sizeof(size_t) * 4, alignof(size_t)> pool4;
	static ff::BytePoolAllocator<sizeof(size_t) * 5, alignof(size_t)> pool5;
	static ff::BytePoolAllocator<sizeof(size_t) * 6, alignof(size_t)> pool6;
	static ff::BytePoolAllocator<sizeof(size_t) * 7, alignof(size_t)> pool7;
	static ff::BytePoolAllocator<sizeof(size_t) * 8, alignof(size_t)> pool8;
	static ff::BytePoolAllocator<sizeof(size_t) * 9, alignof(size_t)> pool9;
	static ff::BytePoolAllocator<sizeof(size_t) * 10, alignof(size_t)> pool10;

	if (typeSize <= sizeof(size_t) * 2) return &pool2;
	if (typeSize <= sizeof(size_t) * 3) return &pool3;
	if (typeSize <= sizeof(size_t) * 4) return &pool4;
	if (typeSize <= sizeof(size_t) * 5) return &pool5;
	if (typeSize <= sizeof(size_t) * 6) return &pool6;
	if (typeSize <= sizeof(size_t) * 7) return &pool7;
	if (typeSize <= sizeof(size_t) * 8) return &pool8;
	if (typeSize <= sizeof(size_t) * 9) return &pool9;
	if (typeSize <= sizeof(size_t) * 10) return &pool10;

	assertSzRetVal(false, L"Value type is too large", nullptr);
}

ff::Value::Value()
	: _entry(nullptr)
	, _refs(-1)
{
}

ff::Value::~Value()
{
}

bool ff::Value::Compare(const Value* value) const
{
	if (!this)
	{
		return !value;
	}

	if (this == value)
	{
		return true;
	}

	if (!value || _entry->_typeIndex != value->_entry->_typeIndex)
	{
		return false;
	}

	return _entry->_valueType->Compare(this, value);
}

bool ff::Value::CanHaveNamedChildren() const
{
	return _entry->_valueType->CanHaveNamedChildren();
}

ff::ValuePtr ff::Value::GetNamedChild(ff::StringRef name) const
{
	return _entry->_valueType->GetNamedChild(this, name);
}

ff::Vector<ff::String> ff::Value::GetChildNames(bool sorted) const
{
	ff::Vector<ff::String> names = _entry->_valueType->GetChildNames(this);
	if (sorted && names.Size())
	{
		std::sort(names.begin(), names.end());
	}

	return names;
}

bool ff::Value::CanHaveIndexedChildren() const
{
	return _entry->_valueType->CanHaveIndexedChildren();
}

ff::ValuePtr ff::Value::GetIndexChild(size_t index) const
{
	return _entry->_valueType->GetIndexChild(this, index);
}

size_t ff::Value::GetIndexChildCount() const
{
	return _entry->_valueType->GetIndexChildCount(this);
}

ff::StringRef ff::Value::GetTypeName() const
{
	return _entry->_typeName;
}

DWORD ff::Value::GetTypeId() const
{
	return _entry->_typeId;
}

std::type_index ff::Value::GetTypeIndex() const
{
	return _entry->_typeIndex;
}

ff::ComPtr<IUnknown> ff::Value::GetComObject() const
{
	return _entry->_valueType->GetComObject(this);
}

ff::String ff::Value::Print() const
{
	return _entry->_valueType->Print(this);
}

void ff::Value::DebugDump() const
{
	ff::DumpValue(ff::GetEmptyString(), this, nullptr, true);
}

bool ff::Value::Save(ff::IDataWriter* stream) const
{
	return _entry->_valueType->Save(this, stream);
}

ff::ValuePtr ff::Value::Load(DWORD typeId, ff::IDataReader* stream)
{
	const TypeEntry* entry = GetTypeEntry(typeId);
	assertRetVal(entry, nullptr);

	return entry->_valueType->Load(stream);
}

void ff::Value::AddRef() const
{
	if (_refs != -1)
	{
		_refs.fetch_add(1);
	}
}

void ff::Value::Release() const
{
	if (_refs != -1 && _refs.fetch_sub(1) == 1)
	{
		Value* value = const_cast<Value*>(this);
		_entry->_destructFunc(value);
		_entry->_allocator->DeleteBytes(value);
	}
}

void ff::Value::UnregisterAllTypes()
{
	s_typeToEntry.Clear();
	s_idToEntry.Clear();
}

bool ff::Value::IsType(std::type_index type) const
{
	return this && _entry->_typeIndex == type;
}

bool ff::Value::IsSameType(const Value* rhs) const
{
	return this && rhs && _entry == rhs->_entry;
}

ff::ValuePtr ff::Value::Convert(std::type_index type) const
{
	if (!this || IsType(type))
	{
		return this;
	}

	ff::ValuePtr newValue = _entry->_valueType->ConvertTo(this, type);
	if (!newValue)
	{
		const TypeEntry* entry = GetTypeEntry(type);
		if (entry)
		{
			newValue = entry->_valueType->ConvertFrom(this);
		}
	}

	return newValue;
}

const ff::Value::TypeEntry* ff::Value::GetTypeEntry(std::type_index typeIndex)
{
	auto iter = s_typeToEntry.GetKey(typeIndex);
	return iter ? &iter->GetValue() : nullptr;
}

const ff::Value::TypeEntry* ff::Value::GetTypeEntry(DWORD typeId)
{
	auto iter = s_idToEntry.GetKey(typeId);
	return iter ? iter->GetValue() : nullptr;
}

void ff::Value::RegisterType(TypeEntry&& entry)
{
	if (!s_typeToEntry.KeyExists(entry._typeIndex))
	{
		entry._allocator = ::GetValuePool(entry._typeSize);
		auto iter = s_typeToEntry.SetKey(std::type_index(entry._typeIndex), std::move(entry));
		s_idToEntry.SetKey(entry._typeId, &iter->GetEditableValue());
	}
}
