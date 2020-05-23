#pragma once

namespace ff
{
	class IData;
	class IDataReader;
	class IDataVector;

	// For standard COM IStreams:
	UTIL_API bool CreateWriteStream(IDataVector* pData, size_t nPos, IStream** ppStream);
	UTIL_API bool CreateWriteStream(IDataVector** ppData, IStream** ppStream); // shortcut
	UTIL_API bool CreateReadStream(IData* pData, size_t nPos, IStream** ppStream);
	UTIL_API bool CreateReadStream(IDataReader* reader, IStream** obj);
	UTIL_API bool CreateReadStream(IDataReader* reader, StringRef mimeType, IMFByteStream** obj);

	class __declspec(uuid("a7004a46-443a-4518-ae11-5f76dd59efc5")) __declspec(novtable)
		IFileStream : public IUnknown
	{
	public:
		virtual bool CreateReader(IDataReader** obj) = 0;
	};
}
