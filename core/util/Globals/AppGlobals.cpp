#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioFactory.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Dict/DictPersist.h"
#include "Globals/GlobalsScope.h"
#include "Globals/ProcessGlobals.h"
#include "Globals/AppGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTargetWindow.h"
#include "Graph/State/GraphContext11.h"
#include "Input/DeviceEvent.h"
#include "Input/Joystick/JoystickInput.h"
#include "Resource/ResourcePersist.h"
#include "State/DebugPageState.h"
#include "State/States.h"
#include "State/StateWrapper.h"
#include "String/StringUtil.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Value/Values.h"
#include "Windows/FileUtil.h"

static void WriteLog(const wchar_t* id, ...)
{
	va_list args;
	va_start(args, id);

	ff::String formatString = ff::String::format_new(L"%s %s: %s\r\n",
		ff::GetDateAsString().c_str(),
		ff::GetTimeAsString().c_str(),
		ff::GetThisModule().GetString(ff::String(id)).c_str());
	ff::Log::GlobalTraceV(formatString.c_str(), args);

	va_end(args);
}

static ff::AppGlobals* s_appGlobals = nullptr;

ff::AppGlobals* ff::AppGlobals::Get()
{
	return s_appGlobals;
}

ff::AppGlobals::AppGlobals()
	: _valid(false)
	, _active(false)
	, _visible(false)
	, _dpiScale(1)
	, _flags(AppGlobalsFlags::None)
	, _helper(nullptr)
	, _stateChanged(false)
	, _gameLoopState(GameLoopState::Stopped)
{
	assert(!s_appGlobals);
	s_appGlobals = this;
}

ff::AppGlobals::~AppGlobals()
{
	assert(!_valid && s_appGlobals == this);
	s_appGlobals = nullptr;
}

ff::IGraphDevice* ff::AppGlobals::GetGraph() const
{
	return _graph;
}

ff::IAudioDevice* ff::AppGlobals::GetAudio() const
{
	return _audio;
}

ff::IPointerDevice* ff::AppGlobals::GetPointer() const
{
	return _pointer;
}

ff::IPointerDevice* ff::AppGlobals::GetPointerDebug() const
{
	return _pointerDebug;
}

ff::IKeyboardDevice* ff::AppGlobals::GetKeys() const
{
	return _keyboard;
}

ff::IKeyboardDevice* ff::AppGlobals::GetKeysDebug() const
{
	return _keyboardDebug;
}

ff::IJoystickInput* ff::AppGlobals::GetJoysticks() const
{
	return _joysticks;
}

ff::IJoystickInput* ff::AppGlobals::GetJoysticksDebug() const
{
	return _joysticksDebug;
}

ff::IRenderTargetWindow* ff::AppGlobals::GetTarget() const
{
	return _target;
}

ff::IRenderDepth* ff::AppGlobals::GetDepth() const
{
	return _depth;
}

ff::IDeviceEventSink* ff::AppGlobals::GetDeviceEvents() const
{
	return _deviceEvents;
}

ff::IDeviceEventSink* ff::AppGlobals::GetDeviceEventsDebug() const
{
	return _deviceEventsDebug;
}

ff::IThreadDispatch* ff::AppGlobals::GetGameDispatch() const
{
	return _gameLoopDispatch ? _gameLoopDispatch : ff::GetMainThreadDispatch();
}

ff::IAppGlobalsHelper* ff::AppGlobals::GetAppGlobalsHelper() const
{
	return _helper;
}

const ff::GlobalTime& ff::AppGlobals::GetGlobalTime() const
{
	return _globalTime;
}

const ff::FrameTime& ff::AppGlobals::GetFrameTime() const
{
	return _frameTime;
}

ff::State* ff::AppGlobals::GetGameState() const
{
	assert(GetGameDispatch()->IsCurrentThread());

	return _gameState.get();
}

void ff::AppGlobals::AddDebugPage(ff::IDebugPages* page)
{
	assert(GetGameDispatch()->IsCurrentThread());

	if (!_debugPages.Contains(page))
	{
		_debugPages.Push(page);
	}
}

void ff::AppGlobals::RemoveDebugPage(ff::IDebugPages* page)
{
	assert(GetGameDispatch()->IsCurrentThread());

	verify(_debugPages.DeleteItem(page));
}

const ff::Vector<ff::IDebugPages*>& ff::AppGlobals::GetDebugPages() const
{
	assert(GetGameDispatch()->IsCurrentThread());

	return _debugPages;
}

ff::DebugPageState* ff::AppGlobals::GetDebugPageState() const
{
	assert(GetGameDispatch()->IsCurrentThread());

	return _debugPageState.get();
}

ff::String ff::AppGlobals::GetLogFile() const
{
	return _logFile;
}

bool ff::AppGlobals::GetAppActive() const
{
	return _active;
}

bool ff::AppGlobals::GetAppVisible() const
{
	return _visible;
}

double ff::AppGlobals::GetDpiScale() const
{
	return _dpiScale;
}

double ff::AppGlobals::PixelToDip(double size) const
{
	return size / _dpiScale;
}

double ff::AppGlobals::DipToPixel(double size) const
{
	return size * _dpiScale;
}

bool ff::AppGlobals::IsFullScreen()
{
	assert(GetGameDispatch()->IsCurrentThread());

	return _target && _target->IsFullScreen();
}

bool ff::AppGlobals::SetFullScreen(bool fullScreen)
{
	return CanSetFullScreen() && _target->SetFullScreen(fullScreen);
}

bool ff::AppGlobals::CanSetFullScreen()
{
	assert(GetGameDispatch()->IsCurrentThread());

	return _target && _target->CanSetFullScreen();
}

bool ff::AppGlobals::LoadState()
{
	ff::LockMutex lock(_stateLock);
	ClearAllState();

	String fileName = GetStateFile();
	if (ff::FileExists(fileName))
	{
		::WriteLog(L"APP_STATE_LOADING", fileName.c_str());

		ComPtr<IDataFile> dataFile;
		ComPtr<IDataReader> dataReader;
		bool valid =
			ff::CreateDataFile(fileName, false, &dataFile) &&
			ff::CreateDataReader(dataFile, 0, &dataReader) &&
			ff::LoadDict(dataReader, _namedStateCache);
		assert(valid);
	}

	_stateChanged = false;
	return true;
}

bool ff::AppGlobals::SaveState()
{
	ff::LockMutex lock(_stateLock);
	noAssertRetVal(_stateChanged, true);

	String fileName = GetStateFile();
	::WriteLog(L"APP_STATE_SAVING", fileName.c_str());

	Dict state = _namedStateCache;
	for (const KeyValue<String, Dict>& kvp : _namedState)
	{
		Dict namedDict = kvp.GetValue();
		if (!namedDict.IsEmpty())
		{
			state.Set<ff::DictValue>(kvp.GetKey(), std::move(namedDict));
		}
	}

	File file;
	ComPtr<IData> data = ff::SaveDict(state);
	assertRetVal(data, false);
	assertRetVal(file.OpenWrite(fileName), false);
	assertRetVal(ff::WriteFile(file, data), false);

	return LoadState();
}

ff::Dict ff::AppGlobals::GetState()
{
	return GetState(ff::GetEmptyString());
}

void ff::AppGlobals::SetState(const Dict& dict)
{
	return SetState(ff::GetEmptyString(), dict);
}

void ff::AppGlobals::ClearAllState()
{
	ff::LockMutex lock(_stateLock);

	if (_namedState.Size() || _namedStateCache.Size())
	{
		_namedState.Clear();
		_namedStateCache.Clear();
		_stateChanged = true;
	}
}

ff::String ff::AppGlobals::GetStateFile() const
{
	String path = ff::GetRoamingUserDirectory();
	assertRetVal(!path.empty(), path);

	String appName = ff::GetMainModule()->GetName();
	String appArch = GetThisModule().GetBuildArch();
	String name = GetThisModule().GetFormattedString(
		String(L"APP_STATE_FILE"),
		appName.c_str(),
		appArch.c_str());
	ff::AppendPathTail(path, name);

	return path;
}

ff::Dict ff::AppGlobals::GetState(StringRef name)
{
	ff::LockMutex lock(_stateLock);
	auto namedIter = _namedState.GetKey(name);
	if (!namedIter)
	{
		Dict state;
		if (_namedStateCache.GetValue(name))
		{
			state = _namedStateCache.Get<ff::DictValue>(name);
		}

		namedIter = _namedState.SetKey(name, state);
		ff::DumpDict(name, state, &ff::ProcessGlobals::Get()->GetLog(), true);
	}

	return namedIter->GetValue();
}

void ff::AppGlobals::SetState(StringRef name, const Dict& dict)
{
	ff::LockMutex lock(_stateLock);
	_namedStateCache.SetValue(name, nullptr);
	_stateChanged = true;

	if (dict.IsEmpty())
	{
		_namedState.UnsetKey(name);
	}
	else
	{
		_namedState.SetKey(name, dict);
	}
}

ff::Event<bool>& ff::AppGlobals::ActiveChanged()
{
	return _activeChangedEvent;
}

bool ff::AppGlobals::OnStartup(AppGlobalsFlags flags, IAppGlobalsHelper* helper)
{
	assertRetVal(ff::DidProgramStart(), false);
	assertRetVal(!_valid, false);

	_flags = flags;
	_gameLoopEvent = ff::CreateEvent();
	_helper = helper ? helper : this;

	::SetThreadDescription(::GetCurrentThread(), L"ff : User Interface");
	assertRetVal(Initialize(), false);

	::WriteLog(L"APP_STARTUP_DONE");
	_valid = true;
	return true;
}

void ff::AppGlobals::OnShutdown()
{
	::WriteLog(L"APP_SHUTDOWN");
	StopRenderLoop();

	_helper->OnShutdown(this);

	_valid = false;
	_gameLoopEvent = nullptr;
	_audio = nullptr;
	_target = nullptr;
	_graph = nullptr;
	_pointer = nullptr;
	_pointerDebug = nullptr;
	_keyboard = nullptr;
	_keyboardDebug = nullptr;
	_joysticks = nullptr;

	::WriteLog(L"APP_SHUTDOWN_DONE");

	if (_logWriter)
	{
		ff::ProcessGlobals::Get()->GetLog().RemoveWriter(_logWriter);
		_logWriter = nullptr;
	}
}

void ff::AppGlobals::OnAppSuspending()
{
	::WriteLog(L"APP_STATE_SUSPENDING");

	PauseRenderLoop();
}

void ff::AppGlobals::OnAppResuming()
{
	::WriteLog(L"APP_STATE_RESUMING");
	StartRenderLoop();
}

void ff::AppGlobals::OnActiveChanged()
{
	if (UpdateTargetActive() && _gameLoopDispatch)
	{
		bool active = GetAppActive();
		_gameLoopDispatch->Post([this, active]()
		{
			_activeChangedEvent.Notify(active);
		});
	}
}

void ff::AppGlobals::OnVisibilityChanged()
{
	if (UpdateTargetVisible())
	{
		if (GetAppVisible())
		{
			StartRenderLoop();
		}
		else
		{
			PauseRenderLoop();
		}
	}
}

void ff::AppGlobals::OnFocusChanged()
{
	KillPendingInput();
}

void ff::AppGlobals::OnSizeChanged()
{
	UpdateSwapChain();
}

void ff::AppGlobals::OnLogicalDpiChanged()
{
	if (UpdateDpiScale())
	{
		UpdateSwapChain();
	}
}

void ff::AppGlobals::OnDisplayContentsInvalidated()
{
	ValidateGraphDevice();
}

bool ff::AppGlobals::HasFlag(AppGlobalsFlags flag) const
{
	unsigned int intFlag = (unsigned int)_flags;
	unsigned int intCheck = (unsigned int)flag;

	return (intFlag & intCheck) == intCheck;
}

bool ff::AppGlobals::Initialize()
{
	UpdateTargetActive();
	UpdateTargetVisible();
	UpdateDpiScale();

	assertRetVal(InitializeTempDirectory(), false);
	assertRetVal(InitializeLog(), false);
	assertRetVal(InitializeGraphics(), false);
	assertRetVal(InitializeRenderTarget(), false);
	assertRetVal(InitializeRenderDepth(), false);
	assertRetVal(InitializeAudio(), false);
	assertRetVal(InitializeDeviceEvents(), false);
	assertRetVal(InitializePointer(), false);
	assertRetVal(InitializeKeyboard(), false);
	assertRetVal(InitializeJoystick(), false);
	assertRetVal(LoadState(), false);
	assertRetVal(OnInitialized(), false);
	assertRetVal(_helper->OnInitialized(this), false);

	if (GetAppVisible())
	{
		StartRenderLoop();
	}

	return true;
}

bool ff::AppGlobals::InitializeTempDirectory()
{
	Vector<String> tempDirs, tempFiles;
	if (ff::GetDirectoryContents(ff::GetTempDirectory(), tempDirs, tempFiles))
	{
		for (StringRef tempDir : tempDirs)
		{
			String unusedTempPath = ff::GetTempDirectory();
			ff::AppendPathTail(unusedTempPath, tempDir);

			ff::GetThreadPool()->AddTask([unusedTempPath]()
			{
				ff::DeleteDirectory(unusedTempPath, false);
			});
		}
	}

	String subDir = String::format_new(L"%u", ::GetCurrentProcessId());
	ff::SetTempSubDirectory(subDir);

	return true;
}

bool ff::AppGlobals::InitializeLog()
{
	String fullPath = ff::GetTempDirectory();
	ff::AppendPathTail(fullPath, String(L"log.txt"));

	ComPtr<IDataFile> file;
	ComPtr<IDataWriter> writer;
	if (ff::CreateDataFile(fullPath, false, &file) &&
		ff::CreateDataWriter(file, 0, &writer))
	{
		_logFile = fullPath;
		_logWriter = writer;

		ff::WriteUnicodeBOM(writer);
		ff::ProcessGlobals::Get()->GetLog().AddWriter(writer);
	}

	::WriteLog(L"APP_STARTUP");

	return true;
}

bool ff::AppGlobals::InitializeGraphics()
{
	noAssertRetVal(HasFlag(AppGlobalsFlags::UseGraphics), true);

	::WriteLog(L"APP_INIT_DIRECT3D");
	assertRetVal(_graph = ff::ProcessGlobals::Get()->GetGraphicFactory()->CreateDevice(), false);

	return true;
}

bool ff::AppGlobals::InitializeRenderTarget()
{
	noAssertRetVal(HasFlag(AppGlobalsFlags::UseWindowRenderTarget), true);

	::WriteLog(L"APP_INIT_RENDER_TARGET");
	assertRetVal(_target = CreateRenderTargetWindow(), false);

	return true;
}

bool ff::AppGlobals::InitializeRenderDepth()
{
	noAssertRetVal(HasFlag(AppGlobalsFlags::UseWindowRenderDepth), true);

	::WriteLog(L"APP_INIT_RENDER_DEPTH");
	assertRetVal(_depth = _graph->CreateRenderDepth(), false);

	return true;
}

bool ff::AppGlobals::InitializeAudio()
{
	noAssertRetVal(HasFlag(AppGlobalsFlags::UseAudio), true);

	::WriteLog(L"APP_INIT_AUDIO");
	assertRetVal(_audio = ff::ProcessGlobals::Get()->GetAudioFactory()->CreateDevice(), false);

	return true;
}

bool ff::AppGlobals::InitializeDeviceEvents()
{
	::WriteLog(L"APP_INIT_DEVICE_EVENTS");
	assertRetVal(_deviceEvents = ff::CreateDeviceEventSink(), false);
	assertRetVal(_deviceEventsDebug = ff::CreateDeviceEventSink(), false);
	return true;
}

bool ff::AppGlobals::InitializePointer()
{
	noAssertRetVal(HasFlag(AppGlobalsFlags::UsePointer), true);

	::WriteLog(L"APP_INIT_POINTER");
	assertRetVal(_pointer = CreatePointerDevice(), false);
	assertRetVal(_pointerDebug = CreatePointerDevice(), false);

	ComPtr<IDeviceEventProvider> provider;
	if (provider.QueryFrom(_pointer))
	{
		provider->SetSink(_deviceEvents);
	}

	if (provider.QueryFrom(_pointerDebug))
	{
		provider->SetSink(_deviceEventsDebug);
	}

	return true;
}

bool ff::AppGlobals::InitializeKeyboard()
{
	noAssertRetVal(HasFlag(AppGlobalsFlags::UseKeyboard), true);

	::WriteLog(L"APP_INIT_KEYBOARD");
	assertRetVal(_keyboard = CreateKeyboardDevice(), false);
	assertRetVal(_keyboardDebug = CreateKeyboardDevice(), false);

	ComPtr<IDeviceEventProvider> provider;
	if (provider.QueryFrom(_keyboard))
	{
		provider->SetSink(_deviceEvents);
	}

	if (provider.QueryFrom(_keyboardDebug))
	{
		provider->SetSink(_deviceEventsDebug);
	}

	return true;
}

bool ff::AppGlobals::InitializeJoystick()
{
	noAssertRetVal(HasFlag(AppGlobalsFlags::UseJoystick), true);

	::WriteLog(L"APP_INIT_JOYSTICKS");
	assertRetVal(_joysticks = CreateJoystickInput(), false);
	assertRetVal(_joysticksDebug = CreateJoystickInput(), false);

	ComPtr<IDeviceEventProvider> provider;
	if (provider.QueryFrom(_joysticks))
	{
		provider->SetSink(_deviceEvents);
	}

	if (provider.QueryFrom(_joysticksDebug))
	{
		provider->SetSink(_deviceEventsDebug);
	}

	return true;
}

void ff::AppGlobals::StartRenderLoop()
{
	assert(ff::GetMainThreadDispatch()->IsCurrentThread());

	::WriteLog(L"APP_RENDER_START");

	if (_gameLoopDispatch)
	{
		_gameLoopDispatch->Post([this]()
		{
			_gameLoopState = GameLoopState::Running;
			FrameResetTimer();
		});
	}
	else if (HasFlag(AppGlobalsFlags::UseGameLoop))
	{
		_gameLoopState = GameLoopState::Running;

		ff::GetThreadPool()->AddThread([this]()
		{
			GameThread();
		});

		ff::WaitForEventAndReset(_gameLoopEvent);
	}
}

void ff::AppGlobals::PauseRenderLoop()
{
	assert(ff::GetMainThreadDispatch()->IsCurrentThread());

	::WriteLog(L"APP_RENDER_PAUSE");

	if (_gameLoopDispatch)
	{
		_gameLoopDispatch->Post([this]()
		{
			if (_gameLoopState == GameLoopState::Running)
			{
				_gameLoopState = GameLoopState::Pausing;
			}
			else
			{
				::SetEvent(_gameLoopEvent);
			}
		});

		ff::WaitForEventAndReset(_gameLoopEvent);
	}

	SaveState();
}

void ff::AppGlobals::StopRenderLoop()
{
	assert(ff::GetMainThreadDispatch()->IsCurrentThread());

	::WriteLog(L"APP_RENDER_STOP");

	if (_gameLoopDispatch)
	{
		_gameLoopDispatch->Post([this]()
		{
			_gameLoopState = GameLoopState::Stopped;
		});

		ff::WaitForEventAndReset(_gameLoopEvent);
	}

	SaveState();
}

void ff::AppGlobals::GameThread()
{
	assert(!ff::GetMainThreadDispatch()->IsCurrentThread());
	::SetThreadDescription(::GetCurrentThread(), L"ff : Game Loop");

	ff::ThreadGlobalsScope<ff::ThreadGlobals> threadScope;
	_gameLoopDispatch = threadScope.GetGlobals().GetDispatch();
	::SetEvent(_gameLoopEvent);

	EnsureGameState();

	while (_gameLoopState != GameLoopState::Stopped)
	{
		if (_gameState->GetStatus() == State::Status::Dead)
		{
			_gameLoopState = GameLoopState::Stopped;
		}
		else if (_gameLoopState == GameLoopState::Pausing)
		{
			PauseGameState();
			::SetEvent(_gameLoopEvent);
		}
		else if (_gameLoopState == GameLoopState::Paused)
		{
			_gameLoopDispatch->WaitForAnyHandle(nullptr, 0);
		}
		else
		{
			FrameAdvanceAndRender();

			if (_target && !_target->Present(true))
			{
				ValidateGraphDevice();
			}
		}
	}

	_gameState->SaveState(this);
	_gameState = nullptr;
	_helper->OnGameThreadShutdown(this);
	_gameLoopDispatch->Flush();
	_gameLoopDispatch = nullptr;

	::SetEvent(_gameLoopEvent);
}

void ff::AppGlobals::EnsureGameState()
{
	if (_debugPageState == nullptr)
	{
		_debugPageState = std::make_shared<DebugPageState>(this);
	}

	_helper->OnGameThreadInitialized(this);

	if (_gameState == nullptr)
	{
		std::shared_ptr<States> states = std::make_shared<States>();
		states->AddBottom(_debugPageState);
		states->AddBottom(_helper->CreateInitialState(this));

		_gameState = std::make_shared<StateWrapper>(states);
	}

	_gameState->LoadState(this);

	FrameResetTimer();
}

void ff::AppGlobals::PauseGameState()
{
	_gameLoopState = GameLoopState::Paused;
	_gameState->SaveState(this);

	if (_graph && _graph->AsGraphDevice11())
	{
		_graph->AsGraphDevice11()->GetStateContext().Clear();
		_graph->AsGraphDeviceDxgi()->GetDxgi()->Trim();
	}
}

void ff::AppGlobals::UpdateSwapChain()
{
	assertRet(ff::GetMainThreadDispatch()->IsCurrentThread());

	if (_target)
	{
		ff::PointInt pixelSize;
		double dpiScale;
		DXGI_MODE_ROTATION nativeOrientation;
		DXGI_MODE_ROTATION currentOrientation;

		if (GetSwapChainSize(pixelSize, dpiScale, nativeOrientation, currentOrientation))
		{
			auto func = [this, pixelSize, dpiScale, nativeOrientation, currentOrientation]()
			{
				_target->SetSize(pixelSize, dpiScale, nativeOrientation, currentOrientation);
			};

			if (_gameLoopDispatch)
			{
				_gameLoopDispatch->Post(func);
			}
			else
			{
				func();
			}
		}
	}
}

bool ff::AppGlobals::UpdateTargetActive()
{
	assert(ff::GetMainThreadDispatch()->IsCurrentThread());

	bool value = IsWindowActive();
	if (_active != value)
	{
		_active = value;
		return true;
	}

	return false;
}

bool ff::AppGlobals::UpdateTargetVisible()
{
	assert(ff::GetMainThreadDispatch()->IsCurrentThread());

	bool value = IsWindowVisible();
	if (_visible != value)
	{
		_visible = value;
		return true;
	}

	return false;
}

bool ff::AppGlobals::UpdateDpiScale()
{
	assert(ff::GetMainThreadDispatch()->IsCurrentThread());

	double value = GetLogicalDpi() / 96.0;
	if (_dpiScale != value)
	{
		_dpiScale = value;
		return true;
	}

	return false;
}

void ff::AppGlobals::ValidateGraphDevice()
{
	ComPtr<IGraphDevice> graph = _graph;
	if (graph)
	{
		auto func = [graph]()
		{
			graph->ResetIfNeeded();
		};

		if (_gameLoopDispatch)
		{
			_gameLoopDispatch->Post(func);
		}
		else
		{
			func();
		}
	}
}

void ff::AppGlobals::KillPendingInput()
{
	if (_keyboard)
	{
		_keyboard->KillPending();
	}

	if (_pointer)
	{
		_pointer->KillPending();
	}

	if (_joysticks)
	{
		_joysticks->KillPending();
	}

	if (_keyboardDebug)
	{
		_keyboardDebug->KillPending();
	}

	if (_pointerDebug)
	{
		_pointerDebug->KillPending();
	}

	if (_joysticksDebug)
	{
		_joysticksDebug->KillPending();
	}
}

void ff::AppGlobals::FrameAdvanceAndRender()
{
	FrameAdvanceDebugResources();

	AdvanceType advanceType = FrameStartTimer();

	while (FrameAdvanceTimer(advanceType))
	{
		if (_frameTime._advanceCount > 1)
		{
			FrameAdvanceDebugResources();
		}

		FrameAdvanceResources();
		_gameState->Advance(this);

		if (_frameTime._advanceCount > 0 && _frameTime._advanceCount <= _frameTime._advanceTime.size())
		{
			_frameTime._advanceTime[_frameTime._advanceCount - 1] = _timer.GetCurrentStoredRawTime();
		}
	}

	FrameRender(advanceType);
}

void ff::AppGlobals::FrameAdvanceResources()
{
	if (_keyboard)
	{
		_keyboard->Advance();
	}

	if (_pointer)
	{
		_pointer->Advance();
	}

	if (_joysticks)
	{
		_joysticks->Advance();
	}

	if (_deviceEvents)
	{
		_deviceEvents->Advance();
	}

	if (_audio)
	{
		_audio->AdvanceEffects();
	}

	_gameState->AdvanceInput(this);
}

void ff::AppGlobals::FrameAdvanceDebugResources()
{
	_gameLoopDispatch->Flush();

	if (_keyboardDebug)
	{
		_keyboardDebug->Advance();
	}

	if (_pointerDebug)
	{
		_pointerDebug->Advance();
	}

	if (_joysticksDebug)
	{
		_joysticksDebug->Advance();
	}

	if (_deviceEventsDebug)
	{
		_deviceEventsDebug->Advance();
	}

	_gameState->AdvanceDebugInput(this);
}

void ff::AppGlobals::FrameResetTimer()
{
	_timer.Tick(0.0);
	_timer.StoreLastTickTime();
}

ff::AdvanceType ff::AppGlobals::FrameStartTimer()
{
	ff::AdvanceType advanceType = _helper->GetAdvanceType(this);
	double timeScale = _helper->GetTimeScale(this);

	if (timeScale <= 0.0)
	{
		advanceType = AdvanceType::Stopped;
	}

	bool running = (advanceType == AdvanceType::Running);
	_globalTime._timeScale = running ? timeScale : 0.0;
	_timer.SetTimeScale(_globalTime._timeScale);

	_globalTime._frameCount++;
	_globalTime._appSeconds = _timer.GetSeconds();
	_globalTime._bankSeconds += _timer.Tick(running ? -1.0 : _globalTime._secondsPerAdvance);
	_globalTime._clockSeconds = _timer.GetClockSeconds();

	_frameTime._advanceCount = 0;
	_frameTime._flipTime = _timer.GetLastTickStoredRawTime();

	_gameState->OnFrameStarted(this, advanceType);

	return advanceType;
}

bool ff::AppGlobals::FrameAdvanceTimer(AdvanceType advanceType)
{
	size_t maxAdvances = FrameTime::MAX_ADVANCE_COUNT;
#if _DEBUG
	if (::IsDebuggerPresent())
	{
		maxAdvances = 1;
	}
#endif

	switch (advanceType)
	{
	case AdvanceType::Stopped:
		return false;

	case AdvanceType::SingleStep:
		if (_frameTime._advanceCount)
		{
			return false;
		}
		break;

	case AdvanceType::Running:
		assert(_timer.GetTimeScale() > 0.0);

		if (_globalTime._bankSeconds < _globalTime._secondsPerAdvance)
		{
			return false;
		}
		else
		{
			size_t multiplier = std::min((size_t)std::ceil(_timer.GetTimeScale()), FrameTime::MAX_ADVANCE_MULTIPLIER);
			maxAdvances *= multiplier;
		}
		break;
	}

	if (_frameTime._advanceCount >= maxAdvances)
	{
		// The game is running way too slow
		_globalTime._bankSeconds = std::fmod(_globalTime._bankSeconds, _globalTime._secondsPerAdvance);
		_globalTime._bankScale = _globalTime._bankSeconds / _globalTime._secondsPerAdvance;
		return false;
	}
	else
	{
		_globalTime._appSeconds += _globalTime._secondsPerAdvance;
		_globalTime._bankSeconds = std::max(_globalTime._bankSeconds - _globalTime._secondsPerAdvance, 0.0);
		_globalTime._bankScale = _globalTime._bankSeconds / _globalTime._secondsPerAdvance;
		_globalTime._advanceCount++;
		_frameTime._advanceCount++;
		return true;
	}
}

void ff::AppGlobals::FrameRender(AdvanceType advanceType)
{
	_gameState->OnFrameRendering(this, advanceType);

	_target->Clear();
	_gameState->Render(this, _target, _depth);

	_frameTime._renderTime = _timer.GetCurrentStoredRawTime();
	_frameTime._graphCounters = _graph->ResetDrawCount();
	_globalTime._renderCount++;

	_gameState->OnFrameRendered(this, advanceType, _target, _depth);
}
