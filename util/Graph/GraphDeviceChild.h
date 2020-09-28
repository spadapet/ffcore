#pragma once

namespace ff
{
	class IGraphDevice;

	class IGraphDeviceChild
	{
	public:
		virtual IGraphDevice* GetDevice() const = 0;
		virtual bool Reset() = 0;
	};
}
