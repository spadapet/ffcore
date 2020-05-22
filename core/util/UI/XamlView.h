#pragma once

namespace Noesis
{
	struct IView;
	class FrameworkElement;
}

namespace ff
{
	class IRenderDepth;
	class IRenderTarget;
	class XamlGlobalState;

	// Renders XAML to any render target at any size
	class XamlView : public std::enable_shared_from_this<XamlView>
	{
	public:
		XamlView(XamlGlobalState* globals, Noesis::FrameworkElement* content, ff::IRenderTarget* target, bool perPixelAntiAlias, bool subPixelRendering);
		virtual ~XamlView();

		void Destroy();
		UTIL_API void Advance();
		UTIL_API void PreRender();
		UTIL_API void Render(ff::IRenderTarget* target, ff::IRenderDepth* depth, const ff::RectFloat* viewRect = nullptr);

		UTIL_API XamlGlobalState* GetGlobals() const;
		UTIL_API Noesis::IView* GetView() const;
		UTIL_API Noesis::FrameworkElement* GetContent() const;
		UTIL_API Noesis::Visual* HitTest(ff::PointFloat screenPos) const;
		UTIL_API void SetSize(ff::IRenderTarget* target);
		UTIL_API void SetSize(ff::PointInt pixelSize, double dpiScale, int rotate);
		UTIL_API ff::PointFloat ScreenToContent(ff::PointFloat pos) const;
		UTIL_API ff::PointFloat ContentToScreen(ff::PointFloat pos) const;

		UTIL_API void SetViewToScreenTransform(ff::PointFloat pos, ff::PointFloat scale);
		UTIL_API void SetViewToScreenTransform(const DirectX::XMMATRIX& matrix);
		UTIL_API ff::PointFloat ScreenToView(ff::PointFloat pos) const;
		UTIL_API ff::PointFloat ViewToScreen(ff::PointFloat pos) const;

		UTIL_API void SetFocus(bool focus);
		UTIL_API bool IsFocused() const;
		UTIL_API void SetEnabled(bool enabled);
		UTIL_API bool IsEnabled() const;
		UTIL_API void SetBlockInputBelow(bool block);
		UTIL_API bool IsInputBelowBlocked() const;

	protected:
		virtual void RenderBegin(ff::IRenderTarget* target, ff::IRenderDepth* depth, const ff::RectFloat* viewRect) = 0;
		virtual void RenderEnd(ff::IRenderTarget* target, ff::IRenderDepth* depth, const ff::RectFloat* viewRect) = 0;

	private:
		XamlGlobalState* _globals;
		DirectX::XMMATRIX* _matrix;
		bool _focused;
		bool _enabled;
		bool _blockedBelow;
		Noesis::Ptr<Noesis::Grid> _viewGrid;
		Noesis::Ptr<Noesis::Viewbox> _viewBox;
		Noesis::Ptr<Noesis::IView> _view;
		Noesis::Ptr<Noesis::RotateTransform> _rotate;
		Noesis::Ptr<Noesis::FrameworkElement> _content;
	};
}
