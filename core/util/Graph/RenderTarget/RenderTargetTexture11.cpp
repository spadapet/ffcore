#include "pch.h"
#include "COM/ComAlloc.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/Texture/Texture.h"
#include "Module/ModuleFactory.h"

class __declspec(uuid("42a1ad55-ca79-4c93-99c2-b8d4b1961de2"))
	RenderTargetTexture11
	: public ff::ComBase
	, public ff::IRenderTarget
	, public ff::IRenderTarget11
{
public:
	DECLARE_HEADER(RenderTargetTexture11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipLevel);

	// IGraphDeviceChild
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IRenderTarget
	virtual ff::PointInt GetBufferSize() const override;
	virtual ff::PointInt GetRotatedSize() const override;
	virtual int GetRotatedDegrees() const override;
	virtual double GetDpiScale() const override;
	virtual void Clear(const DirectX::XMFLOAT4* pColor = nullptr) override;
	virtual ff::IRenderTarget11* AsRenderTarget11() override;

	// IRenderTarget11
	virtual ID3D11Texture2D* GetTexture() override;
	virtual ID3D11RenderTargetView* GetTarget() override;

private:
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<ff::ITexture> _texture;
	ff::ComPtr<ID3D11RenderTargetView> _target;
	size_t _arrayStart;
	size_t _arrayCount;
	size_t _mipLevel;
};

BEGIN_INTERFACES(RenderTargetTexture11)
	HAS_INTERFACE(ff::IRenderTarget)
END_INTERFACES()

bool CreateRenderTargetTexture11(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipLevel, ff::IRenderTarget** ppRender)
{
	assertRetVal(ppRender && texture, false);
	*ppRender = nullptr;

	ff::ComPtr<RenderTargetTexture11, ff::IRenderTarget> pRender;
	assertHrRetVal(ff::ComAllocator<RenderTargetTexture11>::CreateInstance(texture->GetDevice(), &pRender), false);
	assertRetVal(pRender->Init(texture, arrayStart, arrayCount, mipLevel), false);

	*ppRender = pRender.Detach();
	return true;
}

// helper function
static D3D11_RTV_DIMENSION GetViewDimension(const D3D11_TEXTURE2D_DESC& desc)
{
	if (desc.ArraySize > 1)
	{
		return desc.SampleDesc.Count > 1
			? D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY
			: D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	}
	else
	{
		return desc.SampleDesc.Count > 1
			? D3D11_RTV_DIMENSION_TEXTURE2DMS
			: D3D11_RTV_DIMENSION_TEXTURE2D;
	}
}

// helper function
bool CreateRenderTarget11(ff::IGraphDevice* device, ID3D11Texture2D* texture, size_t arrayStart, size_t arrayCount, size_t mipLevel, ID3D11RenderTargetView** ppView)
{
	assertRetVal(device && texture && ppView, false);
	*ppView = nullptr;

	D3D11_TEXTURE2D_DESC descTex;
	texture->GetDesc(&descTex);

	D3D11_RENDER_TARGET_VIEW_DESC desc;
	ff::ZeroObject(desc);

	arrayCount = arrayCount ? arrayCount : descTex.ArraySize - arrayStart;

	desc.Format = descTex.Format;
	desc.ViewDimension = GetViewDimension(descTex);

	switch (desc.ViewDimension)
	{
	case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
		desc.Texture2DMSArray.FirstArraySlice = (UINT)arrayStart;
		desc.Texture2DMSArray.ArraySize = (UINT)arrayCount;
		break;

	case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
		desc.Texture2DArray.FirstArraySlice = (UINT)arrayStart;
		desc.Texture2DArray.ArraySize = (UINT)arrayCount;
		desc.Texture2DArray.MipSlice = (UINT)mipLevel;
		break;

	case D3D_SRV_DIMENSION_TEXTURE2D:
		desc.Texture2D.MipSlice = (UINT)mipLevel;
		break;
	}

	assertHrRetVal(device->AsGraphDevice11()->Get3d()->CreateRenderTargetView(texture, &desc, ppView), false);

	return true;
}

RenderTargetTexture11::RenderTargetTexture11()
	: _arrayStart(0)
	, _mipLevel(0)
{
}

RenderTargetTexture11::~RenderTargetTexture11()
{
	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT RenderTargetTexture11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool RenderTargetTexture11::Init(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipLevel)
{
	assertRetVal(_device && texture, false);

	assertRetVal(_texture.QueryFrom(texture), false);
	_arrayStart = arrayStart;
	_arrayCount = arrayCount;
	_mipLevel = mipLevel;

	return true;
}

ff::IGraphDevice* RenderTargetTexture11::GetDevice() const
{
	return _device;
}

bool RenderTargetTexture11::Reset()
{
	_target = nullptr;
	return true;
}

ff::PointInt RenderTargetTexture11::GetBufferSize() const
{
	return _texture->GetSize();
}

ff::PointInt RenderTargetTexture11::GetRotatedSize() const
{
	return GetBufferSize();
}

void RenderTargetTexture11::Clear(const DirectX::XMFLOAT4* pColor)
{
	if (GetTarget())
	{
		static const DirectX::XMFLOAT4 defaultColor(0, 0, 0, 1);
		const DirectX::XMFLOAT4* pUseColor = pColor ? pColor : &defaultColor;

		_device->AsGraphDevice11()->GetContext()->ClearRenderTargetView(GetTarget(), &pUseColor->x);
	}
}

ff::IRenderTarget11* RenderTargetTexture11::AsRenderTarget11()
{
	return this;
}

ID3D11Texture2D* RenderTargetTexture11::GetTexture()
{
	return _texture ? _texture->AsTexture11()->GetTexture2d() : nullptr;
}

ID3D11RenderTargetView* RenderTargetTexture11::GetTarget()
{
	if (!_target && GetTexture())
	{
		assertRetVal(::CreateRenderTarget11(_device, GetTexture(), _arrayStart, _arrayCount, _mipLevel, &_target), false);
	}

	return _target;
}

int RenderTargetTexture11::GetRotatedDegrees() const
{
	return 0;
}

double RenderTargetTexture11::GetDpiScale() const
{
	return 1.0;
}
