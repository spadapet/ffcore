#include "pch.h"
#include "Dict/Dict.h"
#include "Dict/ValueTable.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Value/Values.h"

class __declspec(uuid("920b9c39-ced0-4ccf-865b-6a2a4e3b6365"))
	ValueTable
	: public ff::ComBase
	, public ff::IValueTable
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(ValueTable);

	// IValueTable
	virtual ff::ValuePtr GetValue(ff::StringRef name) const override;
	virtual ff::String GetString(ff::StringRef name) const override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	void UpdateUserLanguages();

	ff::Vector<ff::String> _userLangs;
	ff::Map<ff::String, ff::Dict> _langToDict;
};

BEGIN_INTERFACES(ValueTable)
	HAS_INTERFACE(ff::IValueTable)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<ValueTable>(L"values");
	});

bool ff::CreateValueTable(const Dict& dict, ff::IValueTable** obj)
{
	assertRetVal(obj, false);

	ComPtr<ValueTable, IValueTable> myObj;
	assertHrRetVal(ff::ComAllocator<ValueTable>::CreateInstance(&myObj), false);
	assertRetVal(myObj->LoadFromCache(dict), false);

	*obj = myObj.Detach();
	return true;
}

ValueTable::ValueTable()
{
	UpdateUserLanguages();
}

ValueTable::~ValueTable()
{
}

ff::ValuePtr ValueTable::GetValue(ff::StringRef name) const
{
	for (ff::StringRef langName : _userLangs)
	{
		auto iter = _langToDict.GetKey(langName);
		if (iter)
		{
			const ff::Dict& dict = iter->GetValue();
			ff::ValuePtr value = dict.GetValue(name);
			if (value)
			{
				return value;
			}
		}
	}

	return nullptr;
}

ff::String ValueTable::GetString(ff::StringRef name) const
{
	ff::ValuePtr value = GetValue(name)->Convert<ff::StringValue>();
	if (value)
	{
		return value->GetValue<ff::StringValue>();
	}

	return ff::GetEmptyString();
}

bool ValueTable::LoadFromSource(const ff::Dict& dict)
{
	return LoadFromCache(dict);
}

bool ValueTable::LoadFromCache(const ff::Dict& dict)
{
	ff::Vector<ff::String> langNames = dict.GetAllNames();
	for (ff::StringRef langName : langNames)
	{
		ff::ValuePtr langDictValue = dict.GetValue(langName)->Convert<ff::DictValue>();
		if (langDictValue)
		{
			auto iter = _langToDict.GetKey(langName);
			if (!iter)
			{
				iter = _langToDict.SetKey(langName, ff::Dict());
			}

			ff::Dict langDict = langDictValue->GetValue<ff::DictValue>();
			langDict.ExpandSavedDictValues();

			iter->GetEditableValue().Add(langDict);
		}
	}

	return true;
}

bool ValueTable::SaveToCache(ff::Dict& dict)
{
	return false;
}

void ValueTable::UpdateUserLanguages()
{
	static ff::StaticString overrideName(L"override");
	static ff::StaticString globalName(L"global");

	_userLangs.Clear();
	_userLangs.Push(overrideName);

	wchar_t fullLangName[LOCALE_NAME_MAX_LENGTH];
	if (::GetUserDefaultLocaleName(fullLangName, LOCALE_NAME_MAX_LENGTH))
	{
		ff::String langName(fullLangName);
		while (langName.size())
		{
			_userLangs.Push(langName);

			size_t lastDash = langName.rfind(L'-');
			if (lastDash == ff::INVALID_SIZE)
			{
				lastDash = 0;
			}

			langName = langName.substr(0, lastDash);
		}
	}

	_userLangs.Push(globalName);
	_userLangs.Push(ff::GetEmptyString());
}
