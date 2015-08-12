#pragma once

namespace ff
{
	class IRendererActive;
	class ISprite;
	enum AnimTweenType;

	class __declspec(uuid("0fb93e0a-8f7f-40fb-80d1-189f53c60a91")) __declspec(novtable)
		IRenderAnimation : public IUnknown
	{
	public:
		virtual void Render(
			IRendererActive* render,
			AnimTweenType type,
			float frame,
			PointFloat pos,
			PointFloat scale,
			float rotate,
			const DirectX::XMFLOAT4& color) = 0;

		virtual float GetLastFrame() const = 0;
		virtual float GetFPS() const = 0;
	};
}
