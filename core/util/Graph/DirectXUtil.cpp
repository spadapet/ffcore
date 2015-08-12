#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphFactory.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Texture/Texture.h"
#include "String/StringUtil.h"

__declspec(align(16)) static const float s_identityMatrix[] =
{
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
};

__declspec(align(16)) static const float s_identityMatrix3x3[] =
{
	1, 0, 0,
	0, 1, 0,
	0, 0, 1,
};

__declspec(align(16)) static const float s_colorWhite[] = { 1, 1, 1, 1 };
__declspec(align(16)) static const float s_colorBlack[] = { 0, 0, 0, 1 };
__declspec(align(16)) static const float s_colorTrans[] = { 0, 0, 0, 0 };
__declspec(align(16)) static const float s_colorRed[] = { 1, 0, 0, 1 };
__declspec(align(16)) static const float s_colorGreen[] = { 0, 1, 0, 1 };
__declspec(align(16)) static const float s_colorBlue[] = { 0, 0, 1, 1 };
__declspec(align(16)) static const float s_colorYellow[] = { 1, 1, 0, 1 };
__declspec(align(16)) static const float s_colorCyan[] = { 0, 1, 1, 1 };
__declspec(align(16)) static const float s_colorMangenta[] = { 1, 0, 1, 1 };

const DirectX::XMFLOAT4& ff::GetColorWhite()
{
	return *(DirectX::XMFLOAT4*)s_colorWhite;
}

const DirectX::XMFLOAT4& ff::GetColorBlack()
{
	return *(DirectX::XMFLOAT4*)s_colorBlack;
}

const DirectX::XMFLOAT4& ff::GetColorNone()
{
	return *(DirectX::XMFLOAT4*)s_colorTrans;
}

const DirectX::XMFLOAT4& ff::GetColorRed()
{
	return *(DirectX::XMFLOAT4*)s_colorRed;
}

const DirectX::XMFLOAT4& ff::GetColorGreen()
{
	return *(DirectX::XMFLOAT4*)s_colorGreen;
}

const DirectX::XMFLOAT4& ff::GetColorBlue()
{
	return *(DirectX::XMFLOAT4*)s_colorBlue;
}

const DirectX::XMFLOAT4& ff::GetColorYellow()
{
	return *(DirectX::XMFLOAT4*)s_colorYellow;
}

const DirectX::XMFLOAT4& ff::GetColorCyan()
{
	return *(DirectX::XMFLOAT4*)s_colorCyan;
}

const DirectX::XMFLOAT4& ff::GetColorMagenta()
{
	return *(DirectX::XMFLOAT4*)s_colorMangenta;
}

const DirectX::XMFLOAT4X4& ff::GetIdentityMatrix()
{
	return *(DirectX::XMFLOAT4X4*)s_identityMatrix;
}

const DirectX::XMFLOAT3X3& ff::GetIdentityMatrix3x3()
{
	return *(DirectX::XMFLOAT3X3*)s_identityMatrix3x3;
}

DXGI_FORMAT ff::ConvertTextureFormat(ff::TextureFormat format)
{
	switch (format)
	{
	default: assertRetVal(false, DXGI_FORMAT_UNKNOWN);
	case ff::TextureFormat::Unknown: return DXGI_FORMAT_UNKNOWN;
	case ff::TextureFormat::A8: return DXGI_FORMAT_A8_UNORM;
	case ff::TextureFormat::R8: return DXGI_FORMAT_R8_UNORM;
	case ff::TextureFormat::R8_UINT: return DXGI_FORMAT_R8_UINT;
	case ff::TextureFormat::R8G8: return DXGI_FORMAT_R8G8_UNORM;
	case ff::TextureFormat::RGBA32: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case ff::TextureFormat::BGRA32: return DXGI_FORMAT_B8G8R8A8_UNORM;
	case ff::TextureFormat::BC1: return DXGI_FORMAT_BC1_UNORM;
	case ff::TextureFormat::BC2: return DXGI_FORMAT_BC2_UNORM;
	case ff::TextureFormat::BC3: return DXGI_FORMAT_BC3_UNORM;
	case ff::TextureFormat::RGBA32_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case ff::TextureFormat::BGRA32_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	case ff::TextureFormat::BC1_SRGB: return DXGI_FORMAT_BC1_UNORM_SRGB;
	case ff::TextureFormat::BC2_SRGB: return DXGI_FORMAT_BC2_UNORM_SRGB;
	case ff::TextureFormat::BC3_SRGB: return DXGI_FORMAT_BC3_UNORM_SRGB;
	}
}

ff::TextureFormat ff::ConvertTextureFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	default: assertRetVal(false, ff::TextureFormat::Unknown);
	case DXGI_FORMAT_UNKNOWN: return ff::TextureFormat::Unknown;
	case DXGI_FORMAT_A8_UNORM: return ff::TextureFormat::A8;
	case DXGI_FORMAT_R8_UNORM: return ff::TextureFormat::R8;
	case DXGI_FORMAT_R8_UINT: return ff::TextureFormat::R8_UINT;
	case DXGI_FORMAT_R8G8_UNORM: return ff::TextureFormat::R8G8;
	case DXGI_FORMAT_R8G8B8A8_UNORM: return ff::TextureFormat::RGBA32;
	case DXGI_FORMAT_B8G8R8A8_UNORM: return ff::TextureFormat::BGRA32;
	case DXGI_FORMAT_BC1_UNORM: return ff::TextureFormat::BC1;
	case DXGI_FORMAT_BC2_UNORM: return ff::TextureFormat::BC2;
	case DXGI_FORMAT_BC3_UNORM: return ff::TextureFormat::BC3;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return ff::TextureFormat::RGBA32_SRGB;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return ff::TextureFormat::BGRA32_SRGB;
	case DXGI_FORMAT_BC1_UNORM_SRGB: return ff::TextureFormat::BC1_SRGB;
	case DXGI_FORMAT_BC2_UNORM_SRGB: return ff::TextureFormat::BC2_SRGB;
	case DXGI_FORMAT_BC3_UNORM_SRGB: return ff::TextureFormat::BC3_SRGB;
	}
}

DXGI_FORMAT ff::ParseDxgiTextureFormat(ff::StringRef szFormat)
{
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	if (szFormat == L"rgba32")
	{
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else if (szFormat == L"bgra32")
	{
		format = DXGI_FORMAT_B8G8R8A8_UNORM;
	}
	else if (szFormat == L"bc1")
	{
		format = DXGI_FORMAT_BC1_UNORM;
	}
	else if (szFormat == L"bc2")
	{
		format = DXGI_FORMAT_BC2_UNORM;
	}
	else if (szFormat == L"bc3")
	{
		format = DXGI_FORMAT_BC3_UNORM;
	}

	return format;
}

ff::TextureFormat ff::ParseTextureFormat(StringRef szFormat)
{
	return ff::ConvertTextureFormat(ParseDxgiTextureFormat(szFormat));
}

bool ff::IsSoftwareAdapter(IDXGIAdapterX* adapter)
{
	assertRetVal(adapter, true);

	DXGI_ADAPTER_DESC3 desc;
	assertHrRetVal(adapter->GetDesc3(&desc), false);
	return (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == DXGI_ADAPTER_FLAG3_SOFTWARE;
}

static ff::hash_t GetAdaptersHash(IDXGIFactoryX* factory)
{
	ff::Vector<LUID, 32> luids;

	ff::ComPtr<IDXGIAdapter> adapter;
	for (UINT i = 0; SUCCEEDED(factory->EnumAdapters(i, &adapter)); i++, adapter = nullptr)
	{
		DXGI_ADAPTER_DESC desc;
		if (SUCCEEDED(adapter->GetDesc(&desc)))
		{
			luids.Push(desc.AdapterLuid);
		}
	}

	return luids.Size() ? ff::HashBytes(luids.ConstData(), luids.ByteSize()) : 0;
}

bool ff::IsFactoryCurrent(IDXGIFactoryX* factory)
{
	assertRetVal(factory, false);

	if (factory->IsCurrent())
	{
		return true;
	}

	ff::IGraphicFactoryDxgi* dxgi = ff::ProcessGlobals::Get()->GetGraphicFactory()->AsGraphicFactoryDxgi();
	assertRetVal(dxgi && dxgi->GetDxgiFactory(), false);

	return ::GetAdaptersHash(factory) == ::GetAdaptersHash(dxgi->GetDxgiFactory());
}

ff::Vector<ff::ComPtr<IDXGIOutputX>> ff::GetAdapterOutputs(IDXGIFactoryX* dxgi, IDXGIAdapterX* card)
{
	ff::ComPtr<IDXGIOutput> output;
	ff::Vector<ff::ComPtr<IDXGIOutputX>> outputs;
	ff::ComPtr<IDXGIAdapter1> defaultAdapter;
	ff::ComPtr<IDXGIAdapterX> defaultAdapterX;

	if (ff::IsSoftwareAdapter(card) && dxgi && SUCCEEDED(dxgi->EnumAdapters1(0, &defaultAdapter)))
	{
		if (defaultAdapterX.QueryFrom(defaultAdapter))
		{
			card = defaultAdapterX;
		}
	}

	for (UINT i = 0; SUCCEEDED(card->EnumOutputs(i++, &output)); output = nullptr)
	{
		ff::ComPtr<IDXGIOutputX> outputX;
		if (outputX.QueryFrom(output))
		{
			outputs.Push(outputX);
		}
	}

	return outputs;
}

#if !METRO_APP

ff::PointInt ff::GetMonitorResolution(HWND hwnd, IDXGIFactoryX* dxgi, IDXGIAdapterX* pCard, IDXGIOutputX** ppOutput)
{
	assertRetVal(hwnd, ff::PointInt(0, 0));

	ff::Vector<ff::ComPtr<IDXGIOutputX>> outputs = ff::GetAdapterOutputs(dxgi, pCard);
	if (outputs.IsEmpty())
	{
		// just use the desktop monitors
		outputs = ff::GetAdapterOutputs(dxgi, nullptr);
		assertRetVal(outputs.Size(), ff::PointInt::Zeros());
	}

	HMONITOR hFoundMonitor = nullptr;
	HMONITOR hTargetMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
	ff::ComPtr<IDXGIOutputX> pOutput;
	ff::PointInt size(0, 0);

	for (size_t i = 0; i < outputs.Size(); i++)
	{
		DXGI_OUTPUT_DESC desc;
		ff::ZeroObject(desc);

		if (SUCCEEDED(outputs[i]->GetDesc(&desc)) && (hFoundMonitor == nullptr || desc.Monitor == hTargetMonitor))
		{
			MONITORINFO mi;
			ff::ZeroObject(mi);
			mi.cbSize = sizeof(mi);

			if (GetMonitorInfo(desc.Monitor, &mi))
			{
				pOutput = outputs[i];
				size.SetPoint(mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
			}

			hFoundMonitor = desc.Monitor;
		}
	}

	assertRetVal(pOutput, ff::PointInt(0, 0));

	if (ppOutput)
	{
		*ppOutput = pOutput.Detach();
	}

	return size;
}

bool ff::GetSwapChainSize(HWND hwnd, ff::PointInt& pixelSize, double& dpiScale, DXGI_MODE_ROTATION& nativeOrientation, DXGI_MODE_ROTATION& currentOrientation)
{
	RECT clientRect;
	assertRetVal(hwnd && ::GetClientRect(hwnd, &clientRect), false);

	dpiScale = ::GetDpiForWindow(hwnd) / 96.0;
	pixelSize = ff::PointInt(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
	nativeOrientation = DXGI_MODE_ROTATION_IDENTITY;
	currentOrientation = DXGI_MODE_ROTATION_IDENTITY;

	HMONITOR monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFOEX mi;
	ff::ZeroObject(mi);
	mi.cbSize = sizeof(mi);
	if (::GetMonitorInfo(monitor, &mi))
	{
		DEVMODE dm;
		ff::ZeroObject(dm);
		dm.dmSize = sizeof(dm);
		if (::EnumDisplaySettings(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm))
		{
			currentOrientation = (DXGI_MODE_ROTATION)(dm.dmDisplayOrientation + 1);
		}
	}

	return true;
}

#endif

D3D_SRV_DIMENSION GetDefaultDimension(const D3D11_TEXTURE2D_DESC& desc)
{
	if (desc.ArraySize > 1)
	{
		return desc.SampleDesc.Count > 1
			? D3D_SRV_DIMENSION_TEXTURE2DMSARRAY
			: D3D_SRV_DIMENSION_TEXTURE2DARRAY;
	}
	else
	{
		return desc.SampleDesc.Count > 1
			? D3D_SRV_DIMENSION_TEXTURE2DMS
			: D3D_SRV_DIMENSION_TEXTURE2D;
	}
}

ff::ComPtr<ID3D11ShaderResourceView> CreateDefaultTextureView(ID3D11DeviceX* device, ID3D11Texture2D* texture)
{
	assertRetVal(texture, nullptr);

	D3D11_TEXTURE2D_DESC textureDesc;
	texture->GetDesc(&textureDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	ff::ZeroObject(viewDesc);

	viewDesc.Format = textureDesc.Format;
	viewDesc.ViewDimension = ::GetDefaultDimension(textureDesc);

	switch (viewDesc.ViewDimension)
	{
	case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
		viewDesc.Texture2DMSArray.ArraySize = textureDesc.ArraySize;
		break;

	case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
		viewDesc.Texture2DArray.ArraySize = textureDesc.ArraySize;
		viewDesc.Texture2DArray.MipLevels = textureDesc.MipLevels;
		break;

	case D3D_SRV_DIMENSION_TEXTURE2DMS:
		// nothing to define
		break;

	case D3D_SRV_DIMENSION_TEXTURE2D:
		viewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
		break;
	}

	ff::ComPtr<ID3D11ShaderResourceView> view;
	assertHrRetVal(device->CreateShaderResourceView(texture, &viewDesc, &view), nullptr);
	return view;
}

ff::SpriteType GetSpriteTypeForImage(const DirectX::ScratchImage& scratch, const ff::RectSize* rect = nullptr)
{
	DirectX::ScratchImage alphaScratch;
	const DirectX::Image* alphaImage = nullptr;
	size_t alphaGap = 1;

	if (scratch.GetMetadata().format == DXGI_FORMAT_A8_UNORM)
	{
		alphaImage = scratch.GetImages();
	}
	else if (scratch.GetMetadata().format == DXGI_FORMAT_R8G8B8A8_UNORM ||
		scratch.GetMetadata().format == DXGI_FORMAT_B8G8R8A8_UNORM)
	{
		alphaImage = scratch.GetImages();
		alphaGap = 4;
	}
	else if (DirectX::IsCompressed(scratch.GetMetadata().format))
	{
		assertHrRetVal(DirectX::Decompress(
			scratch.GetImages(),
			scratch.GetImageCount(),
			scratch.GetMetadata(),
			DXGI_FORMAT_A8_UNORM,
			alphaScratch), ff::SpriteType::Unknown);

		alphaImage = alphaScratch.GetImages();
	}
	else
	{
		assertHrRetVal(DirectX::Convert(
			scratch.GetImages(), 1,
			scratch.GetMetadata(),
			DXGI_FORMAT_A8_UNORM,
			DirectX::TEX_FILTER_DEFAULT,
			0, alphaScratch), ff::SpriteType::Unknown);

		alphaImage = alphaScratch.GetImages();
	}

	ff::SpriteType newType = ff::SpriteType::Opaque;
	ff::RectSize size(0, 0, alphaImage->width, alphaImage->height);
	rect = rect ? rect : &size;

	for (size_t y = rect->top; y < rect->bottom && newType == ff::SpriteType::Opaque; y++)
	{
		const uint8_t* alpha = alphaImage->pixels + y * alphaImage->rowPitch + rect->left + alphaGap - 1;
		for (size_t x = rect->left; x < rect->right; x++, alpha += alphaGap)
		{
			if (*alpha && *alpha != 0xFF)
			{
				newType = ff::SpriteType::Transparent;
				break;
			}
		}
	}

	return newType;
}

DirectX::ScratchImage LoadTextureData(ff::IGraphDevice* device, ff::StringRef path, DXGI_FORMAT format, size_t mips, ff::SpriteType& spriteType)
{
	assertRetVal(device, DirectX::ScratchImage());

	DirectX::ScratchImage scratchFinal;
	assertHrRetVal(DirectX::LoadFromWICFile(
		path.c_str(),
		DirectX::WIC_FLAGS_IGNORE_SRGB,
		nullptr,
		scratchFinal), DirectX::ScratchImage());

	spriteType = ::GetSpriteTypeForImage(scratchFinal);

	if (DirectX::IsCompressed(format))
	{
		// Compressed images have size restrictions. Upon failure, just use RGB
		size_t width = scratchFinal.GetMetadata().width;
		size_t height = scratchFinal.GetMetadata().height;

		if (width % 4 || height % 4)
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (mips != 1 && (ff::NearestPowerOfTwo(width) != width || ff::NearestPowerOfTwo(height) != height))
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	if (mips != 1)
	{
		DirectX::ScratchImage scratchMips;
		assertHrRetVal(DirectX::GenerateMipMaps(
			scratchFinal.GetImages(),
			scratchFinal.GetImageCount(),
			scratchFinal.GetMetadata(),
			DirectX::TEX_FILTER_DEFAULT,
			mips,
			scratchMips), DirectX::ScratchImage());

		scratchFinal = std::move(scratchMips);
	}

	if (DirectX::IsCompressed(format))
	{
		DirectX::ScratchImage scratchNew;
		assertHrRetVal(DirectX::Compress(
			scratchFinal.GetImages(),
			scratchFinal.GetImageCount(),
			scratchFinal.GetMetadata(),
			format,
			DirectX::TEX_COMPRESS_DEFAULT,
			0, // alpharef
			scratchNew), DirectX::ScratchImage());

		scratchFinal = std::move(scratchNew);
	}
	else if (format != scratchFinal.GetMetadata().format)
	{
		DirectX::ScratchImage scratchNew;
		assertHrRetVal(DirectX::Convert(
			scratchFinal.GetImages(),
			scratchFinal.GetImageCount(),
			scratchFinal.GetMetadata(),
			format,
			DirectX::TEX_FILTER_DEFAULT,
			0, // threshold
			scratchNew), DirectX::ScratchImage());

		scratchFinal = std::move(scratchNew);
	}

	return scratchFinal;
}

DirectX::ScratchImage ConvertTextureData(const DirectX::ScratchImage& data, DXGI_FORMAT format, size_t mips)
{
	assertRetVal(data.GetImageCount(), DirectX::ScratchImage());

	DirectX::ScratchImage scratchFinal;
	assertHrRetVal(scratchFinal.InitializeFromImage(*data.GetImages()), DirectX::ScratchImage());

	if (DirectX::IsCompressed(format))
	{
		// Compressed images have size restrictions. Upon failure, just use RGB
		size_t width = scratchFinal.GetMetadata().width;
		size_t height = scratchFinal.GetMetadata().height;

		if (width % 4 || height % 4)
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (mips != 1 && (ff::NearestPowerOfTwo(width) != width || ff::NearestPowerOfTwo(height) != height))
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	if (scratchFinal.GetMetadata().format != format || scratchFinal.GetMetadata().mipLevels != mips)
	{
		if (DirectX::IsCompressed(scratchFinal.GetMetadata().format))
		{
			DirectX::ScratchImage scratchRgb;
			assertHrRetVal(DirectX::Decompress(
				scratchFinal.GetImages(),
				scratchFinal.GetImageCount(),
				scratchFinal.GetMetadata(),
				DXGI_FORMAT_R8G8B8A8_UNORM,
				scratchRgb), DirectX::ScratchImage());

			scratchFinal = std::move(scratchRgb);
		}
		else if (scratchFinal.GetMetadata().format != DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			DirectX::ScratchImage scratchRgb;
			assertHrRetVal(DirectX::Convert(
				scratchFinal.GetImages(),
				scratchFinal.GetImageCount(),
				scratchFinal.GetMetadata(),
				DXGI_FORMAT_R8G8B8A8_UNORM,
				DirectX::TEX_FILTER_DEFAULT,
				0, // threshold
				scratchRgb), DirectX::ScratchImage());

			scratchFinal = std::move(scratchRgb);
		}

		if (mips != 1)
		{
			DirectX::ScratchImage scratchMips;
			assertHrRetVal(DirectX::GenerateMipMaps(
				scratchFinal.GetImages(),
				scratchFinal.GetImageCount(),
				scratchFinal.GetMetadata(),
				DirectX::TEX_FILTER_DEFAULT,
				mips,
				scratchMips), DirectX::ScratchImage());

			scratchFinal = std::move(scratchMips);
		}

		if (DirectX::IsCompressed(format))
		{
			DirectX::ScratchImage scratchNew;
			assertHrRetVal(DirectX::Compress(
				scratchFinal.GetImages(),
				scratchFinal.GetImageCount(),
				scratchFinal.GetMetadata(),
				format,
				DirectX::TEX_COMPRESS_DEFAULT,
				0, // alpharef
				scratchNew), DirectX::ScratchImage());

			scratchFinal = std::move(scratchNew);
		}
		else if (format != scratchFinal.GetMetadata().format)
		{
			DirectX::ScratchImage scratchNew;
			assertHrRetVal(DirectX::Convert(
				scratchFinal.GetImages(),
				scratchFinal.GetImageCount(),
				scratchFinal.GetMetadata(),
				format,
				DirectX::TEX_FILTER_DEFAULT,
				0, // threshold
				scratchNew), DirectX::ScratchImage());

			scratchFinal = std::move(scratchNew);
		}
	}

	return scratchFinal;
}

// This method determines the rotation between the display device's native orientation and the
// current display orientation.
DXGI_MODE_ROTATION ff::ComputeDisplayRotation(DXGI_MODE_ROTATION nativeOrientation, DXGI_MODE_ROTATION currentOrientation)
{
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

	// Note: NativeOrientation can only be Landscape or Portrait even though
	// the DisplayOrientations enum has other values.
	switch (nativeOrientation)
	{
	case DXGI_MODE_ROTATION_IDENTITY:
	case DXGI_MODE_ROTATION_ROTATE180:
		switch (currentOrientation)
		{
		case DXGI_MODE_ROTATION_IDENTITY:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DXGI_MODE_ROTATION_ROTATE90:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DXGI_MODE_ROTATION_ROTATE180:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;

		case DXGI_MODE_ROTATION_ROTATE270:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		}
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
	case DXGI_MODE_ROTATION_ROTATE270:
		switch (currentOrientation)
		{
		case DXGI_MODE_ROTATION_IDENTITY:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;

		case DXGI_MODE_ROTATION_ROTATE90:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DXGI_MODE_ROTATION_ROTATE180:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DXGI_MODE_ROTATION_ROTATE270:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		}
		break;
	}

	return rotation;
}

#if METRO_APP

DXGI_MODE_ROTATION ff::GetDxgiRotation(Windows::Graphics::Display::DisplayOrientations orientation)
{
	switch (orientation)
	{
	default:
	case Windows::Graphics::Display::DisplayOrientations::None:
		return DXGI_MODE_ROTATION_UNSPECIFIED;

	case Windows::Graphics::Display::DisplayOrientations::Landscape:
		return DXGI_MODE_ROTATION_IDENTITY;

	case Windows::Graphics::Display::DisplayOrientations::Portrait:
		return DXGI_MODE_ROTATION_ROTATE90;

	case Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped:
		return DXGI_MODE_ROTATION_ROTATE180;

	case Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
		return DXGI_MODE_ROTATION_ROTATE270;
	}
}

#endif
