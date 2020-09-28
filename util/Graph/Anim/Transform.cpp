#include "pch.h"
#include "Graph/Anim/Transform.h"

static ff::Transform s_identity = ff::Transform::Create();
static ff::PixelTransform s_pixelIdentity = ff::PixelTransform::Create();

const ff::Transform& ff::Transform::Identity()
{
	return s_identity;
}

ff::Transform ff::Transform::Create(PointFloat position, PointFloat scale, float rotation, const DirectX::XMFLOAT4& color)
{
	return Transform{ position, scale, rotation, color };
}

ff::Transform ff::Transform::Create(const PixelTransform& value)
{
	return Transform{ value._position.ToType<float>(), value._scale.ToType<float>(), value._rotation, value._color };
}

DirectX::XMMATRIX ff::Transform::GetMatrix() const
{
	return DirectX::XMMatrixAffineTransformation2D(
		DirectX::XMVectorSet(_scale.x, _scale.y, 1, 1), // scale
		DirectX::XMVectorSet(0, 0, 0, 0), // rotation center
		_rotation * ff::DEG_TO_RAD_F, // rotation
		DirectX::XMVectorSet(_position.x, _position.y, 0, 0));
}

const ff::PixelTransform& ff::PixelTransform::Identity()
{
	return s_pixelIdentity;
}

ff::PixelTransform ff::PixelTransform::Create(PointFixedInt position, PointFixedInt scale, FixedInt rotation, const DirectX::XMFLOAT4& color)
{
	return PixelTransform{ position, scale, rotation, color };
}

ff::PixelTransform ff::PixelTransform::Create(const Transform& value)
{
	return PixelTransform{ value._position.ToType<FixedInt>(), value._scale.ToType<FixedInt>(), FixedInt(value._rotation), value._color };
}

DirectX::XMMATRIX ff::PixelTransform::GetMatrix() const
{
	return DirectX::XMMatrixAffineTransformation2D(
		DirectX::XMVectorSet(_scale.x, _scale.y, 1, 1), // scale
		DirectX::XMVectorSet(0, 0, 0, 0), // rotation center
		(float)_rotation * ff::DEG_TO_RAD_F, // rotation
		DirectX::XMVectorSet(_position.x, _position.y, 0, 0));
}
