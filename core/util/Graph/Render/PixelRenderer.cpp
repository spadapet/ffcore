#include "pch.h"
#include "Graph/Anim/Transform.h"
#include "Graph/Render/MatrixStack.h"
#include "Graph/Render/Renderer.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Render/PixelRenderer.h"

static ff::PointFloat Floor(const ff::PointFixedInt& val)
{
	return val.ToType<int>().ToType<float>();
}

static ff::RectFloat Floor(const ff::RectFixedInt& val)
{
	return val.ToType<int>().ToType<float>();
}

ff::PixelRendererActive::PixelRendererActive(ff::RendererActive& render)
	: _render(render)
{
	assert(render);
}

ff::IRendererActive* ff::PixelRendererActive::BeginRender(ff::IRenderer* render, ff::IRenderTarget* target, ff::IRenderDepth* depth, ff::RectFixedInt viewRect, ff::RectFixedInt worldRect, ff::RendererOptions options)
{
	return render->BeginRender(target, depth, ::Floor(viewRect), ::Floor(worldRect), options);
}

ff::IRendererActive* ff::PixelRendererActive::GetRenderer() const
{
	return _render;
}

ff::IRendererActive11* ff::PixelRendererActive::GetRenderer11() const
{
	return _render->AsRendererActive11();
}

void ff::PixelRendererActive::DrawSprite(ff::ISprite* sprite, const PixelTransform& pos) const
{
	ff::Transform pos2 = ff::Transform::Create(::Floor(pos._position), pos._scale.ToType<float>(), (float)pos._rotation, pos._color);
	_render->DrawSprite(sprite, pos2);
}

void ff::PixelRendererActive::DrawLineStrip(const ff::PointFixedInt* points, size_t count, const DirectX::XMFLOAT4& color, ff::FixedInt thickness) const
{
	ff::Vector<ff::PointFloat, 64> pointFloats;
	pointFloats.Resize(count);

	for (size_t i = 0; i < count; i++)
	{
		pointFloats[i] = ::Floor(points[i]);
	}

	_render->DrawLineStrip(pointFloats.ConstData(), pointFloats.Size(), color, std::floor(thickness), true);
}

void ff::PixelRendererActive::DrawLine(ff::PointFixedInt start, ff::PointFixedInt end, const DirectX::XMFLOAT4& color, ff::FixedInt thickness) const
{
	_render->DrawLine(::Floor(start), ::Floor(end), color, std::floor(thickness), true);
}

void ff::PixelRendererActive::DrawFilledRectangle(ff::RectFixedInt rect, const DirectX::XMFLOAT4& color) const
{
	_render->DrawFilledRectangle(::Floor(rect), color);
}

void ff::PixelRendererActive::DrawFilledCircle(ff::PointFixedInt center, ff::FixedInt radius, const DirectX::XMFLOAT4& color) const
{
	_render->DrawFilledCircle(::Floor(center), std::floor(radius), color);
}

void ff::PixelRendererActive::DrawOutlineRectangle(ff::RectFixedInt rect, const DirectX::XMFLOAT4& color, ff::FixedInt thickness) const
{
	_render->DrawOutlineRectangle(::Floor(rect), color, std::floor(thickness), true);
}

void ff::PixelRendererActive::DrawOutlineCircle(ff::PointFixedInt center, ff::FixedInt radius, const DirectX::XMFLOAT4& color, ff::FixedInt thickness) const
{
	_render->DrawOutlineCircle(::Floor(center), radius, color, std::floor(thickness), true);
}

void ff::PixelRendererActive::DrawPaletteLineStrip(const PointFixedInt* points, size_t count, int color, FixedInt thickness) const
{
	ff::Vector<ff::PointFloat, 64> pointFloats;
	pointFloats.Resize(count);

	for (size_t i = 0; i < count; i++)
	{
		pointFloats[i] = ::Floor(points[i]);
	}

	_render->DrawPaletteLineStrip(pointFloats.ConstData(), pointFloats.Size(), color, std::floor(thickness), true);
}

void ff::PixelRendererActive::DrawPaletteLine(PointFixedInt start, PointFixedInt end, int color, FixedInt thickness) const
{
	_render->DrawPaletteLine(::Floor(start), ::Floor(end), color, std::floor(thickness), true);
}

void ff::PixelRendererActive::DrawPaletteFilledRectangle(RectFixedInt rect, int color) const
{
	_render->DrawPaletteFilledRectangle(::Floor(rect), color);
}

void ff::PixelRendererActive::DrawPaletteFilledCircle(PointFixedInt center, FixedInt radius, int color) const
{
	_render->DrawPaletteFilledCircle(::Floor(center), std::floor(radius), color);
}

void ff::PixelRendererActive::DrawPaletteOutlineRectangle(RectFixedInt rect, int color, FixedInt thickness) const
{
	_render->DrawPaletteOutlineRectangle(::Floor(rect), color, std::floor(thickness), true);
}

void ff::PixelRendererActive::DrawPaletteOutlineCircle(PointFixedInt center, FixedInt radius, int color, FixedInt thickness) const
{
	_render->DrawPaletteOutlineCircle(::Floor(center), radius, color, std::floor(thickness), true);
}

void ff::PixelRendererActive::PushMatrix(const PixelTransform& pos) const
{
	// Translation must be whole pixels
	DirectX::XMMATRIX xmatrix = pos.GetMatrix();
	xmatrix.r[3] = DirectX::XMVectorFloor(xmatrix.r[3]);

	DirectX::XMFLOAT4X4 matrix;
	DirectX::XMStoreFloat4x4(&matrix, xmatrix);

	ff::MatrixStack& stack = _render->GetWorldMatrixStack();
	stack.PushMatrix();
	stack.TransformMatrix(matrix);
}

void ff::PixelRendererActive::PopMatrix() const
{
	_render->GetWorldMatrixStack().PopMatrix();
}
