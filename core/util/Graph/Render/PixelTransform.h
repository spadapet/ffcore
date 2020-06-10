#pragma once

namespace ff
{
	struct PixelTransform
	{
		static const PixelTransform& Identity();
		static PixelTransform Create(PointFixedInt position = PointFixedInt::Zeros(), PointFixedInt scale = PointFixedInt::Ones(), FixedInt rotation = 0_f, const DirectX::XMFLOAT4& color = ff::GetColorWhite());

		PointFixedInt position;
		PointFixedInt scale;
		FixedInt rotation; // degrees CCW
		DirectX::XMFLOAT4 color;
	};
}
