#pragma once

#include "Dict/Dict.h"
#include "Globals/AppGlobalsHelper.h"
#include "Types/Event.h"
#include "Types/Timer.h"
#include "Windows/Handles.h"

namespace ff
{
	class DebugPageState;
	class IAudioDevice;
	class IDebugPages;
	class IDeviceEventSink;
	class IGraphDevice;
	class IJoystickInput;
	class IKeyboardDevice;
	class IPointerDevice;
	class IRenderDepth;
	class IRenderTargetWindow;
	class IThreadDispatch;

	enum class AppGlobalsFlags
	{
		None = 0,
		UseGraphics = 1 << 0,
		UseAudio = 1 << 1,
		UsePointer = 1 << 2,
		UseKeyboard = 1 << 3,
		UseJoystick = 1 << 4,
		UseGameLoop = 1 << 5,
		UseWindowRenderTarget = 1 << 6,
		UseWindowRenderDepth = 1 << 7,

		All =
		UseGraphics |
		UseAudio |
		UsePointer |
		UseKeyboard |
		UseJoystick |
		UseGameLoop |
		UseWindowRenderTarget |
		UseWindowRenderDepth,

		GraphicsAndAudio = UseGraphics | UseAudio,
	};

	class AppGlobals : public IAppGlobalsHelper
	{
	public:
		UTIL_API AppGlobals();
		UTIL_API virtual ~AppGlobals();

		UTIL_API static AppGlobals* Get();

		// AppGlobals
		UTIL_API IGraphDevice* GetGraph() const;
		UTIL_API IAudioDevice* GetAudio() const;
		UTIL_API IPointerDevice* GetPointer() const;
		UTIL_API IPointerDevice* GetPointerDebug() const;
		UTIL_API IKeyboardDevice* GetKeys() const;
		UTIL_API IKeyboardDevice* GetKeysDebug() const;
		UTIL_API IJoystickInput* GetJoysticks() const;
		UTIL_API IJoystickInput* GetJoysticksDebug() const;
		UTIL_API IRenderTargetWindow* GetTarget() const;
		UTIL_API IRenderDepth* GetDepth() const;
		UTIL_API IDeviceEventSink* GetDeviceEvents() const;
		UTIL_API IDeviceEventSink* GetDeviceEventsDebug() const;
		UTIL_API IThreadDispatch* GetGameDispatch() const;
		UTIL_API IAppGlobalsHelper* GetAppGlobalsHelper() const;
		UTIL_API const GlobalTime& GetGlobalTime() const;
		UTIL_API const FrameTime& GetFrameTime() const;
		UTIL_API State* GetGameState() const;
		UTIL_API void AddDebugPage(IDebugPages* page);
		UTIL_API void RemoveDebugPage(IDebugPages* page);
		UTIL_API const Vector<IDebugPages*>& GetDebugPages() const;
		UTIL_API DebugPageState* GetDebugPageState() const;
		UTIL_API String GetLogFile() const;
		UTIL_API bool GetAppActive() const;
		UTIL_API bool GetAppVisible() const;

		UTIL_API double GetDpiScale() const;
		UTIL_API double PixelToDip(double size) const;
		UTIL_API double DipToPixel(double size) const;

		UTIL_API bool IsFullScreen();
		UTIL_API bool SetFullScreen(bool fullScreen);
		UTIL_API bool CanSetFullScreen();

		// App state
		UTIL_API bool LoadState();
		UTIL_API bool SaveState();
		UTIL_API Dict GetState();
		UTIL_API void SetState(const Dict& dict);
		UTIL_API void ClearAllState();
		UTIL_API String GetStateFile() const;
		UTIL_API Dict GetState(StringRef name);
		UTIL_API void SetState(StringRef name, const Dict& dict);

		// Events
		UTIL_API ff::Event<bool>& ActiveChanged();

	protected:
		// Event handlers
		bool OnStartup(AppGlobalsFlags flags, IAppGlobalsHelper* helper);
		void OnShutdown();
		void OnAppSuspending();
		void OnAppResuming();
		void OnActiveChanged();
		void OnVisibilityChanged();
		void OnFocusChanged();
		void OnSizeChanged();
		void OnLogicalDpiChanged();
		void OnDisplayContentsInvalidated();

		virtual bool OnInitialized() = 0;
		virtual double GetLogicalDpi() = 0;
		virtual bool GetSwapChainSize(ff::PointInt& pixelSize, double& dpiScale, DXGI_MODE_ROTATION& nativeOrientation, DXGI_MODE_ROTATION& currentOrientation) = 0;
		virtual bool IsWindowActive() = 0;
		virtual bool IsWindowVisible() = 0;
		virtual bool IsWindowFocused() = 0;
		virtual ff::ComPtr<ff::IRenderTargetWindow> CreateRenderTargetWindow() = 0;
		virtual ff::ComPtr<ff::IPointerDevice> CreatePointerDevice() = 0;
		virtual ff::ComPtr<ff::IKeyboardDevice> CreateKeyboardDevice() = 0;
		virtual ff::ComPtr<ff::IJoystickInput> CreateJoystickInput() = 0;

	private:
		// Initialization
		bool HasFlag(AppGlobalsFlags flag) const;
		bool Initialize();
		bool InitializeTempDirectory();
		bool InitializeLog();
		bool InitializeGraphics();
		bool InitializeRenderTarget();
		bool InitializeRenderDepth();
		bool InitializeAudio();
		bool InitializeDeviceEvents();
		bool InitializePointer();
		bool InitializeKeyboard();
		bool InitializeJoystick();

		// Update/render loop
		void StartRenderLoop();
		void PauseRenderLoop();
		void StopRenderLoop();
		void GameThread();
		void EnsureGameState();
		void PauseGameState();
		void UpdateSwapChain();
		bool UpdateTargetActive();
		bool UpdateTargetVisible();
		bool UpdateDpiScale();
		void ValidateGraphDevice();
		void KillPendingInput();

		// Update during game loop
		void FrameAdvanceAndRender();
		void FrameAdvanceResources();
		void FrameAdvanceDebugResources();
		void FrameResetTimer();
		AdvanceType FrameStartTimer();
		bool FrameAdvanceTimer(AdvanceType advanceType);
		void FrameRender(AdvanceType advanceType);

		// Properties
		bool _valid;
		bool _active;
		bool _visible;
		double _dpiScale;
		AppGlobalsFlags _flags;
		IAppGlobalsHelper* _helper;
		Vector<IDebugPages*> _debugPages;

		// State
		ff::Mutex _stateLock;
		Map<String, Dict> _namedState;
		Dict _namedStateCache;
		bool _stateChanged;

		// Resource context
		ComPtr<IAudioDevice> _audio;
		ComPtr<IGraphDevice> _graph;
		ComPtr<IRenderTargetWindow> _target;
		ComPtr<IRenderDepth> _depth;
		ComPtr<IKeyboardDevice> _keyboard;
		ComPtr<IKeyboardDevice> _keyboardDebug;
		ComPtr<IJoystickInput> _joysticks;
		ComPtr<IJoystickInput> _joysticksDebug;
		ComPtr<IPointerDevice> _pointer;
		ComPtr<IPointerDevice> _pointerDebug;
		ComPtr<IDeviceEventSink> _deviceEvents;
		ComPtr<IDeviceEventSink> _deviceEventsDebug;

		// Log
		String _logFile;
		ComPtr<IDataWriter> _logWriter;

		// Game loop
		enum class GameLoopState { Stopped, Running, Pausing, Paused };
		GameLoopState _gameLoopState;
		WinHandle _gameLoopEvent;
		std::shared_ptr<State> _gameState;
		std::shared_ptr<DebugPageState> _debugPageState;
		ComPtr<IThreadDispatch> _gameLoopDispatch;

		// Game timer
		Timer _timer;
		FrameTime _frameTime;
		GlobalTime _globalTime;

		// Events
		ff::Event<bool> _activeChangedEvent;
	};
}
