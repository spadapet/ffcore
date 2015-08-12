#include "pch.h"
#include "Dict/ValueTable.h"
#include "Globals/AppGlobals.h"
#include "Globals/Log.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
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
#include "UI/XamlConverters.h"
#include "UI/XamlGlobalState.h"
#include "UI/XamlViewState.h"

#define DEBUG_MEM_ALLOC 0

static ff::XamlGlobalState* s_globals;
static Noesis::AssertHandler s_assertHandler;
static Noesis::ErrorHandler s_errorHandler;

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

// The provider ID is a hash of the name, not a random GUID. That way listeners only need to know the name.
// {ec820af9-537e-5097-eb27-b527653f73b5}
TRACELOGGING_DEFINE_PROVIDER(s_traceLoggingProvider, "Microsoft.VisualStudio.DesignTools.XamlTrace",
	(0xec820af9, 0x537e, 0x5097, 0xeb, 0x27, 0xb5, 0x27, 0x65, 0x3f, 0x73, 0xb5));

static void OnBindingFailure(const char* message)
{
	if (::TraceLoggingProviderEnabled(s_traceLoggingProvider, 0, 0))
	{
		static std::regex bindingFailedRegex("Binding failed: Path=(.*?), Source=(.*?)\\('.*?'\\), Target=(.*?)\\('.*?'\\), TargetProperty=(.*)");

		std::cmatch matches;
		if (std::regex_match(message, matches, bindingFailedRegex) && matches.size() == 5)
		{
			std::string pathMatch = matches[1].str();
			std::string sourceMatch = matches[2].str();
			std::string targetMatch = matches[3].str();
			std::string targetPropertyMatch = matches[4].str();

			TraceLoggingWrite(s_traceLoggingProvider, "BindingFailed",
				TraceLoggingUtf8String("Error", "Severity"),
				TraceLoggingUtf8String(sourceMatch.c_str(), "SourceType"),
				TraceLoggingUtf8String(pathMatch.c_str(), "Path"),
				TraceLoggingUtf8String(targetPropertyMatch.c_str(), "TargetProperty"),
				TraceLoggingUtf8String(targetMatch.c_str(), "TargetType"),
				TraceLoggingUtf8String(message, "Description"));
		}
		else
		{
			TraceLoggingWrite(s_traceLoggingProvider, "BindingFailed",
				TraceLoggingUtf8String("Warning", "Severity"),
				TraceLoggingUtf8String(message, "Description"));
		}
	}
}

static void NoesisLogHandler(const char* filename, uint32_t line, uint32_t level, const char* channel, const char* message)
{
	const wchar_t* logLevel = (level < NS_COUNTOF(s_logLevels)) ? s_logLevels[level] : L"";
	ff::String channel2 = ff::String::from_utf8(channel);
	ff::String message2 = ff::String::from_utf8(message);

	ff::Log::GlobalTraceF(L"[NOESIS/%s/%s] %s\n", channel2.c_str(), logLevel, message2.c_str());

	if (channel2 == L"Binding")
	{
		OnBindingFailure(message);
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

	return s_assertHandler(file, line, expr);
}

static void NoesisErrorHandler(const char* file, uint32_t line, const char* message, bool fatal)
{
#ifdef _DEBUG
	if (::IsDebuggerPresent())
	{
		__debugbreak();
	}
#endif

	return s_errorHandler(file, line, message, fatal);
}

static void* NoesisAlloc(void* user, size_t size)
{
	void* ptr = std::malloc(size);
#if DEBUG_MEM_ALLOC
	ff::Log::DebugTraceF(L"[NOESIS/Mem] ALLOC: %lu (%lu)\n", ptr, size);
#endif
	return ptr;
}

static void* NoesisRealloc(void* user, void* ptr, size_t size)
{
	void* ptr2 = std::realloc(ptr, size);
#if DEBUG_MEM_ALLOC
	ff::Log::DebugTraceF(L"[NOESIS/Mem] REALLOC: %lu -> %lu (%lu)\n", ptr, ptr2, size);
#endif
	return ptr2;
}

static void NoesisDealloc(void* user, void* ptr)
{
#if DEBUG_MEM_ALLOC
	ff::Log::DebugTraceF(L"[NOESIS/Mem] FREE: %lu\n", ptr);
#endif
	return std::free(ptr);
}

static size_t NoesisAllocSize(void* user, void* ptr)
{
	size_t size = _msize(ptr);
#if DEBUG_MEM_ALLOC
	ff::Log::DebugTraceF(L"[NOESIS/Mem] SIZE: %lu (%lu)\n", ptr, size);
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
	, _values(nullptr)
	, _focusedView(nullptr)
{
	assert(!s_globals);
	s_globals = this;
}

ff::XamlGlobalState::~XamlGlobalState()
{
	assert(s_globals == this);
	s_globals = nullptr;
}

bool ff::XamlGlobalState::Startup(ff::IResourceAccess* resources, ff::IValueAccess* values, ff::StringRef resourcesName, bool sRGB)
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());
	assertRetVal(!s_assertHandler && !_resources && resources && values && _appGlobals->GetGraph()->AsGraphDevice11(), false);

	::TraceLoggingRegister(s_traceLoggingProvider);

	Noesis::GUI::Init(nullptr, ::NoesisLogHandler, &s_memoryCallbacks);
	s_assertHandler = Noesis::SetAssertHandler(::NoesisAssertHandler);
	s_errorHandler = Noesis::SetErrorHandler(::NoesisErrorHandler);

	_resources = resources;
	_values = values;
	_renderDevice = Noesis::MakePtr<XamlRenderDevice11>(_appGlobals->GetGraph(), sRGB);
	_xamlProvider = Noesis::MakePtr<XamlProvider>(this);
	_fontProvider = Noesis::MakePtr<XamlFontProvider>(this);
	_textureProvider = Noesis::MakePtr<XamlTextureProvider>(this);

	Noesis::GUI::SetCursorCallback(this, ff::XamlGlobalState::StaticUpdateCursorCallback);
	Noesis::GUI::SetOpenUrlCallback(this, ff::XamlGlobalState::StaticOpenUrlCallback);
	Noesis::GUI::SetPlaySoundCallback(this, ff::XamlGlobalState::StaticPlaySoundCallback);
	Noesis::GUI::SetSoftwareKeyboardCallback(this, ff::XamlGlobalState::StaticSoftwareKeyboardCallback);

	Noesis::GUI::SetXamlProvider(_xamlProvider);
	Noesis::GUI::SetTextureProvider(_textureProvider);
	Noesis::GUI::SetFontProvider(_fontProvider);

	RegisterComponents();

	if (!resourcesName.empty())
	{
		ff::Vector<char> resourcesUtf8 = ff::StringToUTF8(resourcesName);
		_applicationResources = Noesis::GUI::LoadXaml<Noesis::ResourceDictionary>(resourcesUtf8.ConstData());
		assertRetVal(_applicationResources, false);
	}

	Noesis::GUI::SetApplicationResources(_applicationResources);

	::NeosisDumpMemUsage();

	return true;
}

void ff::XamlGlobalState::Shutdown()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

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
	_values = nullptr;

	Noesis::GUI::Shutdown();

	::NeosisDumpMemUsage();
	assertSz(!Noesis::GetAllocatedMemory(), L"NOESIS Memory Leak");

	Noesis::SetErrorHandler(s_errorHandler);
	s_errorHandler = nullptr;

	Noesis::SetAssertHandler(s_assertHandler);
	s_assertHandler = nullptr;

	::TraceLoggingUnregister(s_traceLoggingProvider);
}

std::shared_ptr<ff::XamlView> ff::XamlGlobalState::CreateView(ff::StringRef xamlFile, ff::IRenderTarget* target, bool perPixelAntiAlias)
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::Vector<char> xamlFile8 = ff::StringToUTF8(xamlFile);
	return CreateView(Noesis::GUI::LoadXaml<Noesis::FrameworkElement>(xamlFile8.ConstData()), target, perPixelAntiAlias);
}

std::shared_ptr<ff::XamlView> ff::XamlGlobalState::CreateView(Noesis::FrameworkElement* content, ff::IRenderTarget* target, bool perPixelAntiAlias)
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	assertRetVal(content, nullptr);
	return std::make_shared<ff::XamlView11>(this, content, target, perPixelAntiAlias);
}

std::shared_ptr<ff::XamlViewState> ff::XamlGlobalState::CreateViewState(std::shared_ptr<XamlView> view, ff::IRenderTarget* target)
{
	return std::make_shared<XamlViewState>(view, target);
}

ff::AppGlobals* ff::XamlGlobalState::GetAppGlobals() const
{
	return _appGlobals;
}

ff::IResourceAccess* ff::XamlGlobalState::GetResourceAccess() const
{
	return _resources;
}

ff::IValueAccess* ff::XamlGlobalState::GetValueAccess() const
{
	return _values;
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

	if (_focusedView == view)
	{
		_focusedView = nullptr;
	}
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

void ff::XamlGlobalState::OnRenderView(XamlView* view)
{
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

void ff::XamlGlobalState::RegisterComponents()
{
	NsRegisterComponent<ff::BoolToCollapsedConverter>();
	NsRegisterComponent<ff::BoolToObjectConverter>();
	NsRegisterComponent<ff::BoolToVisibleConverter>();
}

void ff::XamlGlobalState::UpdateCursorCallback(Noesis::IView* view, Noesis::Cursor cursor)
{
}

void ff::XamlGlobalState::OpenUrlCallback(ff::StringRef url)
{
}

void ff::XamlGlobalState::PlaySoundCallback(ff::StringRef filename, float volume)
{
}

void ff::XamlGlobalState::SoftwareKeyboardCallback(Noesis::UIElement* focused, bool open)
{
}
