#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class ITexture;
	class ITextureView11;

	class __declspec(uuid("507d8689-7591-4169-b9ba-fca398da1dbd")) __declspec(novtable)
		ITextureView : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual ITexture* GetTexture() = 0;
		virtual ITextureView11* AsTextureView11() = 0;
	};

	class ITextureView11
	{
	public:
		virtual ID3D11ShaderResourceView* GetView() = 0;
	};
}
