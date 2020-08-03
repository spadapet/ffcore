#include "pch.h"
#include "Dict/ValueTable.h"
#include "Globals/AppGlobals.h"
#include "Globals/Log.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/Texture/Palette.h"
#include "Input/DeviceEvent.h"
#include "UI/Internal/XamlFontProvider.h"
#include "UI/Internal/XamlKeyMap.h"
#include "UI/Internal/XamlTextureProvider.h"
#include "UI/Internal/XamlRenderDevice11.h"
#include "UI/Internal/XamlView11.h"
#include "UI/Internal/XamlProvider.h"
#include "Resource/Resources.h"
#include "String/StringUtil.h"
#include "Thread/ThreadDispatch.h"
#include "Types/Timer.h"
#include "UI/Utility/Converters.h"
#include "UI/Utility/SetPanelChildFocusAction.h"
#include "UI/XamlGlobalHelper.h"
#include "UI/XamlGlobalState.h"
#include "UI/XamlViewState.h"

#define DEBUG_MEM_ALLOC 0

static ff::XamlGlobalState* s_globals;
static Noesis::AssertHandler s_assertHandler;
static Noesis::ErrorHandler s_errorHandler;
static Noesis::LogHandler s_logHandler;

ff::XamlGlobalState* ff::XamlGlobalState::Get()
{
	assert(s_globals);
	return s_globals;
}

static const wchar_t* s_logLevels[] =
{
	L"Trace",
	L"Debug",
	L"Info",
	L"Warning",
	L"Error",
};

static void NoesisLogHandler(const char* filename, uint32_t line, uint32_t level, const char* channel, const char* message)
{
	const wchar_t* logLevel = (level < NS_COUNTOF(s_logLevels)) ? s_logLevels[level] : L"";
	ff::String channel2 = ff::String::from_utf8(channel);
	ff::String message2 = ff::String::from_utf8(message);

	ff::Log::GlobalTraceF(L"[NOESIS/%s/%s] %s\n", channel2.c_str(), logLevel, message2.c_str());

	if (s_logHandler)
	{
		s_logHandler(filename, line, level, channel, message);
	}
}

static bool NoesisAssertHandler(const char* file, uint32_t line, const char* expr)
{
#ifdef _DEBUG
	if (::IsDebuggerPresent())
	{
		__debugbreak();
	}
#endif

	return s_assertHandler ? s_assertHandler(file, line, expr) : false;
}

static void NoesisErrorHandler(const char* file, uint32_t line, const char* message, bool fatal)
{
	if (s_errorHandler)
	{
		s_errorHandler(file, line, message, fatal);
	}

#ifdef _DEBUG
	if (::IsDebuggerPresent())
	{
		__debugbreak();
	}
#endif
}

static void* NoesisAlloc(void* user, size_t size)
{
	void* ptr = std::malloc(size);
#if DEBUG_MEM_ALLOC
	ff::Log::DebugTraceF(L"[NOESIS/Mem] ALLOC: 0x%lx (%lu)\n", ptr, size);
#endif
	return ptr;
}

static void* NoesisRealloc(void* user, void* ptr, size_t size)
{
	void* ptr2 = std::realloc(ptr, size);
#if DEBUG_MEM_ALLOC
	ff::Log::DebugTraceF(L"[NOESIS/Mem] REALLOC: 0x%lx -> 0x%lx (%lu)\n", ptr, ptr2, size);
#endif
	return ptr2;
}

static void NoesisDealloc(void* user, void* ptr)
{
#if DEBUG_MEM_ALLOC
	ff::Log::DebugTraceF(L"[NOESIS/Mem] FREE: 0x%lx\n", ptr);
#endif
	return std::free(ptr);
}

static size_t NoesisAllocSize(void* user, void* ptr)
{
	size_t size = _msize(ptr);
#if DEBUG_MEM_ALLOC
	ff::Log::DebugTraceF(L"[NOESIS/Mem] SIZE: 0x%lx (%lu)\n", ptr, size);
#endif
	return size;
}

static void NeosisDumpMemUsage()
{
	ff::Log::GlobalTraceF(L"[NOESIS/Mem] Now=%u, Total=%u\n", Noesis::GetAllocatedMemory(), Noesis::GetAllocatedMemoryAccum());
}

static Noesis::MemoryCallbacks s_memoryCallbacks =
{
	nullptr,
	::NoesisAlloc,
	::NoesisRealloc,
	::NoesisDealloc,
	::NoesisAllocSize,
};

ff::XamlGlobalState::XamlGlobalState(ff::AppGlobals* appGlobals)
	: _appGlobals(appGlobals)
	, _resources(nullptr)
	, _focusedView(nullptr)
{
	assert(!s_globals);
	s_globals = this;
}

ff::XamlGlobalState::~XamlGlobalState()
{
	Shutdown();

	assert(s_globals == this);
	s_globals = nullptr;
}

bool ff::XamlGlobalState::Startup(ff::IXamlGlobalHelper* helper)
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());
	assertRetVal(!s_assertHandler && helper && _appGlobals->GetGraph()->AsGraphDevice11(), false);

	// Global handlers

	s_assertHandler = Noesis::SetAssertHandler(::NoesisAssertHandler);
	s_errorHandler = Noesis::SetErrorHandler(::NoesisErrorHandler);
	s_logHandler = Noesis::SetLogHandler(::NoesisLogHandler);
	Noesis::SetMemoryCallbacks(s_memoryCallbacks);
	Noesis::GUI::Init(ff::StringToUTF8(helper->GetNoesisLicenseName()).Data(), ff::StringToUTF8(helper->GetNoesisLicenseKey()).Data());

	// Callbacks

	Noesis::GUI::SetCursorCallback(this, ff::XamlGlobalState::StaticUpdateCursorCallback);
	Noesis::GUI::SetOpenUrlCallback(this, ff::XamlGlobalState::StaticOpenUrlCallback);
	Noesis::GUI::SetPlayAudioCallback(this, ff::XamlGlobalState::StaticPlaySoundCallback);
	Noesis::GUI::SetSoftwareKeyboardCallback(this, ff::XamlGlobalState::StaticSoftwareKeyboardCallback);

	// Resource providers

	_palette = helper->GetPalette();
	_resources = helper->GetXamlResources();
	_renderDevice = Noesis::MakePtr<XamlRenderDevice11>(_appGlobals->GetGraph(), helper->IsSRGB());
	_xamlProvider = Noesis::MakePtr<XamlProvider>(this);
	_fontProvider = Noesis::MakePtr<XamlFontProvider>(this);
	_textureProvider = Noesis::MakePtr<XamlTextureProvider>(this);

	Noesis::GUI::SetXamlProvider(_xamlProvider);
	Noesis::GUI::SetTextureProvider(_textureProvider);
	Noesis::GUI::SetFontProvider(_fontProvider);

	RegisterComponents();
	helper->RegisterNoesisComponents();

	// Default font
	{
		ff::Vector<char> defaultFont8 = ff::StringToUTF8(helper->GetDefaultFont());
		const char* defaultFonts = defaultFont8.Size() ? defaultFont8.Data() : "Segoe UI";
		Noesis::GUI::SetFontFallbacks(&defaultFonts, 1);
		Noesis::GUI::SetFontDefaultProperties(helper->GetDefaultFontSize(), Noesis::FontWeight_Normal, Noesis::FontStretch_Normal, Noesis::FontStyle_Normal);
	}

	// Application resources
	{
		ff::String resourcesName = helper->GetApplicationResourcesName();
		if (!resourcesName.empty())
		{
			ff::Vector<char> resourcesUtf8 = ff::StringToUTF8(resourcesName);
			_applicationResources = Noesis::GUI::LoadXaml<Noesis::ResourceDictionary>(resourcesUtf8.ConstData());
			assertRetVal(_applicationResources, false);

			helper->OnApplicationResourcesLoaded(_applicationResources);
			Noesis::GUI::SetApplicationResources(_applicationResources);
		}
	}

	::NeosisDumpMemUsage();

	return true;
}

void ff::XamlGlobalState::Shutdown()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	noAssertRet(_renderDevice); // already shut down

	::NeosisDumpMemUsage();

	ff::Vector<XamlView*> views = _views;
	for (XamlView* view : views)
	{
		view->Destroy();
	}

	assert(_views.IsEmpty());

	_applicationResources = nullptr;
	_xamlProvider = nullptr;
	_fontProvider = nullptr;
	_textureProvider = nullptr;
	_renderDevice = nullptr;
	_resources = nullptr;

	Noesis::GUI::Shutdown();

	::NeosisDumpMemUsage();
	assertSz(!Noesis::GetAllocatedMemory(), L"NOESIS Memory Leak");

	Noesis::SetErrorHandler(s_errorHandler);
	s_errorHandler = nullptr;

	Noesis::SetAssertHandler(s_assertHandler);
	s_assertHandler = nullptr;
}

ff::IPalette* ff::XamlGlobalState::GetPalette() const
{
	return _palette;
}

void ff::XamlGlobalState::SetPalette(ff::IPalette* palette)
{
	_palette = palette;
}

std::shared_ptr<ff::XamlView> ff::XamlGlobalState::CreateView(ff::StringRef xamlFile, bool perPixelAntiAlias, bool subPixelRendering)
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::Vector<char> xamlFile8 = ff::StringToUTF8(xamlFile);
	return CreateView(Noesis::GUI::LoadXaml<Noesis::FrameworkElement>(xamlFile8.ConstData()), perPixelAntiAlias, subPixelRendering);
}

std::shared_ptr<ff::XamlView> ff::XamlGlobalState::CreateView(Noesis::FrameworkElement* content, bool perPixelAntiAlias, bool subPixelRendering)
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	assertRetVal(content, nullptr);
	return std::make_shared<ff::XamlView11>(this, content, perPixelAntiAlias, subPixelRendering);
}

ff::AppGlobals* ff::XamlGlobalState::GetAppGlobals() const
{
	return _appGlobals;
}

ff::IResourceAccess* ff::XamlGlobalState::GetResourceAccess() const
{
	return _resources;
}

Noesis::RenderDevice* ff::XamlGlobalState::GetRenderDevice() const
{
	return _renderDevice;
}

void ff::XamlGlobalState::RegisterView(XamlView* view)
{
	assertRet(view && !_views.Contains(view));
	_views.Push(view);
}

void ff::XamlGlobalState::UnregisterView(XamlView* view)
{
	verify(_views.DeleteItem(view));
	_inputViews.DeleteItem(view);
	_renderedViews.DeleteItem(view);

	if (_focusedView == view)
	{
		_focusedView = nullptr;
	}
}

std::shared_ptr<ff::State> ff::XamlGlobalState::Advance(ff::AppGlobals* globals)
{
	if (_xamlProvider)
	{
		_xamlProvider->Advance();
	}

	if (_textureProvider)
	{
		_textureProvider->Advance();
	}

	if (_fontProvider)
	{
		_fontProvider->Advance();
	}

	return ff::State::Advance(globals);
}

void ff::XamlGlobalState::AdvanceDebugInput(ff::AppGlobals* globals)
{
	for (const ff::DeviceEvent& event : globals->GetDeviceEventsDebug()->GetEvents())
	{
		switch (event._type)
		{
		case ff::DeviceEventType::KeyPress:
			if (_focusedView && ff::IsValidKey(event._id))
			{
				if (event._count)
				{
					_focusedView->GetView()->KeyDown(ff::GetKey(event._id));
				}
				else
				{
					_focusedView->GetView()->KeyUp(ff::GetKey(event._id));
				}
			}
			break;

		case ff::DeviceEventType::KeyChar:
			if (_focusedView)
			{
				_focusedView->GetView()->Char(event._id);
			}
			break;

		case ff::DeviceEventType::MousePress:
			if (ff::IsValidMouseButton(event._id))
			{
				for (auto i = _inputViews.rbegin(); i != _inputViews.rend(); i++)
				{
					bool handled = false;
					XamlView* view = *i;
					ff::PointFloat posf = view->ScreenToView(event._pos.ToType<float>());
					ff::PointInt pos = posf.ToType<int>();

					if (event._count == 2)
					{
						handled = view->GetView()->MouseDoubleClick(pos.x, pos.y, ff::GetMouseButton(event._id));
					}
					else if (event._count == 0)
					{
						handled = view->GetView()->MouseButtonUp(pos.x, pos.y, ff::GetMouseButton(event._id));
					}
					else
					{
						handled = view->GetView()->MouseButtonDown(pos.x, pos.y, ff::GetMouseButton(event._id));
					}

					if (handled)
					{
						view->SetFocus(true);
						break;
					}

					if (view->HitTest(posf))
					{
						break;
					}
				}
			}
			break;

		case ff::DeviceEventType::MouseMove:
			for (auto i = _inputViews.rbegin(); i != _inputViews.rend(); i++)
			{
				XamlView* view = *i;
				ff::PointFloat posf = view->ScreenToView(event._pos.ToType<float>());
				ff::PointInt pos = posf.ToType<int>();

				if (view->GetView()->MouseMove(pos.x, pos.y))
				{
					break;
				}

				if (view->HitTest(posf))
				{
					break;
				}
			}
			break;

		case ff::DeviceEventType::MouseWheelX:
			for (auto i = _inputViews.rbegin(); i != _inputViews.rend(); i++)
			{
				XamlView* view = *i;
				ff::PointFloat posf = view->ScreenToView(event._pos.ToType<float>());
				ff::PointInt pos = posf.ToType<int>();

				if (view->GetView()->MouseHWheel(pos.x, pos.y, event._count))
				{
					break;
				}
			}
			break;

		case ff::DeviceEventType::MouseWheelY:
			for (auto i = _inputViews.rbegin(); i != _inputViews.rend(); i++)
			{
				XamlView* view = *i;
				ff::PointFloat posf = view->ScreenToView(event._pos.ToType<float>());
				ff::PointInt pos = posf.ToType<int>();

				if (view->GetView()->MouseWheel(pos.x, pos.y, event._count))
				{
					break;
				}
			}
			break;

		case ff::DeviceEventType::TouchPress:
			for (auto i = _inputViews.rbegin(); i != _inputViews.rend(); i++)
			{
				bool handled = false;
				XamlView* view = *i;
				ff::PointFloat posf = view->ScreenToView(event._pos.ToType<float>());
				ff::PointInt pos = posf.ToType<int>();

				if (event._count == 0)
				{
					handled = view->GetView()->TouchUp(pos.x, pos.y, event._id);
				}
				else
				{
					handled = view->GetView()->TouchDown(pos.x, pos.y, event._id);
				}

				if (handled)
				{
					view->SetFocus(true);
					break;
				}

				if (view->HitTest(posf))
				{
					break;
				}
			}
			break;

		case ff::DeviceEventType::TouchMove:
			for (auto i = _inputViews.rbegin(); i != _inputViews.rend(); i++)
			{
				XamlView* view = *i;
				ff::PointFloat posf = view->ScreenToView(event._pos.ToType<float>());
				ff::PointInt pos = posf.ToType<int>();

				if (view->GetView()->TouchMove(pos.x, pos.y, event._id))
				{
					break;
				}

				if (view->HitTest(posf))
				{
					break;
				}
			}
			break;
		}
	}
}

void ff::XamlGlobalState::OnFocusView(XamlView* view, bool focused)
{
	if (focused)
	{
		if (_focusedView && _focusedView != view)
		{
			_focusedView->SetFocus(false);
		}

		_focusedView = view;
	}
}

const ff::Vector<ff::XamlView*>& ff::XamlGlobalState::GetInputViews() const
{
	return _inputViews;
}

const ff::Vector<ff::XamlView*>& ff::XamlGlobalState::GetRenderedViews() const
{
	return _renderedViews;
}

void ff::XamlGlobalState::OnRenderView(XamlView* view)
{
	_renderedViews.Push(view);

	if (view->IsEnabled())
	{
		if (view->IsInputBelowBlocked())
		{
			_inputViews.Clear();
		}

		_inputViews.Push(view);
	}
}

void ff::XamlGlobalState::OnFrameRendering(ff::AppGlobals* globals, ff::AdvanceType type)
{
	// OnRenderView will need to be called again for each view actually rendered
	_inputViews.Clear();
	_renderedViews.Clear();
}

void ff::XamlGlobalState::OnFrameRendered(ff::AppGlobals* globals, ff::AdvanceType type, ff::IRenderTarget* target, ff::IRenderDepth* depth)
{
	// Fix focus among all views that were actually rendered

	if (_focusedView)
	{
		bool focus = _inputViews.Contains(_focusedView) && _appGlobals->GetAppActive();
		_focusedView->SetFocus(focus);
	}

	if ((!_focusedView || !_focusedView->IsFocused()) && _inputViews.Size() && _appGlobals->GetAppActive())
	{
		// No visible view has focus, so choose a new one
		XamlView* view = _inputViews.GetLast();
		view->SetFocus(true);
		assert(_focusedView == view);
	}
}

void ff::XamlGlobalState::StaticUpdateCursorCallback(void* user, Noesis::IView* view, Noesis::Cursor cursor)
{
	ff::XamlGlobalState* globals = reinterpret_cast<ff::XamlGlobalState*>(user);
	globals->UpdateCursorCallback(view, cursor);
}

void ff::XamlGlobalState::StaticOpenUrlCallback(void* user, const char* url)
{
	ff::XamlGlobalState* globals = reinterpret_cast<ff::XamlGlobalState*>(user);
	globals->OpenUrlCallback(ff::String::from_utf8(url));
}

void ff::XamlGlobalState::StaticPlaySoundCallback(void* user, const char* filename, float volume)
{
	ff::XamlGlobalState* globals = reinterpret_cast<ff::XamlGlobalState*>(user);
	globals->PlaySoundCallback(ff::String::from_utf8(filename), volume);
}

void ff::XamlGlobalState::StaticSoftwareKeyboardCallback(void* user, Noesis::UIElement* focused, bool open)
{
	ff::XamlGlobalState* globals = reinterpret_cast<ff::XamlGlobalState*>(user);
	globals->SoftwareKeyboardCallback(focused, open);
}

void NsRegisterReflectionAppInteractivity();

void ff::XamlGlobalState::RegisterComponents()
{
	::NsRegisterReflectionAppInteractivity();

	Noesis::RegisterComponent<ff::BoolToCollapsedConverter>();
	Noesis::RegisterComponent<ff::BoolToObjectConverter>();
	Noesis::RegisterComponent<ff::BoolToVisibleConverter>();
	Noesis::RegisterComponent<ff::SetPanelChildFocusAction>();
}

void ff::XamlGlobalState::UpdateCursorCallback(Noesis::IView* view, Noesis::Cursor cursor)
{
	for (XamlView* xamlView : _views)
	{
		if (xamlView->GetView() == view)
		{
			xamlView->SetCursor(cursor);
			break;
		}
	}
}

void ff::XamlGlobalState::OpenUrlCallback(ff::StringRef url)
{
#if METRO_APP
	Platform::String^ purl = url.pstring();
	ff::GetMainThreadDispatch()->Post([purl]()
		{
			Windows::System::Launcher::LaunchUriAsync(ref new Windows::Foundation::Uri(purl));
		});
#else
	::ShellExecute(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

void ff::XamlGlobalState::PlaySoundCallback(ff::StringRef filename, float volume)
{
}

void ff::XamlGlobalState::SoftwareKeyboardCallback(Noesis::UIElement* focused, bool open)
{
}
