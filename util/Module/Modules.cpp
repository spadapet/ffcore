#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioFactory.h"
#include "Globals/AppGlobals.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Module/ModuleFactory.h"
#include "Module/Modules.h"
#include "Resource/Resources.h"

ff::Modules::Modules()
{
}

static ff::Module* FindModule(const ff::Vector<std::unique_ptr<ff::Module>>& modules, ff::StringRef name)
{
	for (auto& module : modules)
	{
		if (module->GetName() == name)
		{
			return module.get();
		}
	}

	return nullptr;
}

static ff::Module* FindModule(const ff::Vector<std::unique_ptr<ff::Module>>& modules, REFGUID id)
{
	for (auto& module : modules)
	{
		if (module->GetId() == id)
		{
			return module.get();
		}
	}

	return nullptr;
}

static ff::Module* FindModule(const ff::Vector<std::unique_ptr<ff::Module>>& modules, HINSTANCE instance)
{
	for (auto& module : modules)
	{
		if (module->GetInstance() == instance)
		{
			return module.get();
		}
	}

	return nullptr;
}

ff::Module* ff::Modules::Get(StringRef name)
{
	// Try and create it
	ff::LockMutex lock(_mutex);

	Module* module = FindModule(_modules, name);
	if (module == nullptr)
	{
		std::unique_ptr<Module> newModule = ModuleFactory::Create(name);
		assertRetVal(newModule != nullptr, nullptr);

		module = newModule.get();
		_modules.Insert(0, std::move(newModule));
		module->AfterInit();
	}

	assert(module);
	return module;
}

ff::Module* ff::Modules::Get(REFGUID id)
{
	// Try and create it
	ff::LockMutex lock(_mutex);

	Module* module = FindModule(_modules, id);
	if (module == nullptr)
	{
		std::unique_ptr<Module> newModule = ModuleFactory::Create(id);
		assertRetVal(newModule != nullptr, nullptr);

		module = newModule.get();
		_modules.Insert(0, std::move(newModule));
		module->AfterInit();
	}

	assert(module);
	return module;
}

ff::Module* ff::Modules::Get(HINSTANCE instance)
{
	// Try and create it
	ff::LockMutex lock(_mutex);

	Module* module = FindModule(_modules, instance);
	if (module == nullptr)
	{
		std::unique_ptr<Module> newModule = ModuleFactory::Create(instance);
		assertRetVal(newModule != nullptr, nullptr);

		module = newModule.get();
		_modules.Insert(0, std::move(newModule));
		module->AfterInit();
	}

	assert(module);
	return module;
}

ff::Module* ff::Modules::GetMain()
{
	HINSTANCE instance = ff::GetMainModuleInstance();
#if !METRO_APP
	if (!instance)
	{
		instance = GetModuleHandle(nullptr);
	}
#endif
	return Get(instance);
}

const ff::ModuleClassInfo* ff::Modules::FindClassInfo(ff::StringRef name)
{
	ff::LockMutex lock(_mutex);

	// First try modules that were already created
	for (size_t i = 0; i < 2; i++)
	{
		if (i == 1)
		{
			CreateAllModules();
		}

		for (auto& module : _modules)
		{
			const ModuleClassInfo* info = module->GetClassInfo(name);
			if (info != nullptr)
			{
				return info;
			}
		}
	}

	return nullptr;
}

const ff::ModuleClassInfo* ff::Modules::FindClassInfo(REFGUID classId)
{
	ff::LockMutex lock(_mutex);

	// First try modules that were already created
	for (size_t i = 0; i < 2; i++)
	{
		if (i == 1)
		{
			CreateAllModules();
		}

		for (auto& module : _modules)
		{
			const ModuleClassInfo* info = module->GetClassInfo(classId);
			if (info != nullptr)
			{
				return info;
			}
		}
	}

	return nullptr;
}

bool ff::Modules::FindClassFactory(REFGUID classId, IClassFactory** factory)
{
	ff::LockMutex lock(_mutex);

	// First try modules that were already created
	for (size_t i = 0; i < 2; i++)
	{
		if (i == 1)
		{
			CreateAllModules();
		}

		for (auto& module : _modules)
		{
			if (module->GetClassFactory(classId, factory))
			{
				return true;
			}
		}
	}

	return false;
}

class AppGlobalsWrapper : public IUnknown
{
public:
	AppGlobalsWrapper(ff::AppGlobals* globals)
		: _globals(globals ? globals : ff::AppGlobals::Get())
	{
	}

	COM_FUNC QueryInterface(REFIID riid, void** obj)
	{
		assertRetVal(obj, E_INVALIDARG);
		*obj = nullptr;

		if (riid == __uuidof(ff::IAudioDevice))
		{
			if (_globals)
			{
				*obj = ff::GetAddRef(_globals->GetAudio());
			}
			else
			{
				ff::IAudioFactory* factory = ff::ProcessGlobals::Get()->GetAudioFactory();
				if (factory && factory->GetDeviceCount())
				{
					*obj = ff::GetAddRef(factory->GetDevice(0));
				}
			}
		}
		else if (riid == __uuidof(ff::IAudioFactory))
		{
			*obj = ff::GetAddRef(ff::ProcessGlobals::Get()->GetAudioFactory());
		}
		else if (riid == __uuidof(ff::IGraphDevice))
		{
			if (_globals && _globals->GetGraph())
			{
				_globals->GetGraph()->QueryInterface(riid, obj);
			}
			else
			{
				ff::IGraphicFactory* factory = ff::ProcessGlobals::Get()->GetGraphicFactory();
				if (factory && factory->GetDeviceCount())
				{
					*obj = ff::GetAddRef(factory->GetDevice(0));
				}
			}
		}
		else if (riid == __uuidof(ff::IGraphicFactory))
		{
			*obj = ff::GetAddRef(ff::ProcessGlobals::Get()->GetGraphicFactory());
		}

		return *obj ? S_OK : E_NOINTERFACE;
	}

	COM_FUNC_RET(ULONG) AddRef()
	{
		assertRetVal(false, 0);
	}

	COM_FUNC_RET(ULONG) Release()
	{
		assertRetVal(false, 0);
	}

private:
	ff::AppGlobals* _globals;
};

ff::ComPtr<IUnknown> ff::Modules::CreateClass(ff::StringRef name, AppGlobals* globals)
{
	const ff::ModuleClassInfo* info = FindClassInfo(name);
	assertRetVal(info && info->_factory, nullptr);

	ff::ComPtr<IUnknown> obj;
	AppGlobalsWrapper contextWrapper(globals);
	assertHrRetVal(info->_factory(&contextWrapper, info->_classId, __uuidof(IUnknown), (void**)&obj), nullptr);

	return obj;
}

void ff::Modules::Clear()
{
	ff::LockMutex lock(_mutex);

	for (auto& module : _modules)
	{
		module->BeforeDestruct();
	}

	_modules.ClearAndReduce();
}

void ff::Modules::CreateAllModules()
{
	Vector<HINSTANCE> instances = ModuleFactory::GetAllInstances();
	for (HINSTANCE inst : instances)
	{
		Get(inst);
	}
}
