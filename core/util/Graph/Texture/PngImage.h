#pragma once

namespace ff
{
	class IData;
	class IDataWriter;

	class PngImageReader
	{
	public:
		PngImageReader(const unsigned char* bytes, size_t size);
		~PngImageReader();

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
		ff::Vector<BYTE*> _rows;

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

	class PngImageWriter
	{
	public:
		PngImageWriter(IDataWriter* writer);
		~PngImageWriter();

		bool Write(const DirectX::Image& image, const DirectX::Image* paletteImage);
		ff::StringRef GetError() const;

	private:
		bool InternalWrite(const DirectX::Image& image, const DirectX::Image* paletteImage);

		static void PngErrorCallback(png_struct* png, const char* text);
		static void PngWarningCallback(png_struct* png, const char* text);
		static void PngWriteCallback(png_struct* png, unsigned char* data, size_t size);
		static void PngFlushCallback(png_struct* png);

		void OnPngError(const char* text);
		void OnPngWarning(const char* text);
		void OnPngWrite(const unsigned char* data, size_t size);

		// Data
		png_struct* _png;
		png_info* _info;
		ff::String _errorText;

		// Writing
		ComPtr<IDataWriter> _writer;
		ff::Vector<BYTE*> _rows;
	};
}
