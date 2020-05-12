#pragma once

namespace ff
{
	class PngImage
	{
	public:
		PngImage(const unsigned char* bytes, size_t size);
		~PngImage();

		std::unique_ptr<DirectX::ScratchImage> Read(DXGI_FORMAT requestedFormat = DXGI_FORMAT_UNKNOWN);
		std::unique_ptr<DirectX::ScratchImage> GetPalette() const;
		ff::StringRef GetError() const;

	private:
		std::unique_ptr<DirectX::ScratchImage> InternalRead(DXGI_FORMAT requestedFormat);

		static void PngErrorCallback(png_struct* png, const char* text);
		static void PngWarningCallback(png_struct* png, const char* text);
		static void PngReadCallback(png_struct* png, unsigned char* data, size_t size);

		void OnPngError(const char* text);
		void OnPngWarning(const char* text);
		void OnPngRead(unsigned char* data, size_t size);

		// Data
		png_struct* _png;
		png_info* _info;
		png_info* _endInfo;
		ff::String _errorText;

		// Reading
		const unsigned char* _readPos;
		const unsigned char* _endPos;
		std::vector<BYTE*> _rows;

		// Properties
		unsigned int _width;
		unsigned int _height;
		int _bitDepth;
		int _colorType;
		int _interlaceMethod;

		// Palette
		bool _hasPalette;
		png_color* _palette;
		int _paletteSize;
		bool _hasTransPalette;
		unsigned char* _transPalette;
		int _transPaletteSize;
		png_color_16* _transColor;
	};
}
