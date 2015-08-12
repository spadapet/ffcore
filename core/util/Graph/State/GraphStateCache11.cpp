#include "pch.h"
#include "Graph/GraphDevice.h"
#include "Graph/State/GraphStateCache11.h"

ff::hash_t ff::GraphStateCache11::HashShaderData::Hash(const ComPtr<IData>& value)
{
	return (value != nullptr)
		? HashBytes(value->GetMem(), value->GetSize())
		: 0;
}

bool ff::GraphStateCache11::HashShaderData::Equals(const ComPtr<IData>& lhs, const ComPtr<IData>& rhs)
{
	if (lhs == nullptr || rhs == nullptr)
	{
		return lhs == rhs;
	}

	return lhs->GetSize() == rhs->GetSize() && !std::memcmp(lhs->GetMem(), rhs->GetMem(), lhs->GetSize());
}

ff::GraphStateCache11::GraphStateCache11()
{
}

ff::GraphStateCache11::GraphStateCache11(ID3D11DeviceX* device)
	: _device(device)
{
	assert(device);
}

ff::GraphStateCache11::~GraphStateCache11()
{
}

void ff::GraphStateCache11::Reset(ID3D11DeviceX* device)
{
	_device = device;

	_blendStates.Clear();
	_depthStates.Clear();
	_rasterStates.Clear();
	_samplerStates.Clear();
	_shaders.Clear();
	_layouts.Clear();
	_resources.Clear();
}

ID3D11BlendState* ff::GraphStateCache11::GetBlendState(const D3D11_BLEND_DESC& desc)
{
	auto i = _blendStates.GetKey(desc);

	if (!i)
	{
		ComPtr<ID3D11BlendState> pState;
		assertHrRetVal(_device->CreateBlendState(&desc, &pState), nullptr);

		i = _blendStates.InsertKey(desc, pState);
	}

	return i->GetValue();
}

ID3D11DepthStencilState* ff::GraphStateCache11::GetDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc)
{
	auto i = _depthStates.GetKey(desc);

	if (!i)
	{
		ComPtr<ID3D11DepthStencilState> pState;
		assertHrRetVal(_device->CreateDepthStencilState(&desc, &pState), nullptr);

		i = _depthStates.InsertKey(desc, pState);
	}

	return i->GetValue();
}

ID3D11RasterizerState* ff::GraphStateCache11::GetRasterizerState(const D3D11_RASTERIZER_DESC& desc)
{
	auto i = _rasterStates.GetKey(desc);

	if (!i)
	{
		ComPtr<ID3D11RasterizerState> pState;
		assertHrRetVal(_device->CreateRasterizerState(&desc, &pState), nullptr);

		i = _rasterStates.InsertKey(desc, pState);
	}

	return i->GetValue();
}

ID3D11SamplerState* ff::GraphStateCache11::GetSamplerState(const D3D11_SAMPLER_DESC& desc)
{
	auto i = _samplerStates.GetKey(desc);

	if (!i)
	{
		ComPtr<ID3D11SamplerState> pState;
		assertHrRetVal(_device->CreateSamplerState(&desc, &pState), nullptr);

		i = _samplerStates.InsertKey(desc, pState);
	}

	return i->GetValue();
}

ID3D11VertexShader* ff::GraphStateCache11::GetVertexShader(IResources* resources, StringRef resourceName)
{
	TypedResource<IGraphShader> resource(resources, resourceName);
	ComPtr<IData> data = resource.Flush() ? resource.GetObject()->GetData() : nullptr;
	ComPtr<ID3D11VertexShader> value = GetVertexShader(data);

	if (value)
	{
		_resources.Push(std::move(resource));
	}

	return value;
}

ID3D11VertexShader* ff::GraphStateCache11::GetVertexShaderAndInputLayout(IResources* resources, StringRef resourceName, ComPtr<ID3D11InputLayout>& inputLayout, const D3D11_INPUT_ELEMENT_DESC* layout, size_t count)
{
	inputLayout = GetInputLayout(resources, resourceName, layout, count);
	return GetVertexShader(resources, resourceName);
}

ID3D11GeometryShader* ff::GraphStateCache11::GetGeometryShader(IResources* resources, StringRef resourceName)
{
	TypedResource<IGraphShader> resource(resources, resourceName);
	ComPtr<IData> data = resource.Flush() ? resource.GetObject()->GetData() : nullptr;
	ComPtr<ID3D11GeometryShader> value = GetGeometryShader(data);

	if (value)
	{
		_resources.Push(std::move(resource));
	}

	return value;
}

ID3D11GeometryShader* ff::GraphStateCache11::GetGeometryShaderStreamOutput(IResources* resources, StringRef resourceName, const D3D11_SO_DECLARATION_ENTRY* layout, size_t count, size_t vertexStride)
{
	TypedResource<IGraphShader> resource(resources, resourceName);
	ComPtr<IData> data = resource.Flush() ? resource.GetObject()->GetData() : nullptr;
	ComPtr<ID3D11GeometryShader> value = GetGeometryShaderStreamOutput(data, layout, count, vertexStride);

	if (value)
	{
		_resources.Push(std::move(resource));
	}

	return value;
}

ID3D11PixelShader* ff::GraphStateCache11::GetPixelShader(IResources* resources, StringRef resourceName)
{
	TypedResource<IGraphShader> resource(resources, resourceName);
	ComPtr<IData> data = resource.Flush() ? resource.GetObject()->GetData() : nullptr;
	ComPtr<ID3D11PixelShader> value = GetPixelShader(data);

	if (value)
	{
		_resources.Push(std::move(resource));
	}

	return value;
}

ID3D11InputLayout* ff::GraphStateCache11::GetInputLayout(IResources* resources, StringRef vertexShaderResourceName, const D3D11_INPUT_ELEMENT_DESC* layout, size_t count)
{
	TypedResource<IGraphShader> resource(resources, vertexShaderResourceName);
	ComPtr<IData> data = resource.Flush() ? resource.GetObject()->GetData() : nullptr;
	ComPtr<ID3D11InputLayout> value = GetInputLayout(data, layout, count);

	if (value)
	{
		_resources.Push(std::move(resource));
	}

	return value;
}

ID3D11VertexShader* ff::GraphStateCache11::GetVertexShader(ff::IData* shaderData)
{
	ComPtr<ID3D11VertexShader> value;
	ComPtr<IData> data = shaderData;

	auto iter = _shaders.GetKey(data);
	if (iter)
	{
		value.QueryFrom(iter->GetValue());
	}
	else if (data != nullptr && SUCCEEDED(_device->CreateVertexShader(data->GetMem(), data->GetSize(), nullptr, &value)))
	{
		_shaders.SetKey(std::move(data), ComPtr<IUnknown>(value));
	}

	return value;
}

ID3D11VertexShader* ff::GraphStateCache11::GetVertexShaderAndInputLayout(ff::IData* shaderData, ComPtr<ID3D11InputLayout>& inputLayout, const D3D11_INPUT_ELEMENT_DESC* layout, size_t count)
{
	inputLayout = GetInputLayout(shaderData, layout, count);
	return GetVertexShader(shaderData);
}

ID3D11GeometryShader* ff::GraphStateCache11::GetGeometryShader(ff::IData* shaderData)
{
	ComPtr<ID3D11GeometryShader> value;
	ComPtr<IData> data = shaderData;

	auto iter = _shaders.GetKey(data);
	if (iter)
	{
		value.QueryFrom(iter->GetValue());
	}
	else if (data != nullptr && SUCCEEDED(_device->CreateGeometryShader(data->GetMem(), data->GetSize(), nullptr, &value)))
	{
		_shaders.SetKey(std::move(data), ComPtr<IUnknown>(value));
	}

	return value;
}

ID3D11GeometryShader* ff::GraphStateCache11::GetGeometryShaderStreamOutput(ff::IData* shaderData, const D3D11_SO_DECLARATION_ENTRY* layout, size_t count, size_t vertexStride)
{
	ComPtr<ID3D11GeometryShader> value;
	ComPtr<IData> data = shaderData;
	UINT vertexStride2 = (UINT)vertexStride;

	auto iter = _shaders.GetKey(data);
	if (iter)
	{
		value.QueryFrom(iter->GetValue());
	}
	else if (data != nullptr && SUCCEEDED(_device->CreateGeometryShaderWithStreamOutput(data->GetMem(), data->GetSize(), layout, (UINT)count, &vertexStride2, 1, 0, nullptr, &value)))
	{
		_shaders.SetKey(std::move(data), ComPtr<IUnknown>(value));
	}

	return value;
}

ID3D11PixelShader* ff::GraphStateCache11::GetPixelShader(ff::IData* shaderData)
{
	ComPtr<ID3D11PixelShader> value;
	ComPtr<IData> data = shaderData;

	auto iter = _shaders.GetKey(data);
	if (iter)
	{
		value.QueryFrom(iter->GetValue());
	}
	else if (data != nullptr && SUCCEEDED(_device->CreatePixelShader(data->GetMem(), data->GetSize(), nullptr, &value)))
	{
		_shaders.SetKey(std::move(data), ComPtr<IUnknown>(value));
	}

	return value;
}

ID3D11InputLayout* ff::GraphStateCache11::GetInputLayout(ff::IData* shaderData, const D3D11_INPUT_ELEMENT_DESC* layout, size_t count)
{
	ComPtr<ID3D11InputLayout> value;
	ComPtr<IData> data = shaderData;

	auto iter = _layouts.GetKey(data);
	if (iter)
	{
		value = iter->GetValue();
	}
	else if (data != nullptr && SUCCEEDED(_device->CreateInputLayout(layout, (UINT)count, data->GetMem(), data->GetSize(), &value)))
	{
		_layouts.SetKey(std::move(data), ComPtr<ID3D11InputLayout>(value));
	}

	return value;
}
