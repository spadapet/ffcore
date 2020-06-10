#pragma once

namespace ff
{
	class PixelRendererActive;
	struct PixelTransform;

	class __declspec(uuid("b1b3682a-36fa-4d4d-89ce-208c29118504")) __declspec(novtable)
		IPixelAnimation : public IUnknown
	{
	public:
		virtual void AdvancePixelAnimation() = 0;
		virtual void RenderPixelAnimation(const PixelRendererActive& render, const PixelTransform& pos) = 0;
	};
}
