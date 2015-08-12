#include "pch.h"
#include "Data/Compression.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"

#include <zlib.h>

static size_t GetChunkSizeForDataSize(size_t nDataSize)
{
	static const size_t s_nMaxChunkSize = 1024 * 256;

	return std::min<size_t>(nDataSize, s_nMaxChunkSize);
}

bool ff::CompressData(const BYTE* pFullData, size_t nFullSize, IData** ppCompData, IChunkListener* pListener)
{
	assertRetVal(pFullData && ppCompData, false);

	ComPtr<IData> pData;
	assertRetVal(CreateDataInStaticMem(pFullData, nFullSize, &pData), false);

	return CompressData(pData, ppCompData, pListener);
}

bool ff::UncompressData(const BYTE* pCompData, size_t nCompSize, size_t nFullSize, IData** ppFullData, IChunkListener* pListener)
{
	assertRetVal(pCompData && ppFullData, false);

	ComPtr<IData> pData;
	assertRetVal(CreateDataInStaticMem(pCompData, nCompSize, &pData), false);

	return UncompressData(pData, nFullSize, ppFullData, pListener);
}

bool ff::CompressData(IData* pData, IData** ppCompData, IChunkListener* pListener)
{
	assertRetVal(pData && ppCompData, false);

	ComPtr<IDataReader> pReader;
	assertRetVal(CreateDataReader(pData, 0, &pReader), false);

	ComPtr<IDataVector> pCompData;
	ComPtr<IDataWriter> pWriter;
	assertRetVal(CreateDataWriter(&pCompData, &pWriter), false);

	assertRetVal(CompressData(pReader, pData->GetSize(), pWriter, pListener), false);

	*ppCompData = pCompData.Detach();
	return true;
}

bool ff::UncompressData(IData* pData, size_t nFullSize, IData** ppFullData, IChunkListener* pListener)
{
	assertRetVal(pData && ppFullData, false);

	ComPtr<IDataReader> pReader;
	assertRetVal(CreateDataReader(pData, 0, &pReader), false);

	ComPtr<IDataVector> pFullData;
	ComPtr<IDataWriter> pWriter;
	assertRetVal(CreateDataWriter(&pFullData, &pWriter), false);

	assertRetVal(UncompressData(pReader, pData->GetSize(), pWriter, pListener), false);
	assertRetVal(nFullSize == INVALID_SIZE || nFullSize == pFullData->GetSize(), false);

	*ppFullData = pFullData.Detach();
	return true;
}

bool ff::CompressData(IDataReader* pInput, size_t nFullSize, IDataWriter* pOutput, IChunkListener* pListener)
{
	assertRetVal(pInput && pOutput, false);

	// Init zlib's buffer
	z_stream zlibData;
	ZeroObject(zlibData);
	deflateInit(&zlibData, Z_BEST_COMPRESSION);

	// Init my output buffer
	Vector<BYTE> outputChunk;
	outputChunk.Resize(GetChunkSizeForDataSize(nFullSize));

	bool bStatus = true;
	size_t nProgress = 0;

	for (size_t nPos = 0, nInputChunkSize = GetChunkSizeForDataSize(nFullSize);
		bStatus && nPos < nFullSize; nPos += nInputChunkSize)
	{
		// Read a chunk of input and get ready to pass it to zlib

		size_t nRead = std::min(nFullSize - nPos, nInputChunkSize);
		const BYTE* pChunk = pInput->Read(nRead);
		bool bLastChunk = nPos + nInputChunkSize >= nFullSize;

		if (pChunk)
		{
			zlibData.avail_in = (uInt)nRead;
			zlibData.next_in = (Bytef*)pChunk;

			do
			{
				zlibData.avail_out = (uInt)outputChunk.Size();
				zlibData.next_out = outputChunk.Data();
				deflate(&zlibData, bLastChunk ? Z_FINISH : Z_NO_FLUSH);

				size_t nWrite = outputChunk.Size() - zlibData.avail_out;
				if (nWrite)
				{
					bStatus = pOutput->Write(outputChunk.Data(), nWrite);

					if (bStatus && pListener)
					{
						nProgress += nRead;

						if (!pListener->OnChunk(nRead, nProgress, nFullSize))
						{
							bStatus = false;
							break;
						}
					}
				}
			} while (bStatus && !zlibData.avail_out);
			assert(!zlibData.avail_in);
		}
		else
		{
			bStatus = false;
		}
	}

	deflateEnd(&zlibData);

	if (pListener)
	{
		if (bStatus)
		{
			assert(nProgress == nFullSize);
			pListener->OnChunkSuccess(nFullSize);
		}
		else
		{
			pListener->OnChunkFailure(nProgress, nFullSize);
		}
	}

	assert(bStatus);
	return bStatus;
}

bool ff::UncompressData(IDataReader* pInput, size_t nCompSize, IDataWriter* pOutput, IChunkListener* pListener)
{
	assertRetVal(pInput && pOutput, false);

	if (!nCompSize)
	{
		return true;
	}

	// Init zlib's buffer
	z_stream zlibData;
	ZeroObject(zlibData);
	inflateInit(&zlibData);

	// Init my output buffer
	Vector<BYTE> outputChunk;
	outputChunk.Resize(GetChunkSizeForDataSize(nCompSize * 2));

	bool bStatus = true;
	int nInflateStatus = Z_OK;
	size_t nProgress = 0;
	size_t nPos = 0;

	for (size_t nInputChunkSize = GetChunkSizeForDataSize(nCompSize);
		bStatus && nPos < nCompSize; nPos = std::min(nPos + nInputChunkSize, nCompSize))
	{
		// Read a chunk of input and get ready to pass it to zlib

		size_t nRead = std::min(nCompSize - nPos, nInputChunkSize);
		const BYTE* pChunk = pInput->Read(nRead);

		if (pChunk)
		{
			zlibData.avail_in = (uInt)nRead;
			zlibData.next_in = (Bytef*)pChunk;

			do
			{
				zlibData.avail_out = (uInt)outputChunk.Size();
				zlibData.next_out = outputChunk.Data();
				nInflateStatus = inflate(&zlibData, Z_NO_FLUSH);

				bStatus =
					nInflateStatus != Z_NEED_DICT &&
					nInflateStatus != Z_DATA_ERROR &&
					nInflateStatus != Z_MEM_ERROR;

				size_t nWrite = outputChunk.Size() - zlibData.avail_out;

				if (nWrite)
				{
					bStatus = pOutput->Write(outputChunk.Data(), nWrite);

					if (pListener && bStatus)
					{
						nProgress += nRead;

						if (!pListener->OnChunk(nRead, nProgress, nCompSize))
						{
							bStatus = false;
							break;
						}
					}
				}
			} while (bStatus && !zlibData.avail_out);
			assert(!zlibData.avail_in);
		}
		else
		{
			bStatus = false;
		}
	}

	bStatus = (nPos == nCompSize && nInflateStatus == Z_STREAM_END);

	inflateEnd(&zlibData);

	if (pListener)
	{
		if (bStatus)
		{
			assert(nProgress == nCompSize);
			pListener->OnChunkSuccess(nCompSize);
		}
		else
		{
			pListener->OnChunkFailure(nProgress, nCompSize);
		}
	}

	assert(bStatus);
	return bStatus;
}

static BYTE CHAR_TO_BYTE[] =
{
	62, // +
	0,
	0,
	0,
	63, // /
	52, // 0
	53, // 1
	54, // 2
	55, // 3
	56, // 4
	57, // 5
	58, // 6
	59, // 7
	60, // 8
	61, // 9
	0, // :
	0, // ;
	0, // <
	0, // =
	0, // >
	0, // ?
	0, // @
	0, // A
	1, // B
	2, // C
	3, // D
	4, // E
	5, // F
	6, // G
	7, // H
	8, // I
	9, // J
	10, // K
	11, // L
	12, // M
	13, // N
	14, // O
	15, // P
	16, // Q
	17, // R
	18, // S
	19, // T
	20, // U
	21, // V
	22, // W
	23, // X
	24, // Y
	25, // Z
	0, // [
	0, // slash
	0, // ]
	0, // ^
	0, // _
	0, // `
	26, // a
	27, // b
	28, // c
	29, // d
	30, // e
	31, // f
	32, // g
	33, // h
	34, // i
	35, // j
	36, // k
	37, // l
	38, // m
	39, // n
	40, // o
	41, // p
	42, // q
	43, // r
	44, // s
	45, // t
	46, // u
	47, // v
	48, // w
	49, // x
	50, // y
	51, // z
};

inline static BYTE CharToByte(wchar_t ch)
{
	if (ch >= '+' && ch <= 'z')
	{
		return ::CHAR_TO_BYTE[ch - '+'];
	}

	return 0;
}

ff::ComPtr<ff::IData> ff::DecodeBase64(ff::StringRef input)
{
	// Input string needs to be properly padded
	assertRetVal(input.size() % 4 == 0, nullptr);

	ff::Vector<BYTE> out;
	out.Resize(input.size() / 4 * 3);
	BYTE* pout = out.Data();

	const wchar_t* end = input.c_str() + input.size();
	for (const wchar_t* ch = input.c_str(); ch < end; ch += 4, pout += 3)
	{
		BYTE ch0 = ::CharToByte(ch[0]);
		BYTE ch1 = ::CharToByte(ch[1]);
		BYTE ch2 = ::CharToByte(ch[2]);
		BYTE ch3 = ::CharToByte(ch[3]);

		pout[0] = ((ch0 & 0b00111111) << 2) | ((ch1 & 0b00110000) >> 4);
		pout[1] = ((ch1 & 0b00001111) << 4) | ((ch2 & 0b00111100) >> 2);
		pout[2] = ((ch2 & 0b00000011) << 6) | ((ch3 & 0b00111111) >> 0);
	}

	if (input.size())
	{
		if (end[-2] == '=')
		{
			verify(out.Pop() == 0);
		}

		if (end[-1] == '=')
		{
			verify(out.Pop() == 0);
		}
	}

	return ff::CreateDataVector(std::move(out)).Interface();
}
