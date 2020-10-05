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
}
