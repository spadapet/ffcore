#include "pch.h"
#include "App.xaml.h"
#include "Thread/ThreadDispatch.h"

Proto::App::App(ff::IAppGlobalsHelper* helper)
	: _helper(helper)
{
	InitializeComponent();
}

Proto::App::~App()
{
	_globals.Shutdown();
	_processGlobals.Shutdown();
}

void Proto::App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args)
{
	if (!_processGlobals.IsValid())
	{
		_processGlobals.Startup();

		Platform::WeakReference weakThis(this);
		ff::GetMainThreadDispatch()->Post([weakThis]()
			{
				weakThis.Resolve<App>()->InitializeGlobals();
			});
	}
}

void Proto::App::InitializeGlobals()
{
	auto window = Windows::UI::Xaml::Window::Current;
	auto page = ref new Windows::UI::Xaml::Controls::Page();
	auto panel = ref new Windows::UI::Xaml::Controls::SwapChainPanel();

	page->Content = panel;
	window->Content = page;

	_globals.Startup(ff::AppGlobalsFlags::All, panel, _helper);

	window->Activate();
}
