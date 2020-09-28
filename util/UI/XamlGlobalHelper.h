#pragma once

namespace ff
{
	class IPalette;
	class IResourceAccess;

	class IXamlGlobalHelper
	{
	public:
		UTIL_API virtual ~IXamlGlobalHelper();

		UTIL_API virtual ff::IResourceAccess* GetXamlResources();
		UTIL_API virtual ff::IPalette* GetPalette();
		UTIL_API virtual ff::String GetNoesisLicenseName();
		UTIL_API virtual ff::String GetNoesisLicenseKey();
		UTIL_API virtual ff::String GetApplicationResourcesName();
		UTIL_API virtual ff::String GetDefaultFont();
		UTIL_API virtual float GetDefaultFontSize();
		UTIL_API virtual bool IsSRGB();

		UTIL_API virtual void RegisterNoesisComponents();
		UTIL_API virtual void OnApplicationResourcesLoaded(Noesis::ResourceDictionary* resources);
	};
}
