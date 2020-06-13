#include "pch.h"
#include "Dict/ValueTable.h"
#include "Globals/AppGlobalsHelper.h"
#include "MainUtilInclude.h"
#include "Module/Module.h"
#include "Resource/Resources.h"
#include "State/States.h"
#include "States/TestEntityState.h"
#include "States/TestPaletteState.h"
#include "States/TestUiState.h"
#include "States/TitleState.h"
#include "String/StringUtil.h"
#include "UI/XamlGlobalState.h"

// {3930FE5D-70FE-42E6-93DA-574235C1A187}
static const GUID s_moduleId = { 0x3930fe5d, 0x70fe, 0x42e6,{ 0x93, 0xda, 0x57, 0x42, 0x35, 0xc1, 0xa1, 0x87 } };
static ff::StaticString s_moduleName(L"Proto");
static ff::ModuleFactory CreateThisModule(s_moduleName, s_moduleId, ff::GetDelayLoadInstance, ff::GetModuleStartup);

class AppGlobalsHelper : public ff::IAppGlobalsHelper
{
public:
	virtual void OnGameThreadInitialized(ff::AppGlobals* globals) override
	{
		_uiGlobals = std::make_shared<ff::XamlGlobalState>(globals);
		_uiGlobals->Startup(
			ff::String::from_static(L"9e6fb182-647d-454a-8f95-fcdf88e3c3c2"),
			ff::String::from_static(L"g8nV9oGB1fZ5EP22GHDZv3T6uCQdsGyA3YlNsw6AFmDSr4IV"),
			ff::GetThisModule().GetResources(),
			ff::GetThisModule().GetValueTable(),
			ff::String::from_static(L"ApplicationResources.xaml"));
	}

	virtual void OnGameThreadShutdown(ff::AppGlobals* globals) override
	{
		if (_uiGlobals != nullptr)
		{
			_uiGlobals->Shutdown();
			_uiGlobals = nullptr;
		}
	}

	virtual std::shared_ptr<ff::State> CreateInitialState(ff::AppGlobals* globals) override
	{
		auto states = std::make_shared<ff::States>();
		//states->AddTop(std::make_shared<TestUiState>(_uiGlobals.get()));
		//states->AddTop(std::make_shared<TestEntityState>(globals));
		//states->AddTop(std::make_shared<TestPaletteState>(globals));
		states->AddTop(std::make_shared<TitleState>(globals));
		states->AddTop(_uiGlobals);
		return states;
	}

private:
	std::shared_ptr<ff::XamlGlobalState> _uiGlobals;
};

#if METRO_APP

#include "App.xaml.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^ args)
{
	HINSTANCE instance = (HINSTANCE)&__ImageBase;
	ff::SetCommandLineArgs(args);
	ff::SetMainModule(s_moduleName, s_moduleId, instance);

	AppGlobalsHelper helper;
	auto appCallback = ref new Windows::UI::Xaml::ApplicationInitializationCallback([&helper](Windows::UI::Xaml::ApplicationInitializationCallbackParams^ args)
		{
			auto app = ref new ::Proto::App(&helper);
		});

	Windows::UI::Xaml::Application::Start(appCallback);

	return 0;
}

#else

#include "Globals/DesktopGlobals.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR commandLine, int showCommand)
{
	ff::SetMainModule(s_moduleName, s_moduleId, instance);

	return ff::DesktopGlobals::RunWithWindow(ff::AppGlobalsFlags::All, AppGlobalsHelper()) ? 0 : 1;
}

#endif
