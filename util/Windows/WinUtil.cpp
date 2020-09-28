#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "Globals/ProcessStartup.h"
#include "Thread/ThreadUtil.h"
#include "Windows/WinUtil.h"

#if !METRO_APP

// STATIC_DATA (pod)
static bool s_gotQuitMessage = false;

void ff::HandleMessage(MSG& msg)
{
	::TranslateMessage(&msg);
	::DispatchMessage(&msg);
}

bool ff::HandleMessages()
{
	// flush thread APCs
	::MsgWaitForMultipleObjectsEx(0, nullptr, 0, QS_ALLEVENTS, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);

	MSG msg;
	while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message != WM_QUIT)
		{
			ff::HandleMessage(msg);
		}
		else
		{
			s_gotQuitMessage = true;
		}
	}

	return !s_gotQuitMessage;
}

bool ff::WaitForMessage(DWORD timeout)
{
	if (!s_gotQuitMessage)
	{
		// This is better than calling GetMessage because it allows APCs to be called

		DWORD nResult = ::MsgWaitForMultipleObjectsEx(
			0, // count
			nullptr, // handles
			timeout ? timeout : INFINITE,
			QS_ALLINPUT, // wake mask
			MWMO_ALERTABLE | MWMO_INPUTAVAILABLE); // flags
	}

	return HandleMessages();
}

bool ff::GotQuitMessage()
{
	return s_gotQuitMessage;
}

void ff::PumpMessagesUntilQuit()
{
	for (bool quit = ff::GotQuitMessage(); !quit; )
	{
		quit = !ff::WaitForMessage();
	}
}

ff::Window::Window()
	: _hwnd(nullptr)
{
}

ff::Window::Window(Window&& rhs)
	: _hwnd(rhs._hwnd)
{
	rhs._hwnd = nullptr;
}

ff::Window::~Window()
{
	assert(!_hwnd);
	_hwnd = nullptr;
}

HWND ff::Window::Handle() const
{
	return _hwnd;
}

void ff::Window::PostNcDestroy()
{
	// do nothing
}

ff::CustomWindow::CustomWindow()
{
}

ff::CustomWindow::CustomWindow(CustomWindow&& rhs)
	: Window(std::move(rhs))
{
	if (_hwnd)
	{
		::SetWindowLongPtr(_hwnd, 0, (ULONG_PTR)this);
	}
}

ff::CustomWindow::~CustomWindow()
{
	if (_hwnd)
	{
		::DestroyWindow(_hwnd);
	}
}

// static
bool ff::CustomWindow::CreateClass(
	StringRef name,
	DWORD nStyle,
	HINSTANCE hInstance,
	HCURSOR hCursor,
	HBRUSH hBrush,
	UINT menu,
	HICON hLargeIcon,
	HICON hSmallIcon)
{
	// see if the class was already registered
	{
		WNDCLASSEX clsExisting;
		ff::ZeroObject(clsExisting);
		clsExisting.cbSize = sizeof(clsExisting);

		if (::GetClassInfoEx(hInstance, name.c_str(), &clsExisting))
		{
			return true;
		}
	}

	WNDCLASSEX clsNew =
	{
		sizeof(WNDCLASSEX), // size
		nStyle, // style
		BaseWindowProc, // window procedure
		0, // extra class bytes
		sizeof(ULONG_PTR), // extra window bytes
		hInstance, // program instance
		hLargeIcon, // Icon
		hCursor, // cursor
		hBrush, // background
		MAKEINTRESOURCE(menu), // Menu
		name.c_str(), // class name
		hSmallIcon // small icon
	};

	return (::RegisterClassEx(&clsNew) != 0);
}

bool ff::CustomWindow::Create(
	StringRef className,
	StringRef windowName,
	HWND hParent,
	DWORD nStyle,
	DWORD nExStyle,
	int x, int y, int cx, int cy,
	HINSTANCE hInstance,
	HMENU hMenu)
{
	assert(!_hwnd);

	HWND hNewWindow = ::CreateWindowEx(
		nExStyle, // extended style
		className.c_str(), // class name
		windowName.c_str(), // window name
		nStyle, // style
		x, y, cx, cy, // x, y, width, height
		hParent, // parent
		hMenu, // menu
		hInstance, // instance
		(LPVOID)this); // parameter

	if (hNewWindow)
	{
		assert(_hwnd == hNewWindow);
	}
	else
	{
		assert(!_hwnd);
	}

	return _hwnd != nullptr;
}

bool ff::CustomWindow::CreateBlank(
	StringRef windowName,
	HWND hParent,
	DWORD nStyle,
	DWORD nExStyle,
	int x, int y, int cx, int cy,
	HMENU hMenu)
{
	static StaticString className(L"BlankCustomWindow");

	assertRetVal(CreateClass(
		className.GetString(),
		CS_DBLCLKS,
		GetThisModule().GetInstance(),
		::LoadCursor(nullptr, IDC_ARROW),
		nullptr,
		0, // menu
		nullptr,
		nullptr), false);

	return Create(
		className,
		windowName,
		hParent,
		nStyle,
		nExStyle,
		x, y, cx, cy,
		GetThisModule().GetInstance(),
		hMenu);
}

bool ff::CustomWindow::CreateBlank(HWND hParent, DWORD nStyle, DWORD nExStyle, int x, int y, int cx, int cy)
{
	return CreateBlank(GetEmptyString(), hParent, nStyle, nExStyle, x, y, cx, cy, nullptr);
}

bool ff::CustomWindow::CreateMessageWindow(StringRef className, StringRef windowName)
{
	HINSTANCE hInstance = GetThisModule().GetInstance();
	String realClassName = className.size() ? className : GetMessageWindowClassName();

	return CreateClass(realClassName, 0, hInstance, nullptr, nullptr, 0, nullptr, nullptr) &&
		Create(realClassName, windowName, HWND_MESSAGE, 0, 0, 0, 0, 0, 0, hInstance, nullptr);
}

// static
ff::StringRef ff::CustomWindow::GetMessageWindowClassName()
{
	static StaticString value(L"ff::MessageOnlyWindow");
	return value;
}

LRESULT ff::CustomWindow::DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT ff::CustomWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DoDefault(hwnd, msg, wParam, lParam);
}

// static
LRESULT CALLBACK ff::CustomWindow::BaseWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE)
	{
		CustomWindow* pWnd = (CustomWindow*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
		assert(pWnd && !pWnd->_hwnd);

		pWnd->_hwnd = hwnd;
		::SetWindowLongPtr(hwnd, 0, (ULONG_PTR)pWnd);
	}

	CustomWindow* pWnd = (CustomWindow*)::GetWindowLongPtr(hwnd, 0);

	LRESULT ret = pWnd
		? pWnd->WindowProc(hwnd, msg, wParam, lParam)
		: ::DefWindowProc(hwnd, msg, wParam, lParam);

	if (msg == WM_NCDESTROY && pWnd)
	{
		::SetWindowLongPtr(hwnd, 0, 0);
		pWnd->_hwnd = nullptr;
		pWnd->PostNcDestroy();
	}

	return ret;
}

// static
bool ff::CustomWindow::IsCustomWindow(HWND hwnd)
{
	assertRetVal(hwnd, false);
	WNDPROC proc = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
	return proc == BaseWindowProc;
}

HWND ff::CustomWindow::Detach()
{
	assertRetVal(_hwnd, nullptr);

	// All window proc overrides must detach by now
	assertRetVal(IsCustomWindow(_hwnd), nullptr);

	HWND hwnd = _hwnd;
	_hwnd = nullptr;

	::SetWindowLongPtr(hwnd, 0, 0);
	PostNcDestroy();

	return hwnd;
}

bool ff::CustomWindow::Attach(HWND hwnd)
{
	assertRetVal(!_hwnd && hwnd && IsCustomWindow(hwnd), false);

	_hwnd = hwnd;
	::SetWindowLongPtr(hwnd, 0, (ULONG_PTR)this);

	return true;
}

ff::ListenedWindow::ListenedWindow()
	: _listener(nullptr)
{
}

ff::ListenedWindow::ListenedWindow(ListenedWindow&& rhs)
	: CustomWindow(std::move(rhs))
	, _listener(rhs._listener)
{
	rhs._listener = nullptr;
}

ff::ListenedWindow::~ListenedWindow()
{
}

void ff::ListenedWindow::SetListener(IWindowProcListener* pListener)
{
	_listener = pListener;
}

LRESULT ff::ListenedWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (_listener)
	{
		LRESULT nResult = 0;

		if (_listener->ListenWindowProc(hwnd, msg, wParam, lParam, nResult))
		{
			return nResult;
		}
	}

	return DoDefault(hwnd, msg, wParam, lParam);
}

#endif // METRO_APP
