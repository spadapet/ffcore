#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Graph/GraphBuffer.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/State/GraphContext11.h"
#include "Graph/State/GraphStateCache11.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "UI/Internal/XamlRenderDevice11.h"
#include "UI/Internal/XamlShaders11.h"
#include "UI/Internal/XamlTexture.h"
#include "UI/Internal/XamlRenderTarget.h"
#include "String/StringUtil.h"

static const size_t DYNAMIC_VB_SIZE = 512 * 1024;
static const size_t DYNAMIC_IB_SIZE = 128 * 1024;
static const size_t PREALLOCATED_DYNAMIC_PAGES = 1;
static const size_t VS_CBUFFER_SIZE = 16 * sizeof(float); // projectionMtx
static const size_t PS_CBUFFER_SIZE = 12 * sizeof(float); // rgba | radialGrad opacity | opacity
static const uint32_t VFPos = 0;
static const uint32_t VFColor = 1;
static const uint32_t VFTex0 = 2;
static const uint32_t VFTex1 = 4;
static const uint32_t VFCoverage = 8;

struct Program
{
	int8_t vShaderIdx;
	int8_t pShaderIdx;
};

// Map from batch shader-ID to vertex and pixel shader objects
static const Program Programs[] =
{
	{ 0, 0 },    // RGBA
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ 0, 1 },    // Mask
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ 1, 2 },    // PathSolid
	{ 2, 3 },    // PathLinear
	{ 2, 4 },    // PathRadial
	{ 2, 5 },    // PathPattern
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ 3, 6 },    // PathAASolid
	{ 4, 7 },    // PathAALinear
	{ 4, 8 },    // PathAARadial
	{ 4, 9 },    // PathAAPattern
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ 5, 10 },   // ImagePaintOpacitySolid
	{ 6, 11 },   // ImagePaintOpacityLinear
	{ 6, 12 },   // ImagePaintOpacityRadial
	{ 6, 13 },   // ImagePaintOpacityPattern
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ 5, 14 },   // TextSolid
	{ 6, 15 },   // TextLinear
	{ 6, 16 },   // TextRadial
	{ 6, 17 },   // TextPattern
};

static D3D11_FILTER ToD3D(Noesis::MinMagFilter::Enum minmagFilter, Noesis::MipFilter::Enum mipFilter)
{
	switch (minmagFilter)
	{
	default:
	case Noesis::MinMagFilter::Nearest:
		switch (mipFilter)
		{
		default:
		case Noesis::MipFilter::Disabled:
		case Noesis::MipFilter::Nearest:
			return D3D11_FILTER_MIN_MAG_MIP_POINT;

		case Noesis::MipFilter::Linear:
			return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		}
		break;

	case Noesis::MinMagFilter::Linear:
		switch (mipFilter)
		{
		default:
		case Noesis::MipFilter::Disabled:
		case Noesis::MipFilter::Nearest:
			return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;

		case Noesis::MipFilter::Linear:
			return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
		break;
	}
}

static void ToD3D(Noesis::WrapMode::Enum mode, D3D11_TEXTURE_ADDRESS_MODE& addressU, D3D11_TEXTURE_ADDRESS_MODE& addressV)
{
	switch (mode)
	{
	default:
	case Noesis::WrapMode::ClampToEdge:
		addressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		addressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		break;

	case Noesis::WrapMode::ClampToZero:
		addressU = D3D11_TEXTURE_ADDRESS_BORDER;
		addressV = D3D11_TEXTURE_ADDRESS_BORDER;
		break;

	case Noesis::WrapMode::Repeat:
		addressU = D3D11_TEXTURE_ADDRESS_WRAP;
		addressV = D3D11_TEXTURE_ADDRESS_WRAP;
		break;

	case Noesis::WrapMode::MirrorU:
		addressU = D3D11_TEXTURE_ADDRESS_MIRROR;
		addressV = D3D11_TEXTURE_ADDRESS_WRAP;
		break;

	case Noesis::WrapMode::MirrorV:
		addressU = D3D11_TEXTURE_ADDRESS_WRAP;
		addressV = D3D11_TEXTURE_ADDRESS_MIRROR;
		break;

	case Noesis::WrapMode::Mirror:
		addressU = D3D11_TEXTURE_ADDRESS_MIRROR;
		addressV = D3D11_TEXTURE_ADDRESS_MIRROR;
		break;
	}
}

ff::XamlRenderDevice11::XamlRenderDevice11(ff::IGraphDevice* graph, bool sRGB)
	: _graph(graph)
	, _context(graph->AsGraphDevice11()->GetStateContext())
	, _states(graph->AsGraphDevice11()->GetStateCache())
	, _vertexCBHash(0)
	, _pixelCBHash(0)
{
	_graph->AddChild(this);

	_caps.centerPixelOffset = 0;
	_caps.dynamicIndicesSize = DYNAMIC_IB_SIZE;
	_caps.dynamicVerticesSize = DYNAMIC_VB_SIZE;
	_caps.linearRendering = sRGB;

	CreateBuffers();
	CreateStateObjects();
	CreateShaders();
}

ff::XamlRenderDevice11::~XamlRenderDevice11()
{
	_graph->RemoveChild(this);
}

const Noesis::DeviceCaps& ff::XamlRenderDevice11::GetCaps() const
{
	return _caps;
}

Noesis::Ptr<Noesis::RenderTarget> ff::XamlRenderDevice11::CreateRenderTarget(const char* label, uint32_t width, uint32_t height, uint32_t sampleCount)
{
	ff::String name = label ? ff::StringFromUTF8(label) : ff::GetEmptyString();
	return *new XamlRenderTarget(_graph, width, height, sampleCount, _caps.linearRendering, name);
}

Noesis::Ptr<Noesis::RenderTarget> ff::XamlRenderDevice11::CloneRenderTarget(const char* label, Noesis::RenderTarget* surface)
{
	ff::String name = label ? ff::StringFromUTF8(label) : ff::GetEmptyString();
	return *XamlRenderTarget::Get(surface)->Clone(name).GetPtr();
}

Noesis::Ptr<Noesis::Texture> ff::XamlRenderDevice11::CreateTexture(const char* label, uint32_t width, uint32_t height, uint32_t numLevels, Noesis::TextureFormat::Enum format)
{
	ff::TextureFormat format2 = (format == Noesis::TextureFormat::R8)
		? ff::TextureFormat::R8 : (_caps.linearRendering ? ff::TextureFormat::RGBA32_SRGB : ff::TextureFormat::RGBA32);
	ff::String name = label ? ff::StringFromUTF8(label) : ff::GetEmptyString();
	ff::PointInt size = ff::PointSize(width, height).ToType<int>();
	ff::ComPtr<ff::ITexture> texture = _graph->CreateTexture(size, format2, numLevels, 1, 1);

	return *new XamlTexture(texture, name);
}

void ff::XamlRenderDevice11::UpdateTexture(Noesis::Texture* texture, uint32_t level, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* data)
{
	assertRet(!level);

	XamlTexture* texture2 = XamlTexture::Get(texture);
	ID3D11ShaderResourceView* view = texture2->GetTexture()->AsTextureView()->AsTextureView11()->GetView();

	ff::ComPtr<ID3D11Resource> resource;
	view->GetResource(&resource);

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	view->GetDesc(&desc);

	D3D11_BOX box;
	box.left = x;
	box.top = y;
	box.front = 0;
	box.right = x + width;
	box.bottom = y + height;
	box.back = 1;

	unsigned int pitch = (desc.Format == DXGI_FORMAT_R8_UNORM) ? width : width * 4;
	_graph->AsGraphDevice11()->GetContext()->UpdateSubresource(resource, 0, &box, data, pitch, 0);
}

void ff::XamlRenderDevice11::BeginRender(bool offscreen)
{
	ID3D11Buffer* bufferVS = _bufferVertexCB->AsGraphBuffer11()->GetBuffer();
	ID3D11Buffer* bufferPS = _bufferPixelCB->AsGraphBuffer11()->GetBuffer();

	_context.SetConstantsVS(&bufferVS, 0, 1);
	_context.SetConstantsPS(&bufferPS, 0, 1);
	_context.SetTopologyIA(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_context.SetGS(nullptr);
}

void ff::XamlRenderDevice11::SetRenderTarget(Noesis::RenderTarget* surface)
{
	ClearTextures();

	XamlRenderTarget* surface2 = XamlRenderTarget::Get(surface);
	ff::PointFloat size = surface2->GetMsaaTexture()->GetSize().ToType<float>();
	ID3D11RenderTargetView* view = surface2->GetMsaaTarget()->AsRenderTarget11()->GetTarget();
	_context.SetTargets(&view, 1, surface2->GetDepth()->AsRenderDepth11()->GetView());

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = size.x;
	viewport.Height = size.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	_context.SetViewports(&viewport, 1);
}

void ff::XamlRenderDevice11::BeginTile(const Noesis::Tile& tile, uint32_t surfaceWidth, uint32_t surfaceHeight)
{
	uint32_t x = tile.x;
	uint32_t y = (uint32_t)surfaceHeight - (tile.y + tile.height);

	D3D11_RECT rect;
	rect.left = x;
	rect.top = y;
	rect.right = x + tile.width;
	rect.bottom = y + tile.height;

	_context.SetScissors(&rect, 1);

	ClearRenderTarget();
}

void ff::XamlRenderDevice11::EndTile()
{
}

void ff::XamlRenderDevice11::ResolveRenderTarget(Noesis::RenderTarget* surface, const Noesis::Tile* tiles, uint32_t numTiles)
{
	XamlRenderTarget* surface2 = XamlRenderTarget::Get(surface);

	if (surface2->GetMsaaTexture() != surface2->GetResolvedTexture())
	{
		size_t indexPS;
		switch (surface2->GetMsaaTexture()->GetSampleCount())
		{
		case 2: indexPS = 0; break;
		case 4: indexPS = 1; break;
		case 8: indexPS = 2; break;
		case 16: indexPS = 3; break;
		default: assertRet(false);
		}

		_context.SetLayoutIA(nullptr);
		_context.SetVS(_quadVS);
		_context.SetPS(_resolvePS[indexPS]);

		_context.SetRaster(_rasterizerStates[2]);
		_context.SetBlend(_blendStates[2], ff::GetColorNone(), 0xffffffff);
		_context.SetDepth(_depthStencilStates[0], 0);

		ClearTextures();
		ID3D11RenderTargetView* view = surface2->GetResolvedTarget()->AsRenderTarget11()->GetTarget();
		_context.SetTargets(&view, 1, nullptr);

		ID3D11ShaderResourceView* resourceView = surface2->GetMsaaTexture()->AsTextureView()->AsTextureView11()->GetView();
		_context.SetResourcesPS(&resourceView, 0, 1);

		ff::PointInt size = surface2->GetResolvedTexture()->GetSize();

		for (uint32_t i = 0; i < numTiles; i++)
		{
			const Noesis::Tile& tile = tiles[i];

			D3D11_RECT rect;
			rect.left = tile.x;
			rect.top = size.y - (tile.y + tile.height);
			rect.right = tile.x + tile.width;
			rect.bottom = size.y - tile.y;
			_context.SetScissors(&rect, 1);

			_context.Draw(3, 0);
		}
	}
}

void ff::XamlRenderDevice11::EndRender()
{
}

void* ff::XamlRenderDevice11::MapVertices(uint32_t bytes)
{
	return _bufferVertices->Map(bytes);
}

void ff::XamlRenderDevice11::UnmapVertices()
{
	_bufferVertices->Unmap();
}

void* ff::XamlRenderDevice11::MapIndices(uint32_t bytes)
{
	return _bufferIndices->Map(bytes);
}

void ff::XamlRenderDevice11::UnmapIndices()
{
	_bufferIndices->Unmap();
}

void ff::XamlRenderDevice11::DrawBatch(const Noesis::Batch& batch)
{
	assert(batch.shader.v < _countof(::Programs));
	assert(::Programs[batch.shader.v].vShaderIdx != -1);
	assert(::Programs[batch.shader.v].pShaderIdx != -1);
	assert(::Programs[batch.shader.v].vShaderIdx < _countof(_vertexStages));
	assert(::Programs[batch.shader.v].pShaderIdx < _countof(_pixelShaders));

	SetShaders(batch);
	SetBuffers(batch);
	SetRenderState(batch);
	SetTextures(batch);

	_context.DrawIndexed(batch.numIndices, batch.startIndex, 0);
}

ff::IGraphDevice* ff::XamlRenderDevice11::GetDevice() const
{
	return _graph;
}

bool ff::XamlRenderDevice11::Reset()
{
	CreateStateObjects();
	CreateShaders();

	return true;
}

void ff::XamlRenderDevice11::CreateVertexStage(uint8_t format, const char* label, const void* code, uint32_t size, VertexStage& stage)
{
	stage.stride = 0;
	uint32_t element = 0;

	D3D11_INPUT_ELEMENT_DESC descs[5];

	descs[element].SemanticIndex = 0;
	descs[element].InputSlot = 0;
	descs[element].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	descs[element].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	descs[element].InstanceDataStepRate = 0;
	descs[element].SemanticName = "POSITION";
	descs[element].Format = DXGI_FORMAT_R32G32_FLOAT;
	stage.stride += 8;
	element++;

	if (format & VFColor)
	{
		descs[element].SemanticIndex = 0;
		descs[element].InputSlot = 0;
		descs[element].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		descs[element].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		descs[element].InstanceDataStepRate = 0;
		descs[element].SemanticName = "COLOR";
		descs[element].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		stage.stride += 4;
		element++;
	}

	if (format & VFTex0)
	{
		descs[element].SemanticIndex = 0;
		descs[element].InputSlot = 0;
		descs[element].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		descs[element].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		descs[element].InstanceDataStepRate = 0;
		descs[element].SemanticName = "TEXCOORD";
		descs[element].Format = DXGI_FORMAT_R32G32_FLOAT;
		stage.stride += 8;
		element++;
	}

	if (format & VFTex1)
	{
		descs[element].SemanticIndex = 1;
		descs[element].InputSlot = 0;
		descs[element].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		descs[element].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		descs[element].InstanceDataStepRate = 0;
		descs[element].SemanticName = "TEXCOORD";
		descs[element].Format = DXGI_FORMAT_R32G32_FLOAT;
		stage.stride += 8;
		element++;
	}

	if (format & VFCoverage)
	{
		descs[element].SemanticIndex = 2;
		descs[element].InputSlot = 0;
		descs[element].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		descs[element].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		descs[element].InstanceDataStepRate = 0;
		descs[element].SemanticName = "TEXCOORD";
		descs[element].Format = DXGI_FORMAT_R32_FLOAT;
		stage.stride += 4;
		element++;
	}

	ff::ComPtr<ff::IData> data;
	ff::CreateDataInStaticMem((const BYTE*)code, size, &data);
	stage.shader = _states.GetVertexShaderAndInputLayout(data, stage.layout, descs, element);
}

void ff::XamlRenderDevice11::CreateBuffers()
{
	_bufferVertices = _graph->CreateBuffer(ff::GraphBufferType::Vertex, DYNAMIC_VB_SIZE);
	_bufferIndices = _graph->CreateBuffer(ff::GraphBufferType::Index, DYNAMIC_IB_SIZE);
	_bufferVertexCB = _graph->CreateBuffer(ff::GraphBufferType::Constant, VS_CBUFFER_SIZE);
	_bufferPixelCB = _graph->CreateBuffer(ff::GraphBufferType::Constant, PS_CBUFFER_SIZE);
}

void ff::XamlRenderDevice11::CreateStateObjects()
{
	// Rasterized states
	{
		D3D11_RASTERIZER_DESC desc;
		desc.CullMode = D3D11_CULL_NONE;
		desc.FrontCounterClockwise = false;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = true;
		desc.MultisampleEnable = true;
		desc.AntialiasedLineEnable = false;

		desc.FillMode = D3D11_FILL_SOLID;
		desc.ScissorEnable = false;
		_rasterizerStates[0] = _states.GetRasterizerState(desc);

		desc.FillMode = D3D11_FILL_WIREFRAME;
		desc.ScissorEnable = false;
		_rasterizerStates[1] = _states.GetRasterizerState(desc);

		desc.FillMode = D3D11_FILL_SOLID;
		desc.ScissorEnable = true;
		_rasterizerStates[2] = _states.GetRasterizerState(desc);

		desc.FillMode = D3D11_FILL_WIREFRAME;
		desc.ScissorEnable = true;
		_rasterizerStates[3] = _states.GetRasterizerState(desc);
	}

	// Blend states
	{
		D3D11_BLEND_DESC desc;
		desc.AlphaToCoverageEnable = false;
		desc.IndependentBlendEnable = false;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		desc.RenderTarget[0].BlendEnable = false;
		desc.RenderTarget[0].RenderTargetWriteMask = 0;
		_blendStates[0] = _states.GetBlendState(desc);

		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].RenderTargetWriteMask = 0;
		_blendStates[1] = _states.GetBlendState(desc);

		desc.RenderTarget[0].BlendEnable = false;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		_blendStates[2] = _states.GetBlendState(desc);

		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		_blendStates[3] = _states.GetBlendState(desc);
	}

	// Depth states
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D11_COMPARISON_NEVER;
		desc.StencilReadMask = 0xff;
		desc.StencilWriteMask = 0xff;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

		// Disabled
		desc.StencilEnable = false;
		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		_depthStencilStates[0] = _states.GetDepthStencilState(desc);

		// Equal_Keep
		desc.StencilEnable = true;
		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		_depthStencilStates[1] = _states.GetDepthStencilState(desc);

		// Equal_Incr
		desc.StencilEnable = true;
		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
		desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
		_depthStencilStates[2] = _states.GetDepthStencilState(desc);

		// Equal_Decr
		desc.StencilEnable = true;
		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
		desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
		_depthStencilStates[3] = _states.GetDepthStencilState(desc);

		// Zero
		desc.StencilEnable = true;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
		desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
		_depthStencilStates[4] = _states.GetDepthStencilState(desc);
	}

	// Sampler states
	{
		D3D11_SAMPLER_DESC desc{};
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MinLOD = -D3D11_FLOAT32_MAX;
		desc.MaxLOD = D3D11_FLOAT32_MAX;

		for (uint8_t minmag = 0; minmag < Noesis::MinMagFilter::Count; minmag++)
		{
			for (uint8_t mip = 0; mip < Noesis::MipFilter::Count; mip++)
			{
				desc.Filter = ToD3D(Noesis::MinMagFilter::Enum(minmag), Noesis::MipFilter::Enum(mip));

				for (uint8_t uv = 0; uv < Noesis::WrapMode::Count; uv++)
				{
					ToD3D(Noesis::WrapMode::Enum(uv), desc.AddressU, desc.AddressV);

					Noesis::SamplerState s = { {uv, minmag, mip} };
					assert(s.v < _countof(_samplerStates));
					_samplerStates[s.v] = _states.GetSamplerState(desc);
				}
			}
		}
	}
}

void ff::XamlRenderDevice11::CreateShaders()
{
#define SHADER(n) {#n, n, sizeof(n)}

	struct Shader
	{
		const char* label;
		const BYTE* code;
		uint32_t size;
	};

	const Shader pShaders[] =
	{
		SHADER(RGBA_FS),
		SHADER(Mask_FS),
		SHADER(PathSolid_FS),
		SHADER(PathLinear_FS),
		SHADER(PathRadial_FS),
		SHADER(PathPattern_FS),
		SHADER(PathAASolid_FS),
		SHADER(PathAALinear_FS),
		SHADER(PathAARadial_FS),
		SHADER(PathAAPattern_FS),
		SHADER(ImagePaintOpacitySolid_FS),
		SHADER(ImagePaintOpacityLinear_FS),
		SHADER(ImagePaintOpacityRadial_FS),
		SHADER(ImagePaintOpacityPattern_FS),
		SHADER(TextSolid_FS),
		SHADER(TextLinear_FS),
		SHADER(TextRadial_FS),
		SHADER(TextPattern_FS)
	};

	const Shader vShaders[] =
	{
		SHADER(Pos_VS),
		SHADER(PosColor_VS),
		SHADER(PosTex0_VS),
		SHADER(PosColorCoverage_VS),
		SHADER(PosTex0Coverage_VS),
		SHADER(PosColorTex1_VS),
		SHADER(PosTex0Tex1_VS)
	};

#undef SHADER

	const uint8_t formats[] =
	{
		VFPos,
		VFPos | VFColor,
		VFPos | VFTex0,
		VFPos | VFColor | VFCoverage,
		VFPos | VFTex0 | VFCoverage,
		VFPos | VFColor | VFTex1,
		VFPos | VFTex0 | VFTex1
	};

	static_assert(_countof(vShaders) == _countof(_vertexStages));

	for (uint32_t i = 0; i < _countof(vShaders); i++)
	{
		const Shader& shader = vShaders[i];
		CreateVertexStage(formats[i], shader.label, shader.code, shader.size, _vertexStages[i]);
	}

	static_assert(_countof(pShaders) == _countof(_pixelShaders));

	for (uint32_t i = 0; i < _countof(pShaders); i++)
	{
		const Shader& shader = pShaders[i];
		ff::ComPtr<ff::IData> data;
		ff::CreateDataInStaticMem(shader.code, shader.size, &data);
		_pixelShaders[i] = _states.GetPixelShader(data);
	}

	ff::ComPtr<ff::IData> quadVsData, resolve2Data, resolve4Data, resolve8Data, resolve16Data, clearData;
	ff::CreateDataInStaticMem(Quad_VS, sizeof(Quad_VS), &quadVsData);
	ff::CreateDataInStaticMem(Resolve2_PS, sizeof(Resolve2_PS), &resolve2Data);
	ff::CreateDataInStaticMem(Resolve4_PS, sizeof(Resolve4_PS), &resolve4Data);
	ff::CreateDataInStaticMem(Resolve8_PS, sizeof(Resolve8_PS), &resolve8Data);
	ff::CreateDataInStaticMem(Resolve16_PS, sizeof(Resolve16_PS), &resolve16Data);
	ff::CreateDataInStaticMem(Clear_PS, sizeof(Clear_PS), &clearData);

	_resolvePS[0] = _states.GetPixelShader(resolve2Data);
	_resolvePS[1] = _states.GetPixelShader(resolve4Data);
	_resolvePS[2] = _states.GetPixelShader(resolve8Data);
	_resolvePS[3] = _states.GetPixelShader(resolve16Data);
	_clearPS = _states.GetPixelShader(clearData);
	_quadVS = _states.GetVertexShader(quadVsData);
}

void ff::XamlRenderDevice11::ClearRenderTarget()
{
	_context.SetLayoutIA(nullptr);
	_context.SetVS(_quadVS);
	_context.SetPS(_clearPS);
	_context.SetRaster(_rasterizerStates[2]);
	_context.SetBlend(_blendStates[2], ff::GetColorNone(), 0xffffffff);
	_context.SetDepth(_depthStencilStates[4], 0);
	_context.Draw(3, 0);
}

void ff::XamlRenderDevice11::ClearTextures()
{
	ID3D11ShaderResourceView* textures[(size_t)TextureSlot::Count] = { nullptr };
	_context.SetResourcesPS(textures, 0, _countof(textures));
}

void ff::XamlRenderDevice11::SetShaders(const Noesis::Batch& batch)
{
	const Program& program = Programs[batch.shader.v];

	_context.SetLayoutIA(_vertexStages[program.vShaderIdx].layout);
	_context.SetVS(_vertexStages[program.vShaderIdx].shader);
	_context.SetPS(_pixelShaders[program.pShaderIdx]);
}

void ff::XamlRenderDevice11::SetBuffers(const Noesis::Batch& batch)
{
	// Indices
	_context.SetIndexIA(_bufferIndices->AsGraphBuffer11()->GetBuffer(), DXGI_FORMAT_R16_UINT, 0);

	// Vertices
	const Program& program = Programs[batch.shader.v];
	unsigned int stride = _vertexStages[program.vShaderIdx].stride;
	_context.SetVertexIA(_bufferVertices->AsGraphBuffer11()->GetBuffer(), stride, batch.vertexOffset);

	// Vertex Constants
	if (_vertexCBHash != batch.projMtxHash)
	{
		void* ptr = _bufferVertexCB->Map(16 * sizeof(float));
		::memcpy(ptr, batch.projMtx, 16 * sizeof(float));
		_bufferVertexCB->Unmap();

		_vertexCBHash = batch.projMtxHash;

	}

	// Pixel Constants
	if (batch.rgba != 0 || batch.radialGrad != 0 || batch.opacity != 0)
	{
		uint32_t hash = batch.rgbaHash ^ batch.radialGradHash ^ batch.opacityHash;
		if (_pixelCBHash != hash)
		{
			void* ptr = _bufferPixelCB->Map(12 * sizeof(float));

			if (batch.rgba != 0)
			{
				memcpy(ptr, batch.rgba, 4 * sizeof(float));
				ptr = (uint8_t*)ptr + 4 * sizeof(float);
			}

			if (batch.radialGrad != 0)
			{
				memcpy(ptr, batch.radialGrad, 8 * sizeof(float));
				ptr = (uint8_t*)ptr + 8 * sizeof(float);
			}

			if (batch.opacity != 0)
			{
				memcpy(ptr, batch.opacity, sizeof(float));
			}

			_bufferPixelCB->Unmap();

			_pixelCBHash = hash;

		}
	}
}

void ff::XamlRenderDevice11::SetRenderState(const Noesis::Batch& batch)
{
	Noesis::RenderState renderState = batch.renderState;

	uint32_t rasterizerIdx = renderState.f.wireframe | (renderState.f.scissorEnable << 1);
	ID3D11RasterizerState* rasterizer = _rasterizerStates[rasterizerIdx];
	_context.SetRaster(rasterizer);

	uint32_t blendIdx = renderState.f.blendMode | (renderState.f.colorEnable << 1);
	ID3D11BlendState* blend = _blendStates[blendIdx];
	_context.SetBlend(blend, ff::GetColorNone(), 0xffffffff);

	uint32_t depthIdx = renderState.f.stencilMode;
	ID3D11DepthStencilState* depth = _depthStencilStates[depthIdx];
	_context.SetDepth(depth, batch.stencilRef);
}

void ff::XamlRenderDevice11::SetTextures(const Noesis::Batch& batch)
{
	if (batch.pattern)
	{
		XamlTexture* t = XamlTexture::Get(batch.pattern);
		ID3D11ShaderResourceView* view = t->GetTexture()->AsTextureView()->AsTextureView11()->GetView();
		ID3D11SamplerState* sampler = _samplerStates[batch.patternSampler.v];
		_context.SetResourcesPS(&view, (size_t)TextureSlot::Pattern, 1);
		_context.SetSamplersPS(&sampler, (size_t)TextureSlot::Pattern, 1);
	}

	if (batch.ramps)
	{
		XamlTexture* t = XamlTexture::Get(batch.ramps);
		ID3D11ShaderResourceView* view = t->GetTexture()->AsTextureView()->AsTextureView11()->GetView();
		ID3D11SamplerState* sampler = _samplerStates[batch.rampsSampler.v];
		_context.SetResourcesPS(&view, (size_t)TextureSlot::Ramps, 1);
		_context.SetSamplersPS(&sampler, (size_t)TextureSlot::Ramps, 1);
	}

	if (batch.image)
	{
		XamlTexture* t = XamlTexture::Get(batch.image);
		ID3D11ShaderResourceView* view = t->GetTexture()->AsTextureView()->AsTextureView11()->GetView();
		ID3D11SamplerState* sampler = _samplerStates[batch.imageSampler.v];
		_context.SetResourcesPS(&view, (size_t)TextureSlot::Image, 1);
		_context.SetSamplersPS(&sampler, (size_t)TextureSlot::Image, 1);
	}

	if (batch.glyphs)
	{
		XamlTexture* t = XamlTexture::Get(batch.glyphs);
		ID3D11ShaderResourceView* view = t->GetTexture()->AsTextureView()->AsTextureView11()->GetView();
		ID3D11SamplerState* sampler = _samplerStates[batch.glyphsSampler.v];
		_context.SetResourcesPS(&view, (size_t)TextureSlot::Glyphs, 1);
		_context.SetSamplersPS(&sampler, (size_t)TextureSlot::Glyphs, 1);
	}
}
