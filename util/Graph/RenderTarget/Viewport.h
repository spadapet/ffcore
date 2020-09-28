#pragma once

namespace ff
{
	class IRenderTarget;

	class Viewport
	{
	public:
		UTIL_API Viewport();
		UTIL_API Viewport(PointFloat aspect, RectFloat padding = ff::RectFloat::Zeros());
		UTIL_API ~Viewport();

		UTIL_API void SetAutoSize(PointFloat aspect, RectFloat padding = ff::RectFloat::Zeros());
		UTIL_API RectFloat GetView(IRenderTarget* target);

	private:
		PointFloat _aspect;
		RectFloat _padding;
		RectFloat _viewport;
		PointFloat _targetSize;
	};
}
