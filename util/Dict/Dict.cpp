#include "pch.h"
#include "Data/Data.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Dict/DictPersist.h"
#include "Globals/Log.h"
#include "Globals/ProcessGlobals.h"
#include "String/StringUtil.h"
#include "Value/Values.h"

static const size_t MAX_SMALL_DICT = 128;

ff::Dict::Dict()
{
}

ff::Dict::Dict(const Dict& rhs)
{
	*this = rhs;
}

ff::Dict::Dict(Dict&& rhs)
	: _propsLarge(std::move(rhs._propsLarge))
	, _propsSmall(std::move(rhs._propsSmall))
{
}

ff::Dict::~Dict()
{
	Clear();
}

const ff::Dict& ff::Dict::operator=(const Dict& rhs)
{
	if (this != &rhs)
	{
		Clear();

		_propsSmall = rhs._propsSmall;
		_propsLarge.reset();

		if (rhs._propsLarge != nullptr)
		{
			_propsLarge = std::make_unique<PropsMap>(*rhs._propsLarge);
		}
	}

	return *this;
}

const ff::Dict& ff::Dict::operator=(Dict&& rhs)
{
	std::swap(_propsLarge, rhs._propsLarge);
	std::swap(_propsSmall, rhs._propsSmall);
	return *this;
}

bool ff::Dict::operator==(const Dict& rhs) const
{
	noAssertRetVal(Size() == rhs.Size(), false);

	ff::Vector<ff::String> names1 = GetAllNames(true);
	ff::Vector<ff::String> names2 = rhs.GetAllNames(true);
	noAssertRetVal(names1 == names2, false);

	for (ff::StringRef name : names1)
	{
		noAssertRetVal(GetValue(name)->Compare(rhs.GetValue(name)), false);
	}

	return true;
}

void ff::Dict::Add(const Dict& rhs)
{
	ff::Vector<ff::String> names = rhs.GetAllNames();

	for (ff::StringRef name : names)
	{
		ValuePtr value = rhs.GetValue(name);
		SetValue(name, value);
	}
}

void ff::Dict::Merge(const Dict& rhs)
{
	for (ff::StringRef name : rhs.GetAllNames())
	{
		ValuePtr value = rhs.GetValue(name);
		ValuePtr dictValue = value->Convert<DictValue>();

		if (dictValue)
		{
			ValuePtr myDictValue = GetValue(name)->Convert<DictValue>();
			if (myDictValue)
			{
				Dict myDict = myDictValue->GetValue<DictValue>();
				myDict.Merge(dictValue->GetValue<DictValue>());
				value = Value::New<DictValue>(std::move(myDict));
			}
		}

		SetValue(name, value);
	}
}

void ff::Dict::ExpandSavedDictValues()
{
	for (ff::StringRef name : GetAllNames())
	{
		ValuePtr value = GetValue(name);
		if (value->IsType<ValueVectorValue>())
		{
			Vector<ValuePtr> values = value->GetValue<ValueVectorValue>();
			for (size_t i = 0; i < values.Size(); i++)
			{
				ValuePtrT<DictValue> dictValue = values[i];
				if (dictValue)
				{
					Dict dict = dictValue.GetValue();
					dict.ExpandSavedDictValues();

					value = Value::New<DictValue>(std::move(dict));
					values[i] = value;
				}
			}

			value = Value::New<ValueVectorValue>(std::move(values));
			SetValue(name, value);
		}
		else
		{
			ValuePtrT<DictValue> dictValue = value;
			if (dictValue)
			{
				Dict dict = dictValue.GetValue();
				dict.ExpandSavedDictValues();

				value = Value::New<DictValue>(std::move(dict));
				SetValue(name, value);
			}
		}
	}
}

size_t ff::Dict::Size() const
{
	size_t size = 0;

	if (_propsLarge != nullptr)
	{
		size = _propsLarge->Size();
	}
	else
	{
		size = _propsSmall.Size();
	}

	return size;
}

bool ff::Dict::IsEmpty() const
{
	bool empty = true;

	if (_propsLarge != nullptr)
	{
		empty = _propsLarge->IsEmpty();
	}
	else if (_propsSmall.Size())
	{
		empty = false;
	}

	return empty;
}

void ff::Dict::Reserve(size_t count)
{
	if (_propsLarge == nullptr)
	{
		_propsSmall.Reserve(count, false);
	}
}

void ff::Dict::Clear()
{
	_propsSmall.Clear();
	_propsLarge.reset();
}

void ff::Dict::SetValue(ff::StringRef name, const ff::Value* value)
{
	if (value)
	{
		if (_propsLarge != nullptr)
		{
			ff::hash_t hash = GetAtomizer().CacheString(name);
			_propsLarge->SetKey(hash, value);
		}
		else
		{
			_propsSmall.Set(name, value);
			CheckSize();
		}
	}
	else if (_propsLarge != nullptr)
	{
		ff::hash_t hash = GetAtomizer().CacheString(name);
		_propsLarge->UnsetKey(hash);
	}
	else
	{
		_propsSmall.Remove(name);
	}
}

ff::ValuePtr ff::Dict::GetValue(ff::StringRef name) const
{
	ValuePtr value = GetPathValue(name);

	if (!value)
	{
		if (_propsLarge != nullptr)
		{
			ff::hash_t hash = GetAtomizer().GetHash(name);
			auto iter = _propsLarge->GetKey(hash);

			if (iter)
			{
				value = iter->GetValue();
			}
		}
		else
		{
			value = _propsSmall.GetValue(name);
		}
	}

	return value;
}

ff::Vector<ff::String> ff::Dict::GetAllNames(bool sorted) const
{
	ff::Set<ff::String> nameSet;
	InternalGetAllNames(nameSet);

	ff::Vector<ff::String> names;
	names.Reserve(nameSet.Size());

	for (ff::StringRef name : nameSet)
	{
		names.Push(name);
	}

	if (sorted && names.Size())
	{
		std::sort(names.begin(), names.end());
	}

	return names;
}

void ff::Dict::DebugDump() const
{
	ff::DebugDumpDict(*this);
}

ff::ValuePtr ff::Dict::GetPathValue(ff::StringRef path) const
{
	noAssertRetVal(path.size() && path[0] == '/', nullptr);

	size_t nextThing = path.find_first_of(L"/[", 1, 2);
	ff::String name = path.substr(1, nextThing - ((nextThing != ff::INVALID_SIZE) ? 1 : 0));
	ValuePtr value = GetValue(name);

	for (; value && nextThing != ff::INVALID_SIZE; nextThing = path.find_first_of(L"/[", nextThing + 1, 2))
	{
		if (path[nextThing] == '/')
		{
			value = value->GetNamedChild(path.substr(nextThing));
			break;
		}
		else // '['
		{
			wchar_t* parseEnd = nullptr;
			size_t index = (size_t)wcstoul(path.c_str() + nextThing + 1, &parseEnd, 10);
			value = (*parseEnd == ']') ? value->GetIndexChild(index) : nullptr;
		}
	}

	return value;
}

void ff::Dict::InternalGetAllNames(ff::Set<ff::String>& names) const
{
	if (_propsLarge != nullptr)
	{
		for (const auto& iter : *_propsLarge)
		{
			ff::hash_t hash = iter.GetKey();
			ff::String name = GetAtomizer().GetString(hash);
			names.SetKey(name);
		}
	}
	else
	{
		for (size_t i = 0; i < _propsSmall.Size(); i++)
		{
			ff::String name = _propsSmall.KeyAt(i);
			names.SetKey(name);
		}
	}
}

void ff::Dict::CheckSize()
{
	if (_propsSmall.Size() > MAX_SMALL_DICT)
	{
		_propsLarge = std::make_unique<PropsMap>();

		for (size_t i = 0; i < _propsSmall.Size(); i++)
		{
			SetValue(_propsSmall.KeyAt(i), _propsSmall.ValueAt(i));
		}

		_propsSmall.Clear();
	}
}

ff::StringCache& ff::Dict::GetAtomizer() const
{
	return ff::ProcessGlobals::Get()->GetStringCache();
}
