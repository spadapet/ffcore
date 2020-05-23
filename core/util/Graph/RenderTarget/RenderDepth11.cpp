#include "pch.h"
#include "COM/ComAlloc.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/State/GraphContext11.h"
#include "Module/ModuleFactory.h"

static const DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

class __declspec(uuid("f94a37e0-bec9-44f7-b5ff-7c9f37c1a6b6"))
	RenderDepth11
	: public ff::ComBase
	, public ff::IRenderDepth
	, public ff::IRenderDepth11
{
public:
	DECLARE_HEADER(RenderDepth11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(ff::PointInt size, size_t multiSamples);

	// IGraphDeviceChild
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IRenderDepth
	virtual ff::PointInt GetSize() const override;
	virtual bool SetSize(ff::PointInt size) override;
	virtual void Clear(float depth, BYTE stencil) override;
	virtual void ClearDepth(float depth) override;
	virtual void ClearStencil(BYTE stencil) override;
	virtual void Discard() override;
	virtual ff::IRenderDepth11* AsRenderDepth11() override;

	// IRenderDepth11
	virtual ID3D11Texture2D* GetTexture() override;
	virtual ID3D11DepthStencilView* GetView() override;

private:
	bool CreateTextureAndView(ff::PointInt size, size_t multiSamples);

	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<ID3D11Texture2D> _texture;
	ff::ComPtr<ID3D11DepthStencilView> _view;
};

BEGIN_INTERFACES(RenderDepth11)
	HAS_INTERFACE(ff::IRenderDepth)
END_INTERFACES()

bool CreateRenderDepth11(ff::IGraphDevice* pDevice, ff::PointInt size, size_t samples, ff::IRenderDepth** ppDepth)
{
	assertRetVal(ppDepth && samples && ff::NearestPowerOfTwo(samples) == samples, false);
	*ppDepth = nullptr;

	while (samples > 1)
	{
		UINT levels;
		if (FAILED(pDevice->AsGraphDevice11()->Get3d()->CheckMultisampleQualityLevels(::DepthStencilFormat, (UINT)samples, &levels)) || !levels)
		{
			samples /= 2;
		}
	}

	ff::ComPtr<RenderDepth11> pDepth;
	assertHrRetVal(ff::ComAllocator<RenderDepth11>::CreateInstance(pDevice, &pDepth), false);
	assertRetVal(pDepth->Init(size, samples), false);

	*ppDepth = pDepth.Detach();
	return true;
}

RenderDepth11::RenderDepth11()
{
}

RenderDepth11::~RenderDepth11()
{
	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT RenderDepth11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool RenderDepth11::Init(ff::PointInt size, size_t multiSamples)
{
	assertRetVal(_device, false);
	assertRetVal(CreateTextureAndView(size, multiSamples), false);

	return true;
}

ff::IGraphDevice* RenderDepth11::GetDevice() const
{
	return _device;
}

bool RenderDepth11::Reset()
{
	if (_texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);

		assertRetVal(CreateTextureAndView(ff::PointInt(desc.Width, desc.Height), desc.SampleDesc.Count), false);
	}

	return true;
}

ff::PointInt RenderDepth11::GetSize() const
{
	if (_texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);
		return ff::PointInt(desc.Width, desc.Height);
	}

	return ff::PointInt::Zeros();
}

bool RenderDepth11::SetSize(ff::PointInt size)
{
	if (size != GetSize())
	{
		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);
		assertRetVal(CreateTextureAndView(size, desc.SampleDesc.Count), false);
	}

	Clear(0, 0);

	return true;
}

void RenderDepth11::Clear(float depth, BYTE stencil)
{
	assertRet(_view);

	Discard();

	_device->AsGraphDevice11()->GetStateContext().ClearDepthStencil(_view, true, true, depth, stencil);
}

void RenderDepth11::ClearDepth(float depth)
{
	assertRet(_view);

	_device->AsGraphDevice11()->GetStateContext().ClearDepthStencil(_view, true, false, depth, 0);
}

void RenderDepth11::ClearStencil(BYTE stencil)
{
	assertRet(_view);

	_device->AsGraphDevice11()->GetStateContext().ClearDepthStencil(_view, false, true, 0, stencil);
}

void RenderDepth11::Discard()
{
	assertRet(_view);
	_device->AsGraphDevice11()->GetContext()->DiscardView1(_view, nullptr, 0);
}

ff::IRenderDepth11* RenderDepth11::AsRenderDepth11()
{
	return this;
}

ID3D11Texture2D* RenderDepth11::GetTexture()
{
	return _texture;
}

ID3D11DepthStencilView* RenderDepth11::GetView()
{
	return _view;
}

bool RenderDepth11::CreateTextureAndView(ff::PointInt size, size_t multiSamples)
{
	_view = nullptr;
	_texture = nullptr;

	D3D11_TEXTURE2D_DESC descTex;
	ff::ZeroObject(descTex);

	descTex.ArraySize = 1;
	descTex.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descTex.Format = ::DepthStencilFormat;
	descTex.Width = size.x;
	descTex.Height = size.y;
	descTex.MipLevels = 1;
	descTex.SampleDesc.Count = multiSamples ? (UINT)multiSamples : 1;
	descTex.Usage = D3D11_USAGE_DEFAULT;

	assertHrRetVal(_device->AsGraphDevice11()->Get3d()->CreateTexture2D(&descTex, nullptr, &_texture), false);

	D3D11_DEPTH_STENCIL_VIEW_DESC desc;
	ff::ZeroObject(desc);

	desc.Format = descTex.Format;
	desc.ViewDimension = (descTex.SampleDesc.Count > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;

	assertHrRetVal(_device->AsGraphDevice11()->Get3d()->CreateDepthStencilView(_texture, &desc, &_view), false);

	return true;
}
