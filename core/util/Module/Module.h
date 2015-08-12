#pragma once

#include "COM/ComAlloc.h"
#include "Value/ValueType.h"

namespace ff
{
	class Module;
	class IResources;
	class ISavedData;
	class IValueTable;

	struct ModuleClassInfo
	{
		String _name;
		GUID _classId;
		GUID _mainInterfaceId;
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

		UTIL_API void AddRef() const;
		UTIL_API void Release() const;
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
		UTIL_API IResources* GetResources() const;
		UTIL_API IValueTable* GetValueTable() const;
		UTIL_API ff::ValuePtr GetValue(StringRef name) const;
		UTIL_API String GetString(StringRef name) const;
		UTIL_API String GetFormattedString(String name, ...) const;

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
		void LoadValueTable();
		Vector<ComPtr<ISavedData>> GetResourceSavedDicts() const;
		Vector<ComPtr<ISavedData>> GetValueTableSavedDicts() const;

		// Identity
		mutable std::atomic_long _refs;
		String _name;
		GUID _id;
		HINSTANCE _instance;

		// Resources
		ComPtr<IResources> _resources;
		ComPtr<IValueTable> _valueTable;
		Vector<ComPtr<ITypeLib>> _typeLibs;

		// COM registration
		Map<String, ModuleClassInfo> _classesByName;
		Map<GUID, ModuleClassInfo> _classesById;
	};

	const Module& GetThisModule();
	const Module* GetMainModule();

	UTIL_API HINSTANCE GetMainModuleInstance();
	UTIL_API void SetMainModuleInstance(HINSTANCE instance);
}
