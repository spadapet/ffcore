#pragma once

namespace ff
{
	class GraphContext11
	{
	public:
		UTIL_API GraphContext11();
		UTIL_API GraphContext11(ID3D11DeviceContext* context);
		UTIL_API ~GraphContext11();

		UTIL_API void Clear();
		UTIL_API void Apply(GraphContext11& dest);
		UTIL_API void Reset(ID3D11DeviceContext* context);
		UTIL_API GraphCounters ResetDrawCount();
		UTIL_API void Draw(size_t count, size_t start);
		UTIL_API void DrawIndexed(size_t indexCount, size_t indexStart, int vertexOffset);
		UTIL_API void* Map(ID3D11Resource* buffer, D3D11_MAP type, D3D11_MAPPED_SUBRESOURCE* map = nullptr);
		UTIL_API void Unmap(ID3D11Resource* buffer);
		UTIL_API void UpdateDiscard(ID3D11Resource* buffer, const void* data, size_t size);
		UTIL_API void ClearRenderTarget(ID3D11RenderTargetView* view, const DirectX::XMFLOAT4& color);
		UTIL_API void ClearDepthStencil(ID3D11DepthStencilView* view, bool clearDepth, bool clearStencil, float depth, BYTE stencil);
		UTIL_API void UpdateSubresource(ID3D11Resource* dest, UINT destSubresource, const D3D11_BOX* destBox, const void* srcData, UINT srcRowPitch, UINT srcDepthPitch);
		UTIL_API void CopySubresourceRegion(ID3D11Resource* destResource, UINT destSubresource, UINT destX, UINT destY, UINT destZ, ID3D11Resource* srcResource, UINT srcSubresource, const D3D11_BOX* srcBox);

		UTIL_API void SetVertexIA(ID3D11Buffer* value, size_t stride, size_t offset);
		UTIL_API void SetIndexIA(ID3D11Buffer* value, DXGI_FORMAT format, size_t offset);
		UTIL_API void SetLayoutIA(ID3D11InputLayout* value);
		UTIL_API void SetTopologyIA(D3D_PRIMITIVE_TOPOLOGY value);
		UTIL_API void SetAppendSO(ID3D11Buffer* value);
		UTIL_API void SetOutputSO(ID3D11Buffer* value, size_t offset);

		UTIL_API void SetVS(ID3D11VertexShader* value);
		UTIL_API void SetSamplersVS(ID3D11SamplerState* const* values, size_t start, size_t count);
		UTIL_API void SetConstantsVS(ID3D11Buffer* const* values, size_t start, size_t count);
		UTIL_API void SetResourcesVS(ID3D11ShaderResourceView* const* values, size_t start, size_t count);

		UTIL_API void SetGS(ID3D11GeometryShader* value);
		UTIL_API void SetSamplersGS(ID3D11SamplerState* const* values, size_t start, size_t count);
		UTIL_API void SetConstantsGS(ID3D11Buffer* const* values, size_t start, size_t count);
		UTIL_API void SetResourcesGS(ID3D11ShaderResourceView* const* values, size_t start, size_t count);

		UTIL_API void SetPS(ID3D11PixelShader* value);
		UTIL_API void SetSamplersPS(ID3D11SamplerState* const* value, size_t start, size_t count);
		UTIL_API void SetConstantsPS(ID3D11Buffer* const* valuess, size_t start, size_t count);
		UTIL_API void SetResourcesPS(ID3D11ShaderResourceView* const* values, size_t start, size_t count);

		UTIL_API void SetBlend(ID3D11BlendState* value, const DirectX::XMFLOAT4& blendFactor, UINT sampleMask);
		UTIL_API void SetDepth(ID3D11DepthStencilState* value, UINT stencil);
		UTIL_API void SetTargets(ID3D11RenderTargetView* const* targets, size_t count, ID3D11DepthStencilView* depth);
		UTIL_API void SetRaster(ID3D11RasterizerState* value);
		UTIL_API void SetViewports(const D3D11_VIEWPORT* value, size_t count);
		UTIL_API void SetScissors(const D3D11_RECT* value, size_t count);
		UTIL_API ID3D11DepthStencilView* GetDepthView() const;

	private:
		ComPtr<ID3D11DeviceContext> _context;

		template<typename TShader>
		struct ShaderCachedState
		{
			void Reset();

			ComPtr<TShader> _shader;
			ComPtr<ID3D11SamplerState> _samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
			ComPtr<ID3D11Buffer> _constants[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
			ComPtr<ID3D11ShaderResourceView> _resources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
		};

		// Shaders
		ShaderCachedState<ID3D11VertexShader> _vs;
		ShaderCachedState<ID3D11GeometryShader> _gs;
		ShaderCachedState<ID3D11PixelShader> _ps;

		// Input assembler
		ComPtr<ID3D11Buffer> _iaIndex;
		DXGI_FORMAT _iaIndexFormat;
		UINT _iaIndexOffset;
		ComPtr<ID3D11InputLayout> _iaLayout;
		D3D_PRIMITIVE_TOPOLOGY _iaTopology;
		ComPtr<ID3D11Buffer> _iaVertexes[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		UINT _iaVertexStrides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		UINT _iaVertexOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];

		// Output merger
		UINT _blendSampleMask;
		UINT _depthStencil;
		size_t _targetViewsCount;
		DirectX::XMFLOAT4 _blendFactor;
		ComPtr<ID3D11BlendState> _blend;
		ComPtr<ID3D11DepthStencilState> _depthState;
		ComPtr<ID3D11RenderTargetView> _targetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
		ComPtr<ID3D11DepthStencilView> _depthView;

		// Raster state
		ComPtr<ID3D11RasterizerState> _raster;
		D3D11_VIEWPORT _viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		D3D11_RECT _scissors[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		size_t _viewportsCount;
		size_t _scissorCount;

		// Stream out
		ComPtr<ID3D11Buffer> _soTargets[D3D11_SO_STREAM_COUNT];

		GraphCounters _counters;
	};
}

template<typename TShader>
void ff::GraphContext11::ShaderCachedState<TShader>::Reset()
{
	_shader.Release();

	for (size_t i = 0; i < _countof(_samplers); i++)
	{
		_samplers[i].Release();
	}

	for (size_t i = 0; i < _countof(_constants); i++)
	{
		_constants[i].Release();
	}

	for (size_t i = 0; i < _countof(_resources); i++)
	{
		_resources[i].Release();
	}
}
