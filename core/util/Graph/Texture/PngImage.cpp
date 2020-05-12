#include "pch.h"
#include "Graph/Texture/PngImage.h"

ff::PngImage::PngImage(const unsigned char* bytes, size_t size)
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
	_png = ::png_create_read_struct(PNG_LIBPNG_VER_STRING, this, &PngImage::PngErrorCallback, &PngImage::PngWarningCallback);
	_info = ::png_create_info_struct(_png);
	_endInfo = ::png_create_info_struct(_png);
}

ff::PngImage::~PngImage()
{
	::png_destroy_read_struct(&_png, &_info, &_endInfo);
}

std::unique_ptr<DirectX::ScratchImage> ff::PngImage::Read(DXGI_FORMAT requestedFormat)
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

std::unique_ptr<DirectX::ScratchImage> ff::PngImage::GetPalette() const
{
	std::unique_ptr<DirectX::ScratchImage> scratch;

	if (_hasPalette)
	{
		scratch = std::make_unique<DirectX::ScratchImage>();

		if (FAILED(scratch->Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 256, 1, 1, 1)))
		{
			return nullptr;
		}

		BYTE* pixels = scratch->GetPixels();
		for (size_t i = 0; i < _paletteSize; i++, pixels += 4)
		{
			pixels[0] = _palette[i].red;
			pixels[1] = _palette[i].green;
			pixels[2] = _palette[i].blue;
			pixels[3] = (_hasTransPalette && i < _transPaletteSize) ? _transPalette[i] : 0xFF;
		}
	}

	return scratch;
}

ff::StringRef ff::PngImage::GetError() const
{
	return _errorText;
}

std::unique_ptr<DirectX::ScratchImage> ff::PngImage::InternalRead(DXGI_FORMAT requestedFormat)
{
	if (::png_sig_cmp(_readPos, 0, _endPos - _readPos) != 0)
	{
		return nullptr;
	}

	::png_set_read_fn(_png, this, &PngImage::PngReadCallback);
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

	_rows.resize(_height);
	{
		const DirectX::Image& image = *scratch->GetImage(0, 0, 0);
		BYTE* imagePixels = image.pixels;

		for (unsigned int i = 0; i < _height; i++)
		{
			_rows[i] = &imagePixels[i * image.rowPitch];
		}
	}

	::png_read_image(_png, (unsigned char**)_rows.data());
	::png_read_end(_png, _endInfo);

	return scratch;
}

void ff::PngImage::PngErrorCallback(png_struct* png, const char* text)
{
	PngImage* info = (PngImage*)::png_get_error_ptr(png);
	info->OnPngError(text);
}

void ff::PngImage::PngWarningCallback(png_struct* png, const char* text)
{
	PngImage* info = (PngImage*)::png_get_error_ptr(png);
	info->OnPngWarning(text);
}

void ff::PngImage::PngReadCallback(png_struct* png, unsigned char* data, size_t size)
{
	PngImage* info = (PngImage*)::png_get_io_ptr(png);
	info->OnPngRead(data, size);
}

void ff::PngImage::OnPngError(const char* text)
{
	ff::String wtext = ff::String::from_utf8(text);
	assertSz(false, wtext.c_str());
	throw wtext;
}

void ff::PngImage::OnPngWarning(const char* text)
{
	ff::String wtext = ff::String::from_utf8(text);
	assertSz(false, wtext.c_str());
}

void ff::PngImage::OnPngRead(unsigned char* data, size_t size)
{
	size = std::min<size_t>(size, _endPos - _readPos);
	::memcpy(data, _readPos, size);
	_readPos += size;
}
