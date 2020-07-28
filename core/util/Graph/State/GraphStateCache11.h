#pragma once

#include "Data/Data.h"
#include "Graph/GraphShader.h"
#include "Resource/ResourceValue.h"

namespace ff
{
	class IData;
	class IGraphDevice;

	class GraphStateCache11
	{
	public:
		GraphStateCache11();
		GraphStateCache11(ID3D11DeviceX* device);
		~GraphStateCache11();

		void Reset(ID3D11DeviceX* device);

		UTIL_API ID3D11BlendState* GetBlendState(const D3D11_BLEND_DESC& desc);
		UTIL_API ID3D11DepthStencilState* GetDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc);
		UTIL_API ID3D11RasterizerState* GetRasterizerState(const D3D11_RASTERIZER_DESC& desc);
		UTIL_API ID3D11SamplerState* GetSamplerState(const D3D11_SAMPLER_DESC& desc);

		UTIL_API ID3D11VertexShader* GetVertexShader(ff::IResources* resources, StringRef resourceName);
		UTIL_API ID3D11VertexShader* GetVertexShaderAndInputLayout(ff::IResources* resources, StringRef resourceName, ComPtr<ID3D11InputLayout>& inputLayout, const D3D11_INPUT_ELEMENT_DESC* layout, size_t count);
		UTIL_API ID3D11GeometryShader* GetGeometryShader(ff::IResources* resources, StringRef resourceName);
		UTIL_API ID3D11GeometryShader* GetGeometryShaderStreamOutput(ff::IResources* resources, StringRef resourceName, const D3D11_SO_DECLARATION_ENTRY* layout, size_t count, size_t vertexStride);
		UTIL_API ID3D11PixelShader* GetPixelShader(ff::IResources* resources, StringRef resourceName);
		UTIL_API ID3D11InputLayout* GetInputLayout(ff::IResources* resources, StringRef vertexShaderResourceName, const D3D11_INPUT_ELEMENT_DESC* layout, size_t count);

		UTIL_API ID3D11VertexShader* GetVertexShader(ff::IData* shaderData);
		UTIL_API ID3D11VertexShader* GetVertexShaderAndInputLayout(ff::IData* shaderData, ComPtr<ID3D11InputLayout>& inputLayout, const D3D11_INPUT_ELEMENT_DESC* layout, size_t count);
		UTIL_API ID3D11GeometryShader* GetGeometryShader(ff::IData* shaderData);
		UTIL_API ID3D11GeometryShader* GetGeometryShaderStreamOutput(ff::IData* shaderData, const D3D11_SO_DECLARATION_ENTRY* layout, size_t count, size_t vertexStride);
		UTIL_API ID3D11PixelShader* GetPixelShader(ff::IData* shaderData);
		UTIL_API ID3D11InputLayout* GetInputLayout(ff::IData* shaderData, const D3D11_INPUT_ELEMENT_DESC* layout, size_t count);

	private:
		ComPtr<ID3D11DeviceX> _device;

		struct HashShaderData
		{
			static hash_t Hash(const ComPtr<IData>& value);
			static bool Equals(const ComPtr<IData>& lhs, const ComPtr<IData>& rhs);
		};

		Map<D3D11_BLEND_DESC, ComPtr<ID3D11BlendState>> _blendStates;
		Map<D3D11_DEPTH_STENCIL_DESC, ComPtr<ID3D11DepthStencilState>> _depthStates;
		Map<D3D11_RASTERIZER_DESC, ComPtr<ID3D11RasterizerState>> _rasterStates;
		Map<D3D11_SAMPLER_DESC, ComPtr<ID3D11SamplerState>> _samplerStates;
		Map<ComPtr<IData>, ComPtr<IUnknown>, HashShaderData> _shaders;
		Map<ff::hash_t, ComPtr<ID3D11InputLayout>, ff::NonHasher<ff::hash_t>> _layouts;
		List<TypedResource<IGraphShader>> _resources;
	};
}
