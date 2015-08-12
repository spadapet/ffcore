#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Graph/GraphBuffer.h"
#include "Graph/GraphDevice.h"
#include "Graph/State/GraphContext11.h"

class __declspec(uuid("44b0be5b-b5f9-466a-9983-f3eb122a81ee"))
	GraphBuffer11
	: public ff::ComBase
	, public ff::IGraphBuffer
	, public ff::IGraphBuffer11
{
public:
	DECLARE_HEADER(GraphBuffer11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(ff::GraphBufferType type, size_t size, bool writable, ff::IData* initialData);

	// IGraphDeviceChild
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IGraphBuffer
	virtual ff::GraphBufferType GetType() const override;
	virtual size_t GetSize() const override;
	virtual bool IsWritable() const override;
	virtual void* Map(size_t size) override;
	virtual void Unmap() override;
	virtual IGraphBuffer11* AsGraphBuffer11() override;

	// IGraphBuffer11
	virtual ID3D11Buffer* GetBuffer() override;

private:
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<ff::IData> _data;
	ff::ComPtr<ID3D11Buffer> _buffer;
};

BEGIN_INTERFACES(GraphBuffer11)
	HAS_INTERFACE(ff::IGraphBuffer)
END_INTERFACES()

GraphBuffer11::GraphBuffer11()
{
}

GraphBuffer11::~GraphBuffer11()
{
	if (_device)
	{
		_device->RemoveChild(static_cast<ff::IGraphBuffer*>(this));
	}
}

bool CreateGraphBuffer11(ff::IGraphDevice* device, ff::GraphBufferType type, size_t size, bool writable, ff::IData* initialData, ff::IGraphBuffer** buffer)
{
	assertRetVal(buffer, false);

	ff::ComPtr<GraphBuffer11, ff::IGraphBuffer> obj;
	assertHrRetVal(ff::ComAllocator<GraphBuffer11>::CreateInstance(device, &obj), false);
	assertRetVal(obj->Init(type, size, writable, initialData), false);

	*buffer = obj.Detach();
	return true;
}

HRESULT GraphBuffer11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(static_cast<ff::IGraphBuffer*>(this));

	return __super::_Construct(unkOuter);
}

bool GraphBuffer11::Init(ff::GraphBufferType type, size_t size, bool writable, ff::IData* initialData)
{
	_buffer = nullptr;
	_data = initialData;

	UINT bindFlags = 0;
	switch (type)
	{
	default: assertRetVal(false, false);
	case ff::GraphBufferType::Constant: bindFlags = D3D11_BIND_CONSTANT_BUFFER; break;
	case ff::GraphBufferType::Index: bindFlags = D3D11_BIND_INDEX_BUFFER; break;
	case ff::GraphBufferType::Vertex: bindFlags = D3D11_BIND_VERTEX_BUFFER; break;
	}

	D3D11_SUBRESOURCE_DATA data;
	if (initialData)
	{
		data.pSysMem = initialData->GetMem();
		data.SysMemPitch = (UINT)initialData->GetSize();
		data.SysMemSlicePitch = 0;
	}

	CD3D11_BUFFER_DESC desc((UINT)size, bindFlags, writable ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT, writable ? D3D11_CPU_ACCESS_WRITE : 0);
	assertHrRetVal(_device->AsGraphDevice11()->Get3d()->CreateBuffer(&desc, initialData ? &data : nullptr, &_buffer), nullptr);

	return true;
}

ff::IGraphDevice* GraphBuffer11::GetDevice() const
{
	return _device;
}

bool GraphBuffer11::Reset()
{
	return Init(GetType(), GetSize(), IsWritable(), _data);
}

ff::GraphBufferType GraphBuffer11::GetType() const
{
	D3D11_BUFFER_DESC desc;
	_buffer->GetDesc(&desc);

	if ((desc.BindFlags & D3D11_BIND_VERTEX_BUFFER) != 0)
	{
		return ff::GraphBufferType::Vertex;
	}
	else if ((desc.BindFlags & D3D11_BIND_INDEX_BUFFER) != 0)
	{
		return ff::GraphBufferType::Index;
	}
	else if ((desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) != 0)
	{
		return ff::GraphBufferType::Constant;
	}

	assertRetVal(false, ff::GraphBufferType::Unknown);
}

size_t GraphBuffer11::GetSize() const
{
	D3D11_BUFFER_DESC desc;
	_buffer->GetDesc(&desc);

	return desc.ByteWidth;
}

bool GraphBuffer11::IsWritable() const
{
	D3D11_BUFFER_DESC desc;
	_buffer->GetDesc(&desc);

	return (desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) != 0;
}

void* GraphBuffer11::Map(size_t size)
{
	assertRetVal(IsWritable(), nullptr);

	if (size > GetSize())
	{
		size = ff::NearestPowerOfTwo(GetSize() * 2);

		ff::ComPtr<ID3D11Buffer> oldBuffer = _buffer;
		if (!Init(GetType(), size, true, nullptr))
		{
			_buffer = oldBuffer;
			assertRetVal(false, nullptr);
		}
	}

	return _device->AsGraphDevice11()->GetStateContext().Map(_buffer, D3D11_MAP_WRITE_DISCARD);
}

void GraphBuffer11::Unmap()
{
	_device->AsGraphDevice11()->GetStateContext().Unmap(_buffer);
}

ff::IGraphBuffer11* GraphBuffer11::AsGraphBuffer11()
{
	return this;
}

ID3D11Buffer* GraphBuffer11::GetBuffer()
{
	return _buffer;
}
