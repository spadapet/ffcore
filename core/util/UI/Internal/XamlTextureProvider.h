#pragma once

namespace ff
{
	class XamlGlobalState;

	class XamlTextureProvider : public Noesis::TextureProvider
	{
	public:
		XamlTextureProvider(XamlGlobalState* globals);
		virtual ~XamlTextureProvider() override;

		virtual Noesis::TextureInfo GetTextureInfo(const char* uri) override;
		virtual Noesis::Ptr<Noesis::Texture> LoadTexture(const char* uri, Noesis::RenderDevice* device) override;

	private:
		XamlGlobalState* _globals;
	};
}
