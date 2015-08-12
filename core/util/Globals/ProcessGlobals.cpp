#include "pch.h"
#include "Audio/AudioFactory.h"
#include "Globals/ProcessGlobals.h"
#include "Globals/ProcessStartup.h"
#include "Graph/GraphFactory.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"

static ff::ProcessGlobals* s_processGlobals = nullptr;
static bool s_programShutDown = false;

namespace ff
{
	void HookCrtMemAlloc();
	void UnhookCrtMemAlloc();
}

bool ff::DidProgramStart()
{
	return s_processGlobals != nullptr && s_processGlobals->IsValid();
}

bool ff::IsProgramRunning()
{
	return s_processGlobals != nullptr && s_processGlobals->IsValid() && !s_processGlobals->IsShuttingDown();
}

bool ff::IsProgramShuttingDown()
{
	return s_programShutDown || (s_processGlobals != nullptr && s_processGlobals->IsShuttingDown());
}

void ff::AtProgramShutdown(std::function<void()> func)
{
	assertRet(s_processGlobals != nullptr);

	if (s_processGlobals->IsShuttingDown())
	{
		assertSz(false, L"Why register a program shutdown function during shutdown?");
		func();
	}
	else
	{
		s_processGlobals->AtShutdown(func);
	}
}

ff::ProcessGlobals::ProcessGlobals()
{
	assert(s_processGlobals == nullptr);
	s_processGlobals = this;
	s_programShutDown = false;
}

ff::ProcessGlobals::~ProcessGlobals()
{
	assert(s_processGlobals == this && !s_programShutDown);
	s_processGlobals = nullptr;
	s_programShutDown = true;
}

ff::ProcessGlobals* ff::ProcessGlobals::Get()
{
	assert(s_processGlobals);
	return s_processGlobals;
}

bool ff::ProcessGlobals::Exists()
{
	return s_processGlobals != nullptr;
}

void ff::ProcessGlobals::Startup()
{
	HookCrtMemAlloc();
	ff::ThreadGlobals::Startup();
	ProcessStartup::OnStartup(*this);
}

void ff::ProcessGlobals::Shutdown()
{
	if (_threadPool)
	{
		_threadPool->Destroy();
		_threadPool = nullptr;
	}

	ff::ThreadGlobals::Shutdown();

	_audioFactory = nullptr;
	_graphicFactory = nullptr;
	_modules.Clear();
	_stringCache.Clear();

	ff::ProcessShutdown::OnShutdown(*this);
	ff::ComBaseEx::DumpComObjects();
	ff::UnhookCrtMemAlloc();
}

ff::Log& ff::ProcessGlobals::GetLog()
{
	return _log;
}

ff::Modules& ff::ProcessGlobals::GetModules()
{
	return _modules;
}

ff::StringManager& ff::ProcessGlobals::GetStringManager()
{
	return _stringManager;
}

ff::StringCache& ff::ProcessGlobals::GetStringCache()
{
	return _stringCache;
}

ff::IAudioFactory* ff::ProcessGlobals::GetAudioFactory()
{
	if (!_audioFactory)
	{
		ff::LockMutex lock(_mutex);
		if (!_audioFactory)
		{
			_audioFactory = ff::CreateAudioFactory();
		}
	}

	return _audioFactory;
}

ff::IGraphicFactory* ff::ProcessGlobals::GetGraphicFactory()
{
	if (!_graphicFactory)
	{
		ff::LockMutex lock(_mutex);
		if (!_graphicFactory)
		{
			_graphicFactory = CreateGraphicFactory();
		}
	}

	return _graphicFactory;
}

ff::IThreadPool* ff::ProcessGlobals::GetThreadPool()
{
	if (!_threadPool)
	{
		ff::LockMutex lock(_mutex);
		if (!_threadPool)
		{
			_threadPool = ff::CreateThreadPool();
		}
	}

	return _threadPool;
}

#if !METRO_APP
void ff::InitDesktopProcess()
{
	::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	::EnableMouseInPointer(TRUE);
}
#endif
