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

ff::IPalette* ff::IXamlGlobalHelper::GetPalette()
{
	return nullptr;
}

ff::String ff::IXamlGlobalHelper::GetNoesisLicenseName()
{
	return ff::GetEmptyString();
}

ff::String ff::IXamlGlobalHelper::GetNoesisLicenseKey()
{
	return ff::GetEmptyString();
}

ff::String ff::IXamlGlobalHelper::GetApplicationResourcesName()
{
	return ff::GetEmptyString();
}

ff::String ff::IXamlGlobalHelper::GetDefaultFont()
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
