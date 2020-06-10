#include "pch.h"
#include "Graph/Render/PixelTransform.h"

static ff::PixelTransform s_identity{ ff::PointFixedInt::Zeros(), ff::PointFixedInt::Ones(), 0_f, ff::GetColorWhite() };

const ff::PixelTransform& ff::PixelTransform::Identity()
{
	return s_identity;
}

ff::PixelTransform ff::PixelTransform::Create(PointFixedInt position, PointFixedInt scale, FixedInt rotation, const DirectX::XMFLOAT4& color)
{
	return PixelTransform{ position, scale, rotation, color };
}
