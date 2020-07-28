#include "pch.h"
#include "Data/Compression.h"
#include "Data/DataWriterReader.h"
#include "Globals/AppGlobals.h"
#include "Graph/GraphBuffer.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/State/GraphContext11.h"
#include "Graph/State/GraphStateCache11.h"
#include "Graph/Texture/Palette.h"
#include "Graph/Texture/PaletteData.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "Module/Module.h"
#include "UI/Internal/XamlRenderDevice11.h"
#include "UI/Internal/XamlTexture.h"
#include "UI/Internal/XamlRenderTarget.h"
#include "String/StringUtil.h"

struct TexDimensions
{
	ff::PointFloat imageSize;
	ff::PointFloat imageInverseSize;
	ff::PointFloat patternSize;
	ff::PointFloat patternInverseSize;
	unsigned int paletteRow;
	unsigned int padding[3];
};

static const size_t PREALLOCATED_DYNAMIC_PAGES = 1;
static const size_t VS_CBUFFER_SIZE = 16 * sizeof(float); // projectionMtx
static const size_t PS_CBUFFER_SIZE = 12 * sizeof(float); // rgba | radialGrad opacity | opacity
static const size_t PS_EFFECTS_SIZE = 16 * sizeof(float);
static const size_t TEX_DIMENSIONS_SIZE = sizeof(TexDimensions);
static const uint32_t VFPos = 0;
static const uint32_t VFColor = 1;
static const uint32_t VFTex0 = 2;
static const uint32_t VFTex1 = 4;
static const uint32_t VFTex2 = 8;
static const uint32_t VFCoverage = 16;

static const Noesis::Pair<uint32_t, uint32_t> LAYOUT_FORMATS_AND_STRIDE[] =
{
	{ VFPos, 8 },
	{ VFPos | VFColor, 12 },
	{ VFPos | VFTex0, 16 },
	{ VFPos | VFColor | VFCoverage, 16 },
	{ VFPos | VFTex0 | VFCoverage, 20 },
	{ VFPos | VFColor | VFTex1, 20 },
	{ VFPos | VFTex0 | VFTex1, 24 },
	{ VFPos | VFColor | VFTex1 | VFTex2, 28 },
	{ VFPos | VFTex0 | VFTex1 | VFTex2, 32 },
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
	, _effectCBHash(0)
	, _texDimensionsCBHash(0)
	, _nullTextures{ nullptr }
{
	_graph->AddChild(this);

#ifdef _DEBUG
	_emptyTextureRgb = _graph->CreateTexture(ff::PointInt::Ones(), ff::TextureFormat::RGBA32);
	_emptyTexturePalette = _graph->CreateTexture(ff::PointInt::Ones(), ff::TextureFormat::R8_UINT);
#endif

	_caps.centerPixelOffset = 0;
	_caps.linearRendering = sRGB;
	_caps.subpixelRendering = false;

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
	return XamlRenderTarget::Get(surface)->Clone(name);
}

Noesis::Ptr<Noesis::Texture> ff::XamlRenderDevice11::CreateTexture(const char* label, uint32_t width, uint32_t height, uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** data)
{
	ff::String name = label ? ff::StringFromUTF8(label) : ff::GetEmptyString();
	ff::ComPtr<ff::ITexture> texture;

	if (data == nullptr)
	{
		ff::TextureFormat format2 = (format == Noesis::TextureFormat::R8) ? ff::TextureFormat::R8 : (_caps.linearRendering ? ff::TextureFormat::RGBA32_SRGB : ff::TextureFormat::RGBA32);
		ff::PointInt size = ff::PointSize(width, height).ToType<int>();

		texture = _graph->CreateTexture(size, format2, numLevels, 1, 1);
	}
	else
	{
		DXGI_FORMAT format2 = (format == Noesis::TextureFormat::R8) ? DXGI_FORMAT_R8_UNORM : (_caps.linearRendering ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
		const uint32_t bpp = (format == DXGI_FORMAT_R8_UNORM) ? 1 : 4;

		DirectX::ScratchImage scratch;
		assertHrRetVal(scratch.Initialize2D(format2, width, height, 1, numLevels), nullptr);

		for (size_t i = 0; i < numLevels; i++)
		{
			const DirectX::Image* image = scratch.GetImage(i, 0, 0);
			BYTE* dest = image->pixels;
			const BYTE* source = (const BYTE*)data[i];
			size_t sourcePitch = (width >> i) * bpp;
			size_t destPitch = image->rowPitch;

			for (size_t y = 0; y < image->height; y++, dest += destPitch, source += sourcePitch)
			{
				std::memcpy(dest, source, bpp * image->width);
			}
		}

		texture = _graph->AsGraphDeviceInternal()->CreateTexture(std::move(scratch), nullptr);
	}

	return *new XamlTexture(texture, nullptr, name);
}

void ff::XamlRenderDevice11::UpdateTexture(Noesis::Texture* texture, uint32_t level, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* data)
{
	ff::ITexture* texture2 = XamlTexture::Get(texture)->GetTexture();
	texture2->Update(0, level, ff::RectSize(x, y, x + width, y + height), data, texture2->GetFormat(), true);
}

void ff::XamlRenderDevice11::BeginRender(bool offscreen)
{
	std::array<ID3D11Buffer*, 2> bufferVS =
	{
		_bufferVertexCB->AsGraphBuffer11()->GetBuffer(),
		_texDimensionsCB->AsGraphBuffer11()->GetBuffer(),
	};

	std::array<ID3D11Buffer*, 3> bufferPS =
	{
		_bufferPixelCB->AsGraphBuffer11()->GetBuffer(),
		_texDimensionsCB->AsGraphBuffer11()->GetBuffer(),
		_effectCB->AsGraphBuffer11()->GetBuffer(),
	};

	_context.SetConstantsVS(bufferVS.data(), 0, bufferVS.size());
	_context.SetConstantsPS(bufferPS.data(), 0, bufferPS.size());
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
		_context.SetVS(_quadVS.GetShader(_states));
		_context.SetPS(_resolvePS[indexPS].GetShader(_states));

		_context.SetRaster(_rasterizerStates[2]);
		_context.SetBlend(_blendStates[0], ff::GetColorNone(), 0xffffffff);
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
	assert(batch.shader.v < _countof(_programs));
	const Program& program = _programs[batch.shader.v];
	assert(program._vertexShaderIndex != -1 && program._vertexShaderIndex < _countof(_vertexStages));
	assert(program._pixelShaderIndex != -1 && program._pixelShaderIndex < _countof(_pixelStages));

	SetShaders(batch);
	SetBuffers(batch);
	SetRenderState(batch);
	SetTextures(batch);

	_context.DrawIndexed(batch.numIndices, batch.startIndex, 0);
	_context.SetResourcesPS(_nullTextures.data(), 0, _nullTextures.size());
}

ff::IGraphDevice* ff::XamlRenderDevice11::GetDevice() const
{
	return _graph;
}

bool ff::XamlRenderDevice11::Reset()
{
	_vertexCBHash = 0;
	_pixelCBHash = 0;
	_effectCBHash = 0;
	_texDimensionsCBHash = 0;

	CreateBuffers();
	CreateStateObjects();
	CreateShaders();

	return true;
}

ff::ComPtr<ID3D11InputLayout> ff::XamlRenderDevice11::CreateLayout(ff::GraphStateCache11& states, uint32_t format, ff::StringRef vertexResourceName)
{
	D3D11_INPUT_ELEMENT_DESC descs[5];
	uint32_t element = 0;

	descs[element].SemanticIndex = 0;
	descs[element].InputSlot = 0;
	descs[element].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	descs[element].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	descs[element].InstanceDataStepRate = 0;
	descs[element].SemanticName = "POSITION";
	descs[element].Format = DXGI_FORMAT_R32G32_FLOAT;
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
		element++;
	}

	if (format & VFTex2)
	{
		descs[element].SemanticIndex = 2;
		descs[element].InputSlot = 0;
		descs[element].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		descs[element].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		descs[element].InstanceDataStepRate = 0;
		descs[element].SemanticName = "TEXCOORD";
		descs[element].Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		element++;
	}

	if (format & VFCoverage)
	{
		descs[element].SemanticIndex = 3;
		descs[element].InputSlot = 0;
		descs[element].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		descs[element].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		descs[element].InstanceDataStepRate = 0;
		descs[element].SemanticName = "TEXCOORD";
		descs[element].Format = DXGI_FORMAT_R32_FLOAT;
		element++;
	}

	return states.GetInputLayout(ff::GetThisModule().GetResources(), vertexResourceName, descs, element);
}

void ff::XamlRenderDevice11::CreateBuffers()
{
	_bufferVertices = _graph->CreateBuffer(ff::GraphBufferType::Vertex, DYNAMIC_VB_SIZE);
	_bufferIndices = _graph->CreateBuffer(ff::GraphBufferType::Index, DYNAMIC_IB_SIZE);
	_bufferVertexCB = _graph->CreateBuffer(ff::GraphBufferType::Constant, VS_CBUFFER_SIZE);
	_bufferPixelCB = _graph->CreateBuffer(ff::GraphBufferType::Constant, PS_CBUFFER_SIZE);
	_effectCB = _graph->CreateBuffer(ff::GraphBufferType::Constant, PS_EFFECTS_SIZE);
	_texDimensionsCB = _graph->CreateBuffer(ff::GraphBufferType::Constant, TEX_DIMENSIONS_SIZE);
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
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		// Src
		desc.RenderTarget[0].BlendEnable = false;
		_blendStates[0] = _states.GetBlendState(desc);

		// SrcOver
		desc.RenderTarget[0].BlendEnable = true;
		_blendStates[1] = _states.GetBlendState(desc);

		// SrcOver_Dual
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC1_COLOR;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC1_ALPHA;
		_blendStates[2] = _states.GetBlendState(desc);

		// Color disabled
		desc.RenderTarget[0].BlendEnable = false;
		desc.RenderTarget[0].RenderTargetWriteMask = 0;
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
	for (uint8_t minmag = 0; minmag < Noesis::MinMagFilter::Count; minmag++)
	{
		for (uint8_t mip = 0; mip < Noesis::MipFilter::Count; mip++)
		{
			for (uint8_t uv = 0; uv < Noesis::WrapMode::Count; uv++)
			{
				Noesis::SamplerState s = { {uv, minmag, mip} };
				_samplerStages[s.v]._params = s;
				_samplerStages[s.v]._state = nullptr;
			}
		}
	}
}

void ff::XamlRenderDevice11::CreateShaders()
{
	struct Shader
	{
		const wchar_t* resourceName;
		uint32_t layout;
	};

	const Shader pShaders[] =
	{
		{ L"Noesis.RGBA_FS" },
		{ L"Noesis.Mask_FS" },

		{ L"Noesis.PathSolid_FS" },
		{ L"Noesis.PathLinear_FS" },
		{ L"Noesis.PathRadial_FS" },
		{ L"Noesis.PathPattern_FS" },

		{ L"Noesis.PathAASolid_FS" },
		{ L"Noesis.PathAALinear_FS" },
		{ L"Noesis.PathAARadial_FS" },
		{ L"Noesis.PathAAPattern_FS" },

		{ L"Noesis.SDFSolid_FS" },
		{ L"Noesis.SDFLinear_FS" },
		{ L"Noesis.SDFRadial_FS" },
		{ L"Noesis.SDFPattern_FS" },

		{ L"Noesis.SDFLCDSolid_FS" },
		{ L"Noesis.SDFLCDLinear_FS" },
		{ L"Noesis.SDFLCDRadial_FS" },
		{ L"Noesis.SDFLCDPattern_FS" },

		{ L"Noesis.ImageOpacitySolid_FS" },
		{ L"Noesis.ImageOpacityLinear_FS" },
		{ L"Noesis.ImageOpacityRadial_FS" },
		{ L"Noesis.ImageOpacityPattern_FS" },

		{ L"Noesis.ImageShadow35V_FS" },
		{ L"Noesis.ImageShadow63V_FS" },
		{ L"Noesis.ImageShadow127V_FS" },

		{ L"Noesis.ImageShadow35HSolid_FS" },
		{ L"Noesis.ImageShadow35HLinear_FS" },
		{ L"Noesis.ImageShadow35HRadial_FS" },
		{ L"Noesis.ImageShadow35HPattern_FS" },

		{ L"Noesis.ImageShadow63HSolid_FS" },
		{ L"Noesis.ImageShadow63HLinear_FS" },
		{ L"Noesis.ImageShadow63HRadial_FS" },
		{ L"Noesis.ImageShadow63HPattern_FS" },

		{ L"Noesis.ImageShadow127HSolid_FS" },
		{ L"Noesis.ImageShadow127HLinear_FS" },
		{ L"Noesis.ImageShadow127HRadial_FS" },
		{ L"Noesis.ImageShadow127HPattern_FS" },

		{ L"Noesis.ImageBlur35V_FS" },
		{ L"Noesis.ImageBlur63V_FS" },
		{ L"Noesis.ImageBlur127V_FS" },

		{ L"Noesis.ImageBlur35HSolid_FS" },
		{ L"Noesis.ImageBlur35HLinear_FS" },
		{ L"Noesis.ImageBlur35HRadial_FS" },
		{ L"Noesis.ImageBlur35HPattern_FS" },

		{ L"Noesis.ImageBlur63HSolid_FS" },
		{ L"Noesis.ImageBlur63HLinear_FS" },
		{ L"Noesis.ImageBlur63HRadial_FS" },
		{ L"Noesis.ImageBlur63HPattern_FS" },

		{ L"Noesis.ImageBlur127HSolid_FS" },
		{ L"Noesis.ImageBlur127HLinear_FS" },
		{ L"Noesis.ImageBlur127HRadial_FS" },
		{ L"Noesis.ImageBlur127HPattern_FS" },
	};

	const Shader vShaders[] =
	{
		{ L"Noesis.Pos_VS", 0 },
		{ L"Noesis.PosColor_VS", 1 },
		{ L"Noesis.PosTex0_VS", 2 },
		{ L"Noesis.PosColorCoverage_VS", 3 },
		{ L"Noesis.PosTex0Coverage_VS", 4 },
		{ L"Noesis.PosColorTex1_VS", 5 },
		{ L"Noesis.PosTex0Tex1_VS", 6 },
		{ L"Noesis.PosColorTex1_SDF_VS", 5 },
		{ L"Noesis.PosTex0Tex1_SDF_VS", 6 },
		{ L"Noesis.PosColorTex1Tex2_VS", 7 },
		{ L"Noesis.PosTex0Tex1Tex2_VS", 8 },
	};

	static_assert(_countof(vShaders) == _countof(_vertexStages));
	static_assert(_countof(pShaders) == _countof(_pixelStages));
	static_assert(_countof(pShaders) == _countof(_programs));

	for (uint32_t i = 0; i < _countof(vShaders); i++)
	{
		const Shader& shader = vShaders[i];
		_vertexStages[i]._resourceName = ff::String::from_static(shader.resourceName);
		_vertexStages[i]._shader = nullptr;
		_vertexStages[i]._layout = nullptr;
		_vertexStages[i]._layoutIndex = shader.layout;
	}

	for (uint32_t i = 0; i < _countof(pShaders); i++)
	{
		const Shader& shader = pShaders[i];
		_pixelStages[i]._resourceName = ff::String::from_static(shader.resourceName);
		_pixelStages[i]._shader = nullptr;
	}

	_resolvePS[0]._resourceName = ff::String::from_static(L"Noesis.Resolve2_PS");
	_resolvePS[0]._shader = nullptr;

	_resolvePS[1]._resourceName = ff::String::from_static(L"Noesis.Resolve4_PS");
	_resolvePS[1]._shader = nullptr;

	_resolvePS[2]._resourceName = ff::String::from_static(L"Noesis.Resolve8_PS");
	_resolvePS[2]._shader = nullptr;

	_resolvePS[3]._resourceName = ff::String::from_static(L"Noesis.Resolve16_PS");
	_resolvePS[3]._shader = nullptr;

	_clearPS._resourceName = ff::String::from_static(L"Noesis.Clear_PS");
	_clearPS._shader = nullptr;

	_quadVS._resourceName = ff::String::from_static(L"Noesis.Quad_VS");
	_quadVS._shader = nullptr;

	std::memset(_programs, 255, sizeof(_programs));

	_programs[Noesis::Shader::RGBA] = { 0, 0 };
	_programs[Noesis::Shader::Mask] = { 0, 1 };

	_programs[Noesis::Shader::Path_Solid] = { 1, 2 };
	_programs[Noesis::Shader::Path_Linear] = { 2, 3 };
	_programs[Noesis::Shader::Path_Radial] = { 2, 4 };
	_programs[Noesis::Shader::Path_Pattern] = { 2, 5 };

	_programs[Noesis::Shader::PathAA_Solid] = { 3, 6 };
	_programs[Noesis::Shader::PathAA_Linear] = { 4, 7 };
	_programs[Noesis::Shader::PathAA_Radial] = { 4, 8 };
	_programs[Noesis::Shader::PathAA_Pattern] = { 4, 9 };

	_programs[Noesis::Shader::SDF_Solid] = { 7, 10 };
	_programs[Noesis::Shader::SDF_Linear] = { 8, 11 };
	_programs[Noesis::Shader::SDF_Radial] = { 8, 12 };
	_programs[Noesis::Shader::SDF_Pattern] = { 8, 13 };

	_programs[Noesis::Shader::SDF_LCD_Solid] = { 7, 14 };
	_programs[Noesis::Shader::SDF_LCD_Linear] = { 8, 15 };
	_programs[Noesis::Shader::SDF_LCD_Radial] = { 8, 16 };
	_programs[Noesis::Shader::SDF_LCD_Pattern] = { 8, 17 };

	_programs[Noesis::Shader::Image_Opacity_Solid] = { 5, 18 };
	_programs[Noesis::Shader::Image_Opacity_Linear] = { 6, 19 };
	_programs[Noesis::Shader::Image_Opacity_Radial] = { 6, 20 };
	_programs[Noesis::Shader::Image_Opacity_Pattern] = { 6, 21 };

	_programs[Noesis::Shader::Image_Shadow35V] = { 9, 22 };
	_programs[Noesis::Shader::Image_Shadow63V] = { 9, 23 };
	_programs[Noesis::Shader::Image_Shadow127V] = { 9, 24 };

	_programs[Noesis::Shader::Image_Shadow35H_Solid] = { 9, 25 };
	_programs[Noesis::Shader::Image_Shadow35H_Linear] = { 10, 26 };
	_programs[Noesis::Shader::Image_Shadow35H_Radial] = { 10, 27 };
	_programs[Noesis::Shader::Image_Shadow35H_Pattern] = { 10, 28 };

	_programs[Noesis::Shader::Image_Shadow63H_Solid] = { 9, 29 };
	_programs[Noesis::Shader::Image_Shadow63H_Linear] = { 10, 30 };
	_programs[Noesis::Shader::Image_Shadow63H_Radial] = { 10, 31 };
	_programs[Noesis::Shader::Image_Shadow63H_Pattern] = { 10, 32 };

	_programs[Noesis::Shader::Image_Shadow127H_Solid] = { 9, 33 };
	_programs[Noesis::Shader::Image_Shadow127H_Linear] = { 10, 34 };
	_programs[Noesis::Shader::Image_Shadow127H_Radial] = { 10, 35 };
	_programs[Noesis::Shader::Image_Shadow127H_Pattern] = { 10, 36 };

	_programs[Noesis::Shader::Image_Blur35V] = { 9, 37 };
	_programs[Noesis::Shader::Image_Blur63V] = { 9, 38 };
	_programs[Noesis::Shader::Image_Blur127V] = { 9, 39 };

	_programs[Noesis::Shader::Image_Blur35H_Solid] = { 9, 40 };
	_programs[Noesis::Shader::Image_Blur35H_Linear] = { 10, 41 };
	_programs[Noesis::Shader::Image_Blur35H_Radial] = { 10, 42 };
	_programs[Noesis::Shader::Image_Blur35H_Pattern] = { 10, 43 };

	_programs[Noesis::Shader::Image_Blur63H_Solid] = { 9, 44 };
	_programs[Noesis::Shader::Image_Blur63H_Linear] = { 10, 45 };
	_programs[Noesis::Shader::Image_Blur63H_Radial] = { 10, 46 };
	_programs[Noesis::Shader::Image_Blur63H_Pattern] = { 10, 47 };

	_programs[Noesis::Shader::Image_Blur127H_Solid] = { 9, 48 };
	_programs[Noesis::Shader::Image_Blur127H_Linear] = { 10, 49 };
	_programs[Noesis::Shader::Image_Blur127H_Radial] = { 10, 50 };
	_programs[Noesis::Shader::Image_Blur127H_Pattern] = { 10, 51 };
}

void ff::XamlRenderDevice11::ClearRenderTarget()
{
	_context.SetLayoutIA(nullptr);
	_context.SetGS(nullptr);
	_context.SetVS(_quadVS.GetShader(_states));
	_context.SetPS(_clearPS.GetShader(_states));
	_context.SetRaster(_rasterizerStates[2]);
	_context.SetBlend(_blendStates[0], ff::GetColorNone(), 0xffffffff);
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
	const Program& program = _programs[batch.shader.v];
	VertexAndLayoutStage& vertex = _vertexStages[program._vertexShaderIndex];
	PixelStage& pixel = _pixelStages[program._pixelShaderIndex];

	_context.SetLayoutIA(vertex.GetLayout(_states));
	_context.SetVS(vertex.GetShader(_states));
	_context.SetPS(pixel.GetShader(_states));
}

void ff::XamlRenderDevice11::SetBuffers(const Noesis::Batch& batch)
{
	// Indices
	_context.SetIndexIA(_bufferIndices->AsGraphBuffer11()->GetBuffer(), DXGI_FORMAT_R16_UINT, 0);

	// Vertices
	const Program& program = _programs[batch.shader.v];
	unsigned int stride = ::LAYOUT_FORMATS_AND_STRIDE[_vertexStages[program._vertexShaderIndex]._layoutIndex].second;
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

	// Texture dimensions
	if (batch.glyphs != 0 || batch.image != 0 || batch.pattern != 0)
	{
		XamlTexture* imageTexture = XamlTexture::Get(batch.glyphs ? batch.glyphs : batch.image);
		XamlTexture* patternTexture = XamlTexture::Get(batch.pattern);

		TexDimensions data{};
		data.imageSize = imageTexture ? imageTexture->GetTexture()->GetSize().ToType<float>() : ff::PointFloat::Zeros();
		data.imageInverseSize = imageTexture ? ff::PointFloat::Ones() / data.imageSize : ff::PointFloat::Zeros();
		data.patternSize = patternTexture ? patternTexture->GetTexture()->GetSize().ToType<float>() : ff::PointFloat::Zeros();
		data.patternInverseSize = patternTexture ? ff::PointFloat::Ones() / data.patternSize : ff::PointFloat::Zeros();
		data.paletteRow = (unsigned int)((patternTexture && patternTexture->GetPalette()) ? patternTexture->GetPalette()->GetCurrentRow() : 0);
		ff::hash_t hash = ff::HashFunc(data);

		if (_texDimensionsCBHash != hash)
		{
			TexDimensions* mappedData = (TexDimensions*)_texDimensionsCB->Map(TEX_DIMENSIONS_SIZE);
			std::memcpy(mappedData, &data, TEX_DIMENSIONS_SIZE);
			_texDimensionsCB->Unmap();
			_texDimensionsCBHash = hash;
		}
	}

	// Effects
	if (batch.effectParamsSize != 0 && _effectCBHash != batch.effectParamsHash)
	{
		void* ptr = _effectCB->Map(16 * sizeof(float));
		std::memcpy(ptr, batch.effectParams, batch.effectParamsSize * sizeof(float));
		_effectCB->Unmap();
		_effectCBHash = batch.effectParamsHash;
	}
}

void ff::XamlRenderDevice11::SetRenderState(const Noesis::Batch& batch)
{
	Noesis::RenderState renderState = batch.renderState;

	uint32_t rasterizerIdx = renderState.f.wireframe | (renderState.f.scissorEnable << 1);
	ID3D11RasterizerState* rasterizer = _rasterizerStates[rasterizerIdx];
	_context.SetRaster(rasterizer);

	uint32_t blendIdx = renderState.f.colorEnable ? renderState.f.blendMode : 3;
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
		bool palette = (t->GetTexture()->GetFormat() == ff::TextureFormat::R8_UINT);
		ID3D11ShaderResourceView* view = t->GetTexture()->AsTextureView()->AsTextureView11()->GetView();
		ID3D11ShaderResourceView* paletteView = (palette && t->GetPalette()) ? t->GetPalette()->GetData()->GetTexture()->AsTextureView()->AsTextureView11()->GetView() : nullptr;
#ifdef _DEBUG
		ID3D11ShaderResourceView* emptyView = _emptyTextureRgb->AsTextureView()->AsTextureView11()->GetView();
		ID3D11ShaderResourceView* emptyPaletteView = _emptyTexturePalette->AsTextureView()->AsTextureView11()->GetView();
#else
		ID3D11ShaderResourceView* emptyView = nullptr;
		ID3D11ShaderResourceView* emptyPaletteView = nullptr;
#endif
		ID3D11ShaderResourceView* views[3] = { !palette ? view : emptyView, palette ? view : emptyPaletteView, paletteView };
		ID3D11SamplerState* sampler = _samplerStages[batch.patternSampler.v].GetState(_states);
		_context.SetResourcesPS(views, (size_t)TextureSlot::Pattern, 3);
		_context.SetSamplersPS(&sampler, (size_t)TextureSlot::Pattern, 1);
	}

	if (batch.ramps)
	{
		XamlTexture* t = XamlTexture::Get(batch.ramps);
		ID3D11ShaderResourceView* view = t->GetTexture()->AsTextureView()->AsTextureView11()->GetView();
		ID3D11SamplerState* sampler = _samplerStages[batch.rampsSampler.v].GetState(_states);
		_context.SetResourcesPS(&view, (size_t)TextureSlot::Ramps, 1);
		_context.SetSamplersPS(&sampler, (size_t)TextureSlot::Ramps, 1);
	}

	if (batch.image)
	{
		XamlTexture* t = XamlTexture::Get(batch.image);
		ID3D11ShaderResourceView* view = t->GetTexture()->AsTextureView()->AsTextureView11()->GetView();
		ID3D11SamplerState* sampler = _samplerStages[batch.imageSampler.v].GetState(_states);
		_context.SetResourcesPS(&view, (size_t)TextureSlot::Image, 1);
		_context.SetSamplersPS(&sampler, (size_t)TextureSlot::Image, 1);
	}

	if (batch.glyphs)
	{
		XamlTexture* t = XamlTexture::Get(batch.glyphs);
		ID3D11ShaderResourceView* view = t->GetTexture()->AsTextureView()->AsTextureView11()->GetView();
		ID3D11SamplerState* sampler = _samplerStages[batch.glyphsSampler.v].GetState(_states);
		_context.SetResourcesPS(&view, (size_t)TextureSlot::Glyphs, 1);
		_context.SetSamplersPS(&sampler, (size_t)TextureSlot::Glyphs, 1);
	}

	if (batch.shadow)
	{
		XamlTexture* t = XamlTexture::Get(batch.shadow);
		ID3D11ShaderResourceView* view = t->GetTexture()->AsTextureView()->AsTextureView11()->GetView();
		ID3D11SamplerState* sampler = _samplerStages[batch.shadowSampler.v].GetState(_states);
		_context.SetResourcesPS(&view, (size_t)TextureSlot::Shadow, 1);
		_context.SetSamplersPS(&sampler, (size_t)TextureSlot::Shadow, 1);
	}
}

ID3D11VertexShader* ff::XamlRenderDevice11::VertexStage::GetShader(ff::GraphStateCache11& states)
{
	if (!_shader)
	{
		_shader = states.GetVertexShader(ff::GetThisModule().GetResources(), _resourceName);
	}

	return _shader;
}

ID3D11InputLayout* ff::XamlRenderDevice11::VertexAndLayoutStage::GetLayout(ff::GraphStateCache11& states)
{
	if (!_layout)
	{
		_layout = ff::XamlRenderDevice11::CreateLayout(states, ::LAYOUT_FORMATS_AND_STRIDE[_layoutIndex].first, _resourceName);
	}

	return _layout;
}

ID3D11PixelShader* ff::XamlRenderDevice11::PixelStage::GetShader(ff::GraphStateCache11& states)
{
	if (!_shader)
	{
		_shader = states.GetPixelShader(ff::GetThisModule().GetResources(), _resourceName);
	}

	return _shader;
}

ID3D11SamplerState* ff::XamlRenderDevice11::SamplerStage::GetState(ff::GraphStateCache11& states)
{
	if (!_state)
	{
		D3D11_SAMPLER_DESC desc{};
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MinLOD = -D3D11_FLOAT32_MAX;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		desc.Filter = ToD3D(Noesis::MinMagFilter::Enum(_params.f.minmagFilter), Noesis::MipFilter::Enum(_params.f.mipFilter));
		::ToD3D(Noesis::WrapMode::Enum(_params.f.wrapMode), desc.AddressU, desc.AddressV);

		_state = states.GetSamplerState(desc);
	}

	return _state;
}
