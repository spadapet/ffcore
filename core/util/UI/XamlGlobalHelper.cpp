#include "pch.h"
#include "Module/Module.h"
#include "Resource/Resources.h"
#include "UI/XamlGlobalHelper.h"

ff::IXamlGlobalHelper::~IXamlGlobalHelper()
{
}

ff::IResourceAccess* ff::IXamlGlobalHelper::GetXamlResources()
{
	return ff::GetThisModule().GetResources();
}

UTIL_API ff::String ff::IXamlGlobalHelper::GetNoesisLicenseName()
{
	return ff::GetEmptyString();
}

UTIL_API ff::String ff::IXamlGlobalHelper::GetNoesisLicenseKey()
{
	return ff::GetEmptyString();
}

UTIL_API ff::String ff::IXamlGlobalHelper::GetApplicationResourcesName()
{
	return ff::GetEmptyString();
}

UTIL_API ff::String ff::IXamlGlobalHelper::GetDefaultFont()
{
	return ff::String::from_static(L"Segoe UI");
}

float ff::IXamlGlobalHelper::GetDefaultFontSize()
{
	return 12.0f;
}

bool ff::IXamlGlobalHelper::IsSRGB()
{
	return false;
}

void ff::IXamlGlobalHelper::RegisterNoesisComponents()
{
}

void ff::IXamlGlobalHelper::OnApplicationResourcesLoaded(Noesis::ResourceDictionary* resources)
{
}
