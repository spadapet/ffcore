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

	ff::AutoResourceValue res = _globals->GetResourceAccess()->GetResource(uri);
	if (res.DidInit())
	{
		return Noesis::MakePtr<XamlStream>(std::move(res));
	}

	assertSz(false, ff::String::format_new(L"XamlProvider can't provide: %s", uri.c_str()).c_str());
	return nullptr;
}
