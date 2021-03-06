#pragma once

namespace ff
{
	class IGraphDevice;
	class IPaletteData;
	enum class SpriteType;
	enum class TextureFormat;

	struct GraphCounters
	{
	public:
		void Reset();

		size_t _draw;
		size_t _clear;
		size_t _depthClear;
		size_t _map; // CPU to GPU (indirect)
		size_t _update; // CPU to GPU (direct)
		size_t _copy; // GPU copy
	};

	UTIL_API const DirectX::XMFLOAT4& GetColorWhite();
	UTIL_API const DirectX::XMFLOAT4& GetColorBlack();
	UTIL_API const DirectX::XMFLOAT4& GetColorNone();
	UTIL_API const DirectX::XMFLOAT4& GetColorRed();
	UTIL_API const DirectX::XMFLOAT4& GetColorGreen();
	UTIL_API const DirectX::XMFLOAT4& GetColorBlue();
	UTIL_API const DirectX::XMFLOAT4& GetColorYellow();
	UTIL_API const DirectX::XMFLOAT4& GetColorCyan();
	UTIL_API const DirectX::XMFLOAT4& GetColorMagenta();
	UTIL_API const DirectX::XMFLOAT4X4& GetIdentityMatrix();
	UTIL_API const DirectX::XMFLOAT3X3& GetIdentityMatrix3x3();

	UTIL_API void PaletteIndexToColor(int index, DirectX::XMFLOAT4& color);
	UTIL_API void PaletteIndexToColor(const int* index, DirectX::XMFLOAT4* color, size_t count);

	bool IsCompressedFormat(TextureFormat format);
	bool IsColorFormat(TextureFormat format);
	bool IsPaletteFormat(TextureFormat format);
	bool FormatSupportsPreMultipliedAlpha(TextureFormat format);
	DXGI_FORMAT ConvertTextureFormat(TextureFormat format);
	TextureFormat ConvertTextureFormat(DXGI_FORMAT format);
	UTIL_API DXGI_FORMAT ParseDxgiTextureFormat(StringRef szFormat);
	UTIL_API TextureFormat ParseTextureFormat(StringRef szFormat);
	ff::hash_t GetAdaptersHash(IDXGIFactoryX* factory);
	ff::hash_t GetAdapterOutputsHash(IDXGIFactoryX* dxgi, IDXGIAdapterX* card);
	ff::Vector<ff::ComPtr<IDXGIOutputX>> GetAdapterOutputs(IDXGIFactoryX* dxgi, IDXGIAdapterX* card);

	struct SwapChainSize
	{
		ff::PointInt _pixelSize;
		double _dpiScale;
		DXGI_MODE_ROTATION _nativeOrientation;
		DXGI_MODE_ROTATION _currentOrientation;
	};

	DXGI_MODE_ROTATION ComputeDisplayRotation(DXGI_MODE_ROTATION nativeOrientation, DXGI_MODE_ROTATION currentOrientation);
#if METRO_APP
	DXGI_MODE_ROTATION GetDxgiRotation(Windows::Graphics::Display::DisplayOrientations orientation);
#else
	ff::PointInt GetMonitorResolution(HWND hwnd, IDXGIFactoryX* dxgi, IDXGIAdapterX* pCard, IDXGIOutputX** ppOutput);
	bool GetSwapChainSize(HWND hwnd, SwapChainSize& size);
#endif

	template<typename T>
	ComPtr<T> GetParentDXGI(IUnknown* obj)
	{
		ComPtr<IDXGIObject> obj2;
		assertRetVal(obj2.QueryFrom(obj), nullptr);

		ComPtr<IDXGIObject> parentObj;
		assertHrRetVal(obj2->GetParent(__uuidof(IDXGIObject), (void**)&parentObj), nullptr);

		ComPtr<T> tObj;
		assertHrRetVal(tObj.QueryFrom(parentObj), false);

		return tObj;
	}

#ifdef UTIL_DLL
	ff::ComPtr<ID3D11ShaderResourceView> CreateDefaultTextureView(ID3D11DeviceX* device, ID3D11Texture2D* texture);
	DirectX::ScratchImage LoadTextureData(ff::StringRef path, DXGI_FORMAT format, size_t mips, DirectX::ScratchImage* paletteScratch);
	DirectX::ScratchImage ConvertTextureData(const DirectX::ScratchImage& data, DXGI_FORMAT format, size_t mips);
	ff::SpriteType GetSpriteTypeForImage(const DirectX::ScratchImage& scratch, const ff::RectSize* rect = nullptr);
#endif
}

inline bool operator==(const D3D11_BLEND_DESC& lhs, const D3D11_BLEND_DESC& rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

inline bool operator<(const D3D11_BLEND_DESC& lhs, const D3D11_BLEND_DESC& rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator==(const D3D11_DEPTH_STENCIL_DESC& lhs, const D3D11_DEPTH_STENCIL_DESC& rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

inline bool operator<(const D3D11_DEPTH_STENCIL_DESC& lhs, const D3D11_DEPTH_STENCIL_DESC& rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator==(const D3D11_RASTERIZER_DESC& lhs, const D3D11_RASTERIZER_DESC& rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

inline bool operator<(const D3D11_RASTERIZER_DESC& lhs, const D3D11_RASTERIZER_DESC& rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator==(const D3D11_SAMPLER_DESC& lhs, const D3D11_SAMPLER_DESC& rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

inline bool operator<(const D3D11_SAMPLER_DESC& lhs, const D3D11_SAMPLER_DESC& rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator==(const DirectX::XMVECTOR& lhs, const DirectX::XMVECTOR& rhs)
{
	return DirectX::XMVector4Equal(lhs, rhs);
}

inline bool operator!=(const DirectX::XMVECTOR& lhs, const DirectX::XMVECTOR& rhs)
{
	return DirectX::XMVector4NotEqual(lhs, rhs);
}

inline bool operator==(const DirectX::XMMATRIX& lhs, const DirectX::XMMATRIX& rhs)
{
	return DirectX::XMVector4Equal(lhs.r[0], rhs.r[0]) &&
		DirectX::XMVector4Equal(lhs.r[1], rhs.r[1]) &&
		DirectX::XMVector4Equal(lhs.r[2], rhs.r[2]) &&
		DirectX::XMVector4Equal(lhs.r[3], rhs.r[3]);
}

inline bool operator!=(const DirectX::XMMATRIX& lhs, const DirectX::XMMATRIX& rhs)
{
	return DirectX::XMVector4NotEqual(lhs.r[0], rhs.r[0]) ||
		DirectX::XMVector4NotEqual(lhs.r[1], rhs.r[1]) ||
		DirectX::XMVector4NotEqual(lhs.r[2], rhs.r[2]) ||
		DirectX::XMVector4NotEqual(lhs.r[3], rhs.r[3]);
}

inline bool operator==(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs)
{
	return ::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

inline bool operator!=(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs)
{
	return ::memcmp(&lhs, &rhs, sizeof(lhs)) != 0;
}

inline bool operator==(const DirectX::XMFLOAT4X4& lhs, const DirectX::XMFLOAT4X4& rhs)
{
	return ::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

inline bool operator!=(const DirectX::XMFLOAT4X4& lhs, const DirectX::XMFLOAT4X4& rhs)
{
	return ::memcmp(&lhs, &rhs, sizeof(lhs)) != 0;
}

inline bool operator<(const DirectX::XMFLOAT4X4& lhs, const DirectX::XMFLOAT4X4& rhs)
{
	return ::memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}
