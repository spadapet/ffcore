#include "pch.h"
#include "Globals/AppGlobalsHelper.h"
#include "Module/Module.h"

ff::IAppGlobalsHelper::~IAppGlobalsHelper()
{
}

bool ff::IAppGlobalsHelper::OnInitialized(ff::AppGlobals* globals)
{
	return true;
}

void ff::IAppGlobalsHelper::OnShutdown(ff::AppGlobals* globals)
{
}

void ff::IAppGlobalsHelper::OnGameThreadInitialized(ff::AppGlobals* globals)
{
}

void ff::IAppGlobalsHelper::OnGameThreadShutdown(ff::AppGlobals* globals)
{
}

std::shared_ptr<ff::State> ff::IAppGlobalsHelper::CreateInitialState(ff::AppGlobals* globals)
{
	return nullptr;
}

double ff::IAppGlobalsHelper::GetTimeScale(ff::AppGlobals* globals)
{
	return 1.0;
}

ff::AdvanceType ff::IAppGlobalsHelper::GetAdvanceType(ff::AppGlobals* globals)
{
	return ff::AdvanceType::Running;
}

ff::String ff::IAppGlobalsHelper::GetWindowName()
{
	return ff::GetMainModule()->GetName();
}
