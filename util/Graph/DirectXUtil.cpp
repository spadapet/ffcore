#include "pch.h"
#include "Data/Data.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphFactory.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteType.h"
#include "Graph/Texture/PaletteData.h"
#include "Graph/Texture/PngImage.h"
#include "Graph/Texture/Texture.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

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

void ff::GraphCounters::Reset()
{
	ff::ZeroObject(*this);
}

void ff::PaletteIndexToColor(int index, DirectX::XMFLOAT4& color)
{
	color.x = index / 256.0f;
	color.y = 0;
	color.z = 0;
	color.w = 1.0f * (index != 0);
}

void ff::PaletteIndexToColor(const int* index, DirectX::XMFLOAT4* color, size_t count)
{
	for (size_t i = 0; i != count; i++)
	{
		ff::PaletteIndexToColor(index[i], color[i]);
	}
}

bool ff::IsCompressedFormat(TextureFormat format)
{
	switch (format)
	{
	default:
		return false;

	case ff::TextureFormat::BC1:
	case ff::TextureFormat::BC2:
	case ff::TextureFormat::BC3:
	case ff::TextureFormat::BC1_SRGB:
	case ff::TextureFormat::BC2_SRGB:
	case ff::TextureFormat::BC3_SRGB:
		return true;
	}
}

bool ff::IsColorFormat(TextureFormat format)
{
	switch (format)
	{
	default:
		return ff::IsCompressedFormat(format);

	case ff::TextureFormat::RGBA32:
	case ff::TextureFormat::BGRA32:
	case ff::TextureFormat::RGBA32_SRGB:
	case ff::TextureFormat::BGRA32_SRGB:
		return true;
	}
}

bool ff::IsPaletteFormat(TextureFormat format)
{
	return format == ff::TextureFormat::R8_UINT;
}

bool ff::FormatSupportsPreMultipliedAlpha(TextureFormat format)
{
	switch (format)
	{
	default:
		return false;

	case ff::TextureFormat::RGBA32:
	case ff::TextureFormat::BGRA32:
	case ff::TextureFormat::RGBA32_SRGB:
	case ff::TextureFormat::BGRA32_SRGB:
		return true;
	}
}

DXGI_FORMAT ff::ConvertTextureFormat(ff::TextureFormat format)
{
	switch (format)
	{
	default: assertRetVal(false, DXGI_FORMAT_UNKNOWN);
	case ff::TextureFormat::Unknown: return DXGI_FORMAT_UNKNOWN;
	case ff::TextureFormat::A8: return DXGI_FORMAT_A8_UNORM;
	case ff::TextureFormat::R1: return DXGI_FORMAT_R1_UNORM;
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
	case DXGI_FORMAT_R1_UNORM: return ff::TextureFormat::R1;
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
	else if (szFormat == L"pal" || szFormat == L"palette")
	{
		format = DXGI_FORMAT_R8_UINT;
	}
	else if (szFormat == L"gray")
	{
		format = DXGI_FORMAT_R8_UNORM;
	}
	else if (szFormat == L"bw")
	{
		format = DXGI_FORMAT_R1_UNORM;
	}
	else if (szFormat == L"alpha")
	{
		format = DXGI_FORMAT_A8_UNORM;
	}

	return format;
}

ff::TextureFormat ff::ParseTextureFormat(StringRef szFormat)
{
	return ff::ConvertTextureFormat(ParseDxgiTextureFormat(szFormat));
}

static bool IsSoftwareAdapter(IDXGIAdapterX* adapter)
{
	assertRetVal(adapter, true);

	DXGI_ADAPTER_DESC3 desc;
	assertHrRetVal(adapter->GetDesc3(&desc), false);
	return (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == DXGI_ADAPTER_FLAG3_SOFTWARE;
}

ff::hash_t ff::GetAdaptersHash(IDXGIFactoryX* factory)
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

static ff::ComPtr<IDXGIAdapterX> FixAdapter(IDXGIFactoryX* dxgi, ff::ComPtr<IDXGIAdapterX> card)
{
	if (dxgi && ::IsSoftwareAdapter(card))
	{
		ff::ComPtr<IDXGIAdapter1> defaultAdapter;
		if (SUCCEEDED(dxgi->EnumAdapters1(0, &defaultAdapter)))
		{
			ff::ComPtr<IDXGIAdapterX> defaultAdapterX;
			if (defaultAdapterX.QueryFrom(defaultAdapter))
			{
				card = defaultAdapterX;
			}
		}
	}

	DXGI_ADAPTER_DESC desc;
	if (SUCCEEDED(card->GetDesc(&desc)))
	{
		ff::ComPtr<IDXGIAdapterX> adapterX;
		if (SUCCEEDED(dxgi->EnumAdapterByLuid(desc.AdapterLuid, __uuidof(IDXGIAdapterX), (void**)&adapterX)))
		{
			card = adapterX;
		}
	}

	return card;
}

ff::hash_t ff::GetAdapterOutputsHash(IDXGIFactoryX* dxgi, IDXGIAdapterX* card)
{
	ff::Vector<HMONITOR, 32> monitors;
	ff::ComPtr<IDXGIAdapterX> cardX = ::FixAdapter(dxgi, card);

	ff::ComPtr<IDXGIOutput> output;
	for (UINT i = 0; SUCCEEDED(cardX->EnumOutputs(i++, &output)); output = nullptr)
	{
		DXGI_OUTPUT_DESC desc;
		if (SUCCEEDED(output->GetDesc(&desc)))
		{
			monitors.Push(desc.Monitor);
		}
	}

	return monitors.Size() ? ff::HashBytes(monitors.ConstData(), monitors.ByteSize()) : 0;
}

ff::Vector<ff::ComPtr<IDXGIOutputX>> ff::GetAdapterOutputs(IDXGIFactoryX* dxgi, IDXGIAdapterX* card)
{
	ff::ComPtr<IDXGIOutput> output;
	ff::Vector<ff::ComPtr<IDXGIOutputX>> outputs;
	ff::ComPtr<IDXGIAdapterX> cardX = ::FixAdapter(dxgi, card);

	for (UINT i = 0; SUCCEEDED(cardX->EnumOutputs(i++, &output)); output = nullptr)
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

bool ff::GetSwapChainSize(HWND hwnd, ff::SwapChainSize& size)
{
	RECT clientRect;
	assertRetVal(hwnd && ::GetClientRect(hwnd, &clientRect), false);

	size._dpiScale = ::GetDpiForWindow(hwnd) / 96.0;
	size._pixelSize = ff::PointInt(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
	size._nativeOrientation = DXGI_MODE_ROTATION_IDENTITY;
	size._currentOrientation = DXGI_MODE_ROTATION_IDENTITY;

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
			size._currentOrientation = (DXGI_MODE_ROTATION)(dm.dmDisplayOrientation + 1);
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

ff::ComPtr<ID3D11ShaderResourceView> ff::CreateDefaultTextureView(ID3D11DeviceX* device, ID3D11Texture2D* texture)
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

ff::SpriteType ff::GetSpriteTypeForImage(const DirectX::ScratchImage& scratch, const ff::RectSize* rect)
{
	DirectX::ScratchImage alphaScratch;
	const DirectX::Image* alphaImage = nullptr;
	size_t alphaGap = 1;
	DXGI_FORMAT format = scratch.GetMetadata().format;

	if (format == DXGI_FORMAT_R8_UINT)
	{
		return ff::SpriteType::OpaquePalette;
	}
	else if (!DirectX::HasAlpha(format))
	{
		return ff::SpriteType::Opaque;
	}
	else if (format == DXGI_FORMAT_A8_UNORM)
	{
		alphaImage = scratch.GetImages();
	}
	else if (format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM)
	{
		alphaImage = scratch.GetImages();
		alphaGap = 4;
	}
	else if (DirectX::IsCompressed(format))
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
	ff::RectSize size(0, 0, scratch.GetMetadata().width, scratch.GetMetadata().height);
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

static DirectX::ScratchImage LoadTexturePng(ff::StringRef path, DXGI_FORMAT format, size_t mips, DirectX::ScratchImage* paletteScratch)
{
	DirectX::ScratchImage scratchFinal;
	ff::ComPtr<ff::IData> pngData;
	assertRetVal(ff::ReadWholeFileMemMapped(path, &pngData), scratchFinal);
	ff::PngImageReader png(pngData->GetMem(), pngData->GetSize());
	{
		std::unique_ptr<DirectX::ScratchImage> scratchTemp = png.Read(format);
		if (scratchTemp)
		{
			scratchFinal = std::move(*scratchTemp);
		}
		else
		{
			assertSzRetVal(false, png.GetError().c_str(), scratchFinal);
		}
	}

	if (format == DXGI_FORMAT_UNKNOWN)
	{
		format = scratchFinal.GetMetadata().format;
	}

	if (format == DXGI_FORMAT_R8_UINT && paletteScratch)
	{
		std::unique_ptr<DirectX::ScratchImage> paletteScratch2 = png.GetPalette();
		if (paletteScratch2)
		{
			*paletteScratch = std::move(*paletteScratch2);
		}
	}

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

static DirectX::ScratchImage LoadTexturePal(ff::StringRef path, DXGI_FORMAT format, size_t mips)
{
	DirectX::ScratchImage scratchFinal;
	assertRetVal(format == DXGI_FORMAT_R8G8B8A8_UNORM && mips < 2, scratchFinal);

	ff::ComPtr<ff::IData> data;
	assertRetVal(ff::ReadWholeFileMemMapped(path, &data) && data->GetSize() && data->GetSize() % 3 == 0, scratchFinal);

	assertHrRetVal(scratchFinal.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, ff::PALETTE_SIZE, 1, 1, 1), scratchFinal);
	const DirectX::Image* image = scratchFinal.GetImages();
	BYTE* dest = image->pixels;
	std::memset(dest, 0, ff::PALETTE_ROW_BYTES);

	for (const BYTE* start = data->GetMem(), *end = start + data->GetSize(), *cur = start; cur < end; cur += 3, dest += 4)
	{
		dest[0] = cur[0]; // R
		dest[1] = cur[1]; // G
		dest[2] = cur[2]; // B
		dest[3] = 0xFF; // A
	}

	return scratchFinal;
}

DirectX::ScratchImage ff::LoadTextureData(ff::StringRef path, DXGI_FORMAT format, size_t mips, DirectX::ScratchImage* paletteScratch)
{
	ff::String pathExt = ff::GetPathExtension(path);
	ff::LowerCaseInPlace(pathExt);

	if (pathExt == L"pal")
	{
		return ::LoadTexturePal(path, format, mips);
	}
	else if (pathExt == L"png")
	{
		return ::LoadTexturePng(path, format, mips, paletteScratch);
	}
	else
	{
		assertSzRetVal(false, L"Invalid texture file extension", DirectX::ScratchImage());
	}
}

DirectX::ScratchImage ff::ConvertTextureData(const DirectX::ScratchImage& data, DXGI_FORMAT format, size_t mips)
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
