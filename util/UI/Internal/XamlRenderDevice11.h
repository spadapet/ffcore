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
		enum class TextureSlot { Pattern, PaletteImage, Palette, Ramps, Image, Glyphs, Shadow, Count };

		struct VertexStage
		{
			ID3D11VertexShader* GetShader(ff::GraphStateCache11& states);

			ff::String _resourceName;
			ff::ComPtr<ID3D11VertexShader> _shader;
		};

		struct VertexAndLayoutStage : public VertexStage
		{
			ID3D11InputLayout* GetLayout(ff::GraphStateCache11& states);

			ff::ComPtr<ID3D11InputLayout> _layout;
			uint32_t _layoutIndex;
		};

		struct PixelStage
		{
			ID3D11PixelShader* GetShader(ff::GraphStateCache11& states);

			ff::String _resourceName;
			ff::ComPtr<ID3D11PixelShader> _shader;
		};

		struct SamplerStage
		{
			ID3D11SamplerState* GetState(ff::GraphStateCache11& states);

			Noesis::SamplerState _params;
			ff::ComPtr<ID3D11SamplerState> _state;
		};

		struct Program
		{
			int8_t _vertexShaderIndex;
			int8_t _pixelShaderIndex;
		};

		static ff::ComPtr<ID3D11InputLayout> CreateLayout(ff::GraphStateCache11& states, uint32_t format, ff::StringRef vertexResourceName);
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
		std::array<ID3D11ShaderResourceView*, (size_t)ff::XamlRenderDevice11::TextureSlot::Count> _nullTextures;
#ifdef _DEBUG
		ff::ComPtr<ff::ITexture> _emptyTextureRgb;
		ff::ComPtr<ff::ITexture> _emptyTexturePalette;
#endif

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
		ff::hash_t _texDimensionsCBHash;

		// Shaders
		Program _programs[52];
		VertexAndLayoutStage _vertexStages[11];
		PixelStage _pixelStages[52];
		VertexStage _quadVS;
		PixelStage _clearPS;
		PixelStage _resolvePS[(size_t)MsaaSamples::Count - 1];

		// State
		ff::ComPtr<ID3D11RasterizerState> _rasterizerStates[4];
		ff::ComPtr<ID3D11BlendState> _blendStates[4];
		ff::ComPtr<ID3D11DepthStencilState> _depthStencilStates[5];
		SamplerStage _samplerStages[64];
	};
}
