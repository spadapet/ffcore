#pragma once

namespace ff
{
	struct LineGeometryInput
	{
		DirectX::XMFLOAT2 pos[4]; // adjacency at 0 and 3
		DirectX::XMFLOAT4 color[2];
		float thickness[2];
		float depth;
		UINT matrixIndex;

		UTIL_API static const std::array<D3D11_INPUT_ELEMENT_DESC, 10>& GetLayout11();
	};

	struct LineScreenGeometryInput : public LineGeometryInput
	{
	};

	struct CircleGeometryInput
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 insideColor;
		DirectX::XMFLOAT4 outsideColor;
		float radius;
		float thickness;
		UINT matrixIndex;

		UTIL_API static const std::array<D3D11_INPUT_ELEMENT_DESC, 6>& GetLayout11();
	};

	struct CircleScreenGeometryInput : public CircleGeometryInput
	{
	};

	struct TriangleGeometryInput
	{
		DirectX::XMFLOAT2 pos[3];
		DirectX::XMFLOAT4 color[3];
		float depth;
		UINT matrixIndex;

		UTIL_API static const std::array<D3D11_INPUT_ELEMENT_DESC, 8>& GetLayout11();
	};

	struct SpriteGeometryInput
	{
		DirectX::XMFLOAT4 rect;
		DirectX::XMFLOAT4 uvrect;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 scale;
		DirectX::XMFLOAT3 pos;
		float rotate;
		UINT textureIndex;
		UINT matrixIndex;

		UTIL_API static const std::array<D3D11_INPUT_ELEMENT_DESC, 8>& GetLayout11();
	};

	struct MultiSpriteGeometryInput
	{
		DirectX::XMFLOAT4 rect;
		DirectX::XMFLOAT4 uvrect[4];
		DirectX::XMFLOAT4 color[4];
		DirectX::XMFLOAT2 scale;
		DirectX::XMFLOAT3 pos;
		float rotate;
		UINT textureIndex[4];
		UINT matrixIndex;

		UTIL_API static const std::array<D3D11_INPUT_ELEMENT_DESC, 14>& GetLayout11();
	};

	struct ColorPixelInput
	{
		DirectX::XMFLOAT4 pos;
		DirectX::XMFLOAT4 color;

		UTIL_API static const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetLayout11();
		UTIL_API static const std::array<D3D11_SO_DECLARATION_ENTRY, 2>& GetStreamOutputLayout11();
	};

	struct SpritePixelInput
	{
		DirectX::XMFLOAT4 pos;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 uv;
		UINT textureIndex;

		UTIL_API static const std::array<D3D11_INPUT_ELEMENT_DESC, 4>& GetLayout11();
		UTIL_API static const std::array<D3D11_SO_DECLARATION_ENTRY, 4>& GetStreamOutputLayout11();
	};

	struct MultiSpritePixelInput
	{
		DirectX::XMFLOAT4 pos;
		DirectX::XMFLOAT4 color[4];
		DirectX::XMFLOAT2 uv[4];
		UINT textureIndex[4];

		UTIL_API static const std::array<D3D11_INPUT_ELEMENT_DESC, 10>& GetLayout11();
		UTIL_API static const std::array<D3D11_SO_DECLARATION_ENTRY, 10>& GetStreamOutputLayout11();
	};
}

MAKE_POD(ff::LineGeometryInput);
MAKE_POD(ff::LineScreenGeometryInput);
MAKE_POD(ff::TriangleGeometryInput);
MAKE_POD(ff::SpriteGeometryInput);
MAKE_POD(ff::MultiSpriteGeometryInput);
MAKE_POD(ff::ColorPixelInput);
MAKE_POD(ff::SpritePixelInput);
MAKE_POD(ff::MultiSpritePixelInput);
