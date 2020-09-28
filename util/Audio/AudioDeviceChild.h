#pragma once

namespace ff
{
	class IAudioDevice;

	class IAudioDeviceChild
	{
	public:
		virtual IAudioDevice* GetDevice() const = 0;
		virtual bool Reset() = 0;
	};
}
