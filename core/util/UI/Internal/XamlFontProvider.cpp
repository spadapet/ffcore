#include "pch.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "UI/Internal/XamlFontProvider.h"
#include "UI/Internal/XamlStream.h"
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

Noesis::Ptr<Noesis::Stream> ff::XamlFontProvider::OpenFont(const char* folder8, const char* filename8) const
{
	ff::String path = ff::String::from_utf8(folder8);
	ff::String filename = ff::String::from_utf8(filename8);
	path = ff::AppendPathTail(path, filename);

	ff::ComPtr<ff::IDataFile> dataFile;
	ff::ComPtr<ff::IDataReader> dataReader;

	if (ff::FileReadable(path) &&
		ff::CreateDataFile(path, false, &dataFile) &&
		ff::CreateDataReader(dataFile, 0, &dataReader))
	{
		return Noesis::MakePtr<XamlStream>(dataReader);
	}

	return nullptr;
}

Noesis::FontSource ff::XamlFontProvider::MatchFont(
	const char* baseUri,
	const char* familyName,
	Noesis::FontWeight weight,
	Noesis::FontStretch stretch,
	Noesis::FontStyle style)
{
	return Noesis::CachedFontProvider::MatchFont(baseUri, familyName, weight, stretch, style);
}

bool ff::XamlFontProvider::FamilyExists(const char* baseUri, const char* familyName)
{
	return Noesis::CachedFontProvider::FamilyExists(baseUri, familyName);
}
