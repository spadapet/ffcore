#include "pch.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Globals/GlobalsScope.h"
#include "Globals/Log.h"
#include "Globals/MetroGlobals.h"
#include "Globals/ThreadGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderTargetWindow.h"
#include "Input/Joystick/JoystickInput.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "Input/Pointer/PointerDevice.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadUtil.h"
#include "Windows/FileUtil.h"

#if METRO_APP

static ff::MetroGlobals* s_metroGlobals = nullptr;

// Listens to WinRT events for MetroGlobals
ref class MetroGlobalsEvents sealed
{
public:
	MetroGlobalsEvents();
	virtual ~MetroGlobalsEvents();

internal:
	bool Startup(ff::MetroGlobals* globals);
	void Shutdown();

private:
	void OnAppSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	void OnAppResuming(Platform::Object^ sender, Platform::Object^ arg);
	void OnActiveChanged(Platform::Object^ sender, Windows::UI::Core::WindowActivatedEventArgs^ args);
	void OnVisibilityChanged(Platform::Object^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
	void OnLogicalDpiChanged(Windows::Graphics::Display::DisplayInformation^ displayInfo, Platform::Object^ sender);
	void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ displayInfo, Platform::Object^ sender);
	void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ displayInfo, Platform::Object^ sender);
	void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ panel, Platform::Object^ sender);
	void OnSwapChainSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ args);

	ff::MetroGlobals* _globals;
	Windows::Foundation::EventRegistrationToken _tokens[9];
};

MetroGlobalsEvents::MetroGlobalsEvents()
	: _globals(nullptr)
{
}

MetroGlobalsEvents::~MetroGlobalsEvents()
{
}

bool MetroGlobalsEvents::Startup(ff::MetroGlobals* globals)
{
	assertRetVal(globals && !_globals, false);
	_globals = globals;

	_tokens[0] = Windows::ApplicationModel::Core::CoreApplication::Suspending +=
		ref new Windows::Foundation::EventHandler<Windows::ApplicationModel::SuspendingEventArgs^>(
			this, &MetroGlobalsEvents::OnAppSuspending);

	_tokens[1] = Windows::ApplicationModel::Core::CoreApplication::Resuming +=
		ref new Windows::Foundation::EventHandler<Platform::Object^>(
			this, &MetroGlobalsEvents::OnAppResuming);

	_tokens[2] = _globals->GetWindow()->Activated +=
		ref new Windows::UI::Xaml::WindowActivatedEventHandler(this, &MetroGlobalsEvents::OnActiveChanged);

	_tokens[3] = _globals->GetWindow()->VisibilityChanged +=
		ref new Windows::UI::Xaml::WindowVisibilityChangedEventHandler(this, &MetroGlobalsEvents::OnVisibilityChanged);

	_tokens[4] = _globals->GetDisplayInfo()->DpiChanged +=
		ref new Windows::Foundation::TypedEventHandler<Windows::Graphics::Display::DisplayInformation^, Platform::Object^>(
			this, &MetroGlobalsEvents::OnLogicalDpiChanged);

	_tokens[5] = _globals->GetDisplayInfo()->DisplayContentsInvalidated +=
		ref new Windows::Foundation::TypedEventHandler<Windows::Graphics::Display::DisplayInformation^, Platform::Object^>(
			this, &MetroGlobalsEvents::OnDisplayContentsInvalidated);

	_tokens[6] = _globals->GetDisplayInfo()->OrientationChanged +=
		ref new Windows::Foundation::TypedEventHandler<Windows::Graphics::Display::DisplayInformation^, Platform::Object^>(
			this, &MetroGlobalsEvents::OnOrientationChanged);

	if (_globals->GetSwapChainPanel())
	{
		_tokens[7] = _globals->GetSwapChainPanel()->CompositionScaleChanged +=
			ref new Windows::Foundation::TypedEventHandler<Windows::UI::Xaml::Controls::SwapChainPanel^, Platform::Object^>(
				this, &MetroGlobalsEvents::OnCompositionScaleChanged);

		_tokens[8] = _globals->GetSwapChainPanel()->SizeChanged +=
			ref new Windows::UI::Xaml::SizeChangedEventHandler(
				this, &MetroGlobalsEvents::OnSwapChainSizeChanged);
	}

	return true;
}

void MetroGlobalsEvents::Shutdown()
{
	noAssertRet(_globals);

	Windows::ApplicationModel::Core::CoreApplication::Suspending -= _tokens[0];
	Windows::ApplicationModel::Core::CoreApplication::Resuming -= _tokens[1];

	_globals->GetWindow()->Activated -= _tokens[2];
	_globals->GetWindow()->VisibilityChanged -= _tokens[3];
	_globals->GetDisplayInfo()->DpiChanged -= _tokens[4];
	_globals->GetDisplayInfo()->DisplayContentsInvalidated -= _tokens[5];
	_globals->GetDisplayInfo()->OrientationChanged -= _tokens[6];

	if (_globals->GetSwapChainPanel())
	{
		_globals->GetSwapChainPanel()->CompositionScaleChanged -= _tokens[7];
		_globals->GetSwapChainPanel()->SizeChanged -= _tokens[8];
	}

	_globals = nullptr;

	ff::ZeroObject(_tokens);
}

void MetroGlobalsEvents::OnAppSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args)
{
	Windows::ApplicationModel::SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();
	MetroGlobalsEvents^ self = this;

	ff::GetMainThreadDispatch()->Post([deferral, self]()
		{
			if (self->_globals)
			{
				self->_globals->OnAppSuspending();
			}

			deferral->Complete();
		});
}

void MetroGlobalsEvents::OnAppResuming(Platform::Object^ sender, Platform::Object^ arg)
{
	assertRet(_globals);
	_globals->OnAppResuming();
}

void MetroGlobalsEvents::OnActiveChanged(Platform::Object^ sender, Windows::UI::Core::WindowActivatedEventArgs^ args)
{
	assertRet(_globals);
	_globals->OnActiveChanged();
	_globals->OnFocusChanged();
}

void MetroGlobalsEvents::OnVisibilityChanged(Platform::Object^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args)
{
	assertRet(_globals);
	_globals->OnVisibilityChanged();
}

void MetroGlobalsEvents::OnLogicalDpiChanged(Windows::Graphics::Display::DisplayInformation^ displayInfo, Platform::Object^ sender)
{
	assertRet(_globals);
	_globals->OnLogicalDpiChanged();
}

void MetroGlobalsEvents::OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ displayInfo, Platform::Object^ sender)
{
	assertRet(_globals);
	_globals->OnDisplayContentsInvalidated();
}

void MetroGlobalsEvents::OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ displayInfo, Platform::Object^ sender)
{
	assertRet(_globals);
	_globals->OnSizeChanged();
}

void MetroGlobalsEvents::OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ panel, Platform::Object^ sender)
{
	assertRet(_globals);
	_globals->OnSizeChanged();
}

void MetroGlobalsEvents::OnSwapChainSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ args)
{
	assertRet(_globals);
	_globals->OnSizeChanged();
}

ff::MetroGlobals::MetroGlobals()
{
	assert(!s_metroGlobals);
	s_metroGlobals = this;
}

ff::MetroGlobals::~MetroGlobals()
{
	Shutdown();

	assert(s_metroGlobals == this);
	s_metroGlobals = nullptr;
}

ff::MetroGlobals* ff::MetroGlobals::Get()
{
	return s_metroGlobals;
}

bool ff::MetroGlobals::Startup(AppGlobalsFlags flags, Windows::UI::Xaml::Controls::SwapChainPanel^ swapPanel, IAppGlobalsHelper* helper)
{
	auto pointerSettings = Windows::UI::Input::PointerVisualizationSettings::GetForCurrentView();
	pointerSettings->IsBarrelButtonFeedbackEnabled = false;
	pointerSettings->IsContactFeedbackEnabled = false;

	_window = Windows::UI::Xaml::Window::Current;
	_swapPanel = swapPanel;
	_displayInfo = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();

	return OnStartup(flags, helper);
}

void ff::MetroGlobals::Shutdown()
{
	OnShutdown();

	if (_events)
	{
		_events->Shutdown();
		_events = nullptr;
	}

	_window = nullptr;
	_swapPanel = nullptr;
	_displayInfo = nullptr;
	_pointerEvents = nullptr;
}

Windows::UI::Xaml::Window^ ff::MetroGlobals::GetWindow() const
{
	return _window;
}

Windows::UI::Xaml::Controls::SwapChainPanel^ ff::MetroGlobals::GetSwapChainPanel() const
{
	return _swapPanel;
}

Windows::Graphics::Display::DisplayInformation^ ff::MetroGlobals::GetDisplayInfo() const
{
	return _displayInfo;
}

bool ff::MetroGlobals::OnInitialized()
{
	_events = ref new MetroGlobalsEvents();
	assertRetVal(_events->Startup(this), false);
	return true;
}

double ff::MetroGlobals::GetLogicalDpi()
{
	return _displayInfo->LogicalDpi;
}

bool ff::MetroGlobals::GetSwapChainSize(ff::SwapChainSize& size)
{
	noAssertRetVal(_swapPanel, false);

	size._dpiScale = _swapPanel->CompositionScaleX;
	size._pixelSize = (ff::PointDouble(_swapPanel->ActualWidth, _swapPanel->ActualHeight) * size._dpiScale).ToType<int>();
	size._nativeOrientation = ff::GetDxgiRotation(_displayInfo->NativeOrientation);
	size._currentOrientation = ff::GetDxgiRotation(_displayInfo->CurrentOrientation);

	return true;
}

bool ff::MetroGlobals::IsWindowActive()
{
	return _window && _window->CoreWindow->ActivationMode != Windows::UI::Core::CoreWindowActivationMode::Deactivated;
}

bool ff::MetroGlobals::IsWindowVisible()
{
	return _window && _window->Visible;
}

bool ff::MetroGlobals::IsWindowFocused()
{
	return IsWindowActive();
}

ff::ComPtr<ff::IRenderTargetWindow> ff::MetroGlobals::CreateRenderTargetWindow()
{
	return GetGraph()->CreateRenderTargetWindow(this, _window);
}

ff::ComPtr<ff::IPointerDevice> ff::MetroGlobals::CreatePointerDevice()
{
	if (!_pointerEvents)
	{
		assertRetVal(_pointerEvents = ff::CreatePointerEvents(this), nullptr);
	}

	return ff::CreatePointerDevice(_pointerEvents);
}

ff::ComPtr<ff::IKeyboardDevice> ff::MetroGlobals::CreateKeyboardDevice()
{
	ff::ComPtr<ff::IKeyboardDevice> keyboardDevice;
	assertRetVal(ff::CreateKeyboardDevice(_window, &keyboardDevice), nullptr);
	return keyboardDevice;
}

ff::ComPtr<ff::IJoystickInput> ff::MetroGlobals::CreateJoystickInput()
{
	ff::ComPtr<ff::IJoystickInput> joystickInput;
	assertRetVal(ff::CreateJoystickInput(&joystickInput), nullptr);
	return joystickInput;
}

#endif
