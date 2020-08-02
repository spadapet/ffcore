#include "pch.h"
#include "COM/ComFactory.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Globals/Log.h"
#include "Module/Module.h"
#include "Resource/ResourcePersist.h"
#include "Resource/Resources.h"
#include "String/StringUtil.h"
#include "Thread/ThreadPool.h"
#include "Value/Values.h"
#include "Windows/FileUtil.h"

static HINSTANCE s_mainModuleInstance = nullptr;

HINSTANCE ff::GetMainModuleInstance()
{
	assert(s_mainModuleInstance);
	return s_mainModuleInstance;
}

void ff::SetMainModuleInstance(HINSTANCE instance)
{
	assert(!s_mainModuleInstance && instance);
	s_mainModuleInstance = instance;
}

ff::Module::Module(StringRef name, REFGUID id, HINSTANCE instance)
	: _refs(0)
	, _name(name)
	, _id(id)
	, _instance(instance)
{
}

ff::Module::~Module()
{
}

void ff::Module::AfterInit()
{
	LoadTypeLibs();
	LoadResources();
}

void ff::Module::BeforeDestruct()
{
	_typeLibs.ClearAndReduce();
	_resources.Release();
}

void ff::Module::AddRef() const
{
	_refs.fetch_add(1);
}

void ff::Module::Release() const
{
	_refs.fetch_sub(1);
}

// This isn't 100% safe because another thread could lock this module right after checking _refs
bool ff::Module::IsLocked() const
{
	return _refs != 0;
}

ff::String ff::Module::GetName() const
{
	return _name;
}

REFGUID ff::Module::GetId() const
{
	return _id;
}

HINSTANCE ff::Module::GetInstance() const
{
	return _instance;
}

ff::String ff::Module::GetPath() const
{
#if METRO_APP
	// Assume that the name is a DLL in the application directory
	Windows::ApplicationModel::Package^ package = Windows::ApplicationModel::Package::Current;
	String path = String::from_pstring(package->InstalledLocation->Path);
	AppendPathTail(path, _name);
	ChangePathExtension(path, ff::String(L"dll"));
	assert(FileExists(path));
	return path;
#else
	assert(_instance != nullptr);
	return GetModulePath(_instance);
#endif
}

bool ff::Module::IsDebugBuild() const
{
#ifdef _DEBUG
	return true;
#else
	return false;
#endif
}

bool ff::Module::IsMetroBuild() const
{
#if METRO_APP
	return true;
#else
	return false;
#endif
}

size_t ff::Module::GetBuildBits() const
{
#ifdef _WIN64
	return 64;
#else
	return 32;
#endif
}

ff::String ff::Module::GetBuildArch() const
{
	static StaticString arch
#if defined(_M_ARM)
		(L"arm");
#elif defined(_WIN64)
		(L"x64");
#else
		(L"x86");
#endif
	return arch;
}

void ff::Module::LoadTypeLibs()
{
	assertRet(_typeLibs.IsEmpty());

	// Index zero is for no type lib
	_typeLibs.Push(ComPtr<ITypeLib>());

#if !METRO_APP
	String path = GetPath();

	for (size_t i = 1; ; i++)
	{
		ComPtr<ITypeLib> typeLib;
		if (::FindResourceW(_instance, MAKEINTRESOURCE(i), L"TYPELIB"))
		{
			String pathIndex = String::format_new(L"%s\\%lu", path.c_str(), i);
			verify(SUCCEEDED(::LoadTypeLibEx(pathIndex.c_str(), REGKIND_NONE, &typeLib)));
		}
		else
		{
			break;
		}

		_typeLibs.Push(typeLib);
	}
#endif
}

void ff::Module::LoadResources()
{
	Dict finalDict;
	for (const ComPtr<ISavedData>& savedData : GetResourceSavedDicts())
	{
		Dict savedDict = ff::Value::New<ff::SavedDictValue>(savedData)->Convert<ff::DictValue>()->GetValue<ff::DictValue>();
		finalDict.Add(savedDict);

		const Vector<String>& resFiles = savedDict.Get<ff::StringVectorValue>(ff::RES_FILES);
		for (auto i = resFiles.crbegin(); i != resFiles.crend(); i++)
		{
			ff::StringRef file = *i;
			if (file.size() >= 9 && file.rfind(L".res.json") == file.size() - 9)
			{
				_resourceSourceFiles.Push(file);
				break;
			}
		}
	}

	verify(CreateResources(nullptr, finalDict, &_resources));
}

#if !METRO_APP
static BOOL CALLBACK HandleResourceName(HMODULE module, LPCTSTR type, LPTSTR name, LONG_PTR param)
{
	ff::ComPtr<ff::IData> data;
	noAssertRetVal(ff::CreateDataInResource(module, type, name, &data), TRUE);

	ff::ComPtr<ff::ISavedData> savedData;
	noAssertRetVal(ff::CreateLoadedDataFromMemory(data, false, &savedData), TRUE);

	ff::Vector<ff::ComPtr<ff::ISavedData>>* datas = (ff::Vector<ff::ComPtr<ff::ISavedData>>*)param;
	datas->Push(savedData);

	return TRUE;
}
#endif

static ff::Vector<ff::ComPtr<ff::ISavedData>> GetDictsInDirectory(ff::StringRef assetDir)
{
	ff::Vector<ff::ComPtr<ff::ISavedData>> datas;

	ff::Vector<ff::String> dirs, files;
	if (ff::GetDirectoryContents(assetDir, dirs, files))
	{
		for (ff::StringRef file : files)
		{
			if (file.size() >= 9 && file.rfind(L".res.pack") == file.size() - 9)
			{
				ff::String asset = assetDir;
				ff::AppendPathTail(asset, file);

				ff::ComPtr<ff::IDataFile> dataFile;
				if (ff::CreateDataFile(asset, false, &dataFile))
				{
					bool memMapped = dataFile->OpenReadMemMapped();

					ff::ComPtr<ff::ISavedData> savedData;
					if (ff::CreateSavedDataFromFile(dataFile, 0, dataFile->GetSize(), dataFile->GetSize(), false, &savedData))
					{
						datas.Push(savedData);
					}

					if (memMapped)
					{
						verify(dataFile->CloseMemMapped());
					}
				}
			}
		}
	}

	return datas;
}

ff::Vector<ff::ComPtr<ff::ISavedData>> ff::Module::GetResourceSavedDicts() const
{
#if METRO_APP
	String assetSubDir = (ff::GetMainModuleInstance() == GetInstance()) ? ff::GetEmptyString() : GetName();
	String assetDir = ff::GetExecutableDirectory(assetSubDir);
	Vector<ComPtr<ISavedData>> datas = GetDictsInDirectory(assetDir);
#else
	Vector<ComPtr<ISavedData>> datas;
	::EnumResourceNames(_instance, L"RESDICT", HandleResourceName, (LONG_PTR)&datas);
#endif

	return datas;
}

ITypeLib* ff::Module::GetTypeLib(size_t index) const
{
	assertRetVal(index < _typeLibs.Size(), nullptr);
	return _typeLibs[index];
}

const ff::ModuleClassInfo* ff::Module::GetClassInfo(StringRef name) const
{
	assertRetVal(name.size(), nullptr);

	auto iter = _classesByName.GetKey(name);
	if (iter)
	{
		return &iter->GetValue();
	}

	GUID classId;
	if (ff::StringToGuid(name, classId))
	{
		return GetClassInfo(classId);
	}

	return nullptr;
}

const ff::ModuleClassInfo* ff::Module::GetClassInfo(REFGUID classId) const
{
	assertRetVal(classId != GUID_NULL, nullptr);

	auto iter = _classesById.GetKey(classId);
	if (iter)
	{
		return &iter->GetValue();
	}

	return nullptr;
}

ff::Vector<GUID> ff::Module::GetClasses() const
{
	Vector<GUID> ids;
	ids.Reserve(_classesById.Size());

	GUID id;
	for (const auto& i : _classesById)
	{
		if (i.GetKey() != id)
		{
			id = i.GetKey();
			ids.Push(id);
		}
	}

	return ids;
}

bool ff::Module::GetClassFactory(REFGUID classId, IClassFactory** factory) const
{
	const ModuleClassInfo* info = GetClassInfo(classId);
	if (info != nullptr && info->_factory != nullptr)
	{
		assertRetVal(CreateClassFactory(classId, this, info->_factory, factory), false);
		return true;
	}

	return false;
}

bool ff::Module::CreateClass(REFGUID classId, REFGUID interfaceId, void** obj) const
{
	return CreateClass(nullptr, classId, interfaceId, obj);
}

bool ff::Module::CreateClass(IUnknown* parent, REFGUID classId, REFGUID interfaceId, void** obj) const
{
	const ModuleClassInfo* info = GetClassInfo(classId);
	if (info != nullptr && info->_factory != nullptr)
	{
		assertHrRetVal(info->_factory(parent, classId, interfaceId, obj), false);
		return true;
	}

	return false;
}

void ff::Module::RegisterClass(StringRef name, REFGUID classId, ClassFactoryFunc factory)
{
	assertRet(name.size() && classId != GUID_NULL);

	ModuleClassInfo info;
	info._name = name;
	info._classId = classId;
	info._factory = factory;
	info._module = this;

	_classesByName.InsertKey(name, info);
	_classesById.InsertKey(classId, info);
}

ff::IResourceAccess* ff::Module::GetResources() const
{
	return _resources;
}

void ff::Module::RebuildResourcesFromSourceAsync()
{
	std::shared_ptr<ff::ComPtr<ff::IResources>> newResources = std::make_shared<ff::ComPtr<ff::IResources>>();
	ff::Vector<ff::String> files = _resourceSourceFiles;
	bool debugBuild = IsDebugBuild();

	ff::GetThreadPool()->AddTask([files, debugBuild, newResources]()
		{
			ff::Dict finalDict;

			for (ff::StringRef file : files)
			{
				ff::Vector<ff::String> errors;
				ff::Dict dict = ff::LoadResourcesFromFileCached(file, debugBuild, errors);

				for (ff::String error : errors)
				{
					ff::Log::DebugTraceF(L"%s\r\n", error.c_str());
				}

				finalDict.Add(dict);

				ff::CreateResources(nullptr, finalDict, newResources->Address());
			}
		},
		[this, newResources]()
		{
			if (newResources->IsValid())
			{
				_resources = *newResources;
			}
		});
}

ff::Event<ff::Module*>& ff::Module::GetResourceRebuiltEvent()
{
	return _resourcesRebuiltEvent;
}
