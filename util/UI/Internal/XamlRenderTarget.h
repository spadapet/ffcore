#pragma once

namespace ff
{
	class IGraphDevice;
	class IRenderDepth;
	class IRenderTarget;
	class ITexture;
	class XamlTexture;

	class XamlRenderTarget : public Noesis::RenderTarget
	{
	public:
		XamlRenderTarget(ff::IGraphDevice* graph, size_t width, size_t height, size_t samples, bool sRGB, ff::StringRef name);
		XamlRenderTarget(const XamlRenderTarget& rhs, ff::StringRef name);
		virtual ~XamlRenderTarget() override;

		static XamlRenderTarget* Get(Noesis::RenderTarget* renderTarget);
		Noesis::Ptr<XamlRenderTarget> Clone(ff::StringRef name) const;
		ff::StringRef GetName() const;
		ff::ITexture* GetResolvedTexture() const;
		ff::ITexture* GetMsaaTexture() const;
		ff::IRenderTarget* GetResolvedTarget() const;
		ff::IRenderTarget* GetMsaaTarget() const;
		ff::IRenderDepth* GetDepth() const;

		virtual Noesis::Texture* GetTexture() override;

	private:
		ff::String _name;
		ff::ComPtr<ff::IGraphDevice> _graph;
		ff::ComPtr<ff::ITexture> _resolvedTexture;
		ff::ComPtr<ff::ITexture> _msaaTexture;
		ff::ComPtr<ff::IRenderTarget> _resolvedTarget;
		ff::ComPtr<ff::IRenderTarget> _msaaTarget;
		ff::ComPtr<ff::IRenderDepth> _depth;
		Noesis::Ptr<XamlTexture> _resolvedTextureWrapper;
	};
}
