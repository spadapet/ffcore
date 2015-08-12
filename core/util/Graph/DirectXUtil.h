#pragma once

namespace ff
{
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

	enum class TextureFormat;
	DXGI_FORMAT ConvertTextureFormat(TextureFormat format);
	TextureFormat ConvertTextureFormat(DXGI_FORMAT format);
	UTIL_API DXGI_FORMAT ParseDxgiTextureFormat(StringRef szFormat);
	UTIL_API TextureFormat ParseTextureFormat(StringRef szFormat);
	bool IsSoftwareAdapter(IDXGIAdapterX* adapter);
	bool IsFactoryCurrent(IDXGIFactoryX* factory);
	ff::Vector<ff::ComPtr<IDXGIOutputX>> GetAdapterOutputs(IDXGIFactoryX* dxgi, IDXGIAdapterX* card);

	DXGI_MODE_ROTATION ComputeDisplayRotation(DXGI_MODE_ROTATION nativeOrientation, DXGI_MODE_ROTATION currentOrientation);
#if METRO_APP
	DXGI_MODE_ROTATION GetDxgiRotation(Windows::Graphics::Display::DisplayOrientations orientation);
#else
	ff::PointInt GetMonitorResolution(HWND hwnd, IDXGIFactoryX* dxgi, IDXGIAdapterX* pCard, IDXGIOutputX** ppOutput);
	bool GetSwapChainSize(HWND hwnd, ff::PointInt& pixelSize, double& dpiScale, DXGI_MODE_ROTATION& nativeOrientation, DXGI_MODE_ROTATION& currentOrientation);
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

MAKE_POD(DirectX::XMFLOAT2);
MAKE_POD(DirectX::XMFLOAT2A);
MAKE_POD(DirectX::XMFLOAT3);
MAKE_POD(DirectX::XMFLOAT3A);
MAKE_POD(DirectX::XMFLOAT4);
MAKE_POD(DirectX::XMFLOAT4A);
MAKE_POD(DirectX::XMFLOAT3X3);
MAKE_POD(DirectX::XMFLOAT4X3);
MAKE_POD(DirectX::XMFLOAT4X3A);
MAKE_POD(DirectX::XMFLOAT4X4);
MAKE_POD(DirectX::XMFLOAT4X4A);
MAKE_POD(DirectX::XMINT2);
MAKE_POD(DirectX::XMINT3);
MAKE_POD(DirectX::XMINT4);
MAKE_POD(DirectX::XMUINT2);
MAKE_POD(DirectX::XMUINT3);
MAKE_POD(DirectX::XMUINT4);
MAKE_POD(DirectX::XMMATRIX);
MAKE_POD(DirectX::XMVECTORF32);
MAKE_POD(DirectX::XMVECTORI32);
MAKE_POD(DirectX::XMVECTORU32);
MAKE_POD(DirectX::XMVECTORU8);
