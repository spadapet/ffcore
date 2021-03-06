#pragma once

#include "COM/ComAlloc.h"
#include "Value/ValueType.h"

namespace ff
{
	class Module;
	class IResources;
	class IResourceAccess;
	class ISavedData;
	class IValueAccess;

	struct ModuleClassInfo
	{
		String _name;
		GUID _classId;
		ClassFactoryFunc _factory;
		const Module* _module;
	};

	// A module represents a reusable library of assets and objects
	class Module
	{
	public:
		UTIL_API Module(StringRef name, REFGUID id, HINSTANCE instance);
		UTIL_API virtual ~Module();
		void AfterInit();
		void BeforeDestruct();

		UTIL_API void AddRef();
		UTIL_API void Release();
		UTIL_API bool IsLocked() const;

		UTIL_API String GetName() const;
		UTIL_API REFGUID GetId() const;
		UTIL_API HINSTANCE GetInstance() const;
		UTIL_API String GetPath() const;

		UTIL_API bool IsDebugBuild() const;
		UTIL_API bool IsMetroBuild() const;
		UTIL_API size_t GetBuildBits() const;
		UTIL_API String GetBuildArch() const;

		UTIL_API ITypeLib* GetTypeLib(size_t index) const;
		UTIL_API IResourceAccess* GetResources() const;
		UTIL_API void RebuildResourcesFromSourceAsync();
		UTIL_API entt::sink<void(Module*)> GetResourceRebuiltSink();

		UTIL_API Vector<GUID> GetClasses() const;
		UTIL_API const ModuleClassInfo* GetClassInfo(StringRef name) const;
		UTIL_API const ModuleClassInfo* GetClassInfo(REFGUID classId) const;

		UTIL_API bool GetClassFactory(REFGUID classId, IClassFactory** factory) const;
		UTIL_API bool CreateClass(REFGUID classId, REFGUID interfaceId, void** obj) const;
		UTIL_API bool CreateClass(IUnknown* parent, REFGUID classId, REFGUID interfaceId, void** obj) const;
		UTIL_API void RegisterClass(StringRef name, REFGUID classId, ClassFactoryFunc factory = nullptr);

		template<typename I>
		bool CreateClass(REFGUID classId, I** obj)
		{
			return CreateClass(classId, __uuidof(I), static_cast<void**>(obj));
		}

		template<typename T, size_t nameLen>
		void RegisterClassT(const wchar_t(&name)[nameLen])
		{
			static ff::StaticString staticName(name);
			RegisterClass(staticName, __uuidof(T), ComAllocator<T>::ComClassFactory);
		}

	private:
		void LoadTypeLibs();
		void LoadResources();
		Vector<ComPtr<ISavedData>> GetResourceSavedDicts() const;

		// Identity
		std::atomic_long _refs;
		String _name;
		GUID _id;
		HINSTANCE _instance;

		// Resources
		ComPtr<IResources> _resources;
		Vector<String> _resourceSourceFiles;
		entt::sigh<void(Module*)> _resourcesRebuiltEvent;
		Vector<ComPtr<ITypeLib>> _typeLibs;

		// COM registration
		Map<String, ModuleClassInfo> _classesByName;
		Map<GUID, ModuleClassInfo> _classesById;
	};

	Module& GetThisModule();
	Module* GetMainModule();

	UTIL_API HINSTANCE GetMainModuleInstance();
	UTIL_API void SetMainModuleInstance(HINSTANCE instance);
}
