#pragma once

#include "Graph/GraphDeviceChild.h"
#include "Graph/Texture/TextureFormat.h"

namespace DirectX
{
	class ScratchImage;
}

namespace ff
{
	class ISprite;
	class ITexture11;
	class ITextureDxgi;
	class ITextureView;
	enum class SpriteType;
	enum class TextureFormat;

	class __declspec(uuid("8d9fab28-83b4-4327-8bf1-87b75eb9235e")) __declspec(novtable)
		ITexture : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual PointInt GetSize() const = 0;
		virtual size_t GetMipCount() const = 0;
		virtual size_t GetArraySize() const = 0;
		virtual size_t GetSampleCount() const = 0;
		virtual TextureFormat GetFormat() const = 0;
		virtual SpriteType GetSpriteType() const = 0;
		virtual ComPtr<ITextureView> CreateView(size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount) = 0;
		virtual ComPtr<ITexture> Convert(TextureFormat format, size_t mips) = 0;

		virtual ISprite* AsSprite() = 0;
		virtual ITextureView* AsTextureView() = 0;
		virtual ITextureDxgi* AsTextureDxgi() = 0;
		virtual ITexture11* AsTexture11() = 0;
	};

	class ITextureDxgi
	{
	public:
		virtual DXGI_FORMAT GetDxgiFormat() const = 0;
		virtual const DirectX::ScratchImage* Capture(DirectX::ScratchImage& tempHolder) = 0;
	};

	class ITexture11
	{
	public:
		virtual ID3D11Texture2D* GetTexture2d() = 0;
	};
}
