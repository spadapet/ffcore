#pragma once

#include "Module/Module.h"

namespace ff
{
	// Automatically creates and caches modules.
	class Modules
	{
	public:
		Modules();

		UTIL_API Module* Get(StringRef name);
		UTIL_API Module* Get(REFGUID id);
		UTIL_API Module* Get(HINSTANCE instance);
		UTIL_API Module* GetMain();

		UTIL_API const ModuleClassInfo* FindClassInfo(ff::StringRef name);
		UTIL_API const ModuleClassInfo* FindClassInfo(REFGUID classId);
		UTIL_API bool FindClassFactory(REFGUID classId, IClassFactory** factory);
		UTIL_API ComPtr<IUnknown> CreateClass(ff::StringRef name, AppGlobals* globals);

		UTIL_API void Clear();

	private:
		void CreateAllModules();

		ff::Mutex _mutex;
		Vector<std::unique_ptr<Module>> _modules;
	};
}
