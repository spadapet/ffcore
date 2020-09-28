#pragma once

namespace ff
{
	class IData;

	class __declspec(uuid("34549a41-ce57-4b8f-85b0-53cd5820c2dd")) __declspec(novtable)
		IAudioBuffer : public IUnknown
	{
	public:
		virtual const WAVEFORMATEX& GetFormat() const = 0;
		virtual IData* GetData() const = 0;
	};
}
