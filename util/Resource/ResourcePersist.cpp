#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Data.h"
#include "Data/DataWriterReader.h"
#include "Dict/Dict.h"
#include "Dict/DictPersist.h"
#include "Dict/DictVisitor.h"
#include "Dict/JsonPersist.h"
#include "Globals/AppGlobals.h"
#include "Globals/ProcessGlobals.h"
#include "Resource/ResourcePersist.h"
#include "Resource/ResourceValue.h"
#include "String/StringUtil.h"
#include "Thread/ThreadUtil.h"
#include "Value/Values.h"
#include "Windows/FileUtil.h"
#include "Windows/Handles.h"

ff::StaticString ff::RES_BASE(L"res:base");
ff::StaticString ff::RES_COMPRESS(L"res:compress");
ff::StaticString ff::RES_DEBUG(L"res:debug");
ff::StaticString ff::RES_FILES(L"res:files");
ff::StaticString ff::RES_IMPORT(L"res:import");
ff::StaticString ff::RES_LOAD_LISTENER(L"res:loadListener");
ff::StaticString ff::RES_TEMPLATE(L"res:template");
ff::StaticString ff::RES_TYPE(L"res:type");
ff::StaticString ff::RES_VALUES(L"res:values");

ff::StaticString ff::FILE_PREFIX(L"file:");
ff::StaticString ff::LOC_PREFIX(L"loc:");
ff::StaticString ff::REF_PREFIX(L"ref:");
ff::StaticString ff::RES_PREFIX(L"res:");

class TransformerContext
{
public:
	TransformerContext(const ff::Dict& globals, bool debug);
	~TransformerContext();

	const bool IsDebug() const;
	const ff::Dict& GetGlobals() const;
	const ff::Dict& GetValues() const;
	ff::Dict& PushValues();
	ff::Dict& PushValuesFromOwnerThread(DWORD ownerThreadId);
	void PopValues();

	ff::SharedResourceValue SetReference(ff::SharedResourceValue res);
	void AddFile(ff::StringRef file);
	ff::Vector<ff::String> GetFiles() const;

private:
	ff::Mutex _mutex;
	ff::Dict _globals;
	ff::Dict _emptyDict;
	ff::Map<DWORD, ff::Vector<ff::Dict>> _threadToValues;
	ff::Map<ff::String, ff::SharedResourceValue> _references;
	ff::Set<ff::String> _files;
	bool _debug;
};

TransformerContext::TransformerContext(const ff::Dict& globals, bool debug)
	: _globals(globals)
	, _debug(debug)
{
}

TransformerContext::~TransformerContext()
{
}

const bool TransformerContext::IsDebug() const
{
	return _debug;
}

const ff::Dict& TransformerContext::GetGlobals() const
{
	return _globals;
}

const ff::Dict& TransformerContext::GetValues() const
{
	ff::LockMutex lock(_mutex);

	auto iter = _threadToValues.GetKey(::GetCurrentThreadId());
	noAssertRetVal(iter, _emptyDict);

	return iter->GetValue().GetLast();
}

ff::Dict& TransformerContext::PushValues()
{
	ff::LockMutex lock(_mutex);

	auto iter = _threadToValues.GetKey(::GetCurrentThreadId());
	if (!iter)
	{
		iter = _threadToValues.SetKey(::GetCurrentThreadId(), ff::Vector<ff::Dict>());
	}

	ff::Vector<ff::Dict>& values = iter->GetEditableValue();

	ff::Dict dict;
	if (!values.IsEmpty())
	{
		dict = values.GetLast();
	}

	values.Push(std::move(dict));

	return values.GetLast();
}

ff::Dict& TransformerContext::PushValuesFromOwnerThread(DWORD ownerThreadId)
{
	ff::LockMutex lock(_mutex);

	ff::Dict& dict = PushValues();
	auto iter = _threadToValues.GetKey(ownerThreadId);

	if (iter)
	{
		const ff::Vector<ff::Dict>& ownerValues = iter->GetValue();
		dict.Add(ownerValues.GetLast());
	}

	return dict;
}

void TransformerContext::PopValues()
{
	ff::LockMutex lock(_mutex);

	auto iter = _threadToValues.GetKey(::GetCurrentThreadId());
	assertRet(iter);

	ff::Vector<ff::Dict>& values = iter->GetEditableValue();
	values.Pop();

	if (values.IsEmpty())
	{
		_threadToValues.DeleteKey(*iter);
	}
}

ff::SharedResourceValue TransformerContext::SetReference(ff::SharedResourceValue res)
{
	ff::LockMutex lock(_mutex);

	auto i = _references.GetKey(res->GetName());
	if (i)
	{
		if (res->GetValue()->IsType<ff::NullValue>())
		{
			res = i->GetValue();
		}
		else
		{
			i->GetValue()->Invalidate(res);
		}
	}
	else
	{
		_references.SetKey(res->GetName(), res);
	}

	return res;
}

void TransformerContext::AddFile(ff::StringRef file)
{
	ff::String fileCanon = ff::CanonicalizePath(file, false, true);

	ff::LockMutex lock(_mutex);
	_files.SetKey(fileCanon);
}

ff::Vector<ff::String> TransformerContext::GetFiles() const
{
	ff::Vector<ff::String> files;

	ff::LockMutex lock(_mutex);
	files.Reserve(_files.Size());

	for (ff::StringRef file : _files)
	{
		files.Push(file);
	}

	return files;
}

///////////////////////////////////////

class TransformerBase : public ff::DictVisitorBase
{
public:
	TransformerBase(TransformerContext& context);

	void AddErrorFromListener(ff::String text);
	void AddFileFromListener(ff::String file);
	ff::SharedResourceValue AddResourceReferenceFromListener(ff::StringRef resourceName);

protected:
	virtual bool IsAsyncAllowed(const ff::Dict& dict) override;
	TransformerContext& GetContext() const;

private:
	TransformerContext& _context;
};

TransformerBase::TransformerBase(TransformerContext& context)
	: _context(context)
{
}

void TransformerBase::AddErrorFromListener(ff::String text)
{
	AddError(text);
}

void TransformerBase::AddFileFromListener(ff::String file)
{
	_context.AddFile(file);
}

ff::SharedResourceValue TransformerBase::AddResourceReferenceFromListener(ff::StringRef resourceName)
{
	ff::SharedResourceValue resVal = std::make_shared<ff::ResourceValue>((IUnknown*)nullptr, resourceName);
	return _context.SetReference(resVal);
}

bool TransformerBase::IsAsyncAllowed(const ff::Dict& dict)
{
	return IsRoot();
}

TransformerContext& TransformerBase::GetContext() const
{
	return _context;
}

///////////////////////////////////////

class __declspec(uuid("6d063b9d-9102-4dfa-800f-0838be519754"))
	ResourceListener
	: public ff::ComBase
	, public ff::IResourceLoadListener
{
public:
	DECLARE_HEADER(ResourceListener);

	void SetTransformer(TransformerBase* transformer);
	virtual void AddError(ff::StringRef text) override;
	virtual void AddFile(ff::StringRef file) override;
	virtual ff::SharedResourceValue AddResourceReference(ff::StringRef name) override;

private:
	TransformerBase* _transformer;
};

BEGIN_INTERFACES(ResourceListener)
	HAS_INTERFACE(ff::IResourceLoadListener)
END_INTERFACES()

ResourceListener::ResourceListener()
	: _transformer(nullptr)
{
}

ResourceListener::~ResourceListener()
{
	assert(_transformer == nullptr);
}

void ResourceListener::SetTransformer(TransformerBase* transformer)
{
	_transformer = transformer;
}

void ResourceListener::AddError(ff::StringRef text)
{
	noAssertRet(_transformer);
	_transformer->AddErrorFromListener(text);
}

void ResourceListener::AddFile(ff::StringRef file)
{
	noAssertRet(_transformer);
	_transformer->AddFileFromListener(file);
}

ff::SharedResourceValue ResourceListener::AddResourceReference(ff::StringRef name)
{
	noAssertRetVal(_transformer, ff::SharedResourceValue());
	return _transformer->AddResourceReferenceFromListener(name);
}

///////////////////////////////////////

class ExpandFilePathsTransformer : public TransformerBase
{
public:
	ExpandFilePathsTransformer(TransformerContext& context);

protected:
	virtual ff::ValuePtr TransformValue(ff::ValuePtr value) override;
};

ExpandFilePathsTransformer::ExpandFilePathsTransformer(TransformerContext& context)
	: TransformerBase(context)
{
}

ff::ValuePtr ExpandFilePathsTransformer::TransformValue(ff::ValuePtr value)
{
	value = TransformerBase::TransformValue(value);

	if (value && value->IsType<ff::StringValue>())
	{
		ff::String stringValue = value->GetValue<ff::StringValue>();

		if (!std::wcsncmp(stringValue.c_str(), ff::FILE_PREFIX.GetString().c_str(), ff::FILE_PREFIX.GetString().size()))
		{
			ff::String fullFile = GetContext().GetGlobals().Get<ff::StringValue>(ff::RES_BASE);
			ff::AppendPathTail(fullFile, stringValue.substr(5));
			fullFile = ff::CanonicalizePath(fullFile);

			if (ff::FileExists(fullFile))
			{
				GetContext().AddFile(fullFile);
			}
			else
			{
				AddError(ff::String::format_new(L"File not found: %s", fullFile.c_str()));
			}

			value = ff::Value::New<ff::StringValue>(ff::String(fullFile));
		}
	}

	return value;
}

///////////////////////////////////////

class ExpandValuesAndTemplatesTransformer : public TransformerBase
{
public:
	ExpandValuesAndTemplatesTransformer(TransformerContext& context);

protected:
	virtual ff::ValuePtr TransformDict(const ff::Dict& dict) override;
	virtual ff::ValuePtr TransformValue(ff::ValuePtr value) override;
	virtual void OnAsyncThreadStarted(DWORD mainThreadId) override;
	virtual void OnAsyncThreadDone() override;

private:
	bool Import(ff::StringRef file, ff::Dict& json);
};

ExpandValuesAndTemplatesTransformer::ExpandValuesAndTemplatesTransformer(TransformerContext& context)
	: TransformerBase(context)
{
}

ff::ValuePtr ExpandValuesAndTemplatesTransformer::TransformDict(const ff::Dict& dict)
{
	ff::Dict inputDict = dict;
	bool valuesPushed = false;
	ff::String templateName;

	ff::ValuePtr valuesValue = inputDict.GetValue(ff::RES_VALUES);
	if (valuesValue)
	{
		inputDict.SetValue(ff::RES_VALUES, nullptr);

		ff::ValuePtr valuesDict = valuesValue->Convert<ff::DictValue>();
		if (valuesDict)
		{
			GetContext().PushValues().Add(valuesDict->GetValue<ff::DictValue>());
			valuesPushed = true;
		}
	}

	ff::AtScope popValues([valuesPushed, this]()
		{
			if (valuesPushed)
			{
				GetContext().PopValues();
			}
		});

	ff::ValuePtr templateNameValue = inputDict.GetValue(ff::RES_TEMPLATE);
	if (templateNameValue)
	{
		inputDict.SetValue(ff::RES_TEMPLATE, nullptr);

		if (templateNameValue->IsType<ff::StringValue>())
		{
			templateName = templateNameValue->GetValue<ff::StringValue>();
		}
	}

	ff::ValuePtr importValue = inputDict.GetValue(ff::RES_IMPORT);
	if (importValue)
	{
		inputDict.SetValue(ff::RES_IMPORT, nullptr);

		if (importValue->IsType<ff::StringValue>())
		{
			ff::Dict importDict;
			if (!Import(importValue->GetValue<ff::StringValue>(), importDict))
			{
				return nullptr;
			}

			importDict.Add(inputDict);
			inputDict = importDict;
		}
	}

	if (templateName.size())
	{
		ff::ValuePtr templateValue = GetContext().GetValues().GetValue(templateName);
		if (!templateValue)
		{
			AddError(ff::String::format_new(L"Invalid template name: ", templateName.c_str()));
			return nullptr;
		}

		ff::ValuePtr dictValue = templateValue->Convert<ff::DictValue>();
		if (dictValue)
		{
			ff::Dict outputDict = dictValue->GetValue<ff::DictValue>();
			outputDict.Add(inputDict);
			return TransformDict(outputDict);
		}

		if (inputDict.Size())
		{
			AddError(ff::String::format_new(L"Unexpected extra values for template: ", templateName.c_str()));
			return nullptr;
		}

		return TransformValue(templateValue);
	}

	return TransformerBase::TransformDict(inputDict);
}

ff::ValuePtr ExpandValuesAndTemplatesTransformer::TransformValue(ff::ValuePtr value)
{
	value = TransformerBase::TransformValue(value);

	if (value && value->IsType<ff::StringValue>())
	{
		ff::String stringValue = value->GetValue<ff::StringValue>();

		if (!std::wcsncmp(stringValue.c_str(), ff::RES_PREFIX.GetString().c_str(), ff::RES_PREFIX.GetString().size()))
		{
			value = GetContext().GetValues().GetValue(stringValue.substr(ff::RES_PREFIX.GetString().size()));
			value = TransformValue(value);

			if (!value)
			{
				AddError(ff::String::format_new(L"Undefined value: %s", stringValue.c_str()));
			}
		}
		else if (!std::wcsncmp(stringValue.c_str(), ff::REF_PREFIX.GetString().c_str(), ff::REF_PREFIX.GetString().size()))
		{
			ff::SharedResourceValue resVal = std::make_shared<ff::ResourceValue>((IUnknown*)nullptr, stringValue.substr(4));
			resVal = GetContext().SetReference(resVal);

			value = ff::Value::New<ff::SharedResourceWrapperValue>(resVal);
		}
	}

	return value;
}

void ExpandValuesAndTemplatesTransformer::OnAsyncThreadStarted(DWORD mainThreadId)
{
	TransformerBase::OnAsyncThreadStarted(mainThreadId);
	GetContext().PushValuesFromOwnerThread(mainThreadId);
}

void ExpandValuesAndTemplatesTransformer::OnAsyncThreadDone()
{
	GetContext().PopValues();
	TransformerBase::OnAsyncThreadDone();
}

bool ExpandValuesAndTemplatesTransformer::Import(ff::StringRef file, ff::Dict& json)
{
	json.Clear();

	ff::String jsonText;
	if (!ff::ReadWholeFile(file, jsonText))
	{
		AddError(ff::String::format_new(L"Failed to read JSON import file: %s", file.c_str()));
		return false;
	}

	size_t errorPos = ff::INVALID_SIZE;
	ff::Dict dict = ff::JsonParse(jsonText, &errorPos);

	if (errorPos != ff::INVALID_SIZE)
	{
		AddError(ff::String::format_new(L"Failed to read JSON import file: %s", file.c_str()));
		AddError(ff::String::format_new(L"Failed parsing JSON at pos: %lu", errorPos));
		AddError(ff::String::format_new(L"  -->%s", jsonText.substr(errorPos, std::min<size_t>(32, jsonText.size() - errorPos)).c_str()));
		return false;
	}

	ff::ValuePtr outputValue = TransformDict(dict);
	ff::ValuePtr outputDict = outputValue->Convert<ff::DictValue>();
	if (!outputDict)
	{
		AddError(ff::String::format_new(L"Imported JSON is not a dict: %s", file.c_str()));
		return false;
	}

	json = outputDict->GetValue<ff::DictValue>();
	return true;
}

///////////////////////////////////////

class StartLoadObjectsFromDictTransformer : public TransformerBase
{
public:
	StartLoadObjectsFromDictTransformer(TransformerContext& context);
	~StartLoadObjectsFromDictTransformer();

protected:
	virtual ff::ValuePtr TransformDict(const ff::Dict& dict) override;

private:
	ff::ComPtr<ResourceListener, ff::IResourceLoadListener> _listener;
};

StartLoadObjectsFromDictTransformer::StartLoadObjectsFromDictTransformer(TransformerContext& context)
	: TransformerBase(context)
{
	ff::ComAllocator<ResourceListener>::CreateInstance(&_listener);
	_listener->SetTransformer(this);
}

StartLoadObjectsFromDictTransformer::~StartLoadObjectsFromDictTransformer()
{
	_listener->SetTransformer(nullptr);
}

ff::ValuePtr StartLoadObjectsFromDictTransformer::TransformDict(const ff::Dict& dict)
{
	ff::ValuePtr outputValue = TransformerBase::TransformDict(dict);

	if (IsRoot())
	{
		// Save references to all root objects

		ff::ValuePtr dictValue = outputValue->Convert<ff::DictValue>();
		if (dictValue)
		{
			ff::Dict outputDict = dictValue->GetValue<ff::DictValue>();

			for (ff::StringRef name : outputDict.GetAllNames())
			{
				ff::ValuePtr objectValue = outputDict.GetValue(name);
				if (objectValue->IsType<ff::ObjectValue>())
				{
					GetContext().SetReference(std::make_shared<ff::ResourceValue>(objectValue, name));
				}
			}

			outputValue = ff::Value::New<ff::DictValue>(std::move(outputDict));
		}
	}
	else
	{
		// Convert all typed object dicts into objects

		ff::ValuePtr outputDictValue = outputValue->Convert<ff::DictValue>();
		if (outputDictValue)
		{
			const ff::Dict& outputDict = outputDictValue->GetValue<ff::DictValue>();
			ff::ValuePtr typeValue = outputDict.GetValue(ff::RES_TYPE);

			if (typeValue && typeValue->IsType<ff::StringValue>())
			{
				ff::Dict outputDict2 = GetContext().GetGlobals();
				outputDict2.Set<ff::ObjectValue>(ff::RES_LOAD_LISTENER, _listener);
				outputDict2.Add(outputDict);

				ff::StringRef typeName = typeValue->GetValue<ff::StringValue>();
				ff::ComPtr<IUnknown> obj = ff::ProcessGlobals::Get()->GetModules().CreateClass(typeName, nullptr);

				ff::ComPtr<ff::IResourcePersist> resourcePersist;
				if (resourcePersist.QueryFrom(obj) && resourcePersist->LoadFromSource(outputDict2))
				{
					outputValue = ff::Value::New<ff::ObjectValue>(resourcePersist);
				}
				else
				{
					AddError(ff::String::format_new(L"Failed to create object of type: %s", typeName.c_str()));
				}
			}
		}
	}

	return outputValue;
}

///////////////////////////////////////

class FinishLoadObjectsFromDictTransformer : public TransformerBase
{
public:
	FinishLoadObjectsFromDictTransformer(TransformerContext& context);

protected:
	virtual ff::ValuePtr TransformValue(ff::ValuePtr value) override;

private:
	bool FinishLoadingObject(IUnknown* obj);

	struct FinishingInfo
	{
		DWORD _threadId;
		HANDLE _event;
	};

	ff::Mutex _mutex;
	ff::Map<IUnknown*, FinishingInfo> _currentlyFinishing;
	ff::Set<IUnknown*> _finished;
};

FinishLoadObjectsFromDictTransformer::FinishLoadObjectsFromDictTransformer(TransformerContext& context)
	: TransformerBase(context)
{
}

ff::ValuePtr FinishLoadObjectsFromDictTransformer::TransformValue(ff::ValuePtr value)
{
	value = TransformerBase::TransformValue(value);

	if (value && value->IsType<ff::ObjectValue>())
	{
		if (!FinishLoadingObject(value->GetValue<ff::ObjectValue>()))
		{
			AddError(ff::String(L"Failed to finish loading resource"));
		}
	}

	return value;
}

bool FinishLoadObjectsFromDictTransformer::FinishLoadingObject(IUnknown* obj)
{
	// See if it's already loading
	{
		ff::LockMutex lock(_mutex);
		if (_finished.KeyExists(obj))
		{
			return true;
		}

		auto iter = _currentlyFinishing.GetKey(obj);
		if (iter)
		{
			const FinishingInfo& info = iter->GetValue();
			if (info._threadId == ::GetCurrentThreadId())
			{
				AddError(ff::String(L"Can't finish loading a resource that depends on itself"));
			}

			ff::WinHandle waitHandle = ff::DuplicateHandle(info._event);
			lock.Unlock();

			::WaitForSingleObject(waitHandle, INFINITE);
			return true;
		}
		else
		{
			FinishingInfo info;
			info._threadId = ::GetCurrentThreadId();
			info._event = ff::CreateEvent();
			_currentlyFinishing.SetKey(obj, info);
		}
	}

	// Load dependencies
	bool result = true;
	{
		ff::ComPtr<ff::IResourceFinishLoading> res;
		if (res.QueryFrom(obj))
		{
			for (ff::SharedResourceValue dep : res->GetResourceDependencies())
			{
				ff::ComPtr<IUnknown> depObj;
				if (!dep->QueryObject(__uuidof(IUnknown), (void**)&depObj) || !FinishLoadingObject(depObj))
				{
					AddError(ff::String::format_new(L"Failed to finish loading dependent resource: %s", dep->GetName().c_str()));
					result = false;
					break;
				}
			}

			result = res->FinishLoadFromSource();
		}
	}

	// Done
	{
		ff::LockMutex lock(_mutex);

		auto iter = _currentlyFinishing.GetKey(obj);
		const FinishingInfo& info = iter->GetValue();
		HANDLE eventHandle = info._event;
		_currentlyFinishing.DeleteKey(*iter);

		::SetEvent(eventHandle);
		::CloseHandle(eventHandle);

		_finished.SetKey(obj);
	}

	return result;
}

///////////////////////////////////////

class ExtractResourceSiblingsTransformer : public TransformerBase
{
public:
	ExtractResourceSiblingsTransformer(TransformerContext& context);

protected:
	virtual ff::ValuePtr TransformDict(const ff::Dict& dict) override;
};

ExtractResourceSiblingsTransformer::ExtractResourceSiblingsTransformer(TransformerContext& context)
	: TransformerBase(context)
{
}

ff::ValuePtr ExtractResourceSiblingsTransformer::TransformDict(const ff::Dict& dict)
{
	ff::ValuePtr rootValue = TransformerBase::TransformDict(dict);

	if (IsRoot())
	{
		ff::ValuePtr dictValue = rootValue->Convert<ff::DictValue>();
		if (dictValue)
		{
			ff::Dict outputDict = dictValue->GetValue<ff::DictValue>();

			for (ff::StringRef name : outputDict.GetAllNames())
			{
				ff::ValuePtr objectValue = outputDict.GetValue(name);
				if (objectValue->IsType<ff::ObjectValue>())
				{
					ff::ComPtr<ff::IResourceSaveSiblings> res;
					if (res.QueryFrom(objectValue->GetValue<ff::ObjectValue>()))
					{
						ff::SharedResourceValue resValue = std::make_shared<ff::ResourceValue>(objectValue, name);
						outputDict.Add(res->GetSiblingResources(resValue));
					}
				}
			}

			rootValue = ff::Value::New<ff::DictValue>(std::move(outputDict));
		}
	}

	return rootValue;
}

///////////////////////////////////////

class SaveObjectsToDictTransformer : public TransformerBase
{
public:
	SaveObjectsToDictTransformer(TransformerContext& context);

protected:
	virtual ff::ValuePtr TransformValue(ff::ValuePtr value) override;
};

SaveObjectsToDictTransformer::SaveObjectsToDictTransformer(TransformerContext& context)
	: TransformerBase(context)
{
}

ff::ValuePtr SaveObjectsToDictTransformer::TransformValue(ff::ValuePtr value)
{
	value = TransformerBase::TransformValue(value);

	if (value && value->IsType<ff::ObjectValue>())
	{
		ff::Dict dict;
		if (!ff::SaveResourceToCache(value->GetValue<ff::ObjectValue>(), dict))
		{
			AddError(ff::String(L"Failed to save resource"));
			return nullptr;
		}

		value = ff::Value::New<ff::SavedDictValue>(dict, false);
	}

	return value;
}

///////////////////////////////////////

class CompressRootDictsTransformer : public TransformerBase
{
public:
	CompressRootDictsTransformer(TransformerContext& context);

protected:
	virtual ff::ValuePtr TransformRootValue(ff::ValuePtr value) override;
	virtual ff::ValuePtr TransformDict(const ff::Dict& dict) override;
};

CompressRootDictsTransformer::CompressRootDictsTransformer(TransformerContext& context)
	: TransformerBase(context)
{
}

ff::ValuePtr CompressRootDictsTransformer::TransformRootValue(ff::ValuePtr value)
{
	ff::ValuePtr resDictValue = value->Convert<ff::DictValue>();
	if (resDictValue)
	{
		ff::Dict resDict = resDictValue->GetValue<ff::DictValue>();
		bool compress = resDict.Get<ff::BoolValue>(ff::RES_COMPRESS, true);
		resDict.SetValue(ff::RES_COMPRESS, nullptr);

		value = ff::Value::New<ff::SavedDictValue>(resDict, compress);
	}

	return value;
}

ff::ValuePtr CompressRootDictsTransformer::TransformDict(const ff::Dict& dict)
{
	if (!IsRoot() && dict.GetValue(ff::RES_COMPRESS))
	{
		ff::Dict newDict(dict);
		newDict.SetValue(ff::RES_COMPRESS, nullptr);
		return TransformerBase::TransformDict(newDict);
	}

	return TransformerBase::TransformDict(dict);
}

///////////////////////////////////////

static ff::String GetCacheFileName(ff::StringRef path, bool debug)
{
	ff::String pathCanon = ff::CanonicalizePath(path, false, true);
	ff::String cachePath = ff::GetLocalUserDirectory();
	ff::AppendPathTail(cachePath, ff::String(L"CachedAssetPackages"));

	ff::String name = ff::GetPathTail(path);
	ff::ChangePathExtension(name, ff::GetEmptyString());

	return ff::AppendPathTail(cachePath, ff::String::format_new(
		L"%s.%lu%s.pack",
		name.c_str(),
		ff::HashFunc(pathCanon),
		debug ? L".debug" : L""));
}

static ff::Dict LoadCachedResources(ff::StringRef path, bool debug)
{
	(void)debug; // unused

	noAssertRetVal(path.size(), ff::Dict());

	const FILETIME badTime{ 0, 0 };
	const FILETIME cacheTime = ff::GetFileModifiedTime(path);

	if (::CompareFileTime(&cacheTime, &badTime) == 0)
	{
		// Cache doesn't exist
		return ff::Dict();
	}

	ff::ComPtr<ff::IData> data;
	assertRetVal(ff::ReadWholeFileMemMapped(path, &data), ff::Dict());

	ff::Dict dict;
	ff::ComPtr<ff::IDataReader> reader;
	assertRetVal(ff::CreateDataReader(data, 0, &reader), ff::Dict());
	assertRetVal(ff::LoadDict(reader, dict), ff::Dict());

	ff::ValuePtr filesValue = dict.GetValue(ff::RES_FILES);
	noAssertRetVal(filesValue, ff::Dict());

	ff::ValuePtr vectorValue = filesValue->Convert<ff::StringVectorValue>();
	assertRetVal(vectorValue, ff::Dict());

	const ff::Vector<ff::String>& files = vectorValue->GetValue<ff::StringVectorValue>();
	for (ff::StringRef file : files)
	{
		const FILETIME fileTime = ff::GetFileModifiedTime(file);
		if (::CompareFileTime(&fileTime, &badTime) == 0 || ::CompareFileTime(&cacheTime, &fileTime) < 0)
		{
			// Cache is out of date
			return ff::Dict();
		}
	}

	return dict;
}

ff::Dict ff::LoadResourcesFromFile(StringRef path, bool debug, Vector<String>& errors)
{
	ff::String text;
	assertRetVal(ff::ReadWholeFile(path, text), ff::Dict());

	ff::String basePath = path;
	ff::StripPathTail(basePath);

	Dict dict = ff::LoadResourcesFromJsonText(text, basePath, debug, errors);
	assertRetVal(errors.IsEmpty(), dict);

	ff::Vector<ff::String> files;
	ff::ValuePtr filesValue = dict.GetValue(ff::RES_FILES);
	if (filesValue)
	{
		ff::ValuePtr filesStringVectorValue = filesValue->Convert<ff::StringVectorValue>();
		if (filesStringVectorValue)
		{
			files = filesStringVectorValue->GetValue<ff::StringVectorValue>();
		}
	}

	ff::String pathCanon = ff::CanonicalizePath(path, false, true);
	files.Push(pathCanon);

	dict.SetValue(ff::RES_FILES, ff::Value::New<ff::StringVectorValue>(std::move(files)));

	return dict;
}

ff::Dict ff::LoadResourcesFromFileCached(StringRef path, bool debug, Vector<String>& errors)
{
	ff::String cachePath = ::GetCacheFileName(path, debug);
	ff::Dict dict = ::LoadCachedResources(cachePath, debug);

	if (dict.IsEmpty())
	{
		dict = ff::LoadResourcesFromFile(path, debug, errors);
		ff::SaveResourceCacheToFile(dict, cachePath);
	}

	return dict;
}

ff::Dict ff::LoadResourcesFromJsonText(StringRef jsonText, StringRef basePath, bool debug, Vector<String>& errors)
{
	size_t errorPos = ff::INVALID_SIZE;
	ff::Dict dict = ff::JsonParse(jsonText, &errorPos);

	if (errorPos != ff::INVALID_SIZE)
	{
		errors.Push(ff::String::format_new(L"Failed parsing JSON at pos: %lu", errorPos));
		errors.Push(ff::String::format_new(L"  -->%s", jsonText.substr(errorPos, std::min<size_t>(32, jsonText.size() - errorPos)).c_str()));
		return ff::Dict();
	}

	return ff::LoadResourcesFromJsonDict(dict, basePath, debug, errors);
}

ff::Dict ff::LoadResourcesFromJsonDict(const Dict& jsonDict, StringRef basePath, bool debug, Vector<String>& errors)
{
	ff::Dict globalDict;
	globalDict.Set<ff::StringValue>(ff::RES_BASE, ff::String(basePath));
	globalDict.Set<ff::BoolValue>(ff::RES_DEBUG, debug);

	ff::Dict dict = jsonDict;
	dict.Add(globalDict);

	TransformerContext context(globalDict, debug);
	ExpandFilePathsTransformer t1(context);
	ExpandValuesAndTemplatesTransformer t2(context);
	StartLoadObjectsFromDictTransformer t3(context);
	FinishLoadObjectsFromDictTransformer t4(context);
	ExtractResourceSiblingsTransformer t5(context);
	SaveObjectsToDictTransformer t6(context);
	CompressRootDictsTransformer t7(context);
	std::array<TransformerBase*, 7> transformers = { &t1, &t2, &t3, &t4, &t5, &t6, &t7 };

	for (TransformerBase* transformer : transformers)
	{
		dict = transformer->VisitDict(dict, errors);
	}

	for (ff::StringRef name : globalDict.GetAllNames())
	{
		dict.SetValue(name, nullptr);
	}

	ff::Vector<ff::String> files = context.GetFiles();
	if (files.Size())
	{
		dict.SetValue(ff::RES_FILES, ff::Value::New<ff::StringVectorValue>(std::move(files)));
	}

	return errors.IsEmpty() ? dict : ff::Dict();
}

bool ff::SaveResourceToCache(IUnknown* obj, Dict& dict)
{
	ff::ComPtr<ff::IResourcePersist> res;
	assertRetVal(res.QueryFrom(obj), false);
	assertRetVal(res->SaveToCache(dict), false);

	ff::ValuePtr resType = dict.GetValue(ff::RES_TYPE);
	if (!resType)
	{
		ff::ComPtr<ff::IComObject> comObj;
		assertRetVal(comObj.QueryFrom(obj), false);
		dict.Set<ff::GuidValue>(ff::RES_TYPE, comObj->GetComClassID());
	}
	else if (resType->IsType<ff::StringValue>())
	{
		const ff::ModuleClassInfo* info = ff::ProcessGlobals::Get()->GetModules().FindClassInfo(resType->GetValue<ff::StringValue>());
		assertRetVal(info, false);
		dict.Set<ff::GuidValue>(ff::RES_TYPE, info->_classId);
	}

	return true;
}

bool ff::SaveResourceCacheToFile(const Dict& dict, ff::StringRef outputFile)
{
	if (dict.IsEmpty())
	{
		ff::DeleteFile(outputFile);
		return true;
	}

	ff::ComPtr<ff::IData> outputData = ff::SaveDict(dict);
	assertRetVal(outputData, false);

	ff::String outputDir = outputFile;
	ff::StripPathTail(outputDir);
	assertRetVal(ff::DirectoryExists(outputDir) || ff::CreateDirectory(outputDir), false);

	ff::File file;
	assertRetVal(file.OpenWrite(outputFile), false);
	assertRetVal(ff::WriteFile(file, outputData), false);

	return true;
}

bool ff::IsResourceCacheUpToDate(StringRef inputPath, StringRef cachePath, bool debug)
{
	bool foundInputFile = false;

	ff::Dict dict = ::LoadCachedResources(cachePath, debug);
	if (!dict.IsEmpty())
	{
		const FILETIME badTime{ 0, 0 };
		const FILETIME cacheTime = ff::GetFileModifiedTime(cachePath);
		const FILETIME inputTime = ff::GetFileModifiedTime(inputPath);

		// We know all referenced files are still valid, but make sure the input file is actually referenced
		if (::CompareFileTime(&cacheTime, &badTime) != 0 &&
			::CompareFileTime(&inputTime, &badTime) != 0 &&
			::CompareFileTime(&cacheTime, &inputTime) >= 0)
		{
			const ff::Vector<ff::String>& files = dict.GetValue(ff::RES_FILES)->Convert<ff::StringVectorValue>()->GetValue<ff::StringVectorValue>();
			ff::String inputPath2 = ff::CanonicalizePath(inputPath, false, true);

			for (ff::StringRef file : files)
			{
				ff::String file2 = ff::CanonicalizePath(file, false, true);
				if (inputPath2 == file2)
				{
					foundInputFile = true;
					break;
				}
			}
		}
	}

	return foundInputFile;
}
