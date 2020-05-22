#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class GraphContext11;
	class GraphStateCache11;
	class IGraphBuffer;

	class XamlRenderDevice11 : public Noesis::RenderDevice, public ff::IGraphDeviceChild
	{
	public:
		XamlRenderDevice11(ff::IGraphDevice* graph, bool sRGB = false);
		virtual ~XamlRenderDevice11() override;

		// Noesis::RenderDevice
		virtual const Noesis::DeviceCaps& GetCaps() const override;
		virtual Noesis::Ptr<Noesis::RenderTarget> CreateRenderTarget(const char* label, uint32_t width, uint32_t height, uint32_t sampleCount) override;
		virtual Noesis::Ptr<Noesis::RenderTarget> CloneRenderTarget(const char* label, Noesis::RenderTarget* surface) override;
		virtual Noesis::Ptr<Noesis::Texture> CreateTexture(const char* label, uint32_t width, uint32_t height, uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** data) override;
		virtual void UpdateTexture(Noesis::Texture* texture, uint32_t level, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* data) override;
		virtual void BeginRender(bool offscreen) override;
		virtual void SetRenderTarget(Noesis::RenderTarget* surface) override;
		virtual void BeginTile(const Noesis::Tile& tile, uint32_t surfaceWidth, uint32_t surfaceHeight) override;
		virtual void EndTile() override;
		virtual void ResolveRenderTarget(Noesis::RenderTarget* surface, const Noesis::Tile* tiles, uint32_t numTiles) override;
		virtual void EndRender() override;
		virtual void* MapVertices(uint32_t bytes) override;
		virtual void UnmapVertices() override;
		virtual void* MapIndices(uint32_t bytes) override;
		virtual void UnmapIndices() override;
		virtual void DrawBatch(const Noesis::Batch& batch) override;

		// IGraphDeviceChild
		virtual ff::IGraphDevice* GetDevice() const override;
		virtual bool Reset() override;

	private:
		enum class MsaaSamples { x1, x2, x4, x8, x16, Count };
		enum class TextureSlot { Pattern, Ramps, Image, Glyphs, Shadow, Count };

		struct VertexStage
		{
			ff::ComPtr<ID3D11VertexShader> shader;
			ff::ComPtr<ID3D11InputLayout> layout;
			uint32_t stride;
		};

		struct Program
		{
			int8_t vertexShaderIndex;
			int8_t pixelShaderIndex;
		};

		ff::ComPtr<ID3D11InputLayout> CreateLayout(uint32_t format, const void* code, uint32_t size);
		void CreateBuffers();
		void CreateStateObjects();
		void CreateShaders();

		void ClearRenderTarget();
		void ClearTextures();

		void SetShaders(const Noesis::Batch& batch);
		void SetBuffers(const Noesis::Batch& batch);
		void SetRenderState(const Noesis::Batch& batch);
		void SetTextures(const Noesis::Batch& batch);

		Noesis::DeviceCaps _caps;

		// Device
		ff::ComPtr<ff::IGraphDevice> _graph;
		ff::GraphContext11& _context;
		ff::GraphStateCache11& _states;

		// Buffers
		ff::ComPtr<ff::IGraphBuffer> _bufferVertices;
		ff::ComPtr<ff::IGraphBuffer> _bufferIndices;
		ff::ComPtr<ff::IGraphBuffer> _bufferVertexCB;
		ff::ComPtr<ff::IGraphBuffer> _bufferPixelCB;
		ff::ComPtr<ff::IGraphBuffer> _effectCB;
		ff::ComPtr<ff::IGraphBuffer> _texDimensionsCB;
		uint32_t _vertexCBHash;
		uint32_t _pixelCBHash;
		uint32_t _effectCBHash;
		uint32_t _texDimensionsCBHash;

		// Shaders
		Program _programs[52];
		VertexStage _vertexStages[11];
		ff::ComPtr<ID3D11InputLayout> _layouts[9];
		ff::ComPtr<ID3D11PixelShader> _pixelShaders[52];
		ff::ComPtr<ID3D11VertexShader> _quadVS;
		ff::ComPtr<ID3D11PixelShader> _clearPS;
		ff::ComPtr<ID3D11PixelShader> _resolvePS[(size_t)MsaaSamples::Count - 1];

		// State
		ff::ComPtr<ID3D11RasterizerState> _rasterizerStates[4];
		ff::ComPtr<ID3D11BlendState> _blendStates[4];
		ff::ComPtr<ID3D11DepthStencilState> _depthStencilStates[5];
		ff::ComPtr<ID3D11SamplerState> _samplerStates[64];
	};
}
