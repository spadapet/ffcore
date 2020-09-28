#pragma once

#include <d2d1_3.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <d3d11_4.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DWrite_3.h>
#include <XAudio2.h>
#include <Xinput.h>

#ifdef UTIL_DLL
#include <DirectXTex.h>
#include <png.h>
#endif

#define ID2D1DeviceX ID2D1Device5
#define ID2D1DeviceContextX ID2D1DeviceContext5
#define ID2D1FactoryX ID2D1Factory6

#define IDXGIAdapterX IDXGIAdapter4
#define IDXGIDeviceX IDXGIDevice4
#define IDXGIFactoryX IDXGIFactory5
#define IDXGIOutputX IDXGIOutput6
#define IDXGIResourceX IDXGIResource1
#define IDXGISurfaceX IDXGISurface2
#define IDXGISwapChainX IDXGISwapChain4

#define IDWriteFactoryX IDWriteFactory5
#define IDWriteFontCollectionX IDWriteFontCollection1
#define IDWriteFontFaceX IDWriteFontFace4
#define IDWriteFontSetBuilderX IDWriteFontSetBuilder1
#define IDWriteTextFormatX IDWriteTextFormat2
#define IDWriteTextLayoutX IDWriteTextLayout3

#define ID3D11DeviceX ID3D11Device5
#define ID3D11DeviceContextX ID3D11DeviceContext4
