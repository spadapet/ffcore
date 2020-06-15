#pragma once

namespace ff
{
	struct PixelTransform;

	struct Transform
	{
		UTIL_API static const Transform& Identity();
		UTIL_API static Transform Create(PointFloat position = PointFloat::Zeros(), PointFloat scale = PointFloat::Ones(), float rotation = 0.0f, const DirectX::XMFLOAT4& color = ff::GetColorWhite());
		UTIL_API static Transform Create(const PixelTransform& value);

		UTIL_API DirectX::XMMATRIX GetMatrix() const;
		float GetRotationRadians() const { return _rotation * ff::DEG_TO_RAD_F; }

		PointFloat _position;
		PointFloat _scale;
		float _rotation; // degrees CCW
		DirectX::XMFLOAT4 _color;
	};

	struct PixelTransform
	{
		UTIL_API static const PixelTransform& Identity();
		UTIL_API static PixelTransform Create(PointFixedInt position = PointFixedInt::Zeros(), PointFixedInt scale = PointFixedInt::Ones(), FixedInt rotation = 0_f, const DirectX::XMFLOAT4& color = ff::GetColorWhite());
		UTIL_API static PixelTransform Create(const Transform& value);

		UTIL_API DirectX::XMMATRIX GetMatrix() const;
		float GetRotationRadians() const { return (float)_rotation * ff::DEG_TO_RAD_F; }

		PointFixedInt _position;
		PointFixedInt _scale;
		FixedInt _rotation; // degrees CCW
		DirectX::XMFLOAT4 _color;
	};
}
