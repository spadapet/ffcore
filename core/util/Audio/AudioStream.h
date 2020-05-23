#pragma once

namespace ff
{
	class IDataReader;

	class __declspec(uuid("9cd8283a-7c8e-4c15-866d-1e8f3f7eccd2")) __declspec(novtable)
		IAudioStream : public IUnknown
	{
	public:
		virtual bool CreateReader(IDataReader** obj) = 0;
		virtual ff::StringRef GetMimeType() const = 0;
	};
}
