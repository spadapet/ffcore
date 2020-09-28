#pragma once

namespace ff
{
	class IAudioDevice;
	class IXAudioFactory;

	class __declspec(uuid("90cc78de-7832-436f-8325-d3d2e2ee4330")) __declspec(novtable)
		IAudioFactory : public IUnknown
	{
	public:
		virtual ComPtr<IAudioDevice> CreateDevice() = 0;
		virtual size_t GetDeviceCount() const = 0;
		virtual IAudioDevice* GetDevice(size_t nIndex) const = 0;

		virtual void AddChild(IAudioDevice* child) = 0;
		virtual void RemoveChild(IAudioDevice* child) = 0;

		virtual IXAudioFactory* AsXAudioFactory() = 0;
	};

	class IXAudioFactory
	{
	public:
		virtual IXAudio2* GetAudio() = 0;
	};

	ComPtr<IAudioFactory> CreateAudioFactory();
}
