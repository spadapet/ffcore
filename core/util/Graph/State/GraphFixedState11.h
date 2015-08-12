#pragma once

namespace ff
{
	class GraphContext11;

	struct GraphFixedState11
	{
		UTIL_API GraphFixedState11();
		UTIL_API GraphFixedState11(GraphFixedState11&& rhs);
		UTIL_API GraphFixedState11(const GraphFixedState11& rhs);
		UTIL_API GraphFixedState11& operator=(const GraphFixedState11& rhs);

		UTIL_API void Apply(GraphContext11& context) const;

		ComPtr<ID3D11RasterizerState> _raster;
		ComPtr<ID3D11BlendState> _blend;
		ComPtr<ID3D11DepthStencilState> _depth;
		ComPtr<ID3D11DepthStencilState> _disabledDepth;
		DirectX::XMFLOAT4 _blendFactor;
		UINT _sampleMask;
		UINT _stencil;
	};
}
