#include "pch.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "UI/Internal/XamlFontProvider.h"
#include "UI/Internal/XamlStream.h"
#include "Resource/Resources.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"
#include "UI/XamlGlobalState.h"

ff::XamlFontProvider::XamlFontProvider(XamlGlobalState* globals)
	: _globals(globals)
{
}

ff::XamlFontProvider::~XamlFontProvider()
{
}

void ff::XamlFontProvider::ScanFolder(const char* folder)
{
	ff::String prefix(L"#");
	if (folder && *folder)
	{
		prefix += ff::String::from_utf8(folder);
		prefix += L"/";
	}

	for (ff::String name : _globals->GetResourceAccess()->GetResourceNames())
	{
		if (name.size() > prefix.size() && !_wcsnicmp(name.c_str(), prefix.c_str(), prefix.size()))
		{
			RegisterFont(folder, ff::StringToUTF8(name).Data());
		}
	}

	// Noesis::CachedFontProvider::ScanFolder(folder);
}

Noesis::Ptr<Noesis::Stream> ff::XamlFontProvider::OpenFont(const char* folder8, const char* filename8) const
{
	ff::String name = ff::String::from_utf8(filename8);
	if (name.size() > 0 && name[0] == L'#')
	{
		ff::AutoResourceValue value = _globals->GetResourceAccess()->GetResource(name);
		return Noesis::MakePtr<XamlStream>(value);
	}

	return nullptr;
}

Noesis::FontSource ff::XamlFontProvider::MatchFont(
	const char* baseUri,
	const char* familyName,
	Noesis::FontWeight& weight,
	Noesis::FontStretch& stretch,
	Noesis::FontStyle& style)
{
	return Noesis::CachedFontProvider::MatchFont(baseUri, familyName, weight, stretch, style);
}

bool ff::XamlFontProvider::FamilyExists(const char* baseUri, const char* familyName)
{
	return Noesis::CachedFontProvider::FamilyExists(baseUri, familyName);
}
