#include "pch.h"
#include "Data/Data.h"
#include "Data/DataWriterReader.h"
#include "Data/Stream.h"
#include "Dict/ValueTable.h"
#include "UI/Internal/XamlStream.h"
#include "UI/Internal/XamlProvider.h"
#include "Resource/Resources.h"
#include "String/StringUtil.h"
#include "UI/XamlGlobalState.h"
#include "Value/Values.h"

ff::XamlProvider::XamlProvider(XamlGlobalState* globals)
	: _globals(globals)
{
}

ff::XamlProvider::~XamlProvider()
{
}

Noesis::Ptr<Noesis::Stream> ff::XamlProvider::LoadXaml(const char* uri8)
{
	assertRetVal(_globals && uri8 && *uri8, nullptr);
	ff::String uri = ff::String::from_utf8(uri8);

	ff::ValuePtr value = _globals->GetValueAccess()->GetValue(uri);
	if (value->IsType<ff::StringValue>())
	{
		// Convert to UTF8 for parsing
		ff::StringRef str = value->GetValue<ff::StringValue>();
		ff::Vector<char> str8 = ff::StringToUTF8(str);
		ff::ComPtr<ff::IDataVector> data = ff::CreateDataVector(std::move(*(ff::Vector<BYTE>*)&str8));
		ff::ComPtr<ff::IDataReader> reader;
		assertRetVal(ff::CreateDataReader(data, 0, &reader), nullptr);
		return Noesis::MakePtr<XamlStream>(reader);
	}

	if (!value)
	{
		// Not a localized value, try resources
		ff::AutoResourceValue res = _globals->GetResourceAccess()->GetResource(uri);
		if (res.DidInit())
		{
			return Noesis::MakePtr<XamlStream>(res);
		}
	}

	assertSz(false, ff::String::format_new(L"XamlProvider can't provide: %s", uri.c_str()).c_str());
	return nullptr;
}
