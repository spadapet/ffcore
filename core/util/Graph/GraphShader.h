#pragma once

namespace ff
{
	class IData;

	class __declspec(uuid("4869aaa3-ef89-48b0-870e-ce1662aa48c8")) __declspec(novtable)
		IGraphShader : public IUnknown
	{
	public:
		virtual IData* GetData() const = 0;
	};
}
