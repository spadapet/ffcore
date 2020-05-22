#pragma once

#include "Resource/ResourceValue.h"

namespace ff
{
	class IDataReader;

	class XamlStream : public Noesis::Stream
	{
	public:
		XamlStream(ff::AutoResourceValue resource);
		XamlStream(ff::IDataReader* reader);
		virtual ~XamlStream() override;

		ff::IDataReader* GetReader() const;

		virtual void SetPosition(uint32_t pos) override;
		virtual uint32_t GetPosition() const override;
		virtual uint32_t GetLength() const override;
		virtual uint32_t Read(void* buffer, uint32_t size) override;
		virtual void Close() override;

	private:
		ff::AutoResourceValue _resource;
		ff::ComPtr<ff::IDataReader> _reader;
	};
}
