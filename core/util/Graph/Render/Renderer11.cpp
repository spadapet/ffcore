#include "pch.h"
#include "Data/Data.h"
#include "Graph/Anim/Transform.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphDeviceChild.h"
#include "Graph/GraphShader.h"
#include "Graph/Render/MatrixStack.h"
#include "Graph/Render/Renderer.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Render/RendererVertex.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteType.h"
#include "Graph/State/GraphContext11.h"
#include "Graph/State/GraphFixedState11.h"
#include "Graph/State/GraphStateCache11.h"
#include "Graph/Texture/Palette.h"
#include "Graph/Texture/PaletteData.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "Resource/ResourceValue.h"

static const size_t MAX_TEXTURES = 32;
static const size_t MAX_TEXTURES_USING_PALETTE = 32;
static const size_t MAX_PALETTES = 128; // 256 color palettes only
static const size_t MAX_TRANSFORM_MATRIXES = 1024;
static const size_t MAX_RENDER_COUNT = 524288; // 0x00080000
static const float MAX_RENDER_DEPTH = 1.0f;
static const float RENDER_DEPTH_DELTA = MAX_RENDER_DEPTH / MAX_RENDER_COUNT;

static std::array<ID3D11ShaderResourceView*, ::MAX_TEXTURES + ::MAX_TEXTURES_USING_PALETTE + 1 /* palette */>  NULL_TEXTURES = { nullptr };

enum class GeometryBucketType
{
	Lines,
	Circle,
	Triangles,
	Sprites,
	PaletteSprites,

	LinesAlpha,
	CircleAlpha,
	TrianglesAlpha,
	SpritesAlpha,
	PaletteSpritesAlpha,

	Count,
	FirstAlpha = LinesAlpha,
};

class GeometryBucket
{
private:
	GeometryBucket(GeometryBucketType bucketType, const std::type_info& itemType, size_t itemSize, size_t itemAlign)
		: _bucketType(bucketType)
		, _itemType(&itemType)
		, _itemSize(itemSize)
		, _itemAlign(itemAlign)
		, _renderStart(0)
		, _renderCount(0)
		, _dataStart(nullptr)
		, _dataCur(nullptr)
		, _dataEnd(nullptr)
	{
	}

public:
	template<typename T, GeometryBucketType BucketType>
	static GeometryBucket New()
	{
		return GeometryBucket(BucketType, typeid(T), sizeof(T), alignof(T));
	}

	GeometryBucket(GeometryBucket&& rhs)
		: _layout(std::move(rhs._layout))
		, _vs(std::move(rhs._vs))
		, _gs(std::move(rhs._gs))
		, _ps(std::move(rhs._ps))
		, _psPaletteOut(std::move(rhs._psPaletteOut))
		, _bucketType(rhs._bucketType)
		, _itemType(rhs._itemType)
		, _itemSize(rhs._itemSize)
		, _itemAlign(rhs._itemAlign)
		, _renderStart(0)
		, _renderCount(0)
		, _dataStart(rhs._dataStart)
		, _dataCur(rhs._dataCur)
		, _dataEnd(rhs._dataEnd)
	{
		rhs._dataStart = nullptr;
		rhs._dataCur = nullptr;
		rhs._dataEnd = nullptr;
	}

	~GeometryBucket()
	{
		_aligned_free(_dataStart);
	}

	void Reset(
		ID3D11InputLayout* layout = nullptr,
		ID3D11VertexShader* vs = nullptr,
		ID3D11GeometryShader* gs = nullptr,
		ID3D11PixelShader* ps = nullptr,
		ID3D11PixelShader* psPaletteOut = nullptr)
	{
		_layout = layout;
		_vs = vs;
		_gs = gs;
		_ps = ps;
		_psPaletteOut = psPaletteOut;

		_aligned_free(_dataStart);
		_dataStart = nullptr;
		_dataCur = nullptr;
		_dataEnd = nullptr;
	}

	void Apply(ff::GraphContext11& context, ID3D11Buffer* geometryBuffer, bool paletteOut) const
	{
		context.SetVertexIA(geometryBuffer, GetItemByteSize(), 0);
		context.SetLayoutIA(_layout);
		context.SetVS(_vs);
		context.SetGS(_gs);
		context.SetPS(paletteOut ? _psPaletteOut : _ps);
	}

	void* Add(const void* data = nullptr)
	{
		if (_dataCur == _dataEnd)
		{
			size_t curSize = _dataEnd - _dataStart;
			size_t newSize = std::max<size_t>(curSize * 2, _itemSize * 64);
			_dataStart = (BYTE*)_aligned_realloc(_dataStart, newSize, _itemAlign);
			_dataCur = _dataStart + curSize;
			_dataEnd = _dataStart + newSize;
		}

		if (data)
		{
			std::memcpy(_dataCur, data, _itemSize);
		}

		void* result = _dataCur;
		_dataCur += _itemSize;
		return result;
	}

	size_t GetItemByteSize() const
	{
		return _itemSize;
	}

	const std::type_info& GetItemType() const
	{
		return *_itemType;
	}

	GeometryBucketType GetBucketType() const
	{
		return _bucketType;
	}

	size_t GetCount() const
	{
		return (_dataCur - _dataStart) / _itemSize;
	}

	void ClearItems()
	{
		_dataCur = _dataStart;
	}

	size_t GetByteSize() const
	{
		return _dataCur - _dataStart;
	}

	const void* GetData() const
	{
		return _dataStart;
	}

	void SetRenderStart(size_t renderStart)
	{
		_renderStart = renderStart;
		_renderCount = GetCount();
	}

	size_t GetRenderStart() const
	{
		return _renderStart;
	}

	size_t GetRenderCount() const
	{
		return _renderCount;
	}

private:
	ff::ComPtr<ID3D11InputLayout> _layout;
	ff::ComPtr<ID3D11VertexShader> _vs;
	ff::ComPtr<ID3D11GeometryShader> _gs;
	ff::ComPtr<ID3D11PixelShader> _ps;
	ff::ComPtr<ID3D11PixelShader> _psPaletteOut;

	GeometryBucketType _bucketType;
	const std::type_info* _itemType;
	size_t _itemSize;
	size_t _itemAlign;
	size_t _renderStart;
	size_t _renderCount;
	BYTE* _dataStart;
	BYTE* _dataCur;
	BYTE* _dataEnd;
};

struct AlphaGeometryEntry
{
	const GeometryBucket* _bucket;
	size_t _index;
	float _depth;
};

struct GeometryShaderConstants0
{
	GeometryShaderConstants0()
	{
		ff::ZeroObject(*this);
	}

	DirectX::XMFLOAT4X4 _projection;
	ff::PointFloat _viewSize;
	ff::PointFloat _viewScale;
	float _zoffset;
	float _padding[3];
};

struct GeometryShaderConstants1
{
	ff::Vector<DirectX::XMFLOAT4X4> _model;
};

struct PixelShaderConstants0
{
	PixelShaderConstants0()
	{
		ff::ZeroObject(*this);
	}

	ff::RectFloat _texturePaletteSizes[::MAX_TEXTURES_USING_PALETTE];
};

class Renderer11
	: public ff::IRenderer
	, public ff::IRendererActive
	, public ff::IRendererActive11
	, public ff::IGraphDeviceChild
	, public ff::IMatrixStackOwner
{
public:
	Renderer11(ff::IGraphDevice* device);
	~Renderer11();

	// IRenderer
	virtual bool IsValid() const override;
	virtual IRendererActive* BeginRender(ff::IRenderTarget* target, ff::IRenderDepth* depth, ff::RectFloat viewRect, ff::RectFloat worldRect) override;

	// IRendererActive
	virtual void EndRender() override;
	virtual ff::MatrixStack& GetWorldMatrixStack() override;
	virtual ff::IRendererActive11* AsRendererActive11() override;

	virtual void DrawSprite(ff::ISprite* sprite, const ff::Transform& transform) override;
	virtual void DrawFont(ff::ISprite* sprite, const ff::Transform& transform) override;
	virtual void DrawLineStrip(const ff::PointFloat* points, const DirectX::XMFLOAT4* colors, size_t count, float thickness, bool pixelThickness) override;
	virtual void DrawLineStrip(const ff::PointFloat* points, size_t count, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness) override;
	virtual void DrawLine(ff::PointFloat start, ff::PointFloat end, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness) override;
	virtual void DrawFilledRectangle(ff::RectFloat rect, const DirectX::XMFLOAT4* colors) override;
	virtual void DrawFilledRectangle(ff::RectFloat rect, const DirectX::XMFLOAT4& color) override;
	virtual void DrawFilledTriangles(const ff::PointFloat* points, const DirectX::XMFLOAT4* colors, size_t count) override;
	virtual void DrawFilledCircle(ff::PointFloat center, float radius, const DirectX::XMFLOAT4& color) override;
	virtual void DrawFilledCircle(ff::PointFloat center, float radius, const DirectX::XMFLOAT4& insideColor, const DirectX::XMFLOAT4& outsideColor) override;
	virtual void DrawOutlineRectangle(ff::RectFloat rect, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness) override;
	virtual void DrawOutlineCircle(ff::PointFloat center, float radius, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness) override;
	virtual void DrawOutlineCircle(ff::PointFloat center, float radius, const DirectX::XMFLOAT4& insideColor, const DirectX::XMFLOAT4& outsideColor, float thickness, bool pixelThickness) override;

	virtual void DrawPaletteFont(ff::ISprite* sprite, ff::PointFloat pos, ff::PointFloat scale, int color) override;
	virtual void DrawPaletteLineStrip(const ff::PointFloat* points, const int* colors, size_t count, float thickness, bool pixelThickness = false) override;
	virtual void DrawPaletteLineStrip(const ff::PointFloat* points, size_t count, int color, float thickness, bool pixelThickness = false) override;
	virtual void DrawPaletteLine(ff::PointFloat start, ff::PointFloat end, int color, float thickness, bool pixelThickness = false) override;
	virtual void DrawPaletteFilledRectangle(ff::RectFloat rect, const int* colors) override;
	virtual void DrawPaletteFilledRectangle(ff::RectFloat rect, int color) override;
	virtual void DrawPaletteFilledTriangles(const ff::PointFloat* points, const int* colors, size_t count) override;
	virtual void DrawPaletteFilledCircle(ff::PointFloat center, float radius, int color) override;
	virtual void DrawPaletteFilledCircle(ff::PointFloat center, float radius, int insideColor, int outsideColor) override;
	virtual void DrawPaletteOutlineRectangle(ff::RectFloat rect, int color, float thickness, bool pixelThickness = false) override;
	virtual void DrawPaletteOutlineCircle(ff::PointFloat center, float radius, int color, float thickness, bool pixelThickness = false) override;
	virtual void DrawPaletteOutlineCircle(ff::PointFloat center, float radius, int insideColor, int outsideColor, float thickness, bool pixelThickness = false) override;

	virtual void PushPalette(ff::IPalette* palette) override;
	virtual void PopPalette() override;
	virtual void PushCustomContext(ff::CustomRenderContextFunc11&& func) override;
	virtual void PopCustomContext() override;
	virtual void PushTextureSampler(D3D11_FILTER filter) override;
	virtual void PopTextureSampler() override;
	virtual void PushNoOverlap() override;
	virtual void PopNoOverlap() override;
	virtual void PushOpaque() override;
	virtual void PopOpaque() override;
	virtual void NudgeDepth() override;

	// IGraphDeviceChild
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IMatrixStackOwner
	virtual void OnMatrixChanging(const ff::MatrixStack& stack) override;
	virtual void OnMatrixChanged(const ff::MatrixStack& stack) override;

private:
	void Destroy();
	bool Init();

	enum class LastDepthType
	{
		None,
		Line,
		Circle,
		Triangle,
		Sprite,

		LineNoOverlap,
		CircleNoOverlap,
		TriangleNoOverlap,
		SpriteNoOverlap,
		FontNoOverlap,

		StartNoOverlap = LineNoOverlap,
	};

	void DrawSprite(ff::ISprite* sprite, const ff::Transform& transform, LastDepthType depthType);
	void DrawLineStrip(const ff::PointFloat* points, size_t pointCount, const DirectX::XMFLOAT4* colors, size_t colorCount, float thickness, bool pixelThickness);

	void InitGeometryConstantBuffers0(ff::IRenderTarget* target, const ff::RectFloat& viewRect, const ff::RectFloat& worldRect);
	void UpdateGeometryConstantBuffers0();
	void UpdateGeometryConstantBuffers1();
	void UpdatePixelConstantBuffers0();
	void UpdatePaletteTexture();
	void SetShaderInput();

	void Flush();
	bool CreateGeometryBuffer();
	void DrawOpaqueGeometry();
	void DrawAlphaGeometry();
	void PostFlush();

	bool IsRendering() const;
	float NudgeDepth(LastDepthType depthType);
	unsigned int GetWorldMatrixIndex();
	unsigned int GetWorldMatrixIndexNoFlush();
	unsigned int GetTextureIndexNoFlush(ff::ITextureView* texture, bool usePalette);
	unsigned int GetPaletteIndexNoFlush();
	void GetWorldMatrixAndTextureIndex(ff::ITextureView* texture, bool usePalette, unsigned int& modelIndex, unsigned int& textureIndex);
	void GetWorldMatrixAndTextureIndexes(ff::ITextureView** textures, bool usePalette, unsigned int* textureIndexes, size_t count, unsigned int& modelIndex);
	void AddGeometry(const void* data, GeometryBucketType bucketType, float depth);
	void* AddGeometry(GeometryBucketType bucketType, float depth);

	ff::GraphFixedState11 CreateOpaqueDrawState();
	ff::GraphFixedState11 CreateAlphaDrawState();
	GeometryBucket& GetGeometryBucket(GeometryBucketType type);

	enum class State
	{
		Invalid,
		Valid,
		Rendering,
	} _state;

	ff::ComPtr<ff::IGraphDevice> _device;

	// Constant data for shaders
	ff::ComPtr<ID3D11Buffer> _geometryBuffer;
	ff::ComPtr<ID3D11Buffer> _geometryConstantsBuffer0;
	ff::ComPtr<ID3D11Buffer> _geometryConstantsBuffer1;
	ff::ComPtr<ID3D11Buffer> _pixelConstantsBuffer0;
	GeometryShaderConstants0 _geometryConstants0;
	GeometryShaderConstants1 _geometryConstants1;
	PixelShaderConstants0 _pixelConstants0;
	ff::hash_t _geometryConstantsHash0;
	ff::hash_t _geometryConstantsHash1;
	ff::hash_t _pixelConstantsHash0;

	// Render state
	ff::Vector<ff::ComPtr<ID3D11SamplerState>> _samplerStack;
	ff::GraphFixedState11 _opaqueState;
	ff::GraphFixedState11 _alphaState;
	ff::Vector<ff::CustomRenderContextFunc11> _customContextStack;

	// Matrixes
	DirectX::XMFLOAT4X4 _viewMatrix;
	ff::MatrixStack _worldMatrixStack;
	ff::Map<DirectX::XMFLOAT4X4, unsigned int> _worldMatrixToIndex;
	unsigned int _worldMatrixIndex;

	// Textures
	std::array<ff::ITextureView*, ::MAX_TEXTURES> _textures;
	std::array<ff::ITextureView*, ::MAX_TEXTURES_USING_PALETTE> _texturesUsingPalette;
	size_t _textureCount;
	size_t _texturesUsingPaletteCount;

	// Palettes
	ff::Vector<ff::IPalette*> _paletteStack;
	ff::ComPtr<ff::ITexture> _paletteTexture;
	std::array<ff::hash_t, ::MAX_PALETTES> _paletteTextureHashes;
	ff::Map<ff::hash_t, std::pair<ff::IPalette*, unsigned int>, ff::NonHasher<ff::hash_t>> _paletteToIndex;
	unsigned int _paletteIndex;
	bool _targetRequiresPalette;

	// Render data
	ff::Vector<AlphaGeometryEntry> _alphaGeometry;
	std::array<GeometryBucket, (size_t)GeometryBucketType::Count> _geometryBuckets;
	LastDepthType _lastDepthType;
	float _drawDepth;
	int _forceNoOverlap;
	int _forceOpaque;
};

std::unique_ptr<ff::IRenderer> CreateRenderer11(ff::IGraphDevice* device)
{
	return std::make_unique<Renderer11>(device);
}

enum class AlphaType
{
	Opaque,
	Transparent,
	Invisible,
};

inline static AlphaType GetAlphaType(const DirectX::XMFLOAT4& color, bool forceOpaque)
{
	if (color.w == 0)
	{
		return AlphaType::Invisible;
	}

	if (color.w == 1 || forceOpaque)
	{
		return AlphaType::Opaque;
	}

	return AlphaType::Transparent;
}

static AlphaType GetAlphaType(const DirectX::XMFLOAT4* colors, size_t count, bool forceOpaque)
{
	AlphaType type = AlphaType::Invisible;

	for (size_t i = 0; i < count; i++)
	{
		switch (::GetAlphaType(colors[i], forceOpaque))
		{
		case AlphaType::Opaque:
			type = AlphaType::Opaque;
			break;

		case AlphaType::Transparent:
			return AlphaType::Transparent;
		}
	}

	return type;
}

static AlphaType GetAlphaType(const ff::SpriteData& data, const DirectX::XMFLOAT4& color, bool forceOpaque)
{
	switch (::GetAlphaType(color, forceOpaque))
	{
	case AlphaType::Transparent:
		return ff::HasAllFlags(data._type, ff::SpriteType::Palette) ? AlphaType::Opaque : AlphaType::Transparent;

	case AlphaType::Opaque:
		return (ff::HasAllFlags(data._type, ff::SpriteType::Transparent) && !forceOpaque)
			? AlphaType::Transparent
			: AlphaType::Opaque;

	default:
		return AlphaType::Invisible;
	}
}

static AlphaType GetAlphaType(const ff::SpriteData** datas, const DirectX::XMFLOAT4* colors, size_t count, bool forceOpaque)
{
	AlphaType type = AlphaType::Invisible;

	for (size_t i = 0; i < count; i++)
	{
		switch (::GetAlphaType(*datas[i], colors[i], forceOpaque))
		{
		case AlphaType::Opaque:
			type = AlphaType::Opaque;
			break;

		case AlphaType::Transparent:
			return AlphaType::Transparent;
		}
	}

	return type;
}

void PaletteIndexToColor(int index, DirectX::XMFLOAT4& color)
{
	color.x = index / 256.0f;
	color.y = 0;
	color.z = 0;
	color.w = 1.0f * (index != 0);
}

static void PaletteIndexToColor(const int* index, DirectX::XMFLOAT4* color, size_t count)
{
	for (size_t i = 0; i != count; i++)
	{
		PaletteIndexToColor(index[i], color[i]);
	}
}

static void GetAlphaBlend(D3D11_RENDER_TARGET_BLEND_DESC& desc)
{
	// newColor = (srcColor * SrcBlend) BlendOp (destColor * DestBlend)
	// newAlpha = (srcAlpha * SrcBlendAlpha) BlendOpAlpha (destAlpha * DestBlendAlpha)

	desc.BlendEnable = TRUE;
	desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.BlendOp = D3D11_BLEND_OP_ADD;
	desc.SrcBlendAlpha = D3D11_BLEND_ZERO;
	desc.DestBlendAlpha = D3D11_BLEND_ONE;
	desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
}

static ID3D11SamplerState* GetTextureSamplerState(ff::GraphStateCache11& cache, D3D11_FILTER filter)
{
	CD3D11_SAMPLER_DESC sampler(D3D11_DEFAULT);
	sampler.Filter = filter;
	return cache.GetSamplerState(sampler);
}

static ID3D11BlendState* GetOpaqueBlendState(ff::IGraphDevice* device)
{
	CD3D11_BLEND_DESC blend(D3D11_DEFAULT);
	return device->AsGraphDevice11()->GetStateCache().GetBlendState(blend);
}

static ID3D11BlendState* GetAlphaBlendState(ff::IGraphDevice* device)
{
	CD3D11_BLEND_DESC blend(D3D11_DEFAULT);
	::GetAlphaBlend(blend.RenderTarget[0]);
	return device->AsGraphDevice11()->GetStateCache().GetBlendState(blend);
}

static ID3D11DepthStencilState* GetEnabledDepthState(ff::IGraphDevice* device)
{
	CD3D11_DEPTH_STENCIL_DESC depth(D3D11_DEFAULT);
	depth.DepthFunc = D3D11_COMPARISON_GREATER;
	return device->AsGraphDevice11()->GetStateCache().GetDepthStencilState(depth);
}

static ID3D11DepthStencilState* GetDisabledDepthState(ff::IGraphDevice* device)
{
	CD3D11_DEPTH_STENCIL_DESC depth(D3D11_DEFAULT);
	depth.DepthEnable = FALSE;
	depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	return device->AsGraphDevice11()->GetStateCache().GetDepthStencilState(depth);
}

static ID3D11RasterizerState* GetNoCullRasterState(ff::IGraphDevice* device)
{
	CD3D11_RASTERIZER_DESC raster(D3D11_DEFAULT);
	raster.CullMode = D3D11_CULL_NONE;
	return device->AsGraphDevice11()->GetStateCache().GetRasterizerState(raster);
}

static ff::ComPtr<ID3D11Buffer> CreateBuffer(ff::IGraphDevice* device, size_t byteSize, UINT bindFlags, bool writable, D3D11_SUBRESOURCE_DATA* data)
{
	assertRetVal(byteSize, nullptr);

	if (writable && data == nullptr)
	{
		byteSize = ff::NearestPowerOfTwo(byteSize);
	}

	ff::ComPtr<ID3D11Buffer> buffer;
	CD3D11_BUFFER_DESC desc((UINT)byteSize, bindFlags,
		writable ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
		writable ? D3D11_CPU_ACCESS_WRITE : 0);
	assertHrRetVal(device->AsGraphDevice11()->Get3d()->CreateBuffer(&desc, data, &buffer), nullptr);

	return buffer;
}

static bool EnsureBuffer(ff::IGraphDevice* device, ff::ComPtr<ID3D11Buffer>& buffer, size_t byteSize, UINT bindFlags, bool writable)
{
	if (buffer)
	{
		D3D11_BUFFER_DESC desc;
		buffer->GetDesc(&desc);

		if (desc.ByteWidth >= byteSize)
		{
			return false;
		}
	}

	buffer = ::CreateBuffer(device, byteSize, bindFlags, writable, nullptr);
	return true;
}

template<typename T>
static ff::ComPtr<ID3D11Buffer> CreateConstantBuffer(ff::IGraphDevice* device, const T& data)
{
	D3D11_SUBRESOURCE_DATA constantsData;
	constantsData.pSysMem = &data;
	constantsData.SysMemPitch = sizeof(T);
	constantsData.SysMemSlicePitch = 0;

	return ::CreateBuffer(device, sizeof(T), D3D11_BIND_CONSTANT_BUFFER, true, &constantsData);
}

template<typename T>
static ID3D11VertexShader* GetVertexShaderAndInputLayout(const wchar_t* name, ff::ComPtr<ID3D11InputLayout>& layout, ff::GraphStateCache11& cache)
{
	return cache.GetVertexShaderAndInputLayout(
		ff::GetThisModule().GetResources(),
		ff::String(name),
		layout,
		T::GetLayout11().data(),
		T::GetLayout11().size());
}

static ID3D11GeometryShader* GetGeometryShader(const wchar_t* name, ff::GraphStateCache11& cache)
{
	return cache.GetGeometryShader(ff::GetThisModule().GetResources(), ff::String(name));
}

static ID3D11PixelShader* GetPixelShader(const wchar_t* name, ff::GraphStateCache11& cache)
{
	return cache.GetPixelShader(ff::GetThisModule().GetResources(), ff::String(name));
}

static ff::RectFloat GetRotatedViewRect(ff::IRenderTarget* target, ff::RectFloat viewRect)
{
	ff::RectFloat rotatedViewRect;

	switch (target->GetRotatedDegrees())
	{
	default:
		rotatedViewRect = viewRect;
		break;

	case 90:
	{
		float height = (float)target->GetRotatedSize().y;
		rotatedViewRect.left = height - viewRect.bottom;
		rotatedViewRect.top = viewRect.left;
		rotatedViewRect.right = height - viewRect.top;
		rotatedViewRect.bottom = viewRect.right;
	} break;

	case 180:
	{
		ff::PointFloat targetSize = target->GetRotatedSize().ToType<float>();
		rotatedViewRect.left = targetSize.x - viewRect.right;
		rotatedViewRect.top = targetSize.y - viewRect.bottom;
		rotatedViewRect.right = targetSize.x - viewRect.left;
		rotatedViewRect.bottom = targetSize.y - viewRect.top;
	} break;

	case 270:
	{
		float width = (float)target->GetRotatedSize().x;
		rotatedViewRect.left = viewRect.top;
		rotatedViewRect.top = width - viewRect.right;
		rotatedViewRect.right = viewRect.bottom;
		rotatedViewRect.bottom = width - viewRect.left;
	} break;
	}

	return rotatedViewRect;
}

static DirectX::XMMATRIX GetViewMatrix(ff::RectFloat worldRect)
{
	return DirectX::XMMatrixOrthographicOffCenterLH(
		worldRect.left,
		worldRect.right,
		worldRect.bottom,
		worldRect.top,
		0, ::MAX_RENDER_DEPTH);
}

static DirectX::XMMATRIX GetOrientationMatrix(ff::IRenderTarget* target, ff::RectFloat viewRect, ff::PointFloat worldCenter)
{
	DirectX::XMMATRIX orientationMatrix;

	int degrees = target->GetRotatedDegrees();
	switch (degrees)
	{
	default:
		orientationMatrix = DirectX::XMMatrixIdentity();
		break;

	case 90:
	case 270:
	{
		float viewHeightPerWidth = viewRect.Height() / viewRect.Width();
		float viewWidthPerHeight = viewRect.Width() / viewRect.Height();

		orientationMatrix =
			DirectX::XMMatrixTransformation2D(
				DirectX::XMVectorSet(worldCenter.x, worldCenter.y, 0, 0), 0, // scale center
				DirectX::XMVectorSet(viewHeightPerWidth, viewWidthPerHeight, 1, 1), // scale
				DirectX::XMVectorSet(worldCenter.x, worldCenter.y, 0, 0), // rotation center
				(float)(ff::PI_D * (degrees / 90) / 2), // rotation
				DirectX::XMVectorZero()); // translation
	} break;

	case 180:
		orientationMatrix =
			DirectX::XMMatrixAffineTransformation2D(
				DirectX::XMVectorSet(1, 1, 1, 1), // scale
				DirectX::XMVectorSet(worldCenter.x, worldCenter.y, 0, 0), // rotation center
				ff::PI_F, // rotation
				DirectX::XMVectorZero()); // translation
		break;
	}

	return orientationMatrix;
}

static D3D11_VIEWPORT GetViewport(ff::RectFloat viewRect)
{
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = viewRect.left;
	viewport.TopLeftY = viewRect.top;
	viewport.Width = viewRect.Width();
	viewport.Height = viewRect.Height();
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	return viewport;
}

static bool SetupViewMatrix(ff::IRenderTarget* target, ff::RectFloat viewRect, ff::RectFloat worldRect, DirectX::XMFLOAT4X4& viewMatrix)
{
	noAssertRetVal(worldRect.Width() != 0 && worldRect.Height() != 0, false);
	noAssertRetVal(viewRect.Width() > 0 && viewRect.Height() > 0, false);

	DirectX::XMMATRIX unorientedViewMatrix = ::GetViewMatrix(worldRect);
	DirectX::XMMATRIX orientationMatrix = ::GetOrientationMatrix(target, viewRect, worldRect.Center());
	DirectX::XMStoreFloat4x4(&viewMatrix, DirectX::XMMatrixTranspose(orientationMatrix * unorientedViewMatrix));

	return true;
}

static bool SetupRenderTarget(ff::IGraphDevice* device, ff::IRenderTarget* target, ff::IRenderDepth* depth, ff::RectFloat viewRect)
{
	ID3D11RenderTargetView* targetView = target ? target->AsRenderTarget11()->GetTarget() : nullptr;
	assertRetVal(targetView, false);

	ID3D11DepthStencilView* depthView = nullptr;
	if (depth)
	{
		assertRetVal(depth->SetSize(target->GetBufferSize()), false);
		depthView = depth->AsRenderDepth11()->GetView();
		assertRetVal(depthView, false);
	}

	ff::RectFloat rotatedViewRect = ::GetRotatedViewRect(target, viewRect);
	D3D11_VIEWPORT viewport = ::GetViewport(rotatedViewRect);

	device->AsGraphDevice11()->GetStateContext().SetTargets(&targetView, 1, depthView);
	device->AsGraphDevice11()->GetStateContext().SetViewports(&viewport, 1);

	return true;
}

Renderer11::Renderer11(ff::IGraphDevice* device)
	: _device(device)
	, _worldMatrixStack(this)
	, _geometryBuckets
	{
		GeometryBucket::New<ff::LineGeometryInput, GeometryBucketType::Lines>(),
		GeometryBucket::New<ff::CircleGeometryInput, GeometryBucketType::Circle>(),
		GeometryBucket::New<ff::TriangleGeometryInput, GeometryBucketType::Triangles>(),
		GeometryBucket::New<ff::SpriteGeometryInput, GeometryBucketType::Sprites>(),
		GeometryBucket::New<ff::SpriteGeometryInput, GeometryBucketType::PaletteSprites>(),

		GeometryBucket::New<ff::LineGeometryInput, GeometryBucketType::LinesAlpha>(),
		GeometryBucket::New<ff::CircleGeometryInput, GeometryBucketType::CircleAlpha>(),
		GeometryBucket::New<ff::TriangleGeometryInput, GeometryBucketType::TrianglesAlpha>(),
		GeometryBucket::New<ff::SpriteGeometryInput, GeometryBucketType::SpritesAlpha>(),
		GeometryBucket::New<ff::SpriteGeometryInput, GeometryBucketType::PaletteSpritesAlpha>(),
	}
{
	Init();

	_device->AddChild(this);
}

Renderer11::~Renderer11()
{
	_device->RemoveChild(this);
}

void Renderer11::Destroy()
{
	_state = State::Invalid;

	_geometryBuffer = nullptr;
	_geometryConstantsBuffer0 = nullptr;
	_geometryConstantsBuffer1 = nullptr;
	_pixelConstantsBuffer0 = nullptr;
	_geometryConstants0 = GeometryShaderConstants0();
	_geometryConstants1 = GeometryShaderConstants1();
	_pixelConstants0 = PixelShaderConstants0();
	_geometryConstantsHash0 = 0;
	_geometryConstantsHash1 = 0;
	_pixelConstantsHash0 = 0;

	_samplerStack.Clear();
	_opaqueState = ff::GraphFixedState11();
	_alphaState = ff::GraphFixedState11();
	_customContextStack.Clear();

	_viewMatrix = ff::GetIdentityMatrix();
	_worldMatrixStack.Reset();
	_worldMatrixToIndex.Clear();
	_worldMatrixIndex = ff::INVALID_DWORD;

	ff::ZeroObject(_textures);
	ff::ZeroObject(_texturesUsingPalette);
	_textureCount = 0;
	_texturesUsingPaletteCount = 0;

	ff::ZeroObject(_paletteTextureHashes);
	_paletteStack.Clear();
	_paletteToIndex.Clear();
	_paletteTexture = nullptr;
	_paletteIndex = ff::INVALID_DWORD;

	_alphaGeometry.Clear();
	_lastDepthType = LastDepthType::None;
	_drawDepth = 0;
	_forceNoOverlap = 0;
	_forceOpaque = 0;

	for (auto& bucket : _geometryBuckets)
	{
		bucket.Reset();
	}
}

bool Renderer11::Init()
{
	Destroy();

	ff::GraphStateCache11& cache = _device->AsGraphDevice11()->GetStateCache();
	ff::ComPtr<ID3D11InputLayout> lineLayout;
	ff::ComPtr<ID3D11InputLayout> circleLayout;
	ff::ComPtr<ID3D11InputLayout> triangleLayout;
	ff::ComPtr<ID3D11InputLayout> spriteLayout;

	// Vertex shaders
	ID3D11VertexShader* lineVS = ::GetVertexShaderAndInputLayout<ff::LineGeometryInput>(L"LineVS", lineLayout, cache);
	ID3D11VertexShader* circleVS = ::GetVertexShaderAndInputLayout<ff::CircleGeometryInput>(L"CircleVS", circleLayout, cache);
	ID3D11VertexShader* triangleVS = ::GetVertexShaderAndInputLayout<ff::TriangleGeometryInput>(L"TriangleVS", triangleLayout, cache);
	ID3D11VertexShader* spriteVS = ::GetVertexShaderAndInputLayout<ff::SpriteGeometryInput>(L"SpriteVS", spriteLayout, cache);

	// Geometry shaders
	ID3D11GeometryShader* lineGS = ::GetGeometryShader(L"LineGS", cache);
	ID3D11GeometryShader* circleGS = ::GetGeometryShader(L"CircleGS", cache);
	ID3D11GeometryShader* triangleGS = ::GetGeometryShader(L"TriangleGS", cache);
	ID3D11GeometryShader* spriteGS = ::GetGeometryShader(L"SpriteGS", cache);

	// Pixel shaders
	ID3D11PixelShader* colorPS = ::GetPixelShader(L"ColorPS", cache);
	ID3D11PixelShader* paletteOutColorPS = ::GetPixelShader(L"PaletteOutColorPS", cache);
	ID3D11PixelShader* spritePS = ::GetPixelShader(L"SpritePS", cache);
	ID3D11PixelShader* spritePalettePS = ::GetPixelShader(L"SpritePalettePS", cache);
	ID3D11PixelShader* paletteOutSpritePS = ::GetPixelShader(L"PaletteOutSpritePS", cache);
	ID3D11PixelShader* paletteOutSpritePalettePS = ::GetPixelShader(L"PaletteOutSpritePalettePS", cache);

	assertRetVal(
		lineVS != nullptr &&
		circleVS != nullptr &&
		triangleVS != nullptr &&
		spriteVS != nullptr &&
		lineGS != nullptr &&
		circleGS != nullptr &&
		triangleGS != nullptr &&
		spriteGS != nullptr &&
		colorPS != nullptr &&
		paletteOutColorPS != nullptr &&
		spritePS != nullptr &&
		spritePalettePS != nullptr &&
		paletteOutSpritePS != nullptr &&
		paletteOutSpritePalettePS != nullptr &&
		lineLayout != nullptr &&
		circleLayout != nullptr &&
		triangleLayout != nullptr &&
		spriteLayout != nullptr,
		false);

	// Geometry buckets
	GetGeometryBucket(GeometryBucketType::Lines).Reset(lineLayout, lineVS, lineGS, colorPS, paletteOutColorPS);
	GetGeometryBucket(GeometryBucketType::Circle).Reset(circleLayout, circleVS, circleGS, colorPS, paletteOutColorPS);
	GetGeometryBucket(GeometryBucketType::Triangles).Reset(triangleLayout, triangleVS, triangleGS, colorPS, paletteOutColorPS);
	GetGeometryBucket(GeometryBucketType::Sprites).Reset(spriteLayout, spriteVS, spriteGS, spritePS, paletteOutSpritePS);
	GetGeometryBucket(GeometryBucketType::PaletteSprites).Reset(spriteLayout, spriteVS, spriteGS, spritePalettePS, paletteOutSpritePalettePS);

	GetGeometryBucket(GeometryBucketType::LinesAlpha).Reset(lineLayout, lineVS, lineGS, colorPS, paletteOutColorPS);
	GetGeometryBucket(GeometryBucketType::CircleAlpha).Reset(circleLayout, circleVS, circleGS, colorPS, paletteOutColorPS);
	GetGeometryBucket(GeometryBucketType::TrianglesAlpha).Reset(triangleLayout, triangleVS, triangleGS, colorPS, paletteOutColorPS);
	GetGeometryBucket(GeometryBucketType::SpritesAlpha).Reset(spriteLayout, spriteVS, spriteGS, spritePS, paletteOutSpritePS);
	GetGeometryBucket(GeometryBucketType::PaletteSpritesAlpha).Reset(spriteLayout, spriteVS, spriteGS, spritePalettePS, paletteOutSpritePalettePS);

	// Default palette
	{
		static const BYTE emptyPaletteColors[256 * 4] = { 0 };

		ff::ComPtr<ff::IData> defaultPaletteData;
		assertRetVal(ff::CreateDataInStaticMem(emptyPaletteColors, _countof(emptyPaletteColors), &defaultPaletteData), false);

		ff::ComPtr<ff::IPaletteData> defaultPaletteData2;
		assertRetVal(ff::CreatePaletteData(defaultPaletteData, &defaultPaletteData2), false);

		_paletteStack.Push(defaultPaletteData2->CreatePalette(_device));
	}

	_paletteTexture = _device->CreateTexture(ff::PointInt(256, (int)::MAX_PALETTES), ff::TextureFormat::RGBA32);
	assertRetVal(_paletteTexture, false);

	// States
	_samplerStack.Push(::GetTextureSamplerState(cache, D3D11_FILTER_MIN_MAG_MIP_POINT));
	_opaqueState = CreateOpaqueDrawState();
	_alphaState = CreateAlphaDrawState();

	_state = State::Valid;
	return true;
}

ff::GraphFixedState11 Renderer11::CreateOpaqueDrawState()
{
	ff::GraphFixedState11 state;

	state._blend = ::GetOpaqueBlendState(_device);
	state._depth = ::GetEnabledDepthState(_device);
	state._disabledDepth = ::GetDisabledDepthState(_device);
	state._raster = ::GetNoCullRasterState(_device);

	return state;
}

ff::GraphFixedState11 Renderer11::CreateAlphaDrawState()
{
	ff::GraphFixedState11 state;

	state._blend = ::GetAlphaBlendState(_device);
	state._depth = ::GetEnabledDepthState(_device);
	state._disabledDepth = ::GetDisabledDepthState(_device);
	state._raster = ::GetNoCullRasterState(_device);

	return state;
}

GeometryBucket& Renderer11::GetGeometryBucket(GeometryBucketType type)
{
	return _geometryBuckets[(size_t)type];
}

bool Renderer11::IsValid() const
{
	return _state != State::Invalid;
}

ff::IRendererActive* Renderer11::BeginRender(ff::IRenderTarget* target, ff::IRenderDepth* depth, ff::RectFloat viewRect, ff::RectFloat worldRect)
{
	EndRender();

	noAssertRetVal(::SetupViewMatrix(target, viewRect, worldRect, _viewMatrix), nullptr);
	assertRetVal(::SetupRenderTarget(_device, target, depth, viewRect), nullptr);
	InitGeometryConstantBuffers0(target, viewRect, worldRect);
	_targetRequiresPalette = (target->GetFormat() == ff::TextureFormat::R8_UINT);
	_state = State::Rendering;

	return this;
}

void Renderer11::InitGeometryConstantBuffers0(ff::IRenderTarget* target, const ff::RectFloat& viewRect, const ff::RectFloat& worldRect)
{
	_geometryConstants0._viewSize = viewRect.Size() / (float)target->GetDpiScale();
	_geometryConstants0._viewScale = worldRect.Size() / _geometryConstants0._viewSize;
}

void Renderer11::UpdateGeometryConstantBuffers0()
{
	_geometryConstants0._projection = _viewMatrix;

	ff::hash_t hash0 = ff::HashFunc(_geometryConstants0);
	if (_geometryConstantsBuffer0 == nullptr || _geometryConstantsHash0 != hash0)
	{
		if (_geometryConstantsBuffer0 == nullptr)
		{
			_geometryConstantsBuffer0 = ::CreateConstantBuffer<GeometryShaderConstants0>(_device, _geometryConstants0);
		}
		else
		{
			_device->AsGraphDevice11()->GetStateContext().UpdateDiscard(_geometryConstantsBuffer0, &_geometryConstants0, sizeof(GeometryShaderConstants0));
		}

		_geometryConstantsHash0 = hash0;
	}
}

void Renderer11::UpdateGeometryConstantBuffers1()
{
	// Build up model matrix array
	size_t worldMatrixCount = _worldMatrixToIndex.Size();
	_geometryConstants1._model.Resize(worldMatrixCount);

	for (const auto& iter : _worldMatrixToIndex)
	{
		unsigned int index = iter.GetValue();
		_geometryConstants1._model[index] = iter.GetKey();
	}

	ff::hash_t hash1 = worldMatrixCount
		? ff::HashBytes(_geometryConstants1._model.ConstData(), _geometryConstants1._model.ByteSize())
		: 0;

	if (_geometryConstantsBuffer1 == nullptr || _geometryConstantsHash1 != hash1)
	{
		_geometryConstantsHash1 = hash1;
#if _DEBUG
		size_t bufferSize = sizeof(DirectX::XMFLOAT4X4) * ::MAX_TRANSFORM_MATRIXES;
#else
		size_t bufferSize = sizeof(DirectX::XMFLOAT4X4) * worldMatrixCount;
#endif
		::EnsureBuffer(_device, _geometryConstantsBuffer1, bufferSize, D3D11_BIND_CONSTANT_BUFFER, true);
		_device->AsGraphDevice11()->GetStateContext().UpdateDiscard(_geometryConstantsBuffer1, _geometryConstants1._model.ConstData(), _geometryConstants1._model.ByteSize());
	}
}

void Renderer11::UpdatePixelConstantBuffers0()
{
	noAssertRet(_texturesUsingPaletteCount);

	for (size_t i = 0; i < _texturesUsingPaletteCount; i++)
	{
		ff::RectFloat& rect = _pixelConstants0._texturePaletteSizes[i];
		ff::PointInt size = _texturesUsingPalette[i]->GetTexture()->GetSize();
		rect.left = (float)size.x;
		rect.top = (float)size.y;
	}

	ff::hash_t hash0 = ff::HashFunc(_pixelConstants0);
	if (_pixelConstantsBuffer0 == nullptr || _pixelConstantsHash0 != hash0)
	{
		if (_pixelConstantsBuffer0 == nullptr)
		{
			_pixelConstantsBuffer0 = ::CreateConstantBuffer<PixelShaderConstants0>(_device, _pixelConstants0);
		}
		else
		{
			_device->AsGraphDevice11()->GetStateContext().UpdateDiscard(_pixelConstantsBuffer0, &_pixelConstants0, sizeof(PixelShaderConstants0));
		}

		_pixelConstantsHash0 = hash0;
	}
}

void Renderer11::UpdatePaletteTexture()
{
	noAssertRet(_texturesUsingPaletteCount);

	ff::GraphContext11& deviceContext = _device->AsGraphDevice11()->GetStateContext();
	ID3D11Resource* destResource = _paletteTexture->AsTexture11()->GetTexture2d();
	CD3D11_BOX srcBox(0, 0, 0, 256, 1, 1);

	for (const auto& iter : _paletteToIndex)
	{
		ff::IPalette* palette = iter.GetValue().first;
		unsigned int index = iter.GetValue().second;

		if (_paletteTextureHashes[index] != palette->GetTextureHash())
		{
			_paletteTextureHashes[index] = palette->GetTextureHash();
			ID3D11Resource* srcResource = palette->GetTexture()->AsTexture11()->GetTexture2d();
			deviceContext.CopySubresourceRegion(destResource, 0, 0, index, 0, srcResource, 0, &srcBox);
		}
	}
}

void Renderer11::SetShaderInput()
{
	ff::GraphContext11& context = _device->AsGraphDevice11()->GetStateContext();
	std::array<ID3D11Buffer*, 2> buffersGS = { _geometryConstantsBuffer0, _geometryConstantsBuffer1 };
	context.SetConstantsGS(buffersGS.data(), 0, buffersGS.size());

	std::array<ID3D11Buffer*, 1> buffersPS = { _pixelConstantsBuffer0 };
	context.SetConstantsPS(buffersPS.data(), 0, buffersPS.size());

	std::array<ID3D11SamplerState*, 1> sampleStates = { _samplerStack.GetLast() };
	context.SetSamplersPS(sampleStates.data(), 0, sampleStates.size());

	if (_textureCount)
	{
		std::array<ID3D11ShaderResourceView*, ::MAX_TEXTURES> textures;
		for (size_t i = 0; i < _textureCount; i++)
		{
			textures[i] = _textures[i]->AsTextureView11()->GetView();
		}

		context.SetResourcesPS(textures.data(), 0, _textureCount);
	}

	if (_texturesUsingPaletteCount)
	{
		std::array<ID3D11ShaderResourceView*, ::MAX_TEXTURES_USING_PALETTE> texturesUsingPalette;
		for (size_t i = 0; i < _texturesUsingPaletteCount; i++)
		{
			texturesUsingPalette[i] = _texturesUsingPalette[i]->AsTextureView11()->GetView();
		}

		context.SetResourcesPS(texturesUsingPalette.data(), ::MAX_TEXTURES, _texturesUsingPaletteCount);

		std::array < ID3D11ShaderResourceView*, 1> palettes = { _paletteTexture->AsTextureView()->AsTextureView11()->GetView() };
		context.SetResourcesPS(palettes.data(), ::MAX_TEXTURES + ::MAX_TEXTURES_USING_PALETTE, palettes.size());
	}
}

void Renderer11::Flush()
{
	if (_lastDepthType != LastDepthType::None && CreateGeometryBuffer())
	{
		UpdateGeometryConstantBuffers0();
		UpdateGeometryConstantBuffers1();
		UpdatePixelConstantBuffers0();
		UpdatePaletteTexture();
		SetShaderInput();
		DrawOpaqueGeometry();
		DrawAlphaGeometry();
		PostFlush();
	}
}

bool Renderer11::CreateGeometryBuffer()
{
	size_t byteSize = 0;

	for (GeometryBucket& bucket : _geometryBuckets)
	{
		byteSize = ff::RoundUp(byteSize, bucket.GetItemByteSize());
		bucket.SetRenderStart(byteSize / bucket.GetItemByteSize());
		byteSize += bucket.GetByteSize();
	}

	assertRetVal(byteSize, false);

	::EnsureBuffer(_device, _geometryBuffer, byteSize, D3D11_BIND_VERTEX_BUFFER, true);
	assertRetVal(_geometryBuffer, false);

	void* bufferData = _device->AsGraphDevice11()->GetStateContext().Map(_geometryBuffer, D3D11_MAP_WRITE_DISCARD);

	for (GeometryBucket& bucket : _geometryBuckets)
	{
		if (bucket.GetRenderCount())
		{
			::memcpy((BYTE*)bufferData + bucket.GetRenderStart() * bucket.GetItemByteSize(), bucket.GetData(), bucket.GetByteSize());
			bucket.ClearItems();
		}
	}

	_device->AsGraphDevice11()->GetStateContext().Unmap(_geometryBuffer);

	return true;
}

void Renderer11::DrawOpaqueGeometry()
{
	const ff::CustomRenderContextFunc11* customFunc = _customContextStack.Size() ? &_customContextStack.GetLast() : nullptr;
	ff::GraphContext11& context = _device->AsGraphDevice11()->GetStateContext();
	context.SetTopologyIA(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	_opaqueState.Apply(context);

	for (const GeometryBucket& bucket : _geometryBuckets)
	{
		if (bucket.GetBucketType() >= GeometryBucketType::FirstAlpha)
		{
			break;
		}

		if (bucket.GetRenderCount())
		{
			bucket.Apply(context, _geometryBuffer, _targetRequiresPalette);

			if (!customFunc || (*customFunc)(context, bucket.GetItemType(), true))
			{
				context.Draw(bucket.GetRenderCount(), bucket.GetRenderStart());
			}
		}
	}
}

void Renderer11::DrawAlphaGeometry()
{
	const size_t alphaGeometrySize = _alphaGeometry.Size();
	noAssertRet(alphaGeometrySize);

	const ff::CustomRenderContextFunc11* customFunc = _customContextStack.Size() ? &_customContextStack.GetLast() : nullptr;
	ff::GraphContext11& context = _device->AsGraphDevice11()->GetStateContext();
	context.SetTopologyIA(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	_alphaState.Apply(context);

	for (size_t i = 0; i < alphaGeometrySize; )
	{
		const AlphaGeometryEntry& entry = _alphaGeometry[i];
		size_t geometryCount = 1;

		for (i++; i < alphaGeometrySize; i++, geometryCount++)
		{
			const AlphaGeometryEntry& entry2 = _alphaGeometry[i];
			if (entry2._bucket != entry._bucket ||
				entry2._depth != entry._depth ||
				entry2._index != entry._index + geometryCount)
			{
				break;
			}
		}

		entry._bucket->Apply(context, _geometryBuffer, _targetRequiresPalette);

		if (!customFunc || (*customFunc)(context, entry._bucket->GetItemType(), false))
		{
			context.Draw(geometryCount, entry._bucket->GetRenderStart() + entry._index);
		}
	}
}

void Renderer11::PostFlush()
{
	_worldMatrixToIndex.Clear();
	_worldMatrixIndex = ff::INVALID_DWORD;

	_paletteToIndex.Clear();
	_paletteIndex = ff::INVALID_DWORD;

	_textureCount = 0;
	_texturesUsingPaletteCount = 0;

	_alphaGeometry.Clear();
	_lastDepthType = LastDepthType::None;
}

void Renderer11::EndRender()
{
	noAssertRet(IsRendering());

	Flush();

	_device->AsGraphDevice11()->GetStateContext().SetResourcesPS(::NULL_TEXTURES.data(), 0, ::NULL_TEXTURES.size());

	_state = State::Valid;
	_paletteStack.Resize(1);
	_samplerStack.Resize(1);
	_customContextStack.Clear();
	_worldMatrixStack.Reset();
	_drawDepth = 0;
	_forceNoOverlap = 0;
	_forceOpaque = 0;
}

bool Renderer11::IsRendering() const
{
	return _state == State::Rendering;
}

void Renderer11::NudgeDepth()
{
	_lastDepthType = LastDepthType::None;
}

float Renderer11::NudgeDepth(LastDepthType depthType)
{
	if (depthType < LastDepthType::StartNoOverlap || _lastDepthType != depthType)
	{
		_drawDepth += ::RENDER_DEPTH_DELTA;
	}

	_lastDepthType = depthType;
	return _drawDepth;
}

unsigned int Renderer11::GetWorldMatrixIndex()
{
	unsigned int index = GetWorldMatrixIndexNoFlush();
	if (index == ff::INVALID_DWORD)
	{
		Flush();
		index = GetWorldMatrixIndexNoFlush();
	}

	return index;
}

unsigned int Renderer11::GetWorldMatrixIndexNoFlush()
{
	if (_worldMatrixIndex == ff::INVALID_DWORD)
	{
		DirectX::XMFLOAT4X4 wm;
		DirectX::XMStoreFloat4x4(&wm, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&_worldMatrixStack.GetMatrix())));
		auto iter = _worldMatrixToIndex.GetKey(wm);

		if (!iter && _worldMatrixToIndex.Size() != ::MAX_TRANSFORM_MATRIXES)
		{
			iter = _worldMatrixToIndex.SetKey(wm, (unsigned int)_worldMatrixToIndex.Size());
		}

		if (iter)
		{
			_worldMatrixIndex = iter->GetValue();
		}
	}

	return _worldMatrixIndex;
}

unsigned int Renderer11::GetTextureIndexNoFlush(ff::ITextureView* texture, bool usePalette)
{
	assert(texture);

	if (usePalette)
	{
		unsigned int paletteIndex = (_paletteIndex == ff::INVALID_DWORD) ? GetPaletteIndexNoFlush() : _paletteIndex;
		if (paletteIndex == ff::INVALID_DWORD)
		{
			return ff::INVALID_DWORD;
		}

		unsigned int textureIndex = ff::INVALID_DWORD;

		for (size_t i = _texturesUsingPaletteCount; i != 0; i--)
		{
			if (_texturesUsingPalette[i - 1] == texture)
			{
				textureIndex = (unsigned int)(i - 1);
				break;
			}
		}

		if (textureIndex == ff::INVALID_DWORD)
		{
			if (_texturesUsingPaletteCount == ::MAX_TEXTURES_USING_PALETTE)
			{
				return ff::INVALID_DWORD;
			}

			_texturesUsingPalette[_texturesUsingPaletteCount] = texture;
			textureIndex = (unsigned int)_texturesUsingPaletteCount++;
		}

		return textureIndex | (paletteIndex << 8);
	}
	else
	{
		unsigned int textureIndex = ff::INVALID_DWORD;

		for (size_t i = _textureCount; i != 0; i--)
		{
			if (_textures[i - 1] == texture)
			{
				textureIndex = (unsigned int)(i - 1);
				break;
			}
		}

		if (textureIndex == ff::INVALID_DWORD)
		{
			if (_textureCount == ::MAX_TEXTURES)
			{
				return ff::INVALID_DWORD;
			}

			_textures[_textureCount] = texture;
			textureIndex = (unsigned int)_textureCount++;
		}

		return textureIndex;
	}
}

unsigned int Renderer11::GetPaletteIndexNoFlush()
{
	if (_paletteIndex == ff::INVALID_DWORD)
	{
		if (_targetRequiresPalette)
		{
			// Not converting palette to RGBA, so don't use a palette
			_paletteIndex = 0;
		}
		else
		{
			ff::IPalette* palette = _paletteStack.GetLast();
			ff::hash_t paletteHash = palette->GetTextureHash();
			auto iter = _paletteToIndex.GetKey(paletteHash);

			if (!iter && _paletteToIndex.Size() != ::MAX_PALETTES)
			{
				iter = _paletteToIndex.SetKey(paletteHash, std::make_pair(palette, (unsigned int)_paletteToIndex.Size()));
			}

			if (iter)
			{
				_paletteIndex = iter->GetValue().second;
			}
		}
	}

	return _paletteIndex;
}

void Renderer11::GetWorldMatrixAndTextureIndex(ff::ITextureView* texture, bool usePalette, unsigned int& modelIndex, unsigned int& textureIndex)
{
	modelIndex = (_worldMatrixIndex == ff::INVALID_DWORD) ? GetWorldMatrixIndexNoFlush() : _worldMatrixIndex;
	textureIndex = GetTextureIndexNoFlush(texture, usePalette);

	if (modelIndex == ff::INVALID_DWORD || textureIndex == ff::INVALID_DWORD)
	{
		Flush();
		GetWorldMatrixAndTextureIndex(texture, usePalette, modelIndex, textureIndex);
	}
}

void Renderer11::GetWorldMatrixAndTextureIndexes(ff::ITextureView** textures, bool usePalette, unsigned int* textureIndexes, size_t count, unsigned int& modelIndex)
{
	modelIndex = (_worldMatrixIndex == ff::INVALID_DWORD) ? GetWorldMatrixIndexNoFlush() : _worldMatrixIndex;
	bool flush = (modelIndex == ff::INVALID_DWORD);

	for (size_t i = 0; !flush && i < count; i++)
	{
		textureIndexes[i] = GetTextureIndexNoFlush(textures[i], usePalette);
		flush |= (textureIndexes[i] == ff::INVALID_DWORD);
	}

	if (flush)
	{
		Flush();
		GetWorldMatrixAndTextureIndexes(textures, usePalette, textureIndexes, count, modelIndex);
	}
}

void Renderer11::AddGeometry(const void* data, GeometryBucketType bucketType, float depth)
{
	GeometryBucket& bucket = GetGeometryBucket(bucketType);

	if (bucketType >= GeometryBucketType::FirstAlpha)
	{
		assert(!_forceOpaque);

		_alphaGeometry.Push(AlphaGeometryEntry
			{
				&bucket,
				bucket.GetCount(),
				depth
			});
	}

	bucket.Add(data);
}

void* Renderer11::AddGeometry(GeometryBucketType bucketType, float depth)
{
	GeometryBucket& bucket = GetGeometryBucket(bucketType);

	if (bucketType >= GeometryBucketType::FirstAlpha)
	{
		assert(!_forceOpaque);

		_alphaGeometry.Push(AlphaGeometryEntry
			{
				&bucket,
				bucket.GetCount(),
				depth
			});
	}

	return bucket.Add();
}

ff::MatrixStack& Renderer11::GetWorldMatrixStack()
{
	return _worldMatrixStack;
}

ff::IRendererActive11* Renderer11::AsRendererActive11()
{
	return this;
}

void Renderer11::PushPalette(ff::IPalette* palette)
{
	assertRet(!_targetRequiresPalette && palette);
	_paletteStack.Push(palette);
	_paletteIndex = ff::INVALID_DWORD;
}

void Renderer11::PopPalette()
{
	assertRet(_paletteStack.Size() > 1);
	_paletteStack.Pop();
	_paletteIndex = ff::INVALID_DWORD;
}

void Renderer11::PushCustomContext(ff::CustomRenderContextFunc11&& func)
{
	Flush();
	_customContextStack.Push(std::move(func));
}

void Renderer11::PopCustomContext()
{
	assertRet(_customContextStack.Size());

	Flush();
	_customContextStack.Pop();
}

void Renderer11::PushTextureSampler(D3D11_FILTER filter)
{
	Flush();
	_samplerStack.Push(::GetTextureSamplerState(_device->AsGraphDevice11()->GetStateCache(), filter));
}

void Renderer11::PopTextureSampler()
{
	assertRet(_samplerStack.Size() > 1);

	Flush();
	_samplerStack.Pop();
}

void Renderer11::PushNoOverlap()
{
	_forceNoOverlap++;
}

void Renderer11::PopNoOverlap()
{
	assertRet(_forceNoOverlap > 0);
	_forceNoOverlap--;
}

void Renderer11::PushOpaque()
{
	_forceOpaque++;
}

void Renderer11::PopOpaque()
{
	assertRet(_forceOpaque > 0);
	_forceOpaque--;
}

void Renderer11::DrawSprite(ff::ISprite* sprite, const ff::Transform& transform, LastDepthType depthType)
{
	const ff::SpriteData& data = sprite->GetSpriteData();
	noAssertRet(data._textureView); // an async sprite resource isn't done loading yet

	AlphaType alphaType = ::GetAlphaType(data, transform._color, _forceOpaque);
	noAssertRet(alphaType != AlphaType::Invisible);

	bool usePalette = ff::HasAllFlags(data._type, ff::SpriteType::Palette);
	GeometryBucketType bucketType = (alphaType == AlphaType::Transparent && !_targetRequiresPalette)
		? (usePalette ? GeometryBucketType::PaletteSpritesAlpha : GeometryBucketType::SpritesAlpha)
		: (usePalette ? GeometryBucketType::PaletteSprites : GeometryBucketType::Sprites);

	float depth = NudgeDepth(depthType);
	ff::SpriteGeometryInput& input = *(ff::SpriteGeometryInput*)AddGeometry(bucketType, depth);

	GetWorldMatrixAndTextureIndex(data._textureView, usePalette, input.matrixIndex, input.textureIndex);
	input.pos.x = transform._position.x;
	input.pos.y = transform._position.y;
	input.pos.z = depth;
	input.scale = *(DirectX::XMFLOAT2*)&transform._scale;
	input.rotate = transform.GetRotationRadians();
	input.color = transform._color;
	input.uvrect = *(DirectX::XMFLOAT4*) & data._textureUV;
	input.rect = *(DirectX::XMFLOAT4*) & data._worldRect;
}

void Renderer11::DrawSprite(ff::ISprite* sprite, const ff::Transform& transform)
{
	return DrawSprite(sprite, transform, _forceNoOverlap ? LastDepthType::SpriteNoOverlap : LastDepthType::Sprite);
}

void Renderer11::DrawFont(ff::ISprite* sprite, const ff::Transform& transform)
{
	return DrawSprite(sprite, transform, LastDepthType::FontNoOverlap);
}

void Renderer11::DrawLineStrip(
	const ff::PointFloat* points,
	size_t pointCount,
	const DirectX::XMFLOAT4* colors,
	size_t colorCount,
	float thickness,
	bool pixelThickness)
{
	assert(colorCount == 1 || colorCount == pointCount);
	thickness = pixelThickness ? -std::abs(thickness) : std::abs(thickness);

	ff::LineGeometryInput input;
	input.matrixIndex = (_worldMatrixIndex == ff::INVALID_DWORD) ? GetWorldMatrixIndex() : _worldMatrixIndex;
	input.depth = NudgeDepth(_forceNoOverlap ? LastDepthType::LineNoOverlap : LastDepthType::Line);
	input.color[0] = colors[0];
	input.color[1] = colors[0];
	input.thickness[0] = thickness;
	input.thickness[1] = thickness;

	const DirectX::XMFLOAT2* dxpoints = (const DirectX::XMFLOAT2*)points;
	bool closed = pointCount > 2 && points[0] == points[pointCount - 1];
	AlphaType alphaType = ::GetAlphaType(colors[0], _forceOpaque);

	for (size_t i = 0; i < pointCount - 1; i++)
	{
		input.pos[1] = dxpoints[i];
		input.pos[2] = dxpoints[i + 1];

		input.pos[0] = (i == 0)
			? (closed ? dxpoints[pointCount - 2] : dxpoints[i])
			: dxpoints[i - 1];

		input.pos[3] = (i == pointCount - 2)
			? (closed ? dxpoints[1] : dxpoints[i + 1])
			: dxpoints[i + 2];

		if (colorCount != 1)
		{
			input.color[0] = colors[i];
			input.color[1] = colors[i + 1];
			alphaType = ::GetAlphaType(colors + i, 2, _forceOpaque);
		}

		if (alphaType != AlphaType::Invisible)
		{
			GeometryBucketType bucketType = (alphaType == AlphaType::Transparent) ? GeometryBucketType::LinesAlpha : GeometryBucketType::Lines;
			AddGeometry(&input, bucketType, input.depth);
		}
	}
}

void Renderer11::DrawLineStrip(const ff::PointFloat* points, const DirectX::XMFLOAT4* colors, size_t count, float thickness, bool pixelThickness)
{
	DrawLineStrip(points, count, colors, count, thickness, pixelThickness);
}

void Renderer11::DrawLineStrip(const ff::PointFloat* points, size_t count, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness)
{
	DrawLineStrip(points, count, &color, 1, thickness, pixelThickness);
}

void Renderer11::DrawLine(ff::PointFloat start, ff::PointFloat end, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness)
{
	const ff::PointFloat points[2] =
	{
		start,
		end,
	};

	DrawLineStrip(points, 2, color, thickness, pixelThickness);
}

void Renderer11::DrawFilledRectangle(ff::RectFloat rect, const DirectX::XMFLOAT4* colors)
{
	const float triPoints[12] =
	{
		rect.left, rect.top,
		rect.right, rect.top,
		rect.right, rect.bottom,
		rect.right, rect.bottom,
		rect.left, rect.bottom,
		rect.left, rect.top,
	};

	const DirectX::XMFLOAT4 triColors[6] =
	{
		colors[0],
		colors[1],
		colors[2],
		colors[2],
		colors[3],
		colors[0],
	};

	DrawFilledTriangles((const ff::PointFloat*)triPoints, triColors, 2);
}

void Renderer11::DrawFilledRectangle(ff::RectFloat rect, const DirectX::XMFLOAT4& color)
{
	const float triPoints[12] =
	{
		rect.left, rect.top,
		rect.right, rect.top,
		rect.right, rect.bottom,
		rect.right, rect.bottom,
		rect.left, rect.bottom,
		rect.left, rect.top,
	};

	const DirectX::XMFLOAT4 triColors[6] =
	{
		color,
		color,
		color,
		color,
		color,
		color,
	};

	DrawFilledTriangles((const ff::PointFloat*)triPoints, triColors, 2);
}

void Renderer11::DrawFilledTriangles(const ff::PointFloat* points, const DirectX::XMFLOAT4* colors, size_t count)
{
	ff::TriangleGeometryInput input;
	input.matrixIndex = (_worldMatrixIndex == ff::INVALID_DWORD) ? GetWorldMatrixIndex() : _worldMatrixIndex;
	input.depth = NudgeDepth(_forceNoOverlap ? LastDepthType::TriangleNoOverlap : LastDepthType::Triangle);

	for (size_t i = 0; i < count; i++, points += 3, colors += 3)
	{
		::memcpy(input.pos, points, sizeof(input.pos));
		::memcpy(input.color, colors, sizeof(input.color));

		AlphaType alphaType = ::GetAlphaType(colors, 3, _forceOpaque);
		if (alphaType != AlphaType::Invisible)
		{
			GeometryBucketType bucketType = (alphaType == AlphaType::Transparent) ? GeometryBucketType::TrianglesAlpha : GeometryBucketType::Triangles;
			AddGeometry(&input, bucketType, input.depth);
		}
	}
}

void Renderer11::DrawFilledCircle(ff::PointFloat center, float radius, const DirectX::XMFLOAT4& color)
{
	DrawOutlineCircle(center, radius, color, color, std::abs(radius), false);
}

void Renderer11::DrawFilledCircle(ff::PointFloat center, float radius, const DirectX::XMFLOAT4& insideColor, const DirectX::XMFLOAT4& outsideColor)
{
	DrawOutlineCircle(center, radius, insideColor, outsideColor, std::abs(radius), false);
}

void Renderer11::DrawOutlineRectangle(ff::RectFloat rect, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness)
{
	rect = rect.Normalize();

	if (thickness < 0)
	{
		ff::PointFloat deflate = _geometryConstants0._viewScale * thickness;
		rect = rect.Deflate(deflate);
		DrawOutlineRectangle(rect, color, -thickness, pixelThickness);
	}
	else if (!pixelThickness && (thickness * 2 >= rect.Width() || thickness * 2 >= rect.Height()))
	{
		DrawFilledRectangle(rect, color);
	}
	else
	{
		ff::PointFloat halfThickness(thickness / 2, thickness / 2);
		if (pixelThickness)
		{
			halfThickness *= _geometryConstants0._viewScale;
		}

		rect = rect.Deflate(halfThickness);

		const ff::PointFloat points[5] =
		{
			rect.TopLeft(),
			rect.TopRight(),
			rect.BottomRight(),
			rect.BottomLeft(),
			rect.TopLeft(),
		};

		DrawLineStrip(points, 5, color, thickness, pixelThickness);
	}
}

void Renderer11::DrawOutlineCircle(ff::PointFloat center, float radius, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness)
{
	DrawOutlineCircle(center, radius, color, color, thickness, pixelThickness);
}

void Renderer11::DrawOutlineCircle(ff::PointFloat center, float radius, const DirectX::XMFLOAT4& insideColor, const DirectX::XMFLOAT4& outsideColor, float thickness, bool pixelThickness)
{
	ff::CircleGeometryInput input;
	input.matrixIndex = (_worldMatrixIndex == ff::INVALID_DWORD) ? GetWorldMatrixIndex() : _worldMatrixIndex;
	input.pos.x = center.x;
	input.pos.y = center.y;
	input.pos.z = NudgeDepth(_forceNoOverlap ? LastDepthType::CircleNoOverlap : LastDepthType::Circle);
	input.radius = std::abs(radius);
	input.thickness = pixelThickness ? -std::abs(thickness) : std::min(std::abs(thickness), input.radius);
	input.insideColor = insideColor;
	input.outsideColor = outsideColor;

	AlphaType alphaType = ::GetAlphaType(&input.insideColor, 2, _forceOpaque);
	if (alphaType != AlphaType::Invisible)
	{
		GeometryBucketType bucketType = (alphaType == AlphaType::Transparent) ? GeometryBucketType::CircleAlpha : GeometryBucketType::Circle;
		AddGeometry(&input, bucketType, input.pos.z);
	}
}

void Renderer11::DrawPaletteFont(ff::ISprite* sprite, ff::PointFloat pos, ff::PointFloat scale, int color)
{
	DirectX::XMFLOAT4 color2;
	::PaletteIndexToColor(color, color2);
	DrawFont(sprite, ff::Transform::Create(pos, scale, 0.0f, color2));
}

void Renderer11::DrawPaletteLineStrip(const ff::PointFloat* points, const int* colors, size_t count, float thickness, bool pixelThickness)
{
	ff::Vector<DirectX::XMFLOAT4, 64> colors2;
	colors2.Resize(count);
	::PaletteIndexToColor(colors, colors2.Data(), count);
	DrawLineStrip(points, count, colors2.Data(), count, thickness, pixelThickness);
}

void Renderer11::DrawPaletteLineStrip(const ff::PointFloat* points, size_t count, int color, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 color2;
	::PaletteIndexToColor(color, color2);
	DrawLineStrip(points, count, &color2, 1, thickness, pixelThickness);
}

void Renderer11::DrawPaletteLine(ff::PointFloat start, ff::PointFloat end, int color, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 color2;
	::PaletteIndexToColor(color, color2);
	DrawLine(start, end, color2, thickness, pixelThickness);
}

void Renderer11::DrawPaletteFilledRectangle(ff::RectFloat rect, const int* colors)
{
	std::array<DirectX::XMFLOAT4, 4> colors2;
	::PaletteIndexToColor(colors, colors2.data(), colors2.size());
	DrawFilledRectangle(rect, colors2.data());
}

void Renderer11::DrawPaletteFilledRectangle(ff::RectFloat rect, int color)
{
	DirectX::XMFLOAT4 color2;
	::PaletteIndexToColor(color, color2);
	DrawFilledRectangle(rect, color2);
}

void Renderer11::DrawPaletteFilledTriangles(const ff::PointFloat* points, const int* colors, size_t count)
{
	ff::Vector<DirectX::XMFLOAT4, 64 * 3> colors2;
	colors2.Resize(count * 3);
	::PaletteIndexToColor(colors, colors2.Data(), count);
	DrawFilledTriangles(points, colors2.Data(), count);
}

void Renderer11::DrawPaletteFilledCircle(ff::PointFloat center, float radius, int color)
{
	DirectX::XMFLOAT4 color2;
	::PaletteIndexToColor(color, color2);
	DrawFilledCircle(center, radius, color2);
}

void Renderer11::DrawPaletteFilledCircle(ff::PointFloat center, float radius, int insideColor, int outsideColor)
{
	DirectX::XMFLOAT4 insideColor2, outsideColor2;
	::PaletteIndexToColor(insideColor, insideColor2);
	::PaletteIndexToColor(outsideColor, outsideColor2);
	DrawFilledCircle(center, radius, insideColor2, outsideColor2);
}

void Renderer11::DrawPaletteOutlineRectangle(ff::RectFloat rect, int color, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 color2;
	::PaletteIndexToColor(color, color2);
	DrawOutlineRectangle(rect, color2, thickness, pixelThickness);
}

void Renderer11::DrawPaletteOutlineCircle(ff::PointFloat center, float radius, int color, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 color2;
	::PaletteIndexToColor(color, color2);
	DrawOutlineCircle(center, radius, color2, thickness, pixelThickness);
}

void Renderer11::DrawPaletteOutlineCircle(ff::PointFloat center, float radius, int insideColor, int outsideColor, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 insideColor2, outsideColor2;
	::PaletteIndexToColor(insideColor, insideColor2);
	::PaletteIndexToColor(outsideColor, outsideColor2);
	DrawOutlineCircle(center, radius, insideColor2, outsideColor2, thickness, pixelThickness);
}

ff::IGraphDevice* Renderer11::GetDevice() const
{
	return _device;
}

bool Renderer11::Reset()
{
	return Init();
}

void Renderer11::OnMatrixChanging(const ff::MatrixStack& stack)
{
	_worldMatrixIndex = ff::INVALID_DWORD;
}

void Renderer11::OnMatrixChanged(const ff::MatrixStack& stack)
{
}
