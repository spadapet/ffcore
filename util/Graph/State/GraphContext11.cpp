#include "pch.h"
#include "Graph/GraphDevice.h"
#include "Graph/State/GraphContext11.h"

ff::GraphContext11::GraphContext11()
{
	Reset(nullptr);
}

ff::GraphContext11::GraphContext11(ID3D11DeviceContext* context)
{
	assert(context);
	Reset(context);
}

ff::GraphContext11::~GraphContext11()
{
}

void ff::GraphContext11::Clear()
{
	if (_context)
	{
		_context->ClearState();
		_context->Flush();
	}

	Reset(_context);
}

void ff::GraphContext11::Apply(GraphContext11& dest)
{
	dest.SetVertexIA(_iaVertexes[0], _iaVertexStrides[0], _iaVertexOffsets[0]);
	dest.SetIndexIA(_iaIndex, _iaIndexFormat, _iaIndexOffset);
	dest.SetLayoutIA(_iaLayout);
	dest.SetTopologyIA(_iaTopology);

	dest.SetVS(_vs._shader);
	dest.SetSamplersVS(_vs._samplers[0].Address(), 0, _countof(_vs._samplers));
	dest.SetConstantsVS(_vs._constants[0].Address(), 0, _countof(_vs._constants));
	dest.SetResourcesVS(_vs._resources[0].Address(), 0, _countof(_vs._resources));

	dest.SetGS(_gs._shader);
	dest.SetSamplersGS(_gs._samplers[0].Address(), 0, _countof(_gs._samplers));
	dest.SetConstantsGS(_gs._constants[0].Address(), 0, _countof(_gs._constants));
	dest.SetResourcesGS(_gs._resources[0].Address(), 0, _countof(_gs._resources));

	dest.SetPS(_ps._shader);
	dest.SetSamplersPS(_ps._samplers[0].Address(), 0, _countof(_ps._samplers));
	dest.SetConstantsPS(_ps._constants[0].Address(), 0, _countof(_ps._constants));
	dest.SetResourcesPS(_ps._resources[0].Address(), 0, _countof(_ps._resources));

	dest.SetBlend(_blend, _blendFactor, _blendSampleMask);
	dest.SetDepth(_depthState, _depthStencil);
	dest.SetTargets(_targetViewsCount ? _targetViews[0].Address() : nullptr, _targetViewsCount, _depthView);
	dest.SetRaster(_raster);
	dest.SetViewports(_viewportsCount ? _viewports : nullptr, _viewportsCount);
	dest.SetScissors(_scissorCount ? _scissors : nullptr, _scissorCount);
}

void ff::GraphContext11::Reset(ID3D11DeviceContext* context)
{
	_context = context;
	_counters.Reset();

	_vs.Reset();
	_gs.Reset();
	_ps.Reset();

	_iaIndex.Release();
	_iaIndexFormat = DXGI_FORMAT_UNKNOWN;
	_iaIndexOffset = 0;
	_iaLayout.Release();
	_iaTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

	for (size_t i = 0; i < _countof(_iaVertexes); i++)
	{
		_iaVertexes[i].Release();
	}

	ff::ZeroObject(_iaVertexStrides);
	ff::ZeroObject(_iaVertexOffsets);

	_blend.Release();
	_blendFactor = ff::GetColorWhite();
	_blendSampleMask = 0xFFFFFFFF;
	_depthState.Release();
	_depthStencil = 0;
	_targetViewsCount = 0;
	_viewportsCount = 0;
	_scissorCount = 0;

	for (size_t i = 0; i < _countof(_targetViews); i++)
	{
		_targetViews[i].Release();
	}

	for (size_t i = 0; i < _countof(_soTargets); i++)
	{
		_soTargets[i].Release();
	}

	_depthView.Release();
	_raster.Release();

	ff::ZeroObject(_viewports);
	ff::ZeroObject(_scissors);

	noAssertRet(context);

	context->IAGetIndexBuffer(&_iaIndex, &_iaIndexFormat, &_iaIndexOffset);
	context->IAGetInputLayout(&_iaLayout);
	context->IAGetPrimitiveTopology(&_iaTopology);
	context->IAGetVertexBuffers(0, _countof(_iaVertexes), _iaVertexes[0].Address(), &_iaVertexStrides[0], &_iaVertexOffsets[0]);

	context->VSGetShader(&_vs._shader, nullptr, nullptr);
	context->VSGetSamplers(0, _countof(_vs._samplers), _vs._samplers[0].Address());
	context->VSGetConstantBuffers(0, _countof(_vs._constants), _vs._constants[0].Address());
	context->VSGetShaderResources(0, _countof(_vs._resources), _vs._resources[0].Address());

	context->GSGetShader(&_gs._shader, nullptr, nullptr);
	context->GSGetSamplers(0, _countof(_gs._samplers), _gs._samplers[0].Address());
	context->GSGetConstantBuffers(0, _countof(_gs._constants), _gs._constants[0].Address());
	context->GSGetShaderResources(0, _countof(_gs._resources), _gs._resources[0].Address());

	context->PSGetShader(&_ps._shader, nullptr, nullptr);
	context->PSGetSamplers(0, _countof(_ps._samplers), _ps._samplers[0].Address());
	context->PSGetConstantBuffers(0, _countof(_ps._constants), _ps._constants[0].Address());
	context->PSGetShaderResources(0, _countof(_ps._resources), _ps._resources[0].Address());

	context->OMGetBlendState(&_blend, (float*)&_blendFactor, &_blendSampleMask);
	context->OMGetDepthStencilState(&_depthState, &_depthStencil);
	context->OMGetRenderTargets(_countof(_targetViews), _targetViews[0].Address(), &_depthView);
	for (_targetViewsCount = 0; _targetViewsCount < _countof(_targetViews) && _targetViews[_targetViewsCount]; _targetViewsCount++);

	context->SOGetTargets(_countof(_soTargets), _soTargets[0].Address());
	context->RSGetState(&_raster);

	UINT count = _countof(_viewports);
	context->RSGetViewports(&count, _viewports);
	_viewportsCount = count;

	count = _countof(_scissors);
	context->RSGetScissorRects(&count, _scissors);
	_scissorCount = count;
}

ff::GraphCounters ff::GraphContext11::ResetDrawCount()
{
	GraphCounters counters = _counters;
	_counters.Reset();
	return counters;
}

void ff::GraphContext11::Draw(size_t count, size_t start)
{
	assertRet(_context);
	_context->Draw((UINT)count, (UINT)start);
	_counters._draw++;
}

void ff::GraphContext11::DrawIndexed(size_t indexCount, size_t indexStart, int vertexOffset)
{
	assertRet(_context);
	_context->DrawIndexed((UINT)indexCount, (UINT)indexStart, vertexOffset);
	_counters._draw++;
}

void* ff::GraphContext11::Map(ID3D11Resource* buffer, D3D11_MAP type, D3D11_MAPPED_SUBRESOURCE* map)
{
	assertRetVal(_context && buffer, nullptr);
	D3D11_MAPPED_SUBRESOURCE stackMap;
	map = map ? map : &stackMap;

	_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, map);
	_counters._map++;

	return map->pData;
}

void ff::GraphContext11::Unmap(ID3D11Resource* buffer)
{
	assertRet(_context && buffer);
	_context->Unmap(buffer, 0);
}

void ff::GraphContext11::UpdateDiscard(ID3D11Resource* buffer, const void* data, size_t size)
{
	noAssertRet(size);

	void* dest = Map(buffer, D3D11_MAP_WRITE_DISCARD);
	assertRet(dest);

	::memcpy(dest, data, size);

	Unmap(buffer);
}

void ff::GraphContext11::ClearRenderTarget(ID3D11RenderTargetView* view, const DirectX::XMFLOAT4& color)
{
	_context->ClearRenderTargetView(view, &color.x);
	_counters._clear++;
}

void ff::GraphContext11::ClearDepthStencil(ID3D11DepthStencilView* view, bool clearDepth, bool clearStencil, float depth, BYTE stencil)
{
	_context->ClearDepthStencilView(view, (clearDepth ? D3D11_CLEAR_DEPTH : 0) | (clearStencil ? D3D11_CLEAR_STENCIL : 0), depth, stencil);
	_counters._depthClear++;
}

void ff::GraphContext11::UpdateSubresource(ID3D11Resource* dest, UINT destSubresource, const D3D11_BOX* destBox, const void* srcData, UINT srcRowPitch, UINT srcDepthPitch)
{
	_context->UpdateSubresource(dest, destSubresource, destBox, srcData, srcRowPitch, srcDepthPitch);
	_counters._update++;
}

void ff::GraphContext11::CopySubresourceRegion(ID3D11Resource* destResource, UINT destSubresource, UINT destX, UINT destY, UINT destZ, ID3D11Resource* srcResource, UINT srcSubresource, const D3D11_BOX* srcBox)
{
	_context->CopySubresourceRegion(destResource, destSubresource, destX, destY, destZ, srcResource, srcSubresource, srcBox);
	_counters._copy++;
}

void ff::GraphContext11::SetVertexIA(ID3D11Buffer* value, size_t stride, size_t offset)
{
	if (_iaVertexes[0] != value || _iaVertexStrides[0] != stride || _iaVertexOffsets[0] != offset)
	{
		_iaVertexes[0] = value;
		_iaVertexStrides[0] = (UINT)stride;
		_iaVertexOffsets[0] = (UINT)offset;

		if (_context)
		{
			_context->IASetVertexBuffers(0, 1, &value, &_iaVertexStrides[0], &_iaVertexOffsets[0]);
		}
	}
}

void ff::GraphContext11::SetIndexIA(ID3D11Buffer* value, DXGI_FORMAT format, size_t offset)
{
	if (_iaIndex != value || _iaIndexFormat != format || _iaIndexOffset != offset)
	{
		_iaIndex = value;
		_iaIndexFormat = format;
		_iaIndexOffset = (UINT)offset;

		if (_context)
		{
			_context->IASetIndexBuffer(value, _iaIndexFormat, _iaIndexOffset);
		}
	}
}

void ff::GraphContext11::SetLayoutIA(ID3D11InputLayout* value)
{
	if (_iaLayout != value)
	{
		_iaLayout = value;

		if (_context)
		{
			_context->IASetInputLayout(value);
		}
	}
}

void ff::GraphContext11::SetTopologyIA(D3D_PRIMITIVE_TOPOLOGY value)
{
	if (_iaTopology != value)
	{
		_iaTopology = value;

		if (_context)
		{
			_context->IASetPrimitiveTopology(value);
		}
	}
}

void ff::GraphContext11::SetAppendSO(ID3D11Buffer* value)
{
	if (_soTargets[0] != value)
	{
		SetOutputSO(value, 0);
	}
}

void ff::GraphContext11::SetOutputSO(ID3D11Buffer* value, size_t offset)
{
	_soTargets[0] = value;

	if (_context)
	{
		UINT offset2 = (UINT)offset;
		_context->SOSetTargets(1, &value, &offset2);
	}
}

void ff::GraphContext11::SetVS(ID3D11VertexShader* value)
{
	if (_vs._shader != value)
	{
		_vs._shader = value;

		if (_context)
		{
			_context->VSSetShader(value, nullptr, 0);
		}
	}
}

void ff::GraphContext11::SetSamplersVS(ID3D11SamplerState* const* values, size_t start, size_t count)
{
	if (::memcmp(values, _vs._samplers[start].Address(), sizeof(ID3D11SamplerState*) * count))
	{
		for (size_t i = start; i < start + count; i++)
		{
			_vs._samplers[i] = values[i - start];
		}

		if (_context)
		{
			_context->VSSetSamplers((UINT)start, (UINT)count, values);
		}
	}
}

void ff::GraphContext11::SetConstantsVS(ID3D11Buffer* const* values, size_t start, size_t count)
{
	if (::memcmp(values, _vs._constants[start].Address(), sizeof(ID3D11Buffer*) * count))
	{
		for (size_t i = start; i < start + count; i++)
		{
			_vs._constants[i] = values[i - start];
		}

		if (_context)
		{
			_context->VSSetConstantBuffers((UINT)start, (UINT)count, values);
		}
	}
}

void ff::GraphContext11::SetResourcesVS(ID3D11ShaderResourceView* const* values, size_t start, size_t count)
{
	if (::memcmp(values, _vs._resources[start].Address(), sizeof(ID3D11ShaderResourceView*) * count))
	{
		for (size_t i = start; i < start + count; i++)
		{
			_vs._resources[i] = values[i - start];
		}

		if (_context)
		{
			_context->VSSetShaderResources((UINT)start, (UINT)count, values);
		}
	}
}

void ff::GraphContext11::SetGS(ID3D11GeometryShader* value)
{
	if (_gs._shader != value)
	{
		_gs._shader = value;

		if (_context)
		{
			_context->GSSetShader(value, nullptr, 0);
		}
	}
}

void ff::GraphContext11::SetSamplersGS(ID3D11SamplerState* const* values, size_t start, size_t count)
{
	if (::memcmp(values, _gs._samplers[start].Address(), sizeof(ID3D11SamplerState*) * count))
	{
		for (size_t i = start; i < start + count; i++)
		{
			_gs._samplers[i] = values[i - start];
		}

		if (_context)
		{
			_context->GSSetSamplers((UINT)start, (UINT)count, values);
		}
	}
}

void ff::GraphContext11::SetConstantsGS(ID3D11Buffer* const* values, size_t start, size_t count)
{
	if (::memcmp(values, _gs._constants[start].Address(), sizeof(ID3D11Buffer*) * count))
	{
		for (size_t i = start; i < start + count; i++)
		{
			_gs._constants[i] = values[i - start];
		}

		if (_context)
		{
			_context->GSSetConstantBuffers((UINT)start, (UINT)count, values);
		}
	}
}

void ff::GraphContext11::SetResourcesGS(ID3D11ShaderResourceView* const* values, size_t start, size_t count)
{
	if (::memcmp(values, _gs._resources[start].Address(), sizeof(ID3D11ShaderResourceView*) * count))
	{
		for (size_t i = start; i < start + count; i++)
		{
			_gs._resources[i] = values[i - start];
		}

		if (_context)
		{
			_context->GSSetShaderResources((UINT)start, (UINT)count, values);
		}
	}
}

void ff::GraphContext11::SetPS(ID3D11PixelShader* value)
{
	if (_ps._shader != value)
	{
		_ps._shader = value;


		if (_context)
		{
			_context->PSSetShader(value, nullptr, 0);
		}
	}
}

void ff::GraphContext11::SetSamplersPS(ID3D11SamplerState* const* values, size_t start, size_t count)
{
	if (::memcmp(values, _ps._samplers[start].Address(), sizeof(ID3D11SamplerState*) * count))
	{
		for (size_t i = start; i < start + count; i++)
		{
			_ps._samplers[i] = values[i - start];
		}

		if (_context)
		{
			_context->PSSetSamplers((UINT)start, (UINT)count, values);
		}
	}
}

void ff::GraphContext11::SetConstantsPS(ID3D11Buffer* const* values, size_t start, size_t count)
{
	if (::memcmp(values, _ps._constants[start].Address(), sizeof(ID3D11Buffer*) * count))
	{
		for (size_t i = start; i < start + count; i++)
		{
			_ps._constants[i] = values[i - start];
		}

		if (_context)
		{
			_context->PSSetConstantBuffers((UINT)start, (UINT)count, values);
		}
	}
}

void ff::GraphContext11::SetResourcesPS(ID3D11ShaderResourceView* const* values, size_t start, size_t count)
{
	if (::memcmp(values, _ps._resources[start].Address(), sizeof(ID3D11ShaderResourceView*) * count))
	{
		for (size_t i = start; i < start + count; i++)
		{
			_ps._resources[i] = values[i - start];
		}

		if (_context)
		{
			_context->PSSetShaderResources((UINT)start, (UINT)count, values);
		}
	}
}

void ff::GraphContext11::SetBlend(ID3D11BlendState* value, const DirectX::XMFLOAT4& blendFactor, UINT sampleMask)
{
	if (_blend != value || _blendFactor != blendFactor || _blendSampleMask != sampleMask)
	{
		_blend = value;
		_blendFactor = blendFactor;
		_blendSampleMask = (UINT)sampleMask;

		if (_context)
		{
			_context->OMSetBlendState(value, (const float*)&blendFactor, _blendSampleMask);
		}
	}
}

void ff::GraphContext11::SetDepth(ID3D11DepthStencilState* value, UINT stencil)
{
	if (_depthState != value || _depthStencil != stencil)
	{
		_depthState = value;
		_depthStencil = stencil;

		if (_context)
		{
			_context->OMSetDepthStencilState(value, stencil);
		}
	}
}

void ff::GraphContext11::SetTargets(ID3D11RenderTargetView* const* targets, size_t count, ID3D11DepthStencilView* depth)
{
	if (_depthView != depth || _targetViewsCount != count ||
		(count && ::memcmp(targets, _targetViews[0].Address(), sizeof(ID3D11RenderTargetView*) * count)))
	{
		_depthView = depth;
		_targetViewsCount = count;

		for (size_t i = 0; i < count; i++)
		{
			_targetViews[i] = targets[i];
		}

		for (size_t i = count; i < _countof(_targetViews); i++)
		{
			_targetViews[i].Release();
		}

		if (_context)
		{
			_context->OMSetRenderTargets((UINT)count, count ? targets : nullptr, depth);
		}
	}
}

void ff::GraphContext11::SetRaster(ID3D11RasterizerState* value)
{
	if (_raster != value)
	{
		_raster = value;

		if (_context)
		{
			_context->RSSetState(value);
		}
	}
}

void ff::GraphContext11::SetViewports(const D3D11_VIEWPORT* value, size_t count)
{
	if (_viewportsCount != count || (count && ::memcmp(value, _viewports, sizeof(D3D11_VIEWPORT) * count)))
	{
		_viewportsCount = count;

		if (count)
		{
			::memcpy(_viewports, value, sizeof(D3D11_VIEWPORT) * count);
		}

		if (_context)
		{
			_context->RSSetViewports((UINT)count, value);
		}
	}
}

void ff::GraphContext11::SetScissors(const D3D11_RECT* value, size_t count)
{
	if (_scissorCount != count || (count && ::memcmp(value, _scissors, sizeof(D3D11_RECT) * count)))
	{
		_scissorCount = count;

		if (count)
		{
			::memcpy(_scissors, value, sizeof(D3D11_RECT) * count);
		}

		if (_context)
		{
			_context->RSSetScissorRects((UINT)count, value);
		}
	}
}

ID3D11DepthStencilView* ff::GraphContext11::GetDepthView() const
{
	return _depthView;
}
