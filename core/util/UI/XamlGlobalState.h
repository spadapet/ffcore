#pragma once

#include "State/State.h"
#include "Types/Event.h"

namespace Noesis
{
	enum Cursor;
	struct IView;
	class UIElement;
}

namespace ff
{
	class AppGlobals;
	class IRenderTarget;
	class IRenderTargetWindow;
	class IResourceAccess;
	class IXamlGlobalHelper;
	class XamlFontProvider;
	class XamlProvider;
	class XamlTextureProvider;
	class XamlView;
	class XamlViewState;

	class XamlGlobalState : public ff::State
	{
	public:
		UTIL_API XamlGlobalState(ff::AppGlobals* appGlobals);
		UTIL_API ~XamlGlobalState();

		static XamlGlobalState* Get();

		UTIL_API bool Startup(ff::IXamlGlobalHelper* helper);
		UTIL_API void Shutdown();
		UTIL_API std::shared_ptr<XamlView> CreateView(ff::StringRef xamlFile, bool perPixelAntiAlias = false, bool subPixelRendering = false);
		UTIL_API std::shared_ptr<XamlView> CreateView(Noesis::FrameworkElement* content, bool perPixelAntiAlias = false, bool subPixelRendering = false);
		UTIL_API const ff::Vector<XamlView*>& GetInputViews() const;
		UTIL_API const ff::Vector<XamlView*>& GetRenderedViews() const;

		UTIL_API ff::AppGlobals* GetAppGlobals() const;
		ff::IResourceAccess* GetResourceAccess() const;
		Noesis::RenderDevice* GetRenderDevice() const;

		void RegisterView(XamlView* view);
		void UnregisterView(XamlView* view);
		void OnRenderView(XamlView* view);
		void OnFocusView(XamlView* view, bool focused);

		// State
		virtual void AdvanceDebugInput(ff::AppGlobals* globals) override;
		virtual void OnFrameRendering(ff::AppGlobals* globals, ff::AdvanceType type) override;
		virtual void OnFrameRendered(ff::AppGlobals* globals, ff::AdvanceType type, ff::IRenderTarget* target, ff::IRenderDepth* depth) override;

	private:
		static void StaticUpdateCursorCallback(void* user, Noesis::IView* view, Noesis::Cursor cursor);
		static void StaticOpenUrlCallback(void* user, const char* url);
		static void StaticPlaySoundCallback(void* user, const char* filename, float volume);
		static void StaticSoftwareKeyboardCallback(void* user, Noesis::UIElement* focused, bool open);

		void RegisterComponents();
		void UpdateCursorCallback(Noesis::IView* view, Noesis::Cursor cursor);
		void OpenUrlCallback(ff::StringRef url);
		void PlaySoundCallback(ff::StringRef filename, float volume);
		void SoftwareKeyboardCallback(Noesis::UIElement* focused, bool open);

		ff::AppGlobals* _appGlobals;
		ff::IResourceAccess* _resources;
		ff::Vector<XamlView*> _views;
		ff::Vector<XamlView*> _inputViews;
		ff::Vector<XamlView*> _renderedViews;
		Noesis::Ptr<XamlFontProvider> _fontProvider;
		Noesis::Ptr<XamlTextureProvider> _textureProvider;
		Noesis::Ptr<XamlProvider> _xamlProvider;
		Noesis::Ptr<Noesis::RenderDevice> _renderDevice;
		Noesis::Ptr<Noesis::ResourceDictionary> _applicationResources;
		XamlView* _focusedView;
	};
}
