#pragma once

namespace ff
{
	class Module;

	UTIL_API bool CreateClassFactory(REFGUID clsid, Module* module, ClassFactoryFunc func, IClassFactory** factory);

	template<typename T>
	bool CreateClassFactory(REFGUID clsid, Module* module, IClassFactory** factory)
	{
		return CreateClassFactory(clsid, module, ComAllocator<T>::ComClassFactory, factory);
	}
}
