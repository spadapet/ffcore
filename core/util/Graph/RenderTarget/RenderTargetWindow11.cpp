#include "pch.h"
#include "COM/ComAlloc.h"
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

	bool Init(HWND hwnd);

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
	virtual bool SetSize(ff::PointInt pixelSize, double dpiScale, DXGI_MODE_ROTATION nativeOrientation, DXGI_MODE_ROTATION currentOrientation) override;
	virtual ff::Event<ff::PointInt, double, int>& SizeChanged() override;
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

	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<IDXGISwapChainX> _swapChain;
	ff::ComPtr<ID3D11Texture2D> _backBuffer;
	ff::ComPtr<ID3D11RenderTargetView> _target;
	ff::Event<ff::PointInt, double, int> _sizeChangedEvent;
	HWND _hwnd;
	HWND _hwndTop;
	bool _mainWindow;
	double _dpiScale;
};

BEGIN_INTERFACES(RenderTargetWindow11)
	HAS_INTERFACE(ff::IRenderTarget)
	HAS_INTERFACE(ff::IRenderTargetSwapChain)
	HAS_INTERFACE(ff::IRenderTargetWindow)
END_INTERFACES()

bool CreateRenderTargetWindow11(ff::IGraphDevice* pDevice, HWND hwnd, ff::IRenderTargetWindow** ppRender)
{
	assertRetVal(ppRender, false);
	*ppRender = nullptr;

	ff::ComPtr<RenderTargetWindow11, ff::IRenderTarget> pRender;
	assertHrRetVal(ff::ComAllocator<RenderTargetWindow11>::CreateInstance(pDevice, &pRender), false);
	assertRetVal(pRender->Init(hwnd), false);

	*ppRender = pRender.Detach();
	return true;
}

RenderTargetWindow11::RenderTargetWindow11()
	: _hwnd(nullptr)
	, _hwndTop(nullptr)
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
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool RenderTargetWindow11::Init(HWND hwnd)
{
	assertRetVal(_device && hwnd, false);

	_mainWindow = (GetWindowStyle(hwnd) & WS_CHILD) == 0;
	_hwnd = hwnd;
	_hwndTop = _mainWindow ? hwnd : GetAncestor(hwnd, GA_ROOT);

	ff::ThreadGlobals::Get()->AddWindowListener(_hwnd, this);
	ff::ThreadGlobals::Get()->AddWindowListener(_hwndTop, this);

	assertRetVal(InitSetSize(), false);

	return true;
}

bool RenderTargetWindow11::InitSetSize()
{
	ff::PointInt pixelSize;
	double dpiScale;
	DXGI_MODE_ROTATION nativeOrientation;
	DXGI_MODE_ROTATION currentOrientation;

	assertRetVal(ff::GetSwapChainSize(_hwnd, pixelSize, dpiScale, nativeOrientation, currentOrientation), false);
	assertRetVal(SetSize(pixelSize, dpiScale, nativeOrientation, currentOrientation), false);

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

bool RenderTargetWindow11::SetSize(ff::PointInt pixelSize, double dpiScale, DXGI_MODE_ROTATION nativeOrientation, DXGI_MODE_ROTATION currentOrientation)
{
	pixelSize.x = std::max(pixelSize.x, 1);
	pixelSize.y = std::max(pixelSize.y, 1);

	_dpiScale = dpiScale;

	DXGI_MODE_ROTATION displayRotation = ff::ComputeDisplayRotation(nativeOrientation, currentOrientation);
	bool swapDimensions =
		displayRotation == DXGI_MODE_ROTATION_ROTATE90 ||
		displayRotation == DXGI_MODE_ROTATION_ROTATE270;

	ff::PointInt bufferSize(
		swapDimensions ? pixelSize.y : pixelSize.x,
		swapDimensions ? pixelSize.x : pixelSize.y);

	if (_swapChain)
	{
		if (bufferSize != GetBufferSize())
		{
			assertRetVal(ResizeSwapChain(bufferSize), false);
		}
	}
	else // first init
	{
		DXGI_SWAP_CHAIN_DESC1 desc;
		ff::ZeroObject(desc);

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
		HWND hwnd = _hwnd;

		assertHrRetVal(factory->CreateSwapChainForHwnd(_device->AsGraphDevice11()->Get3d(), hwnd, &desc, nullptr, nullptr, &swapChain), false);
		assertRetVal(_swapChain.QueryFrom(swapChain), false);

		ff::GetMainThreadDispatch()->Post([swapChain, factory, hwnd]()
			{
				verifyHr(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES));
			}, true);

		assertHrRetVal(_device->AsGraphDeviceDxgi()->GetDxgi()->SetMaximumFrameLatency(1), false);
	}

	EnsureRenderTarget();

	assertHrRetVal(_swapChain->SetRotation(displayRotation), false);

	_sizeChangedEvent.Notify(GetBufferSize(), GetDpiScale(), GetRotatedDegrees());

	return true;
}

ff::Event<ff::PointInt, double, int>& RenderTargetWindow11::SizeChanged()
{
	return _sizeChangedEvent;
}

ff::IGraphDevice* RenderTargetWindow11::GetDevice() const
{
	return _device;
}

bool RenderTargetWindow11::Reset()
{
	SetFullScreen(false);
	_swapChain = nullptr;
	FlushBeforeResize();
	assertRetVal(InitSetSize(), false);

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

	DXGI_PRESENT_PARAMETERS pp = { 0 };
	HRESULT hr = _swapChain->Present1(vsync ? 1 : 0, 0, &pp);

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

	ff::ComPtr<IDXGIOutput> pOutput;
	BOOL bFullScreen = FALSE;

	return SUCCEEDED(_swapChain->GetFullscreenState(&bFullScreen, &pOutput)) && bFullScreen;
}

bool RenderTargetWindow11::SetFullScreen(bool bFullScreen)
{
	if (!bFullScreen != !IsFullScreen() && SUCCEEDED(_swapChain->SetFullscreenState(bFullScreen, nullptr)))
	{
		return InitSetSize();
	}

	return false;
}

bool RenderTargetWindow11::ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult)
{
	if (hwnd == _hwnd)
	{
		if (DeviceWindowProc(hwnd, msg, wParam, lParam, nResult))
		{
			return true;
		}
	}

	if (hwnd == _hwndTop)
	{
		if (DeviceTopWindowProc(hwnd, msg, wParam, lParam, nResult))
		{
			return true;
		}
	}

	return false;
}

bool RenderTargetWindow11::DeviceWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
	switch (msg)
	{
	case WM_DESTROY:
		Destroy();
		break;

	case WM_KEYDOWN:
		if (wParam == VK_LWIN || wParam == VK_RWIN)
		{
			ff::ComPtr<RenderTargetWindow11> self = this;
			ff::GetGameThreadDispatch()->Post([self]()
				{
					self->SetFullScreen(false);
				});
		}
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN) // ALT-ENTER to toggle full screen mode
		{
			ff::ComPtr<RenderTargetWindow11> self = this;
			ff::GetGameThreadDispatch()->Post([self]()
				{
					self->SetFullScreen(!self->IsFullScreen());
				});

			result = 0;
			return true;
		}
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
			ff::ComPtr<RenderTargetWindow11> self = this;
			ff::GetGameThreadDispatch()->Post([self]()
				{
					self->SetFullScreen(false);
				});
		}
		break;
	}

	return false;
}

#endif // !METRO_APP
