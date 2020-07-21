#include "pch.h"
#include "Globals/DesktopGlobals.h"
#include "Globals/GlobalsScope.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderTargetWindow.h"
#include "Input/Joystick/JoystickInput.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "Input/Pointer/PointerDevice.h"

#if !METRO_APP

static ff::DesktopGlobals* s_desktopGlobals = nullptr;

ff::DesktopGlobals* ff::DesktopGlobals::Get()
{
	return s_desktopGlobals;
}

ff::DesktopGlobals::DesktopGlobals()
	: _hwnd(nullptr)
	, _hwndTop(nullptr)
	, _shutdown(false)
{
	assert(!s_desktopGlobals);
	s_desktopGlobals = this;
}

ff::DesktopGlobals::~DesktopGlobals()
{
	Shutdown();

	assert(s_desktopGlobals == this);
	s_desktopGlobals = nullptr;
}

ff::CustomWindow ff::DesktopGlobals::CreateMainWindow(StringRef name)
{
	ff::CustomWindow mainWindow;
	if (mainWindow.CreateBlank(name, nullptr, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT))
	{
		return mainWindow;
	}

	assert(false);
	return ff::CustomWindow();
}

bool ff::DesktopGlobals::RunWithWindow(AppGlobalsFlags flags, std::shared_ptr<IAppGlobalsHelper>& helper)
{
	ff::ThreadGlobalsScope<ff::ProcessGlobals> globals;
	ff::DesktopGlobals app;
	ff::CustomWindow mainWindow = ff::DesktopGlobals::CreateMainWindow(helper->GetWindowName());
	bool status = false;

	if (app.Startup(ff::AppGlobalsFlags::All, mainWindow.Handle(), helper.get()))
	{
		ff::PumpMessagesUntilQuit();
		status = true;
	}

	helper.reset();
	return status;
}

bool ff::DesktopGlobals::Startup(AppGlobalsFlags flags, HWND hwnd, IAppGlobalsHelper* helper)
{
	bool isChild = hwnd && (GetWindowStyle(hwnd) & WS_CHILD) != 0;

	_hwnd = hwnd;
	_hwndTop = isChild ? GetAncestor(hwnd, GA_ROOT) : hwnd;

	ff::ThreadGlobals::Get()->AddWindowListener(_hwnd, this);
	ff::ThreadGlobals::Get()->AddWindowListener(_hwndTop, this);

	return OnStartup(flags, helper);
}

void ff::DesktopGlobals::Shutdown()
{
	if (!_shutdown)
	{
		_shutdown = true;
		OnShutdown();
	}

	if (_hwndTop)
	{
		ff::ThreadGlobals::Get()->RemoveWindowListener(_hwndTop, this);
		_hwndTop = nullptr;
	}

	if (_hwnd)
	{
		ff::ThreadGlobals::Get()->RemoveWindowListener(_hwnd, this);
		_hwnd = nullptr;
	}
}

bool ff::DesktopGlobals::ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult)
{
	switch (msg)
	{
	case WM_ACTIVATEAPP:
		if (hwnd == _hwndTop)
		{
			OnActiveChanged();
		}
		break;

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		if (hwnd == _hwnd)
		{
			OnFocusChanged();
		}
		break;

	case WM_SIZE:
		if (hwnd == _hwndTop)
		{
			OnVisibilityChanged();
		}

		if (hwnd == _hwnd && wParam != SIZE_MINIMIZED)
		{
			OnSizeChanged();
		}
		break;

	case WM_DPICHANGED:
		if (hwnd == _hwndTop)
		{
			OnLogicalDpiChanged();

			RECT* rect = (RECT*)lParam;
			::SetWindowPos(hwnd, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;

	case WM_DISPLAYCHANGE:
		if (hwnd == _hwndTop)
		{
			OnLogicalDpiChanged();
			OnDisplayContentsInvalidated();
		}
		break;

	case WM_DESTROY:
		if (hwnd == _hwndTop)
		{
			Shutdown();
			::PostQuitMessage(0);
		}
		break;
	}

	return false;
}

bool ff::DesktopGlobals::OnInitialized()
{
	return true;
}

double ff::DesktopGlobals::GetLogicalDpi()
{
	return _hwnd
		? (double)::GetDpiForWindow(_hwnd)
		: (double)::GetDpiForSystem();
}

bool ff::DesktopGlobals::GetSwapChainSize(ff::SwapChainSize& size)
{
	return ff::GetSwapChainSize(_hwnd, size);
}

bool ff::DesktopGlobals::IsWindowActive()
{
	if (_hwndTop)
	{
		for (HWND active = ::GetActiveWindow(); active; active = ::GetParent(active))
		{
			if (active == _hwndTop)
			{
				return true;
			}
		}
	}

	return false;
}

bool ff::DesktopGlobals::IsWindowVisible()
{
	return _hwnd && !::IsIconic(_hwnd);
}

bool ff::DesktopGlobals::IsWindowFocused()
{
	return _hwnd && ::GetFocus() == _hwnd;
}

bool ff::DesktopGlobals::CloseWindow()
{
	assertRetVal(_hwnd, false);
	::PostMessage(_hwnd, WM_CLOSE, 0, 0);
	return true;
}

ff::ComPtr<ff::IRenderTargetWindow> ff::DesktopGlobals::CreateRenderTargetWindow()
{
	return GetGraph()->CreateRenderTargetWindow(this, _hwnd);
}

ff::ComPtr<ff::IPointerDevice> ff::DesktopGlobals::CreatePointerDevice()
{
	ff::ComPtr<ff::IPointerDevice> pointerDevice;
	assertRetVal(ff::CreatePointerDevice(_hwnd, &pointerDevice), nullptr);
	return pointerDevice;
}

ff::ComPtr<ff::IKeyboardDevice> ff::DesktopGlobals::CreateKeyboardDevice()
{
	ff::ComPtr<ff::IKeyboardDevice> keyboardDevice;
	assertRetVal(ff::CreateKeyboardDevice(_hwnd, &keyboardDevice), nullptr);
	return keyboardDevice;
}

ff::ComPtr<ff::IJoystickInput> ff::DesktopGlobals::CreateJoystickInput()
{
	ff::ComPtr<ff::IJoystickInput> joystickInput;
	assertRetVal(ff::CreateJoystickInput(_hwnd, &joystickInput), nullptr);
	return joystickInput;
}

#endif
