#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/AppGlobals.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/RenderTarget/RenderTargetWindow.h"
#include "Graph/State/GraphContext11.h"
#include "Graph/Texture/Texture.h"
#include "Module/ModuleFactory.h"
#include "Thread/ThreadDispatch.h"
#include "Windows/WinUtil.h"

#if !METRO_APP

// from RenderTargetTexture11.cpp
bool CreateRenderTarget11(ff::IGraphDevice* pDevice, ID3D11Texture2D* pTexture, size_t nArrayStart, size_t nArrayCount, size_t nMipLevel, ID3D11RenderTargetView** ppView);

class __declspec(uuid("e3a230cb-287b-406a-8dfd-a05073c08904"))
	RenderTargetWindow11
	: public ff::ComBase
	, public ff::IRenderTargetWindow
	, public ff::IRenderTarget11
	, public ff::IWindowProcListener
{
public:
	DECLARE_HEADER(RenderTargetWindow11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;

	bool Init(ff::AppGlobals* globals, HWND hwnd);

	// IGraphDeviceChild
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IRenderTarget
	virtual ff::TextureFormat GetFormat() const override;
	virtual ff::PointInt GetBufferSize() const override;
	virtual ff::PointInt GetRotatedSize() const override;
	virtual int GetRotatedDegrees() const override;
	virtual double GetDpiScale() const override;
	virtual void Clear(const DirectX::XMFLOAT4* pColor = nullptr) override;
	virtual ff::IRenderTarget11* AsRenderTarget11() override;

	// IRenderTarget11
	virtual ID3D11Texture2D* GetTexture() override;
	virtual ID3D11RenderTargetView* GetTarget() override;

	// IRenderTargetWindow
	virtual bool SetSize(const ff::SwapChainSize& size) override;
	virtual entt::sink<void(ff::PointInt, double, int)> SizeChangedSink() override;
	virtual bool Present(bool vsync) override;
	virtual bool CanSetFullScreen() const override;
	virtual bool IsFullScreen() override;
	virtual bool SetFullScreen(bool fullScreen) override;

	// IWindowProcListener
	virtual bool ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult) override;

private:
	void Destroy();
	bool InitSetSize();
	void FlushBeforeResize();
	bool ResizeSwapChain(ff::PointInt size);
	bool EnsureRenderTarget();

	bool DeviceWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result);
	bool DeviceTopWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result);

	ff::AppGlobals* _globals;
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<IDXGISwapChainX> _swapChain;
	ff::ComPtr<ID3D11Texture2D> _backBuffer;
	ff::ComPtr<ID3D11RenderTargetView> _target;
	entt::sigh<void(ff::PointInt, double, int)> _sizeChangedEvent;
	ff::Mutex _hwndMutex;
	HWND _hwnd;
	HWND _hwndTop;
	bool _mainWindow;
	bool _wasFullScreenOnClose;
	double _dpiScale;
};

BEGIN_INTERFACES(RenderTargetWindow11)
	HAS_INTERFACE(ff::IRenderTarget)
	HAS_INTERFACE(ff::IRenderTargetSwapChain)
	HAS_INTERFACE(ff::IRenderTargetWindow)
END_INTERFACES()

bool CreateRenderTargetWindow11(ff::AppGlobals* globals, ff::IGraphDevice* pDevice, HWND hwnd, ff::IRenderTargetWindow** ppRender)
{
	assertRetVal(ppRender, false);
	*ppRender = nullptr;

	ff::ComPtr<RenderTargetWindow11, ff::IRenderTarget> pRender;
	assertHrRetVal(ff::ComAllocator<RenderTargetWindow11>::CreateInstance(pDevice, &pRender), false);
	assertRetVal(pRender->Init(globals, hwnd), false);

	*ppRender = pRender.Detach();
	return true;
}

RenderTargetWindow11::RenderTargetWindow11()
	: _globals(nullptr)
	, _hwnd(nullptr)
	, _hwndTop(nullptr)
	, _mainWindow(false)
	, _wasFullScreenOnClose(false)
	, _dpiScale(1)
{
}

RenderTargetWindow11::~RenderTargetWindow11()
{
	Destroy();

	if (_swapChain)
	{
		_swapChain->SetFullscreenState(FALSE, nullptr);
	}

	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT RenderTargetWindow11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	_device->AddChild(this, -100);

	return ff::ComBase::_Construct(unkOuter);
}

bool RenderTargetWindow11::Init(ff::AppGlobals* globals, HWND hwnd)
{
	assertRetVal(_device && globals && hwnd, false);

	_globals = globals;
	_mainWindow = (GetWindowStyle(hwnd) & WS_CHILD) == 0;
	_hwnd = hwnd;
	_hwndTop = _mainWindow ? hwnd : GetAncestor(hwnd, GA_ROOT);

	ff::ThreadGlobals::Get()->AddWindowListener(_hwnd, this);
	ff::ThreadGlobals::Get()->AddWindowListener(_hwndTop, this);

	assertRetVal(InitSetSize(), false);

	return true;
}

// Game thread (accessing HWND on background thread)
bool RenderTargetWindow11::InitSetSize()
{
	ff::LockMutex lock(_hwndMutex);
	noAssertRetVal(_hwnd, false);

	ff::SwapChainSize size;
	assertRetVal(ff::GetSwapChainSize(_hwnd, size), false);
	assertRetVal(SetSize(size), false);

	return true;
}

void RenderTargetWindow11::FlushBeforeResize()
{
	_target = nullptr;
	_backBuffer = nullptr;
	_device->AsGraphDevice11()->GetStateContext().Clear();
}

bool RenderTargetWindow11::ResizeSwapChain(ff::PointInt size)
{
	assertRetVal(_swapChain, false);

	FlushBeforeResize();

	DXGI_SWAP_CHAIN_DESC1 desc;
	_swapChain->GetDesc1(&desc);
	assertHrRetVal(_swapChain->ResizeBuffers(0, size.x, size.y, desc.Format, desc.Flags), false);

	return true;
}

bool RenderTargetWindow11::EnsureRenderTarget()
{
	assertRetVal(_swapChain, false);

	if (!_backBuffer)
	{
		assertHrRetVal(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&_backBuffer), false);
	}

	if (!_target)
	{
		assertRetVal(::CreateRenderTarget11(_device, _backBuffer, 0, 1, 0, &_target), false);
	}

	return true;
}

void RenderTargetWindow11::Destroy()
{
	ff::LockMutex lock(_hwndMutex);

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

bool RenderTargetWindow11::SetSize(const ff::SwapChainSize& size)
{
	_dpiScale = size._dpiScale;

	DXGI_MODE_ROTATION displayRotation = ff::ComputeDisplayRotation(size._nativeOrientation, size._currentOrientation);
	bool swapDimensions = (displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270);
	ff::PointInt pixelSize(std::max(size._pixelSize.x, 1), std::max(size._pixelSize.y, 1));
	ff::PointInt bufferSize(swapDimensions ? pixelSize.y : pixelSize.x, swapDimensions ? pixelSize.x : pixelSize.y);

	if (_swapChain) // on game thread
	{
		if (bufferSize != GetBufferSize())
		{
			assertRetVal(ResizeSwapChain(bufferSize), false);
		}
	}
	else // first init on UI thread, reset is on game thread
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
		ff::ComPtr<IDXGIFactoryX> factory = _device->AsGraphDeviceDxgi()->GetDxgiFactory();

		assertHrRetVal(factory->CreateSwapChainForHwnd(_device->AsGraphDevice11()->Get3d(), _hwnd, &desc, nullptr, nullptr, &swapChain), false);
		assertHrRetVal(factory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_WINDOW_CHANGES), false);
		assertRetVal(_swapChain.QueryFrom(swapChain), false);
	}

	EnsureRenderTarget();

	assertHrRetVal(_swapChain->SetRotation(displayRotation), false);

	_sizeChangedEvent.publish(GetBufferSize(), GetDpiScale(), GetRotatedDegrees());

	return true;
}

entt::sink<void(ff::PointInt, double, int)> RenderTargetWindow11::SizeChangedSink()
{
	return _sizeChangedEvent;
}

ff::IGraphDevice* RenderTargetWindow11::GetDevice() const
{
	return _device;
}

bool RenderTargetWindow11::Reset()
{
	BOOL fullScreen = FALSE;
	if (_swapChain)
	{
		ff::ComPtr<IDXGIOutput> output;
		_swapChain->GetFullscreenState(&fullScreen, &output);
		_swapChain->SetFullscreenState(FALSE, nullptr);
		_swapChain = nullptr;
	}

	FlushBeforeResize();
	assertRetVal(InitSetSize(), false);

	if (_swapChain && fullScreen)
	{
		_swapChain->SetFullscreenState(TRUE, nullptr);
	}

	return true;
}

ff::TextureFormat RenderTargetWindow11::GetFormat() const
{
	return ff::TextureFormat::BGRA32;
}

ff::PointInt RenderTargetWindow11::GetBufferSize() const
{
	assertRetVal(_swapChain, ff::PointInt(0, 0));

	DXGI_SWAP_CHAIN_DESC1 desc;
	_swapChain->GetDesc1(&desc);

	return ff::PointInt(desc.Width, desc.Height);
}

ff::PointInt RenderTargetWindow11::GetRotatedSize() const
{
	ff::PointInt size = GetBufferSize();

	int rotation = GetRotatedDegrees();
	if (rotation == 90 || rotation == 270)
	{
		std::swap(size.x, size.y);
	}

	return size;
}

int RenderTargetWindow11::GetRotatedDegrees() const
{
	DXGI_MODE_ROTATION rotation;
	if (_swapChain && SUCCEEDED(_swapChain->GetRotation(&rotation)) &&
		rotation != DXGI_MODE_ROTATION_UNSPECIFIED)
	{
		return (rotation - 1) * 90;
	}

	return 0;
}

double RenderTargetWindow11::GetDpiScale() const
{
	return _dpiScale;
}

void RenderTargetWindow11::Clear(const DirectX::XMFLOAT4* pColor)
{
	assertRet(_target);
	_device->AsGraphDevice11()->GetStateContext().ClearRenderTarget(_target, pColor ? *pColor : ff::GetColorBlack());
}

ff::IRenderTarget11* RenderTargetWindow11::AsRenderTarget11()
{
	return this;
}

ID3D11Texture2D* RenderTargetWindow11::GetTexture()
{
	return _backBuffer;
}

ID3D11RenderTargetView* RenderTargetWindow11::GetTarget()
{
	return _target;
}

bool RenderTargetWindow11::Present(bool vsync)
{
	assertRetVal(_swapChain && _target, false);
	HRESULT hr = E_FAIL;

	if (_hwnd)
	{
		ff::LockMutex lock(_hwndMutex);
		if (_hwnd)
		{
			DXGI_PRESENT_PARAMETERS pp = { 0 };
			hr = _swapChain->Present1(vsync ? 1 : 0, 0, &pp);
		}
	}

	_device->AsGraphDevice11()->GetContext()->DiscardView1(_target, nullptr, 0);
	_device->AsGraphDevice11()->GetStateContext().SetTargets(nullptr, 0, nullptr);

	return hr != DXGI_ERROR_DEVICE_RESET && hr != DXGI_ERROR_DEVICE_REMOVED;
}

bool RenderTargetWindow11::CanSetFullScreen() const
{
	return true;
}

bool RenderTargetWindow11::IsFullScreen()
{
	noAssertRetVal(_swapChain && _mainWindow, false);

	if (_hwnd)
	{
		ff::LockMutex lock(_hwndMutex);
		if (_hwnd)
		{
			ff::ComPtr<IDXGIOutput> output;
			BOOL fullScreen = FALSE;
			return SUCCEEDED(_swapChain->GetFullscreenState(&fullScreen, &output)) && fullScreen;
		}
	}

	return _wasFullScreenOnClose;
}

bool RenderTargetWindow11::SetFullScreen(bool bFullScreen)
{
	if (_hwnd)
	{
		ff::LockMutex lock(_hwndMutex);
		if (_hwnd && !bFullScreen != !IsFullScreen())
		{
			// SetFullscreenState will send messages to the main thread, don't hold any mutex
			lock.Unlock();

			if (SUCCEEDED(_swapChain->SetFullscreenState(bFullScreen, nullptr)))
			{
				return InitSetSize();
			}
		}
	}

	return false;
}

bool RenderTargetWindow11::ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult)
{
	if (hwnd == _hwnd && DeviceWindowProc(hwnd, msg, wParam, lParam, nResult))
	{
		return true;
	}

	if (hwnd == _hwndTop && DeviceTopWindowProc(hwnd, msg, wParam, lParam, nResult))
	{
		return true;
	}

	return false;
}

bool RenderTargetWindow11::DeviceWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
	switch (msg)
	{
	case WM_CLOSE:
		_wasFullScreenOnClose = IsFullScreen();
		break;

	case WM_DESTROY:
		Destroy();
		break;

	case WM_KEYDOWN:
		if (wParam == VK_LWIN || wParam == VK_RWIN)
		{
			_globals->SetFullScreen(false);
		}
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN) // ALT-ENTER to toggle full screen mode
		{
			_globals->SetFullScreen(!IsFullScreen());
			result = 0;
			return true;
		}
#ifdef _DEBUG
		else if (wParam == VK_BACK)
		{
			_globals->ValidateGraphDevice(true);
		}
#endif

		break;

	case WM_SYSCHAR:
		if (wParam == VK_RETURN)
		{
			// prevent a 'ding' sound when switching between modes
			result = 0;
			return true;
		}
		break;
	}

	return false;
}

bool RenderTargetWindow11::DeviceTopWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE && _mainWindow)
		{
			_globals->SetFullScreen(false);
		}
		break;
	}

	return false;
}

#endif // !METRO_APP
