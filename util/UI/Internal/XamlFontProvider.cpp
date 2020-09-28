#include "pch.h"
#include "Resource/Resources.h"
#include "String/StringUtil.h"
#include "UI/Internal/XamlFontProvider.h"
#include "UI/Internal/XamlStream.h"
#include "UI/XamlGlobalState.h"

ff::XamlFontProvider::XamlFontProvider(XamlGlobalState* globals)
	: _cache(globals)
{
}

void ff::XamlFontProvider::Advance()
{
	_cache.Advance();
}

void ff::XamlFontProvider::ScanFolder(const char* folder)
{
	ff::String prefix(L"#");
	if (folder && *folder)
	{
		prefix += ff::String::from_utf8(folder);
		prefix += L"/";
	}

	for (ff::String name : _cache.GetResourceNames())
	{
		if (name.size() > prefix.size() && !_wcsnicmp(name.c_str(), prefix.c_str(), prefix.size()))
		{
			RegisterFont(folder, ff::StringToUTF8(name).Data());
		}
	}
}

Noesis::Ptr<Noesis::Stream> ff::XamlFontProvider::OpenFont(const char* folder8, const char* filename8) const
{
	ff::String name = ff::String::from_utf8(filename8);
	if (name.size() > 0 && name[0] == L'#')
	{
		XamlResourceCache& cache = const_cast<XamlResourceCache&>(_cache);
		ff::AutoResourceValue value = cache.GetResource(name);
		if (value.DidInit())
		{
			return Noesis::MakePtr<XamlStream>(std::move(value));
		}
	}

	return nullptr;
}
