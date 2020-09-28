#pragma once

#include "Globals/Log.h"
#include "Globals/ThreadGlobals.h"
#include "Module/Modules.h"
#include "String/StringCache.h"
#include "String/StringManager.h"

namespace ff
{
	class IAudioFactory;
	class IGraphicFactory;
	class IThreadPool;

	class ProcessGlobals : public ThreadGlobals
	{
	public:
		UTIL_API ProcessGlobals();
		UTIL_API virtual ~ProcessGlobals();

		UTIL_API static ProcessGlobals* Get();
		UTIL_API static bool Exists();

		virtual void Startup() override;
		virtual void Shutdown() override;

		UTIL_API Log& GetLog();
		UTIL_API Modules& GetModules();
		UTIL_API StringCache& GetStringCache();
		UTIL_API StringManager& GetStringManager();

		UTIL_API IAudioFactory* GetAudioFactory();
		UTIL_API IGraphicFactory* GetGraphicFactory();
		UTIL_API IThreadPool* GetThreadPool();

	private:
		ff::Mutex _mutex;
		Log _log;
		Modules _modules;
		StringCache _stringCache;
		StringManager _stringManager;

		ComPtr<IAudioFactory> _audioFactory;
		ComPtr<IGraphicFactory> _graphicFactory;
		ComPtr<IThreadPool> _threadPool;
	};

	UTIL_API bool DidProgramStart();
	UTIL_API bool IsProgramRunning();
	UTIL_API bool IsProgramShuttingDown();
	UTIL_API void AtProgramShutdown(std::function<void()> func);

#if !METRO_APP
	UTIL_API void InitDesktopProcess();
#endif
}
