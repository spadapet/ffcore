#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/AppGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/RenderTarget/RenderTargetWindow.h"
#include "Graph/State/GraphContext11.h"
#include "Module/Module.h"
#include "Module/ModuleFactory.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadUtil.h"

#if METRO_APP

#include "Windows.UI.XAML.Media.DxInterop.h"

class __declspec(uuid("52e3f024-2b70-4e42-afe2-1070ab22f42d"))
	RenderTargetWindowMetro11
	: public ff::ComBase
	, public ff::IRenderTargetWindow
	, public ff::IRenderTarget11
{
public:
	DECLARE_HEADER(RenderTargetWindowMetro11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(ff::AppGlobals* globals, Windows::UI::Xaml::Window^ window, Windows::UI::ViewManagement::ApplicationView^ view, Windows::UI::Xaml::Controls::SwapChainPanel^ panel);

	// IGraphDeviceChild
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IRenderTarget
	virtual ff::TextureFormat GetFormat() const override;
	virtual ff::PointInt GetBufferSize() const override;
	virtual ff::PointInt GetRotatedSize() const override;
	virtual int GetRotatedDegrees() const override;
	virtual double GetDpiScale() const override;
	virtual void Clear(const DirectX::XMFLOAT4* pColor) override;
	virtual ff::IRenderTarget11* AsRenderTarget11() override;

	// IRenderTarget11
	virtual ID3D11Texture2D* GetTexture() override;
	virtual ID3D11RenderTargetView* GetTarget() override;

	// IRenderTargetSwapChain
	virtual bool Present(bool vsync) override;
	virtual bool SetSize(const ff::SwapChainSize& size) override;
	virtual ff::Event<ff::PointInt, double, int>& SizeChanged() override;

	// IRenderTargetWindow
	virtual bool CanSetFullScreen() const override;
	virtual bool IsFullScreen() override;
	virtual bool SetFullScreen(bool fullScreen) override;

private:
	ref class KeyEvents
	{
	internal:
		KeyEvents(RenderTargetWindowMetro11* pParent);

	public:
		virtual ~KeyEvents();

		void Destroy();

	private:
		void OnAcceleratorKeyDown(Windows::UI::Core::CoreDispatcher^ sender, Windows::UI::Core::AcceleratorKeyEventArgs^ args);

		RenderTargetWindowMetro11* _parent;
		Windows::Foundation::EventRegistrationToken _tokens[1];
	};

	void OnAcceleratorKeyDown(Windows::UI::Core::CoreDispatcher^ sender, Windows::UI::Core::AcceleratorKeyEventArgs^ args);

	ff::AppGlobals* _globals;
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<IDXGISwapChainX> _swapChain;
	ff::ComPtr<ID3D11Texture2D> _backBuffer;
	ff::ComPtr<ID3D11RenderTargetView> _target;

	Windows::UI::Xaml::Controls::SwapChainPanel^ _panel;
	Windows::UI::ViewManagement::ApplicationView^ _view;
	Windows::UI::Xaml::Window^ _window;
	KeyEvents^ _keyEvents;

	ff::Event<ff::PointInt, double, int> _sizeChangedEvent;
	ff::SwapChainSize _size;
	bool _cachedFullScreenMode;
	bool _fullScreenMode;
};

BEGIN_INTERFACES(RenderTargetWindowMetro11)
	HAS_INTERFACE(ff::IRenderTarget)
	HAS_INTERFACE(ff::IRenderTargetSwapChain)
	HAS_INTERFACE(ff::IRenderTargetWindow)
END_INTERFACES()

RenderTargetWindowMetro11::KeyEvents::KeyEvents(RenderTargetWindowMetro11* pParent)
	: _parent(pParent)
{
	_parent->_window->Dispatcher->AcceleratorKeyActivated +=
		ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreDispatcher^, Windows::UI::Core::AcceleratorKeyEventArgs^>(
			this, &KeyEvents::OnAcceleratorKeyDown);
}

RenderTargetWindowMetro11::KeyEvents::~KeyEvents()
{
	Destroy();
}

void RenderTargetWindowMetro11::KeyEvents::Destroy()
{
	if (_parent)
	{
		_parent->_window->Dispatcher->AcceleratorKeyActivated -= _tokens[0];
		_parent = nullptr;
	}
}

void RenderTargetWindowMetro11::KeyEvents::OnAcceleratorKeyDown(Windows::UI::Core::CoreDispatcher^ sender, Windows::UI::Core::AcceleratorKeyEventArgs^ args)
{
	assertRet(_parent);
	_parent->OnAcceleratorKeyDown(sender, args);
}

bool CreateRenderTargetWindow11(ff::AppGlobals* globals, ff::IGraphDevice* pDevice, Windows::UI::Xaml::Window^ window, ff::IRenderTargetWindow** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	auto windowContent = dynamic_cast<Windows::UI::Xaml::Controls::UserControl^>(window->Content);
	assertRetVal(windowContent, false);

	auto panel = dynamic_cast<Windows::UI::Xaml::Controls::SwapChainPanel^>(windowContent->Content);
	assertRetVal(panel, false);

	ff::ComPtr<RenderTargetWindowMetro11, ff::IRenderTarget> myObj;
	assertHrRetVal(ff::ComAllocator<RenderTargetWindowMetro11>::CreateInstance(pDevice, &myObj), false);

	Windows::UI::ViewManagement::ApplicationView^ view = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
	assertRetVal(myObj->Init(globals, window, view, panel), false);

	*obj = myObj.Detach();
	return true;
}

bool CreateRenderTargetSwapChain11(ff::AppGlobals* globals, ff::IGraphDevice* pDevice, Windows::UI::Xaml::Controls::SwapChainPanel^ panel, ff::IRenderTargetSwapChain** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ComPtr<RenderTargetWindowMetro11, ff::IRenderTarget> myObj;
	assertHrRetVal(ff::ComAllocator<RenderTargetWindowMetro11>::CreateInstance(pDevice, &myObj), false);
	assertRetVal(myObj->Init(globals, nullptr, nullptr, panel), false);

	*obj = myObj.Detach();
	return true;
}

RenderTargetWindowMetro11::RenderTargetWindowMetro11()
	: _globals(nullptr)
	, _size{}
	, _cachedFullScreenMode(false)
	, _fullScreenMode(false)
{
	_size._dpiScale = 1;
}

RenderTargetWindowMetro11::~RenderTargetWindowMetro11()
{
	if (_keyEvents)
	{
		_keyEvents->Destroy();
	}

	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT RenderTargetWindowMetro11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	_device->AddChild(this);

	return ff::ComBase::_Construct(unkOuter);
}

bool RenderTargetWindowMetro11::Init(ff::AppGlobals* globals, Windows::UI::Xaml::Window^ window, Windows::UI::ViewManagement::ApplicationView^ view, Windows::UI::Xaml::Controls::SwapChainPanel^ panel)
{
	assertRetVal(_device && globals && panel != nullptr, false);

	_globals = globals;
	_window = window;
	_view = view;
	_panel = panel;
	_keyEvents = (_window != nullptr) ? ref new KeyEvents(this) : nullptr;

	ff::PointDouble panelScale(_panel->CompositionScaleX, _panel->CompositionScaleY);
	Windows::Graphics::Display::DisplayInformation^ displayInfo = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();

	ff::SwapChainSize size;
	size._pixelSize = ff::PointInt((int)(panelScale.x * _panel->ActualWidth), (int)(panelScale.y * _panel->ActualHeight));
	size._dpiScale = panelScale.x;
	size._nativeOrientation = ff::GetDxgiRotation(displayInfo->NativeOrientation);
	size._currentOrientation = ff::GetDxgiRotation(displayInfo->CurrentOrientation);

	assertRetVal(SetSize(size), false);

	return true;
}

// from RenderTargetTexture11.cpp
bool CreateRenderTarget11(
	ff::IGraphDevice* pDevice,
	ID3D11Texture2D* pTexture,
	size_t nArrayStart,
	size_t nArrayCount,
	size_t nMipLevel,
	ID3D11RenderTargetView** ppView);

bool RenderTargetWindowMetro11::SetSize(const ff::SwapChainSize& size)
{
	_size._pixelSize = ff::PointInt(std::max(size._pixelSize.x, 1), std::max(size._pixelSize.y, 1));
	_size._dpiScale = size._dpiScale;
	_size._nativeOrientation = size._nativeOrientation;
	_size._currentOrientation = size._currentOrientation;
	_cachedFullScreenMode = false;
	_fullScreenMode = false;

	DXGI_MODE_ROTATION displayRotation = ff::ComputeDisplayRotation(size._nativeOrientation, size._currentOrientation);
	bool swapDimensions = (displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270);
	ff::PointInt bufferSize(swapDimensions ? _size._pixelSize.y : _size._pixelSize.x, swapDimensions ? _size._pixelSize.x : _size._pixelSize.y);

	if (_swapChain)
	{
		if (bufferSize != GetBufferSize())
		{
			// Clear old state
			_target = nullptr;
			_backBuffer = nullptr;
			_device->AsGraphDevice11()->GetStateContext().Clear();

			DXGI_SWAP_CHAIN_DESC1 desc;
			_swapChain->GetDesc1(&desc);
			assertHrRetVal(_swapChain->ResizeBuffers(0, bufferSize.x, bufferSize.y, desc.Format, desc.Flags), false);
		}
	}
	else // first init
	{
		DXGI_SWAP_CHAIN_DESC1 desc{};
		desc.Width = bufferSize.x;
		desc.Height = bufferSize.y;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.BufferCount = 2;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		ff::ComPtr<IDXGISwapChain1> swapChain;
		assertHrRetVal(_device->AsGraphDeviceDxgi()->GetDxgiFactory()->CreateSwapChainForComposition(_device->AsGraphDevice11()->Get3d(), &desc, nullptr, &swapChain), false);
		assertRetVal(_swapChain.QueryFrom(swapChain), false);
		Windows::UI::Xaml::Controls::SwapChainPanel^ panel = _panel;

		ff::GetMainThreadDispatch()->Post([panel, swapChain]()
			{
				ff::ComPtr<ISwapChainPanelNative> nativePanel;
				assertRet(nativePanel.QueryFrom(panel));
				verifyHr(nativePanel->SetSwapChain(swapChain));
			}, true);
	}

	// Create render target
	if (!_backBuffer)
	{
		assertHrRetVal(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&_backBuffer), false);
		assertRetVal(::CreateRenderTarget11(_device, _backBuffer, 0, 1, 0, &_target), false);
	}

	// Scale the back buffer to the panel
	DXGI_MATRIX_3X2_F inverseScale = { 0 };
	inverseScale._11 = 1 / (float)_size._dpiScale;
	inverseScale._22 = 1 / (float)_size._dpiScale;

	assertHrRetVal(_swapChain->SetRotation(displayRotation), false);
	assertHrRetVal(_swapChain->SetMatrixTransform(&inverseScale), false);

	_sizeChangedEvent.Notify(GetBufferSize(), GetDpiScale(), GetRotatedDegrees());

	return true;
}

ff::Event<ff::PointInt, double, int>& RenderTargetWindowMetro11::SizeChanged()
{
	return _sizeChangedEvent;
}

ff::IGraphDevice* RenderTargetWindowMetro11::GetDevice() const
{
	return _device;
}

bool RenderTargetWindowMetro11::Reset()
{
	_target = nullptr;
	_backBuffer = nullptr;
	_swapChain = nullptr;

	assertRetVal(SetSize(_size), false);

	return true;
}

ff::TextureFormat RenderTargetWindowMetro11::GetFormat() const
{
	return ff::TextureFormat::BGRA32;
}

ff::PointInt RenderTargetWindowMetro11::GetBufferSize() const
{
	assertRetVal(_swapChain, ff::PointInt(0, 0));

	DXGI_SWAP_CHAIN_DESC1 desc;
	_swapChain->GetDesc1(&desc);

	return ff::PointInt(desc.Width, desc.Height);
}

ff::PointInt RenderTargetWindowMetro11::GetRotatedSize() const
{
	ff::PointInt size = GetBufferSize();

	int rotation = GetRotatedDegrees();
	if (rotation == 90 || rotation == 270)
	{
		std::swap(size.x, size.y);
	}

	return size;
}

int RenderTargetWindowMetro11::GetRotatedDegrees() const
{
	DXGI_MODE_ROTATION rotation;
	if (_swapChain && SUCCEEDED(_swapChain->GetRotation(&rotation)) &&
		rotation != DXGI_MODE_ROTATION_UNSPECIFIED)
	{
		return (rotation - 1) * 90;
	}

	return 0;
}

double RenderTargetWindowMetro11::GetDpiScale() const
{
	return _size._dpiScale;
}

void RenderTargetWindowMetro11::Clear(const DirectX::XMFLOAT4* pColor)
{
	assertRet(_target);
	_device->AsGraphDevice11()->GetStateContext().ClearRenderTarget(_target, pColor ? *pColor : ff::GetColorBlack());
}

ff::IRenderTarget11* RenderTargetWindowMetro11::AsRenderTarget11()
{
	return this;
}

ID3D11Texture2D* RenderTargetWindowMetro11::GetTexture()
{
	return _backBuffer;
}

ID3D11RenderTargetView* RenderTargetWindowMetro11::GetTarget()
{
	return _target;
}

bool RenderTargetWindowMetro11::Present(bool vsync)
{
	assertRetVal(_swapChain && _target, false);

	DXGI_PRESENT_PARAMETERS pp = { 0 };
	HRESULT hr = _swapChain->Present1(vsync ? 1 : 0, 0, &pp);

	_device->AsGraphDevice11()->GetContext()->DiscardView1(_target, nullptr, 0);
	_device->AsGraphDevice11()->GetStateContext().SetTargets(nullptr, 0, nullptr);

	return hr != DXGI_ERROR_DEVICE_RESET && hr != DXGI_ERROR_DEVICE_REMOVED;
}

bool RenderTargetWindowMetro11::CanSetFullScreen() const
{
	return _view != nullptr;
}

bool RenderTargetWindowMetro11::IsFullScreen()
{
	noAssertRetVal(_view, false);

	if (!_cachedFullScreenMode)
	{
		_fullScreenMode = _view->IsFullScreenMode;
		_cachedFullScreenMode = true;
	}

	return _fullScreenMode;
}

bool RenderTargetWindowMetro11::SetFullScreen(bool fullScreen)
{
	if (!_view)
	{
		return false;
	}
	else if (fullScreen)
	{
		return _view->TryEnterFullScreenMode();
	}
	else
	{
		_view->ExitFullScreenMode();
		return true;
	}
}

void RenderTargetWindowMetro11::OnAcceleratorKeyDown(Windows::UI::Core::CoreDispatcher^ sender, Windows::UI::Core::AcceleratorKeyEventArgs^ args)
{
	if (args->EventType == Windows::UI::Core::CoreAcceleratorKeyEventType::SystemKeyDown && args->KeyStatus.IsMenuKeyDown)
	{
		if (args->VirtualKey == Windows::System::VirtualKey::Enter)
		{
			args->Handled = true;
			_globals->SetFullScreen(!IsFullScreen());
		}
		else if (args->VirtualKey == Windows::System::VirtualKey::Back)
		{
			args->Handled = true;
			_globals->ValidateGraphDevice(true);
		}
	}
}

#endif // METRO_APP
