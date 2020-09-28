#include "pch.h"
#include "Data/Data.h"
#include "Module/Module.h"
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
static const size_t MAX_PALETTE_REMAPS = 128; // 256 entries only
static const size_t MAX_TRANSFORM_MATRIXES = 1024;
static const size_t MAX_RENDER_COUNT = 524288; // 0x00080000
static const float MAX_RENDER_DEPTH = 1.0f;
static const float RENDER_DEPTH_DELTA = MAX_RENDER_DEPTH / MAX_RENDER_COUNT;

static std::array<ID3D11ShaderResourceView*, ::MAX_TEXTURES + ::MAX_TEXTURES_USING_PALETTE + 2 /* palette + remap */>  NULL_TEXTURES = { nullptr };

static std::array<unsigned char, ff::PALETTE_SIZE> DEFAULT_PALETTE_REMAP =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
	65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
	81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
	97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
	113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
	129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
	145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
	161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
	177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192,
	193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
	209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
	225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
	241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
};

static ff::hash_t DEFAULT_PALETTE_REMAP_HASH = ff::HashBytes(DEFAULT_PALETTE_REMAP.data(), DEFAULT_PALETTE_REMAP.size());

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

	Count,
	FirstAlpha = LinesAlpha,
};

class GeometryBucket
{
private:
	GeometryBucket(GeometryBucketType bucketType, const std::type_info& itemType, size_t itemSize, size_t itemAlign, const D3D11_INPUT_ELEMENT_DESC* elementDesc, size_t elementCount)
		: _bucketType(bucketType)
		, _itemType(&itemType)
		, _itemSize(itemSize)
		, _itemAlign(itemAlign)
		, _renderStart(0)
		, _renderCount(0)
		, _dataStart(nullptr)
		, _dataCur(nullptr)
		, _dataEnd(nullptr)
		, _elementDesc(elementDesc)
		, _elementCount(elementCount)
	{
	}

public:
	template<typename T, GeometryBucketType BucketType>
	static GeometryBucket New()
	{
		return GeometryBucket(BucketType, typeid(T), sizeof(T), alignof(T), T::GetLayout11().data(), T::GetLayout11().size());
	}

	GeometryBucket(GeometryBucket&& rhs)
		: _layout(std::move(rhs._layout))
		, _vs(std::move(rhs._vs))
		, _gs(std::move(rhs._gs))
		, _ps(std::move(rhs._ps))
		, _psPaletteOut(std::move(rhs._psPaletteOut))
		, _elementDesc(rhs._elementDesc)
		, _elementCount(rhs._elementCount)
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

	// Input strings must be static
	void Reset(const wchar_t* vsRes, const wchar_t* gsRes, const wchar_t* psRes, const wchar_t* psPaletteOutRes)
	{
		Reset();

		ff::IResourceAccess* resources = ff::GetThisModule().GetResources();
		_vsRes.Init(resources, ff::String::from_static(vsRes));
		_gsRes.Init(resources, ff::String::from_static(gsRes));
		_psRes.Init(resources, ff::String::from_static(psRes));
		_psPaletteOutRes.Init(resources, ff::String::from_static(psPaletteOutRes));
	}

	void Reset()
	{
		_layout = nullptr;
		_vs = nullptr;
		_gs = nullptr;
		_ps = nullptr;
		_psPaletteOut = nullptr;

		_aligned_free(_dataStart);
		_dataStart = nullptr;
		_dataCur = nullptr;
		_dataEnd = nullptr;
	}

	void Apply(ff::GraphContext11& context, ff::GraphStateCache11& cache, ID3D11Buffer* geometryBuffer, bool paletteOut) const
	{
		const_cast<GeometryBucket*>(this)->CreateShaders(cache, paletteOut);

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
	void CreateShaders(ff::GraphStateCache11& cache, bool paletteOut)
	{
		if (!_vs)
		{
			_vs = cache.GetVertexShaderAndInputLayout(ff::GetThisModule().GetResources(), _vsRes.GetName(), _layout, _elementDesc, _elementCount);
		}

		if (!_gs)
		{
			_gs = cache.GetGeometryShader(ff::GetThisModule().GetResources(), _gsRes.GetName());
		}

		ff::ComPtr<ID3D11PixelShader>& ps = paletteOut ? _psPaletteOut : _ps;
		if (!ps)
		{
			ps = cache.GetPixelShader(ff::GetThisModule().GetResources(), paletteOut ? _psPaletteOutRes.GetName() : _psRes.GetName());
		}
	}

	ff::AutoResourceValue _vsRes;
	ff::AutoResourceValue _gsRes;
	ff::AutoResourceValue _psRes;
	ff::AutoResourceValue _psPaletteOutRes;

	ff::ComPtr<ID3D11InputLayout> _layout;
	ff::ComPtr<ID3D11VertexShader> _vs;
	ff::ComPtr<ID3D11GeometryShader> _gs;
	ff::ComPtr<ID3D11PixelShader> _ps;
	ff::ComPtr<ID3D11PixelShader> _psPaletteOut;

	const D3D11_INPUT_ELEMENT_DESC* _elementDesc;
	size_t _elementCount;

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
	virtual IRendererActive* BeginRender(ff::IRenderTarget* target, ff::IRenderDepth* depth, ff::RectFloat viewRect, ff::RectFloat worldRect, ff::RendererOptions options) override;

	// IRendererActive
	virtual void EndRender() override;
	virtual ff::MatrixStack& GetWorldMatrixStack() override;
	virtual ff::IRendererActive11* AsRendererActive11() override;

	virtual void DrawSprite(ff::ISprite* sprite, const ff::Transform& transform) override;

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
	virtual void PushPaletteRemap(const unsigned char* remap, ff::hash_t hash) override;
	virtual void PopPaletteRemap() override;
	virtual void PushCustomContext(ff::CustomRenderContextFunc11&& func) override;
	virtual void PopCustomContext() override;
	virtual void PushTextureSampler(D3D11_FILTER filter) override;
	virtual void PopTextureSampler() override;
	virtual void PushNoOverlap() override;
	virtual void PopNoOverlap() override;
	virtual void PushOpaque() override;
	virtual void PopOpaque() override;
	virtual void PushPreMultipliedAlpha() override;
	virtual void PopPreMultipliedAlpha() override;
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
		Nudged,

		Line,
		Circle,
		Triangle,
		Sprite,

		LineNoOverlap,
		CircleNoOverlap,
		TriangleNoOverlap,
		SpriteNoOverlap,

		StartNoOverlap = LineNoOverlap,
	};

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
	unsigned int GetPaletteRemapIndexNoFlush();
	int RemapPaletteIndex(int color);
	void GetWorldMatrixAndTextureIndex(ff::ITextureView* texture, bool usePalette, unsigned int& modelIndex, unsigned int& textureIndex);
	void GetWorldMatrixAndTextureIndexes(ff::ITextureView** textures, bool usePalette, unsigned int* textureIndexes, size_t count, unsigned int& modelIndex);
	void AddGeometry(const void* data, GeometryBucketType bucketType, float depth);
	void* AddGeometry(GeometryBucketType bucketType, float depth);

	ff::GraphFixedState11 CreateOpaqueDrawState();
	ff::GraphFixedState11 CreateAlphaDrawState();
	ff::GraphFixedState11 CreatePreMultipliedAlphaDrawState();
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
	ff::GraphFixedState11 _premultipliedAlphaState;
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
	bool _targetRequiresPalette;

	ff::Vector<ff::IPalette*> _paletteStack;
	ff::ComPtr<ff::ITexture> _paletteTexture;
	std::array<ff::hash_t, ::MAX_PALETTES> _paletteTextureHashes;
	ff::Map<ff::hash_t, std::pair<ff::IPalette*, unsigned int>, ff::NonHasher<ff::hash_t>> _paletteToIndex;
	unsigned int _paletteIndex;

	ff::Vector<std::pair<const unsigned char*, ff::hash_t>> _paletteRemapStack;
	ff::ComPtr<ff::ITexture> _paletteRemapTexture;
	std::array<ff::hash_t, ::MAX_PALETTE_REMAPS> _paletteRemapTextureHashes;
	ff::Map<ff::hash_t, std::pair<const unsigned char*, unsigned int>, ff::NonHasher<ff::hash_t>> _paletteRemapToIndex;
	unsigned int _paletteRemapIndex;

	// Render data
	ff::Vector<AlphaGeometryEntry> _alphaGeometry;
	std::array<GeometryBucket, (size_t)GeometryBucketType::Count> _geometryBuckets;
	LastDepthType _lastDepthType;
	float _drawDepth;
	int _forceNoOverlap;
	int _forceOpaque;
	int _forcePMA;
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

static void GetAlphaBlend(D3D11_RENDER_TARGET_BLEND_DESC& desc)
{
	// newColor = (srcColor * SrcBlend) BlendOp (destColor * DestBlend)
	// newAlpha = (srcAlpha * SrcBlendAlpha) BlendOpAlpha (destAlpha * DestBlendAlpha)

	desc.BlendEnable = TRUE;
	desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.BlendOp = D3D11_BLEND_OP_ADD;
	desc.SrcBlendAlpha = D3D11_BLEND_ONE;
	desc.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
}

static void GetPreMultipliedAlphaBlend(D3D11_RENDER_TARGET_BLEND_DESC& desc)
{
	// newColor = (srcColor * SrcBlend) BlendOp (destColor * DestBlend)
	// newAlpha = (srcAlpha * SrcBlendAlpha) BlendOpAlpha (destAlpha * DestBlendAlpha)

	desc.BlendEnable = TRUE;
	desc.SrcBlend = D3D11_BLEND_ONE;
	desc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.BlendOp = D3D11_BLEND_OP_ADD;
	desc.SrcBlendAlpha = D3D11_BLEND_ONE;
	desc.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
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

static ID3D11BlendState* GetPreMultipliedAlphaBlendState(ff::IGraphDevice* device)
{
	CD3D11_BLEND_DESC blend(D3D11_DEFAULT);
	::GetPreMultipliedAlphaBlend(blend.RenderTarget[0]);
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
	_premultipliedAlphaState = ff::GraphFixedState11();
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

	ff::ZeroObject(_paletteRemapTextureHashes);
	_paletteRemapStack.Clear();
	_paletteRemapToIndex.Clear();
	_paletteRemapTexture = nullptr;
	_paletteRemapIndex = ff::INVALID_DWORD;

	_alphaGeometry.Clear();
	_lastDepthType = LastDepthType::None;
	_drawDepth = 0;
	_forceNoOverlap = 0;
	_forceOpaque = 0;
	_forcePMA = 0;

	for (auto& bucket : _geometryBuckets)
	{
		bucket.Reset();
	}
}

bool Renderer11::Init()
{
	Destroy();

	// Geometry buckets
	GetGeometryBucket(GeometryBucketType::Lines).Reset(L"Renderer.LineVS", L"Renderer.LineGS", L"Renderer.ColorPS", L"Renderer.PaletteOutColorPS");
	GetGeometryBucket(GeometryBucketType::Circle).Reset(L"Renderer.CircleVS", L"Renderer.CircleGS", L"Renderer.ColorPS", L"Renderer.PaletteOutColorPS");
	GetGeometryBucket(GeometryBucketType::Triangles).Reset(L"Renderer.TriangleVS", L"Renderer.TriangleGS", L"Renderer.ColorPS", L"Renderer.PaletteOutColorPS");
	GetGeometryBucket(GeometryBucketType::Sprites).Reset(L"Renderer.SpriteVS", L"Renderer.SpriteGS", L"Renderer.SpritePS", L"Renderer.PaletteOutSpritePS");
	GetGeometryBucket(GeometryBucketType::PaletteSprites).Reset(L"Renderer.SpriteVS", L"Renderer.SpriteGS", L"Renderer.SpritePalettePS", L"Renderer.PaletteOutSpritePalettePS");

	GetGeometryBucket(GeometryBucketType::LinesAlpha).Reset(L"Renderer.LineVS", L"Renderer.LineGS", L"Renderer.ColorPS", L"Renderer.PaletteOutColorPS");
	GetGeometryBucket(GeometryBucketType::CircleAlpha).Reset(L"Renderer.CircleVS", L"Renderer.CircleGS", L"Renderer.ColorPS", L"Renderer.PaletteOutColorPS");
	GetGeometryBucket(GeometryBucketType::TrianglesAlpha).Reset(L"Renderer.TriangleVS", L"Renderer.TriangleGS", L"Renderer.ColorPS", L"Renderer.PaletteOutColorPS");
	GetGeometryBucket(GeometryBucketType::SpritesAlpha).Reset(L"Renderer.SpriteVS", L"Renderer.SpriteGS", L"Renderer.SpritePS", L"Renderer.PaletteOutSpritePS");

	// Palette
	_paletteStack.Push(nullptr);
	_paletteTexture = _device->CreateTexture(ff::PointInt((int)ff::PALETTE_SIZE, (int)::MAX_PALETTES), ff::TextureFormat::RGBA32);

	_paletteRemapStack.Push(std::make_pair(::DEFAULT_PALETTE_REMAP.data(), ::DEFAULT_PALETTE_REMAP_HASH));
	_paletteRemapTexture = _device->CreateTexture(ff::PointInt((int)ff::PALETTE_SIZE, (int)::MAX_PALETTE_REMAPS), ff::TextureFormat::R8_UINT);

	// States
	_samplerStack.Push(::GetTextureSamplerState(_device->AsGraphDevice11()->GetStateCache(), D3D11_FILTER_MIN_MAG_MIP_POINT));
	_opaqueState = CreateOpaqueDrawState();
	_alphaState = CreateAlphaDrawState();
	_premultipliedAlphaState = CreatePreMultipliedAlphaDrawState();

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

ff::GraphFixedState11 Renderer11::CreatePreMultipliedAlphaDrawState()
{
	ff::GraphFixedState11 state;

	state._blend = ::GetPreMultipliedAlphaBlendState(_device);
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

ff::IRendererActive* Renderer11::BeginRender(ff::IRenderTarget* target, ff::IRenderDepth* depth, ff::RectFloat viewRect, ff::RectFloat worldRect, ff::RendererOptions options)
{
	EndRender();

	noAssertRetVal(::SetupViewMatrix(target, viewRect, worldRect, _viewMatrix), nullptr);
	assertRetVal(::SetupRenderTarget(_device, target, depth, viewRect), nullptr);
	InitGeometryConstantBuffers0(target, viewRect, worldRect);
	_targetRequiresPalette = ff::IsPaletteFormat(target->GetFormat());
	_forcePMA = ff::HasAllFlags(options, ff::RendererOptions::PreMultipliedAlpha) && ff::FormatSupportsPreMultipliedAlpha(target->GetFormat()) ? 1 : 0;
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
	ID3D11Resource* destRemapResource = _paletteRemapTexture->AsTexture11()->GetTexture2d();
	CD3D11_BOX box(0, 0, 0, static_cast<int>(ff::PALETTE_SIZE), 1, 1);

	for (const auto& iter : _paletteToIndex)
	{
		ff::IPalette* palette = iter.GetValue().first;
		if (palette)
		{
			unsigned int index = iter.GetValue().second;
			size_t paletteRow = palette->GetCurrentRow();
			ff::IPaletteData* paletteData = palette->GetData();
			ff::hash_t rowHash = paletteData->GetRowHash(paletteRow);

			if (_paletteTextureHashes[index] != rowHash)
			{
				_paletteTextureHashes[index] = rowHash;
				ID3D11Resource* srcResource = paletteData->GetTexture()->AsTexture11()->GetTexture2d();
				box.top = (UINT)paletteRow;
				box.bottom = box.top + 1;
				deviceContext.CopySubresourceRegion(destResource, 0, 0, index, 0, srcResource, 0, &box);
			}
		}
	}

	for (const auto& iter : _paletteRemapToIndex)
	{
		const unsigned char* remap = iter.GetValue().first;
		unsigned int row = iter.GetValue().second;
		ff::hash_t rowHash = iter.GetKey();

		if (_paletteRemapTextureHashes[row] != rowHash)
		{
			_paletteRemapTextureHashes[row] = rowHash;
			box.top = row;
			box.bottom = row + 1;
			deviceContext.UpdateSubresource(destRemapResource, 0, &box, remap, static_cast<UINT>(ff::PALETTE_SIZE), 0);
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

		std::array<ID3D11ShaderResourceView*, 2> palettes =
		{
			_paletteTexture->AsTextureView()->AsTextureView11()->GetView(),
			_paletteRemapTexture->AsTextureView()->AsTextureView11()->GetView(),
		};
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
	ff::GraphStateCache11& cache = _device->AsGraphDevice11()->GetStateCache();
	ff::GraphContext11& context = _device->AsGraphDevice11()->GetStateContext();
	context.SetTopologyIA(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	_opaqueState.Apply(context);

	for (GeometryBucket& bucket : _geometryBuckets)
	{
		if (bucket.GetBucketType() >= GeometryBucketType::FirstAlpha)
		{
			break;
		}

		if (bucket.GetRenderCount())
		{
			bucket.Apply(context, cache, _geometryBuffer, _targetRequiresPalette);

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
	ff::GraphStateCache11& cache = _device->AsGraphDevice11()->GetStateCache();
	ff::GraphContext11& context = _device->AsGraphDevice11()->GetStateContext();
	context.SetTopologyIA(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	ff::GraphFixedState11& alphaState = _forcePMA ? _premultipliedAlphaState : _alphaState;
	alphaState.Apply(context);

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

		entry._bucket->Apply(context, cache, _geometryBuffer, _targetRequiresPalette);

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
	_paletteRemapToIndex.Clear();
	_paletteRemapIndex = ff::INVALID_DWORD;

	_textureCount = 0;
	_texturesUsingPaletteCount = 0;

	_alphaGeometry.Clear();
	_lastDepthType = LastDepthType::None;
}

void Renderer11::EndRender()
{
	noAssertRet(IsRendering());

	Flush();

	ff::GraphContext11& context = _device->AsGraphDevice11()->GetStateContext();
	context.SetResourcesPS(::NULL_TEXTURES.data(), 0, ::NULL_TEXTURES.size());

	_state = State::Valid;
	_paletteStack.Resize(1);
	_paletteRemapStack.Resize(1);
	_samplerStack.Resize(1);
	_customContextStack.Clear();
	_worldMatrixStack.Reset();
	_drawDepth = 0;
	_forceNoOverlap = 0;
	_forceOpaque = 0;
	_forcePMA = 0;
}

bool Renderer11::IsRendering() const
{
	return _state == State::Rendering;
}

void Renderer11::NudgeDepth()
{
	_lastDepthType = LastDepthType::Nudged;
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

		unsigned int paletteRemapIndex = (_paletteRemapIndex == ff::INVALID_DWORD) ? GetPaletteRemapIndexNoFlush() : _paletteRemapIndex;
		if (paletteRemapIndex == ff::INVALID_DWORD)
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

		return textureIndex | (paletteIndex << 8) | (paletteRemapIndex << 16);
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
			ff::hash_t paletteHash = palette ? palette->GetData()->GetRowHash(palette->GetCurrentRow()) : 0;
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

unsigned int Renderer11::GetPaletteRemapIndexNoFlush()
{
	if (_paletteRemapIndex == ff::INVALID_DWORD)
	{
		auto& remapPair = _paletteRemapStack.GetLast();
		auto iter = _paletteRemapToIndex.GetKey(remapPair.second);

		if (!iter && _paletteRemapToIndex.Size() != ::MAX_PALETTE_REMAPS)
		{
			iter = _paletteRemapToIndex.SetKey(remapPair.second, std::make_pair(remapPair.first, (unsigned int)_paletteRemapToIndex.Size()));
		}

		if (iter)
		{
			_paletteRemapIndex = iter->GetValue().second;
		}
	}

	return _paletteRemapIndex;
}

int Renderer11::RemapPaletteIndex(int color)
{
	return _paletteRemapStack.GetLast().first[color];
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

	PushPaletteRemap(palette->GetRemap(), palette->GetRemapHash());
}

void Renderer11::PopPalette()
{
	assertRet(_paletteStack.Size() > 1);
	_paletteStack.Pop();
	_paletteIndex = ff::INVALID_DWORD;

	PopPaletteRemap();
}

void Renderer11::PushPaletteRemap(const unsigned char* remap, ff::hash_t hash)
{
	_paletteRemapStack.Push(std::make_pair(
		remap ? remap : ::DEFAULT_PALETTE_REMAP.data(),
		remap ? (hash ? hash : ff::HashBytes(remap, ff::PALETTE_SIZE)) : ::DEFAULT_PALETTE_REMAP_HASH));
	_paletteRemapIndex = ff::INVALID_DWORD;
}

void Renderer11::PopPaletteRemap()
{
	assertRet(_paletteRemapStack.Size() > 1);
	_paletteRemapStack.Pop();
	_paletteRemapIndex = ff::INVALID_DWORD;
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

	if (!--_forceNoOverlap)
	{
		NudgeDepth();
	}
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

void Renderer11::PushPreMultipliedAlpha()
{
	if (!_forcePMA)
	{
		Flush();
	}

	_forcePMA++;
}

void Renderer11::PopPreMultipliedAlpha()
{
	assertRet(_forcePMA > 0);

	if (_forcePMA == 1)
	{
		Flush();
	}

	_forcePMA--;
}

void Renderer11::DrawSprite(ff::ISprite* sprite, const ff::Transform& transform)
{
	const ff::SpriteData& data = sprite->GetSpriteData();
	noAssertRet(data._textureView); // an async sprite resource isn't done loading yet

	AlphaType alphaType = ::GetAlphaType(data, transform._color, _forceOpaque);
	noAssertRet(alphaType != AlphaType::Invisible);

	bool usePalette = ff::HasAllFlags(data._type, ff::SpriteType::Palette);
	GeometryBucketType bucketType = (alphaType == AlphaType::Transparent && !_targetRequiresPalette)
		? (usePalette ? GeometryBucketType::PaletteSprites : GeometryBucketType::SpritesAlpha)
		: (usePalette ? GeometryBucketType::PaletteSprites : GeometryBucketType::Sprites);

	float depth = NudgeDepth(_forceNoOverlap ? LastDepthType::SpriteNoOverlap : LastDepthType::Sprite);
	ff::SpriteGeometryInput& input = *(ff::SpriteGeometryInput*)AddGeometry(bucketType, depth);

	GetWorldMatrixAndTextureIndex(data._textureView, usePalette, input.matrixIndex, input.textureIndex);
	input.pos.x = transform._position.x;
	input.pos.y = transform._position.y;
	input.pos.z = depth;
	input.scale = *(DirectX::XMFLOAT2*)&transform._scale;
	input.rotate = transform.GetRotationRadians();
	input.color = transform._color;
	input.uvrect = *(DirectX::XMFLOAT4*)&data._textureUV;
	input.rect = *(DirectX::XMFLOAT4*)&data._worldRect;
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

void Renderer11::DrawPaletteLineStrip(const ff::PointFloat* points, const int* colors, size_t count, float thickness, bool pixelThickness)
{
	ff::Vector<DirectX::XMFLOAT4, 64> colors2;
	colors2.Resize(count);

	for (size_t i = 0; i != colors2.Size(); i++)
	{
		ff::PaletteIndexToColor(RemapPaletteIndex(colors[i]), colors2[i]);
	}

	DrawLineStrip(points, count, colors2.Data(), count, thickness, pixelThickness);
}

void Renderer11::DrawPaletteLineStrip(const ff::PointFloat* points, size_t count, int color, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 color2;
	ff::PaletteIndexToColor(RemapPaletteIndex(color), color2);
	DrawLineStrip(points, count, &color2, 1, thickness, pixelThickness);
}

void Renderer11::DrawPaletteLine(ff::PointFloat start, ff::PointFloat end, int color, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 color2;
	ff::PaletteIndexToColor(RemapPaletteIndex(color), color2);
	DrawLine(start, end, color2, thickness, pixelThickness);
}

void Renderer11::DrawPaletteFilledRectangle(ff::RectFloat rect, const int* colors)
{
	std::array<DirectX::XMFLOAT4, 4> colors2;

	for (size_t i = 0; i != colors2.size(); i++)
	{
		ff::PaletteIndexToColor(RemapPaletteIndex(colors[i]), colors2[i]);
	}

	DrawFilledRectangle(rect, colors2.data());
}

void Renderer11::DrawPaletteFilledRectangle(ff::RectFloat rect, int color)
{
	DirectX::XMFLOAT4 color2;
	ff::PaletteIndexToColor(RemapPaletteIndex(color), color2);
	DrawFilledRectangle(rect, color2);
}

void Renderer11::DrawPaletteFilledTriangles(const ff::PointFloat* points, const int* colors, size_t count)
{
	ff::Vector<DirectX::XMFLOAT4, 64 * 3> colors2;
	colors2.Resize(count * 3);

	for (size_t i = 0; i != colors2.Size(); i++)
	{
		ff::PaletteIndexToColor(RemapPaletteIndex(colors[i]), colors2[i]);
	}

	DrawFilledTriangles(points, colors2.Data(), count);
}

void Renderer11::DrawPaletteFilledCircle(ff::PointFloat center, float radius, int color)
{
	DirectX::XMFLOAT4 color2;
	ff::PaletteIndexToColor(RemapPaletteIndex(color), color2);
	DrawFilledCircle(center, radius, color2);
}

void Renderer11::DrawPaletteFilledCircle(ff::PointFloat center, float radius, int insideColor, int outsideColor)
{
	DirectX::XMFLOAT4 insideColor2, outsideColor2;
	ff::PaletteIndexToColor(RemapPaletteIndex(insideColor), insideColor2);
	ff::PaletteIndexToColor(RemapPaletteIndex(outsideColor), outsideColor2);
	DrawFilledCircle(center, radius, insideColor2, outsideColor2);
}

void Renderer11::DrawPaletteOutlineRectangle(ff::RectFloat rect, int color, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 color2;
	ff::PaletteIndexToColor(RemapPaletteIndex(color), color2);
	DrawOutlineRectangle(rect, color2, thickness, pixelThickness);
}

void Renderer11::DrawPaletteOutlineCircle(ff::PointFloat center, float radius, int color, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 color2;
	ff::PaletteIndexToColor(RemapPaletteIndex(color), color2);
	DrawOutlineCircle(center, radius, color2, thickness, pixelThickness);
}

void Renderer11::DrawPaletteOutlineCircle(ff::PointFloat center, float radius, int insideColor, int outsideColor, float thickness, bool pixelThickness)
{
	DirectX::XMFLOAT4 insideColor2, outsideColor2;
	ff::PaletteIndexToColor(RemapPaletteIndex(insideColor), insideColor2);
	ff::PaletteIndexToColor(RemapPaletteIndex(outsideColor), outsideColor2);
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
