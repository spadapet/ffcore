#pragma once

namespace ff
{
	class IData;

	class __declspec(uuid("125774c8-eb02-45bf-9e45-0f9094e83aec")) __declspec(novtable)
		IFontData : public IUnknown
	{
	public:
		virtual bool GetBold() const = 0;
		virtual bool GetItalic() const = 0;
		virtual size_t GetIndex() const = 0;
		virtual IData* GetData() const = 0;
		virtual IDWriteFontFaceX* GetFontFace() = 0;
	};
}
