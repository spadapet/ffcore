#include "pch.h"
#include "Data/Data.h"
#include "Data/DataWriterReader.h"
#include "Graph/Texture/PngImage.h"

ff::PngImageReader::PngImageReader(const unsigned char* bytes, size_t size)
	: _png(nullptr)
	, _info(nullptr)
	, _endInfo(nullptr)
	, _readPos(bytes)
	, _endPos(bytes + size)
	, _width(0)
	, _height(0)
	, _bitDepth(0)
	, _colorType(0)
	, _interlaceMethod(0)
	, _hasPalette(false)
	, _palette(nullptr)
	, _paletteSize(0)
	, _hasTransPalette(false)
	, _transPalette(nullptr)
	, _transPaletteSize(0)
	, _transColor(nullptr)
{
	_png = ::png_create_read_struct(PNG_LIBPNG_VER_STRING, this, &PngImageReader::PngErrorCallback, &PngImageReader::PngWarningCallback);
	_info = ::png_create_info_struct(_png);
	_endInfo = ::png_create_info_struct(_png);
}

ff::PngImageReader::~PngImageReader()
{
	::png_destroy_read_struct(&_png, &_info, &_endInfo);
}

std::unique_ptr<DirectX::ScratchImage> ff::PngImageReader::Read(DXGI_FORMAT requestedFormat)
{
	std::unique_ptr<DirectX::ScratchImage> scratch;

	try
	{
		scratch = InternalRead(requestedFormat);
		if (!scratch && _errorText.empty())
		{
			_errorText = L"Failed to read PNG data";
		}
	}
	catch (ff::String errorText)
	{
		_errorText = !errorText.empty() ? errorText : ff::String(L"Failed to read PNG data");
	}

	return scratch;
}

ff::Vector<BYTE> ff::PngImageReader::GetPalette() const
{
	ff::Vector<BYTE> colors;

	if (_hasPalette)
	{
		colors.Resize(256 * 4);
		std::memset(colors.Data(), 0, colors.ByteSize());

		for (size_t i = 0; i < (size_t)_paletteSize; i++)
		{
			colors[i * 4 + 0] = _palette[i].red;
			colors[i * 4 + 1] = _palette[i].green;
			colors[i * 4 + 2] = _palette[i].blue;
			colors[i * 4 + 3] = (_hasTransPalette && i < (size_t)_transPaletteSize) ? _transPalette[i] : 0xFF;
		}
	}

	return colors;
}

ff::StringRef ff::PngImageReader::GetError() const
{
	return _errorText;
}

std::unique_ptr<DirectX::ScratchImage> ff::PngImageReader::InternalRead(DXGI_FORMAT requestedFormat)
{
	if (::png_sig_cmp(_readPos, 0, _endPos - _readPos) != 0)
	{
		return nullptr;
	}

	::png_set_read_fn(_png, this, &PngImageReader::PngReadCallback);
	::png_set_keep_unknown_chunks(_png, PNG_HANDLE_CHUNK_NEVER, nullptr, 0);
	::png_read_info(_png, _info);

	// General image info
	if (!::png_get_IHDR(
		_png,
		_info,
		&_width,
		&_height,
		&_bitDepth,
		&_colorType,
		&_interlaceMethod,
		nullptr,
		nullptr))
	{
		return nullptr;
	}

	// Palette
	_hasPalette = ::png_get_PLTE(_png, _info, &_palette, &_paletteSize) != 0;
	_hasTransPalette = ::png_get_tRNS(_png, _info, &_transPalette, &_transPaletteSize, &_transColor) != 0;

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	switch (_colorType)
	{
	default:
		_errorText = L"Invalid color type";
		return nullptr;

	case PNG_COLOR_TYPE_GRAY:
		format = (_bitDepth == 1) ? DXGI_FORMAT_R1_UNORM : DXGI_FORMAT_R8_UNORM;
		if (requestedFormat != format)
		{
			::png_set_gray_to_rgb(_png);
			::png_set_add_alpha(_png, 0xFF, PNG_FILLER_AFTER);
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (_bitDepth > 1 && _bitDepth < 8)
		{
			::png_set_expand_gray_1_2_4_to_8(_png);
		}
		break;

	case PNG_COLOR_TYPE_PALETTE:
		format = DXGI_FORMAT_R8_UINT;
		if (requestedFormat != format)
		{
			::png_set_palette_to_rgb(_png);

			if (_hasTransPalette)
			{
				::png_set_tRNS_to_alpha(_png);
			}
			else
			{
				::png_set_add_alpha(_png, 0xFF, PNG_FILLER_AFTER);
			}

			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (_paletteSize < 256)
		{
			::png_set_packing(_png);
		}
		break;

	case PNG_COLOR_TYPE_RGB:
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		::png_set_add_alpha(_png, 0xFF, PNG_FILLER_AFTER);
		break;

	case PNG_COLOR_TYPE_RGB_ALPHA:
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;

	case PNG_COLOR_TYPE_GRAY_ALPHA:
		::png_set_gray_to_rgb(_png);
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	}

	std::unique_ptr<DirectX::ScratchImage> scratch = std::make_unique<DirectX::ScratchImage>();
	if (FAILED(scratch->Initialize2D(format, _width, _height, 1, 1)))
	{
		return nullptr;
	}

	_rows.Resize(_height);
	{
		const DirectX::Image& image = *scratch->GetImage(0, 0, 0);
		BYTE* imagePixels = image.pixels;

		for (unsigned int i = 0; i < _height; i++)
		{
			_rows[i] = &imagePixels[i * image.rowPitch];
		}
	}

	::png_read_image(_png, _rows.Data());
	::png_read_end(_png, _endInfo);

	return scratch;
}

void ff::PngImageReader::PngErrorCallback(png_struct* png, const char* text)
{
	PngImageReader* info = (PngImageReader*)::png_get_error_ptr(png);
	info->OnPngError(text);
}

void ff::PngImageReader::PngWarningCallback(png_struct* png, const char* text)
{
	PngImageReader* info = (PngImageReader*)::png_get_error_ptr(png);
	info->OnPngWarning(text);
}

void ff::PngImageReader::PngReadCallback(png_struct* png, unsigned char* data, size_t size)
{
	PngImageReader* info = (PngImageReader*)::png_get_io_ptr(png);
	info->OnPngRead(data, size);
}

void ff::PngImageReader::OnPngError(const char* text)
{
	ff::String wtext = ff::String::from_utf8(text);
	assertSz(false, wtext.c_str());
	throw wtext;
}

void ff::PngImageReader::OnPngWarning(const char* text)
{
	ff::String wtext = ff::String::from_utf8(text);
	assertSz(false, wtext.c_str());
}

void ff::PngImageReader::OnPngRead(unsigned char* data, size_t size)
{
	size = std::min<size_t>(size, _endPos - _readPos);
	::memcpy(data, _readPos, size);
	_readPos += size;
}

ff::PngImageWriter::PngImageWriter(ff::IDataWriter* writer)
	: _writer(writer)
{
	_png = ::png_create_write_struct(PNG_LIBPNG_VER_STRING, this, &PngImageWriter::PngErrorCallback, &PngImageWriter::PngWarningCallback);
	_info = ::png_create_info_struct(_png);
}

ff::PngImageWriter::~PngImageWriter()
{
	::png_destroy_write_struct(&_png, &_info);
}

bool ff::PngImageWriter::Write(const DirectX::Image& image, ff::IData* palette)
{
	try
	{
		return InternalWrite(image, palette);
	}
	catch (ff::String errorText)
	{
		_errorText = !errorText.empty() ? errorText : ff::String(L"Failed to write PNG data");
		return false;
	}
}

ff::StringRef ff::PngImageWriter::GetError() const
{
	return _errorText;
}

bool ff::PngImageWriter::InternalWrite(const DirectX::Image& image, IData* palette)
{
	int bitDepth = 8;
	int colorType;

	switch (image.format)
	{
	case DXGI_FORMAT_R8_UINT:
		colorType = PNG_COLOR_TYPE_PALETTE;
		break;

	case DXGI_FORMAT_R8G8B8A8_UNORM:
		colorType = PNG_COLOR_TYPE_RGB_ALPHA;
		break;

	default:
		_errorText = L"Unsupported texture format for saving to PNG";
		break;
	}

	::png_set_write_fn(_png, this, &PngImageWriter::PngWriteCallback, &PngImageWriter::PngFlushCallback);

	::png_set_IHDR(
		_png,
		_info,
		(unsigned int)image.width,
		(unsigned int)image.height,
		bitDepth,
		colorType,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	if (palette)
	{
		size_t colorCount = palette->GetSize() / 4;
		::png_color_16 transColor{ 0 };
		bool foundTrans = false;

		ff::Vector<::png_color> colors;
		colors.Resize(colorCount);

		ff::Vector<BYTE> trans;
		trans.Resize(colorCount);

		const BYTE* src = palette->GetMem();
		for (size_t i = 0; i < colorCount; i++, src += 4)
		{
			colors[i].red = src[0];
			colors[i].green = src[1];
			colors[i].blue = src[2];

			if (colorType == PNG_COLOR_TYPE_PALETTE)
			{
				trans[i] = src[3];
			}
		}

		::png_set_PLTE(_png, _info, colors.Data(), (int)colors.Size());

		if (foundTrans && colorType == PNG_COLOR_TYPE_PALETTE)
		{
			::png_set_tRNS(_png, _info, trans.Data(), (int)trans.Size(), &transColor);
		}
	}

	_rows.Resize(image.height);

	for (size_t i = 0; i < image.height; i++)
	{
		_rows[i] = &image.pixels[i * image.rowPitch];
	}

	::png_set_rows(_png, _info, _rows.Data());
	::png_write_png(_png, _info, PNG_TRANSFORM_IDENTITY, nullptr);

	return true;
}

void ff::PngImageWriter::PngErrorCallback(png_struct* png, const char* text)
{
	PngImageWriter* info = (PngImageWriter*)::png_get_error_ptr(png);
	info->OnPngError(text);
}

void ff::PngImageWriter::PngWarningCallback(png_struct* png, const char* text)
{
	PngImageWriter* info = (PngImageWriter*)::png_get_error_ptr(png);
	info->OnPngWarning(text);
}

void ff::PngImageWriter::PngWriteCallback(png_struct* png, unsigned char* data, size_t size)
{
	PngImageWriter* info = (PngImageWriter*)::png_get_io_ptr(png);
	info->OnPngWrite(data, size);
}

void ff::PngImageWriter::PngFlushCallback(png_struct* png)
{
	// not needed
}

void ff::PngImageWriter::OnPngError(const char* text)
{
	ff::String wtext = ff::String::from_utf8(text);
	assertSz(false, wtext.c_str());
	throw wtext;
}

void ff::PngImageWriter::OnPngWarning(const char* text)
{
	ff::String wtext = ff::String::from_utf8(text);
	assertSz(false, wtext.c_str());
}

void ff::PngImageWriter::OnPngWrite(const unsigned char* data, size_t size)
{
	_writer->Write(data, size);
}
