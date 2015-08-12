#pragma once

#include "App.g.h"
#include "Globals/MetroGlobals.h"
#include "Globals/ProcessGlobals.h"

namespace ff
{
	class IAppGlobalsHelper;
}

namespace Proto
{
	ref class MainPage;

	ref class App sealed
	{
	public:
		virtual ~App();

		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^args) override;

	internal:
		App(ff::IAppGlobalsHelper* helper);

	private:
		void InitializeGlobals();

		ff::ProcessGlobals _processGlobals;
		ff::MetroGlobals _globals;
		ff::IAppGlobalsHelper* _helper;
	};
}
