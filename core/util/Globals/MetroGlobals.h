#pragma once

#include "Globals/AppGlobals.h"

#if METRO_APP

ref class MetroGlobalsEvents;

namespace ff
{
	class IRenderTargetWindow;

	class MetroGlobals : public AppGlobals
	{
	public:
		UTIL_API MetroGlobals();
		UTIL_API virtual ~MetroGlobals();

		UTIL_API static MetroGlobals* Get();

		UTIL_API bool Startup(AppGlobalsFlags flags, Windows::UI::Xaml::Controls::SwapChainPanel^ swapPanel = nullptr, IAppGlobalsHelper* helper = nullptr);
		UTIL_API void Shutdown();

		UTIL_API Windows::UI::Xaml::Window^ GetWindow() const;
		UTIL_API Windows::UI::Xaml::Controls::SwapChainPanel^ GetSwapChainPanel() const;
		UTIL_API Windows::Graphics::Display::DisplayInformation^ GetDisplayInfo() const;

	protected:
		virtual bool OnInitialized() override;
		virtual double GetLogicalDpi() override;
		virtual bool GetSwapChainSize(ff::PointInt& pixelSize, double& dpiScale, DXGI_MODE_ROTATION& nativeOrientation, DXGI_MODE_ROTATION& currentOrientation) override;
		virtual bool IsWindowActive() override;
		virtual bool IsWindowVisible() override;
		virtual bool IsWindowFocused() override;
		virtual ff::ComPtr<ff::IRenderTargetWindow> CreateRenderTargetWindow() override;
		virtual ff::ComPtr<ff::IPointerDevice> CreatePointerDevice() override;
		virtual ff::ComPtr<ff::IKeyboardDevice> CreateKeyboardDevice() override;
		virtual ff::ComPtr<ff::IJoystickInput> CreateJoystickInput() override;

	private:
		friend ref class ::MetroGlobalsEvents;

		// Metro Data
		MetroGlobalsEvents^ _events;
		Windows::UI::Xaml::Window^ _window;
		Windows::UI::Xaml::Controls::SwapChainPanel^ _swapPanel;
		Windows::Graphics::Display::DisplayInformation^ _displayInfo;
		Platform::Object^ _pointerEvents;
	};
}

#endif
