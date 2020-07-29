#include "pch.h"
#include "UI/Internal/XamlProvider.h"
#include "UI/Internal/XamlStream.h"
#include "UI/XamlGlobalState.h"

ff::XamlProvider::XamlProvider(XamlGlobalState* globals)
	: _cache(globals)
{
}

void ff::XamlProvider::Advance()
{
	_cache.Advance();
}

Noesis::Ptr<Noesis::Stream> ff::XamlProvider::LoadXaml(const char* uri8)
{
	ff::String uri = ff::String::from_utf8(uri8);
	ff::AutoResourceValue res = _cache.GetResource(uri);
	if (res.DidInit())
	{
		return Noesis::MakePtr<XamlStream>(std::move(res));
	}

	assertSz(false, ff::String::format_new(L"XamlProvider can't provide: %s", uri.c_str()).c_str());
	return nullptr;
}
