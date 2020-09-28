#pragma once

#include "Resource/SharedResourceValue.h"

namespace ff
{
	class Dict;

	// Resource parsing specific properties
	UTIL_API extern StaticString RES_BASE;
	UTIL_API extern StaticString RES_COMPRESS;
	UTIL_API extern StaticString RES_DEBUG;
	UTIL_API extern StaticString RES_FILES;
	UTIL_API extern StaticString RES_IMPORT;
	UTIL_API extern StaticString RES_LOAD_LISTENER;
	UTIL_API extern StaticString RES_TEMPLATE;
	UTIL_API extern StaticString RES_TYPE;
	UTIL_API extern StaticString RES_VALUES;

	UTIL_API extern StaticString FILE_PREFIX;
	UTIL_API extern StaticString LOC_PREFIX;
	UTIL_API extern StaticString REF_PREFIX;
	UTIL_API extern StaticString RES_PREFIX;

	class __declspec(uuid("b300f926-ff48-4935-a23a-bb4e41bc126b")) __declspec(novtable)
		IResourcePersist : public IUnknown
	{
	public:
		// Loading from JSON text
		virtual bool LoadFromSource(const ff::Dict& dict) = 0;

		// Binary file cache, can store any data
		virtual bool LoadFromCache(const ff::Dict& dict) = 0;
		virtual bool SaveToCache(ff::Dict& dict) = 0;
	};

	// Used when loading this resource depends on another resource being loaded
	class __declspec(uuid("51c1442e-f87b-4771-a42c-fd2e5c3bdbf0")) __declspec(novtable)
		IResourceFinishLoading : public IUnknown
	{
	public:
		virtual Vector<ff::SharedResourceValue> GetResourceDependencies() = 0;
		virtual bool FinishLoadFromSource() = 0;
	};

	class __declspec(uuid("3ad96b43-e89a-4b1e-b033-bf0c9d3cb20f")) __declspec(novtable)
		IResourceSaveSiblings : public IUnknown
	{
	public:
		virtual ff::Dict GetSiblingResources(ff::SharedResourceValue parentValue) = 0;
	};

	class __declspec(uuid("eee78975-0c81-49f9-a380-3ac3c1a650f8")) __declspec(novtable)
		IResourceSaveFile : public IUnknown
	{
	public:
		virtual ff::String GetFileExtension() const = 0;
		virtual bool SaveToFile(ff::StringRef file) = 0;
	};

	class __declspec(uuid("e3d6c9aa-9de7-4537-b930-effec5f760c6")) __declspec(novtable)
		IResourceGetClone : public IUnknown
	{
	public:
		virtual ComPtr<IUnknown> GetResourceClone() = 0;
	};

	class __declspec(uuid("4012b652-b4a8-4793-ba2b-8e9a2478638a")) __declspec(novtable)
		IResourceLoadListener : public IUnknown
	{
	public:
		virtual void AddError(StringRef text) = 0;
		virtual void AddFile(StringRef file) = 0;
		virtual ff::SharedResourceValue AddResourceReference(StringRef name) = 0;
	};

	UTIL_API ff::Dict LoadResourcesFromFile(StringRef path, bool debug, Vector<String>& errors);
	UTIL_API ff::Dict LoadResourcesFromFileCached(StringRef path, bool debug, Vector<String>& errors);
	UTIL_API ff::Dict LoadResourcesFromJsonText(StringRef jsonText, StringRef basePath, bool debug, Vector<String>& errors);
	UTIL_API ff::Dict LoadResourcesFromJsonDict(const ff::Dict& jsonDict, StringRef basePath, bool debug, Vector<String>& errors);
	UTIL_API bool SaveResourceToCache(IUnknown* obj, ff::Dict& dict);
	UTIL_API bool SaveResourceCacheToFile(const ff::Dict& dict, ff::StringRef outputFile);
	UTIL_API bool IsResourceCacheUpToDate(StringRef inputPath, StringRef cachePath, bool debug);
}
