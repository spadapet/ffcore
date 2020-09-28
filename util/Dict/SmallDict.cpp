#include "pch.h"
#include "Dict/SmallDict.h"
#include "Globals/ProcessGlobals.h"
#include "Value/Values.h"

ff::SmallDict::SmallDict()
	: _data(nullptr)
{
}

ff::SmallDict::SmallDict(const SmallDict& rhs)
	: _data(nullptr)
{
	*this = rhs;
}

ff::SmallDict::SmallDict(SmallDict&& rhs)
	: _data(rhs._data)
{
	rhs._data = nullptr;
}

ff::SmallDict::~SmallDict()
{
	Clear();
}

ff::SmallDict& ff::SmallDict::operator=(const SmallDict& rhs)
{
	if (this != &rhs)
	{
		Clear();

		size_t size = rhs.Size();
		if (size)
		{
			Reserve(size, false);
			_data->size = size;

			::memcpy(_data->entries, rhs._data->entries, size * sizeof(Entry));

			for (size_t i = 0; i < size; i++)
			{
				_data->entries[i].value->AddRef();
			}
		}
	}

	return *this;
}

ff::SmallDict& ff::SmallDict::operator=(SmallDict&& rhs)
{
	std::swap(_data, rhs._data);
	return *this;
}

size_t ff::SmallDict::Size() const
{
	return _data ? _data->size : 0;
}

size_t ff::SmallDict::Allocated() const
{
	return _data ? _data->allocated : 0;
}

ff::String ff::SmallDict::KeyAt(size_t index) const
{
	assertRetVal(index < Size(), ff::GetEmptyString());

	ff::hash_t hash = _data->entries[index].hash;
	return GetAtomizer().GetString(hash);
}

ff::hash_t ff::SmallDict::KeyHashAt(size_t index) const
{
	assertRetVal(index < Size(), 0);
	return _data->entries[index].hash;
}

const ff::Value* ff::SmallDict::ValueAt(size_t index) const
{
	assertRetVal(index < Size(), nullptr);
	return _data->entries[index].value;
}

const ff::Value* ff::SmallDict::GetValue(ff::StringRef key) const
{
	size_t i = IndexOf(key);
	return (i != ff::INVALID_SIZE) ? _data->entries[i].value : nullptr;
}

size_t ff::SmallDict::IndexOf(ff::StringRef key) const
{
	size_t size = Size();
	noAssertRetVal(size, ff::INVALID_SIZE);

	ff::hash_t hash = GetAtomizer().GetHash(key);
	Entry* end = _data->entries + size;

	for (Entry* entry = _data->entries; entry != end; entry++)
	{
		if (entry->hash == hash)
		{
			return entry - _data->entries;
		}
	}

	return ff::INVALID_SIZE;
}

void ff::SmallDict::Add(ff::StringRef key, const ff::Value* value)
{
	assertRet(value);
	value->AddRef();

	size_t size = Size();
	Reserve(size + 1);

	ff::hash_t hash = GetAtomizer().CacheString(key);

	_data->entries[size].hash = hash;
	_data->entries[size].value = value;
	_data->size++;
}

void ff::SmallDict::Set(ff::StringRef key, const ff::Value* value)
{
	if (value == nullptr)
	{
		Remove(key);
		return;
	}

	size_t index = IndexOf(key);
	if (index != ff::INVALID_SIZE)
	{
		SetAt(index, value);
		return;
	}

	Add(key, value);
}

void ff::SmallDict::SetAt(size_t index, const ff::Value* value)
{
	assertRet(index < Size());
	if (value)
	{
		value->AddRef();
		_data->entries[index].value->Release();
		_data->entries[index].value = value;
	}
	else
	{
		RemoveAt(index);
	}
}

void ff::SmallDict::Remove(ff::StringRef key)
{
	if (Size())
	{
		ff::hash_t hash = GetAtomizer().GetHash(key);

		for (size_t i = 0; i < Size(); )
		{
			if (_data->entries[i].hash == hash)
			{
				RemoveAt(i);
			}
			else
			{
				i++;
			}
		}
	}
}

void ff::SmallDict::RemoveAt(size_t index)
{
	size_t size = Size();
	assertRet(index < size);

	_data->entries[index].value->Release();
	::memmove(_data->entries + index, _data->entries + index + 1, (size - index - 1) * sizeof(Entry));
	_data->size--;
}

void ff::SmallDict::Reserve(size_t newAllocated, bool allowEmptySpace)
{
	size_t oldAllocated = Allocated();
	if (newAllocated > oldAllocated)
	{
		if (allowEmptySpace)
		{
			newAllocated = std::max<size_t>(ff::NearestPowerOfTwo(newAllocated), 4);
		}

		size_t byteSize = sizeof(Data) + newAllocated * sizeof(Entry) - sizeof(Entry);
		_data = (Data*)_aligned_realloc(_data, byteSize, alignof(Data));
		_data->allocated = newAllocated;
		_data->size = oldAllocated ? _data->size : 0;
	}
}

void ff::SmallDict::Clear()
{
	size_t size = Size();
	for (size_t i = 0; i < size; i++)
	{
		_data->entries[i].value->Release();
	}

	_aligned_free(_data);
	_data = nullptr;
}

ff::StringCache& ff::SmallDict::GetAtomizer() const
{
	return ff::ProcessGlobals::Get()->GetStringCache();
}
