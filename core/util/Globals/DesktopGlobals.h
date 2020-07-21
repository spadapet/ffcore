#pragma once

#include "Globals/AppGlobals.h"
#include "Windows/WinUtil.h"

#if !METRO_APP

namespace ff
{
	class DesktopGlobals : public AppGlobals, public IWindowProcListener
	{
	public:
		UTIL_API DesktopGlobals();
		UTIL_API virtual ~DesktopGlobals();

		UTIL_API static DesktopGlobals* Get();
		UTIL_API static CustomWindow CreateMainWindow(StringRef name);
		UTIL_API static bool RunWithWindow(AppGlobalsFlags flags, std::shared_ptr<IAppGlobalsHelper>& helper);

		UTIL_API bool Startup(AppGlobalsFlags flags, HWND hwnd = nullptr, IAppGlobalsHelper* helper = nullptr);
		UTIL_API void Shutdown();

		// IWindowProcListener
		virtual bool ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult) override;

	protected:
		virtual bool OnInitialized() override;
		virtual double GetLogicalDpi() override;
		virtual bool GetSwapChainSize(ff::SwapChainSize& size) override;
		virtual bool IsWindowActive() override;
		virtual bool IsWindowVisible() override;
		virtual bool IsWindowFocused() override;
		virtual bool CloseWindow() override;
		virtual ff::ComPtr<ff::IRenderTargetWindow> CreateRenderTargetWindow() override;
		virtual ff::ComPtr<ff::IPointerDevice> CreatePointerDevice() override;
		virtual ff::ComPtr<ff::IKeyboardDevice> CreateKeyboardDevice() override;
		virtual ff::ComPtr<ff::IJoystickInput> CreateJoystickInput() override;

	private:
		HWND _hwnd;
		HWND _hwndTop;
		bool _shutdown;
	};
}

#endif
