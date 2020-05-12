#include "pch.h"
#include "Data/Data.h"
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
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "Resource/ResourceValue.h"

static const size_t MAX_TEXTURES = 32;
static const size_t MAX_MODEL_MATRIXES = 1024;
static const size_t MAX_RENDER_COUNT = 524288;
static const float MAX_RENDER_DEPTH = 1.0f;
static const float RENDER_DEPTH_DELTA = MAX_RENDER_DEPTH / MAX_RENDER_COUNT;

ID3D11ShaderResourceView* NULL_TEXTURES[::MAX_TEXTURES] = { nullptr };

enum class GeometryBucketType
{
	Lines,
	LinesScreen,
	Circle,
	CircleScreen,
	Triangles,
	Sprites,
	MultiSprites,
	PaletteSprites,

	LinesAlpha,
	LinesScreenAlpha,
	CircleAlpha,
	CircleScreenAlpha,
	TrianglesAlpha,
	SpritesAlpha,
	MultiSpritesAlpha,

	Count,
	FirstAlpha = LinesAlpha,
};

static const size_t GEOMETRY_BUCKET_COUNT = (size_t)GeometryBucketType::Count;

class GeometryBucketBase
{
public:
	virtual ~GeometryBucketBase()
	{
	}

	virtual void Reset(
		ID3D11InputLayout* layout = nullptr,
		ID3D11VertexShader* vs = nullptr,
		ID3D11GeometryShader* gs = nullptr,
		ID3D11PixelShader* ps = nullptr)
	{
		_layout = layout;
		_vs = vs;
		_gs = gs;
		_ps = ps;
	}

	void Apply(ff::GraphContext11& context, ID3D11Buffer* geometryBuffer) const
	{
		context.SetVertexIA(geometryBuffer, GetItemByteSize(), 0);
		context.SetLayoutIA(_layout);
		context.SetVS(_vs);
		context.SetGS(_gs);
		context.SetPS(_ps);
	}

	virtual void ClearItems() = 0;
	virtual void Add(const void* data) = 0;
	virtual void* Add() = 0;
	virtual size_t GetItemByteSize() const = 0;
	virtual const std::type_info& GetItemType() const = 0;
	virtual GeometryBucketType GetBucketType() const = 0;
	virtual size_t GetCount() const = 0;
	virtual const void* GetData() const = 0;
	virtual size_t GetDataSize() const = 0;

private:
	ff::ComPtr<ID3D11InputLayout> _layout;
	ff::ComPtr<ID3D11VertexShader> _vs;
	ff::ComPtr<ID3D11GeometryShader> _gs;
	ff::ComPtr<ID3D11PixelShader> _ps;
};

template<typename T, GeometryBucketType BucketType>
class GeometryBucket : public GeometryBucketBase
{
public:
	virtual void Reset(ID3D11InputLayout* layout, ID3D11VertexShader* vs, ID3D11GeometryShader* gs, ID3D11PixelShader* ps) override
	{
		_geometry.ClearAndReduce();
		GeometryBucketBase::Reset(layout, vs, gs, ps);
	}

	virtual void ClearItems() override
	{
		_geometry.Clear();
	}

	virtual void Add(const void* data) override
	{
		_geometry.Push(*(T*)data);
	}

	virtual void* Add() override
	{
		_geometry.InsertDefault(_geometry.Size());
		return &_geometry.GetLast();
	}

	virtual size_t GetItemByteSize() const override
	{
		return sizeof(T);
	}

	virtual const std::type_info& GetItemType() const override
	{
		return typeid(T);
	}

	virtual GeometryBucketType GetBucketType() const override
	{
		return BucketType;
	}

	virtual size_t GetCount() const override
	{
		return _geometry.Size();
	}

	virtual const void* GetData() const override
	{
		return _geometry.Size() ? _geometry.ConstData() : nullptr;
	}

	virtual size_t GetDataSize() const override
	{
		return _geometry.ByteSize();
	}

private:
	ff::Vector<T> _geometry;
};

typedef std::array<std::unique_ptr<GeometryBucketBase>, ::GEOMETRY_BUCKET_COUNT> GeometryBucketArray;

struct GeometryRenderInfo
{
	const GeometryBucketBase* _bucket;
	size_t _startGeometry;
	size_t _countGeometry;
};

typedef std::array<GeometryRenderInfo, ::GEOMETRY_BUCKET_COUNT> GeometryRenderInfoArray;

struct AlphaGeometryEntry
{
	GeometryBucketType _bucketType;
	size_t _index;
	float _depth;
};

static_assert(std::is_trivially_copyable<GeometryRenderInfo>::value, "GeometryRenderInfo must be POD");
static_assert(std::is_trivially_copyable<AlphaGeometryEntry>::value, "AlphaGeometryEntry must be POD");

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

	virtual void DrawSprite(ff::ISprite* sprite, ff::PointFloat pos, ff::PointFloat scale, const float rotate, const DirectX::XMFLOAT4& color) override;
	virtual void DrawMultiSprite(ff::ISprite** sprites, const DirectX::XMFLOAT4* colors, size_t count, ff::PointFloat pos, ff::PointFloat scale, const float rotate) override;
	virtual void DrawFont(ff::ISprite* sprite, ff::PointFloat pos, ff::PointFloat scale, const DirectX::XMFLOAT4& color) override;
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
		LineScreen,
		Circle,
		CircleScreen,
		Triangle,
		Sprite,
		MultiSprite,

		LineNoOverlap,
		LineScreenNoOverlap,
		CircleNoOverlap,
		CircleScreenNoOverlap,
		TriangleNoOverlap,
		SpriteNoOverlap,
		MultiSpriteNoOverlap,
		FontNoOverlap,

		StartNoOverlap = LineNoOverlap,
	};

	void DrawSprite(ff::ISprite* sprite, const ff::PointFloat& pos, const ff::PointFloat& scale, const float rotate, const DirectX::XMFLOAT4& color, LastDepthType depthType);
	void DrawMultiSprite(ff::ISprite** sprites, const DirectX::XMFLOAT4* colors, size_t count, const ff::PointFloat& pos, const ff::PointFloat& scale, const float rotate, LastDepthType depthType);
	void DrawLineStrip(const ff::PointFloat* points, size_t pointCount, const DirectX::XMFLOAT4* colors, size_t colorCount, float thickness, bool pixelThickness);

	void InitConstantBuffers0(ff::IRenderTarget* target, const ff::RectFloat& viewRect, const ff::RectFloat& worldRect);
	void UpdateConstantBuffers0(const DirectX::XMFLOAT4X4* worldTransform = nullptr, float zoffset = 0);
	void UpdateConstantBuffers1(const ff::Vector<DirectX::XMFLOAT4X4>* worldMatrixesOverride = nullptr);
	void SetShaderInput(const ff::Vector<ff::ComPtr<ID3D11ShaderResourceView>>* texturesOverride = nullptr);

	void Flush();
	bool CreateGeometryBuffer(GeometryRenderInfoArray& infos, ff::ComPtr<ID3D11Buffer>& geometryBuffer);
	void DrawOpaqueGeometry(const GeometryRenderInfoArray& infos, ID3D11Buffer* geometryBuffer);
	void DrawAlphaGeometry(const GeometryRenderInfoArray& infos, const ff::Vector<AlphaGeometryEntry>& alphaGeometry, ID3D11Buffer* geometryBuffer);
	void PostFlush();

	bool IsRendering() const;
	float NudgeDepth(LastDepthType depthType);
	unsigned int GetWorldMatrixIndex();
	unsigned int GetTextureIndex(ff::ITextureView* texture);
	void GetWorldMatrixAndTextureIndex(ff::ITextureView* texture, unsigned int& modelIndex, unsigned int& textureIndex);
	void GetWorldMatrixAndTextureIndexes(ff::ITextureView** textures, unsigned int* textureIndexes, size_t count, unsigned int& modelIndex);
	void AddGeometry(const void* data, size_t byteSize, GeometryBucketType bucketType, float depth);
	void* AddGeometry(size_t byteSize, GeometryBucketType bucketType, float depth);

	template<typename T>
	void AddGeometry(const T& data, GeometryBucketType bucketType, float depth)
	{
		AddGeometry(&data, sizeof(T), bucketType, depth);
	}

	template<typename T>
	T* AddGeometry(GeometryBucketType bucketType, float depth)
	{
		return (T*)AddGeometry(sizeof(T), bucketType, depth);
	}
	ff::GraphFixedState11 CreateOpaqueDrawState();
	ff::GraphFixedState11 CreateAlphaDrawState();
	GeometryBucketBase& GetGeometryBucket(GeometryBucketType type);

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
	GeometryShaderConstants0 _geometryConstants0;
	GeometryShaderConstants1 _geometryConstants1;
	ff::hash_t _geometryConstantsHash0;
	ff::hash_t _geometryConstantsHash1;

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
	std::array<ff::ITextureView11*, ::MAX_TEXTURES> _textures;
	size_t _textureCount;

	// Render data
	ff::Vector<AlphaGeometryEntry> _alphaGeometry;
	GeometryBucketArray _geometryBuckets;
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
		return AlphaType::Transparent;

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

template<typename T, GeometryBucketType BucketType>
static void InitGeometryBucket(GeometryBucketArray& buckets)
{
	buckets[(size_t)BucketType] = std::make_unique<GeometryBucket<T, BucketType>>();
}

Renderer11::Renderer11(ff::IGraphDevice* device)
	: _device(device)
	, _worldMatrixStack(this)
{
	assert(_geometryBuckets.size() == 14);

	::InitGeometryBucket<ff::LineGeometryInput, GeometryBucketType::Lines>(_geometryBuckets);
	::InitGeometryBucket<ff::LineScreenGeometryInput, GeometryBucketType::LinesScreen>(_geometryBuckets);
	::InitGeometryBucket<ff::CircleGeometryInput, GeometryBucketType::Circle>(_geometryBuckets);
	::InitGeometryBucket<ff::CircleScreenGeometryInput, GeometryBucketType::CircleScreen>(_geometryBuckets);
	::InitGeometryBucket<ff::TriangleGeometryInput, GeometryBucketType::Triangles>(_geometryBuckets);
	::InitGeometryBucket<ff::SpriteGeometryInput, GeometryBucketType::Sprites>(_geometryBuckets);
	::InitGeometryBucket<ff::MultiSpriteGeometryInput, GeometryBucketType::MultiSprites>(_geometryBuckets);

	::InitGeometryBucket<ff::LineGeometryInput, GeometryBucketType::LinesAlpha>(_geometryBuckets);
	::InitGeometryBucket<ff::LineScreenGeometryInput, GeometryBucketType::LinesScreenAlpha>(_geometryBuckets);
	::InitGeometryBucket<ff::CircleGeometryInput, GeometryBucketType::CircleAlpha>(_geometryBuckets);
	::InitGeometryBucket<ff::CircleScreenGeometryInput, GeometryBucketType::CircleScreenAlpha>(_geometryBuckets);
	::InitGeometryBucket<ff::TriangleGeometryInput, GeometryBucketType::TrianglesAlpha>(_geometryBuckets);
	::InitGeometryBucket<ff::SpriteGeometryInput, GeometryBucketType::SpritesAlpha>(_geometryBuckets);
	::InitGeometryBucket<ff::MultiSpriteGeometryInput, GeometryBucketType::MultiSpritesAlpha>(_geometryBuckets);

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
	_geometryConstants0 = GeometryShaderConstants0();
	_geometryConstants1 = GeometryShaderConstants1();
	_geometryConstantsHash0 = 0;
	_geometryConstantsHash1 = 0;

	_samplerStack.Clear();
	_opaqueState = ff::GraphFixedState11();
	_alphaState = ff::GraphFixedState11();
	_customContextStack.Clear();

	_viewMatrix = ff::GetIdentityMatrix();
	_worldMatrixStack.Reset();
	_worldMatrixToIndex.Clear();
	_worldMatrixIndex = ff::INVALID_DWORD;

	ff::ZeroObject(_textures);
	_textureCount = 0;

	_alphaGeometry.Clear();
	_lastDepthType = LastDepthType::None;
	_drawDepth = 0;
	_forceNoOverlap = 0;
	_forceOpaque = 0;

	for (auto& bucket : _geometryBuckets)
	{
		bucket->Reset();
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
	ff::ComPtr<ID3D11InputLayout> multiSpriteLayout;

	// Vertex shaders
	ID3D11VertexShader* lineVS = ::GetVertexShaderAndInputLayout<ff::LineGeometryInput>(L"LineVS", lineLayout, cache);
	ID3D11VertexShader* circleVS = ::GetVertexShaderAndInputLayout<ff::CircleGeometryInput>(L"CircleVS", circleLayout, cache);
	ID3D11VertexShader* triangleVS = ::GetVertexShaderAndInputLayout<ff::TriangleGeometryInput>(L"TriangleVS", triangleLayout, cache);
	ID3D11VertexShader* spriteVS = ::GetVertexShaderAndInputLayout<ff::SpriteGeometryInput>(L"SpriteVS", spriteLayout, cache);
	ID3D11VertexShader* multiSpriteVS = ::GetVertexShaderAndInputLayout<ff::MultiSpriteGeometryInput>(L"MultiSpriteVS", multiSpriteLayout, cache);

	// Geometry shaders
	ID3D11GeometryShader* lineGS = ::GetGeometryShader(L"LineGS", cache);
	ID3D11GeometryShader* lineScreenGS = ::GetGeometryShader(L"LineScreenGS", cache);
	ID3D11GeometryShader* circleGS = ::GetGeometryShader(L"CircleGS", cache);
	ID3D11GeometryShader* circleScreenGS = ::GetGeometryShader(L"CircleScreenGS", cache);
	ID3D11GeometryShader* triangleGS = ::GetGeometryShader(L"TriangleGS", cache);
	ID3D11GeometryShader* spriteGS = ::GetGeometryShader(L"SpriteGS", cache);
	ID3D11GeometryShader* multiSpriteGS = ::GetGeometryShader(L"MultiSpriteGS", cache);

	// Pixel shaders
	ID3D11PixelShader* colorPS = ::GetPixelShader(L"ColorPS", cache);
	ID3D11PixelShader* spritePS = ::GetPixelShader(L"SpritePS", cache);
	ID3D11PixelShader* multiSpritePS = ::GetPixelShader(L"MultiSpritePS", cache);

	assertRetVal(
		lineVS != nullptr &&
		circleVS != nullptr &&
		triangleVS != nullptr &&
		spriteVS != nullptr &&
		multiSpriteVS != nullptr &&
		lineGS != nullptr &&
		lineScreenGS != nullptr &&
		circleGS != nullptr &&
		circleScreenGS != nullptr &&
		triangleGS != nullptr &&
		spriteGS != nullptr &&
		multiSpriteGS != nullptr &&
		colorPS != nullptr &&
		spritePS != nullptr &&
		multiSpritePS != nullptr &&
		lineLayout != nullptr &&
		circleLayout != nullptr &&
		triangleLayout != nullptr &&
		spriteLayout != nullptr &&
		multiSpriteLayout != nullptr,
		false);

	// Geometry buckets
	assert(_geometryBuckets.size() == 14);

	GetGeometryBucket(GeometryBucketType::Lines).Reset(lineLayout, lineVS, lineGS, colorPS);
	GetGeometryBucket(GeometryBucketType::LinesScreen).Reset(lineLayout, lineVS, lineScreenGS, colorPS);
	GetGeometryBucket(GeometryBucketType::Circle).Reset(circleLayout, circleVS, circleGS, colorPS);
	GetGeometryBucket(GeometryBucketType::CircleScreen).Reset(circleLayout, circleVS, circleScreenGS, colorPS);
	GetGeometryBucket(GeometryBucketType::Triangles).Reset(triangleLayout, triangleVS, triangleGS, colorPS);
	GetGeometryBucket(GeometryBucketType::Sprites).Reset(spriteLayout, spriteVS, spriteGS, spritePS);
	GetGeometryBucket(GeometryBucketType::MultiSprites).Reset(multiSpriteLayout, multiSpriteVS, multiSpriteGS, multiSpritePS);

	GetGeometryBucket(GeometryBucketType::LinesAlpha).Reset(lineLayout, lineVS, lineGS, colorPS);
	GetGeometryBucket(GeometryBucketType::CircleAlpha).Reset(circleLayout, circleVS, circleGS, colorPS);
	GetGeometryBucket(GeometryBucketType::CircleScreenAlpha).Reset(circleLayout, circleVS, circleScreenGS, colorPS);
	GetGeometryBucket(GeometryBucketType::LinesScreenAlpha).Reset(lineLayout, lineVS, lineScreenGS, colorPS);
	GetGeometryBucket(GeometryBucketType::TrianglesAlpha).Reset(triangleLayout, triangleVS, triangleGS, colorPS);
	GetGeometryBucket(GeometryBucketType::SpritesAlpha).Reset(spriteLayout, spriteVS, spriteGS, spritePS);
	GetGeometryBucket(GeometryBucketType::MultiSpritesAlpha).Reset(multiSpriteLayout, multiSpriteVS, multiSpriteGS, multiSpritePS);

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

GeometryBucketBase& Renderer11::GetGeometryBucket(GeometryBucketType type)
{
	return *_geometryBuckets[(size_t)type];
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
	InitConstantBuffers0(target, viewRect, worldRect);

	_state = State::Rendering;
	return this;
}

void Renderer11::InitConstantBuffers0(ff::IRenderTarget* target, const ff::RectFloat& viewRect, const ff::RectFloat& worldRect)
{
	_geometryConstants0._viewSize = viewRect.Size() / (float)target->GetDpiScale();
	_geometryConstants0._viewScale = worldRect.Size() / _geometryConstants0._viewSize;
}

void Renderer11::UpdateConstantBuffers0(const DirectX::XMFLOAT4X4* worldTransform, float zoffset)
{
	_geometryConstants0._zoffset = zoffset;

	if (worldTransform)
	{
		DirectX::XMMATRIX worldMatrix = DirectX::XMLoadFloat4x4(worldTransform);
		DirectX::XMMATRIX viewMatrix = DirectX::XMLoadFloat4x4(&_viewMatrix);
		DirectX::XMStoreFloat4x4(&_geometryConstants0._projection, DirectX::XMMatrixTranspose(viewMatrix * worldMatrix));
	}
	else
	{
		_geometryConstants0._projection = _viewMatrix;
	}

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

void Renderer11::UpdateConstantBuffers1(const ff::Vector<DirectX::XMFLOAT4X4>* worldMatrixesOverride)
{
	// Build up model matrix array
	size_t worldMatrixCount = 0;
	if (worldMatrixesOverride)
	{
		worldMatrixCount = worldMatrixesOverride->Size();
		_geometryConstants1._model = *worldMatrixesOverride;
	}
	else
	{
		worldMatrixCount = _worldMatrixToIndex.Size();
		_geometryConstants1._model.Resize(worldMatrixCount);

		for (const auto& iter : _worldMatrixToIndex)
		{
			unsigned int index = iter.GetValue();
			_geometryConstants1._model[index] = iter.GetKey();
		}
	}

	ff::hash_t hash1 = worldMatrixCount
		? ff::HashBytes(_geometryConstants1._model.ConstData(), _geometryConstants1._model.ByteSize())
		: 0;

	if (_geometryConstantsBuffer1 == nullptr || _geometryConstantsHash1 != hash1)
	{
		_geometryConstantsHash1 = hash1;
#if _DEBUG
		size_t bufferSize = sizeof(DirectX::XMFLOAT4X4) * ::MAX_MODEL_MATRIXES;
#else
		size_t bufferSize = sizeof(DirectX::XMFLOAT4X4) * worldMatrixCount;
#endif
		::EnsureBuffer(_device, _geometryConstantsBuffer1, bufferSize, D3D11_BIND_CONSTANT_BUFFER, true);
		_device->AsGraphDevice11()->GetStateContext().UpdateDiscard(_geometryConstantsBuffer1, _geometryConstants1._model.ConstData(), _geometryConstants1._model.ByteSize());
	}
}

void Renderer11::SetShaderInput(const ff::Vector<ff::ComPtr<ID3D11ShaderResourceView>>* texturesOverride)
{
	ff::GraphContext11& context = _device->AsGraphDevice11()->GetStateContext();
	std::array<ID3D11Buffer*, 2> buffersGS =
	{
		_geometryConstantsBuffer0,
		_geometryConstantsBuffer1,
	};

	context.SetConstantsGS(buffersGS.data(), 0, buffersGS.size());
	context.SetSamplersPS(_samplerStack.GetLast().Address(), 0, 1);

	if (texturesOverride)
	{
		if (texturesOverride->Size())
		{
			context.SetResourcesPS(texturesOverride->Data()->Address(), 0, texturesOverride->Size());
		}
	}
	else if (_textureCount)
	{
		ID3D11ShaderResourceView* textures[::MAX_TEXTURES] = { nullptr };

		for (size_t i = 0; i < _textureCount; i++)
		{
			textures[i] = _textures[i]->GetView();
		}

		context.SetResourcesPS(textures, 0, _textureCount);
	}
}

void Renderer11::Flush()
{
	noAssertRet(_lastDepthType != LastDepthType::None);

	GeometryRenderInfoArray infos;
	noAssertRet(CreateGeometryBuffer(infos, _geometryBuffer));

	UpdateConstantBuffers0();
	UpdateConstantBuffers1();
	SetShaderInput();
	DrawOpaqueGeometry(infos, _geometryBuffer);
	DrawAlphaGeometry(infos, _alphaGeometry, _geometryBuffer);

	PostFlush();
}

bool Renderer11::CreateGeometryBuffer(GeometryRenderInfoArray& infos, ff::ComPtr<ID3D11Buffer>& buffer)
{
	size_t byteSize = 0;

	for (size_t i = 0; i < ::GEOMETRY_BUCKET_COUNT; i++)
	{
		GeometryRenderInfo& info = infos[i];
		const GeometryBucketBase& bucket = *_geometryBuckets[i];
		size_t vertexSize = bucket.GetItemByteSize();

		byteSize = ff::RoundUp(byteSize, vertexSize);

		info._bucket = &bucket;
		info._startGeometry = byteSize / vertexSize;
		info._countGeometry = bucket.GetCount();

		byteSize += bucket.GetDataSize();
	}

	assertRetVal(byteSize, false);

	::EnsureBuffer(_device, buffer, byteSize, D3D11_BIND_VERTEX_BUFFER, true);
	assertRetVal(buffer, false);

	void* bufferData = _device->AsGraphDevice11()->GetStateContext().Map(buffer, D3D11_MAP_WRITE_DISCARD);

	for (const GeometryRenderInfo& info : infos)
	{
		if (info._countGeometry)
		{
			::memcpy((BYTE*)bufferData + info._startGeometry * info._bucket->GetItemByteSize(),
				info._bucket->GetData(),
				info._bucket->GetDataSize());
		}
	}

	_device->AsGraphDevice11()->GetStateContext().Unmap(buffer);

	return true;
}

void Renderer11::DrawOpaqueGeometry(const GeometryRenderInfoArray& infos, ID3D11Buffer* geometryBuffer)
{
	const ff::CustomRenderContextFunc11* customFunc = _customContextStack.Size() ? &_customContextStack.GetLast() : nullptr;
	ff::GraphContext11& context = _device->AsGraphDevice11()->GetStateContext();
	context.SetTopologyIA(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	_opaqueState.Apply(context);

	for (const GeometryRenderInfo& info : infos)
	{
		if (info._bucket->GetBucketType() >= GeometryBucketType::FirstAlpha)
		{
			break;
		}

		if (info._countGeometry)
		{
			info._bucket->Apply(context, geometryBuffer);

			if (!customFunc || (*customFunc)(context, info._bucket->GetItemType(), true))
			{
				context.Draw(info._countGeometry, info._startGeometry);
			}
		}
	}
}

void Renderer11::DrawAlphaGeometry(const GeometryRenderInfoArray& infos, const ff::Vector<AlphaGeometryEntry>& alphaGeometry, ID3D11Buffer* geometryBuffer)
{
	noAssertRet(alphaGeometry.Size());

	const ff::CustomRenderContextFunc11* customFunc = _customContextStack.Size() ? &_customContextStack.GetLast() : nullptr;
	ff::GraphContext11& context = _device->AsGraphDevice11()->GetStateContext();
	context.SetTopologyIA(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	_alphaState.Apply(context);

	for (size_t i = 0; i < alphaGeometry.Size(); )
	{
		const AlphaGeometryEntry& entry = alphaGeometry[i];
		const GeometryRenderInfo& info = infos[(size_t)entry._bucketType];
		size_t geometryCount = 1;

		for (i++; i < alphaGeometry.Size(); i++, geometryCount++)
		{
			const AlphaGeometryEntry& entry2 = alphaGeometry[i];
			if (entry2._bucketType != entry._bucketType ||
				entry2._depth != entry._depth ||
				entry2._index != entry._index + geometryCount)
			{
				break;
			}
		}

		info._bucket->Apply(context, geometryBuffer);

		if (!customFunc || (*customFunc)(context, info._bucket->GetItemType(), false))
		{
			context.Draw(geometryCount, info._startGeometry + entry._index);
		}
	}
}

void Renderer11::PostFlush()
{
	_worldMatrixToIndex.Clear();
	_worldMatrixIndex = ff::INVALID_DWORD;
	ff::ZeroObject(_textures);
	_textureCount = 0;
	_alphaGeometry.Clear();
	_lastDepthType = LastDepthType::None;

	for (auto& bucket : _geometryBuckets)
	{
		bucket->ClearItems();
	}
}

void Renderer11::EndRender()
{
	noAssertRet(IsRendering());

	Flush();

	_device->AsGraphDevice11()->GetStateContext().SetResourcesPS(::NULL_TEXTURES, 0, ::MAX_TEXTURES);

	_state = State::Valid;
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
	if (_worldMatrixIndex == ff::INVALID_DWORD)
	{
		DirectX::XMFLOAT4X4 wm;
		DirectX::XMStoreFloat4x4(&wm, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&_worldMatrixStack.GetMatrix())));
		auto iter = _worldMatrixToIndex.GetKey(wm);

		if (!iter)
		{
			if (_worldMatrixToIndex.Size() == ::MAX_MODEL_MATRIXES)
			{
				// Make room for a new matrix
				Flush();
			}

			iter = _worldMatrixToIndex.SetKey(wm, (unsigned int)_worldMatrixToIndex.Size());
		}

		_worldMatrixIndex = iter->GetValue();
	}

	return _worldMatrixIndex;
}

unsigned int Renderer11::GetTextureIndex(ff::ITextureView* texture)
{
	ff::ITextureView11* texture11 = texture ? texture->AsTextureView11() : nullptr;
	noAssertRetVal(texture11, ff::INVALID_DWORD);

	for (size_t i = 0; i < _textureCount; i++)
	{
		if (_textures[i] == texture11)
		{
			return (unsigned int)i;
		}
	}

	if (_textureCount == ::MAX_TEXTURES)
	{
		// Make room for more textures
		Flush();
	}

	_textures[_textureCount] = texture11;
	return (unsigned int)_textureCount++;
}

void Renderer11::GetWorldMatrixAndTextureIndex(ff::ITextureView* texture, unsigned int& modelIndex, unsigned int& textureIndex)
{
	LastDepthType depthType = _lastDepthType;

	modelIndex = GetWorldMatrixIndex();
	textureIndex = GetTextureIndex(texture);

	if (depthType != _lastDepthType)
	{
		// Do it again since there was a Flush
		GetWorldMatrixAndTextureIndex(texture, modelIndex, textureIndex);
	}
}

void Renderer11::GetWorldMatrixAndTextureIndexes(ff::ITextureView** textures, unsigned int* textureIndexes, size_t count, unsigned int& modelIndex)
{
	LastDepthType depthType = _lastDepthType;

	modelIndex = GetWorldMatrixIndex();

	for (size_t i = 0; i < count; i++)
	{
		textureIndexes[i] = GetTextureIndex(textures[i]);
	}

	if (depthType != _lastDepthType)
	{
		// Do it again since there was a Flush
		GetWorldMatrixAndTextureIndexes(textures, textureIndexes, count, modelIndex);
	}
}

void Renderer11::AddGeometry(const void* data, size_t byteSize, GeometryBucketType bucketType, float depth)
{
	GeometryBucketBase& bucket = GetGeometryBucket(bucketType);

	if (bucketType >= GeometryBucketType::FirstAlpha)
	{
		assert(!_forceOpaque);

		_alphaGeometry.Push(AlphaGeometryEntry
			{
				bucketType,
				bucket.GetCount(),
				depth
			});
	}

	assert(byteSize == bucket.GetItemByteSize());
	bucket.Add(data);
}

void* Renderer11::AddGeometry(size_t byteSize, GeometryBucketType bucketType, float depth)
{
	GeometryBucketBase& bucket = GetGeometryBucket(bucketType);

	if (bucketType >= GeometryBucketType::FirstAlpha)
	{
		assert(!_forceOpaque);

		_alphaGeometry.Push(AlphaGeometryEntry
			{
				bucketType,
				bucket.GetCount(),
				depth
			});
	}

	assert(byteSize == bucket.GetItemByteSize());
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

void Renderer11::DrawSprite(ff::ISprite* sprite, const ff::PointFloat& pos, const ff::PointFloat& scale, const float rotate, const DirectX::XMFLOAT4& color, LastDepthType depthType)
{
	const ff::SpriteData& data = sprite->GetSpriteData();
	AlphaType alphaType = ::GetAlphaType(data, color, _forceOpaque);
	if (alphaType != AlphaType::Invisible)
	{
		float depth = NudgeDepth(depthType);
		GeometryBucketType bucketType = (alphaType == AlphaType::Transparent) ? GeometryBucketType::SpritesAlpha : GeometryBucketType::Sprites;
		ff::SpriteGeometryInput& input = *AddGeometry<ff::SpriteGeometryInput>(bucketType, depth);

		GetWorldMatrixAndTextureIndex(data._textureView, input.matrixIndex, input.textureIndex);
		input.pos.x = pos.x;
		input.pos.y = pos.y;
		input.pos.z = depth;
		input.scale = *(DirectX::XMFLOAT2*) & scale;
		input.rotate = rotate;
		input.color = color;
		input.uvrect = *(DirectX::XMFLOAT4*) & data._textureUV;
		input.rect = *(DirectX::XMFLOAT4*) & data._worldRect;
	}
}

void Renderer11::DrawMultiSprite(ff::ISprite** sprites, const DirectX::XMFLOAT4* colors, size_t count, const ff::PointFloat& pos, const ff::PointFloat& scale, const float rotate, LastDepthType depthType)
{
	noAssertRet(count);

	if (count == 1)
	{
		DrawSprite(sprites[0], pos, scale, rotate, colors[0], depthType);
		return;
	}

	const ff::SpriteData* datas[4] = { nullptr };
	ff::ITextureView* textures[4] = { nullptr };

	for (size_t i = 0; i < count; i++)
	{
		datas[i] = &sprites[i]->GetSpriteData();
		textures[i] = datas[i]->_textureView;
	}

	AlphaType alphaType = ::GetAlphaType(datas, colors, count, _forceOpaque);
	if (alphaType != AlphaType::Invisible)
	{
		float depth = NudgeDepth(depthType);
		GeometryBucketType bucketType = (alphaType == AlphaType::Transparent) ? GeometryBucketType::MultiSpritesAlpha : GeometryBucketType::MultiSprites;
		ff::MultiSpriteGeometryInput& input = *AddGeometry<ff::MultiSpriteGeometryInput>(bucketType, depth);

		for (size_t i = 0; i < count; i++)
		{
			input.color[i] = colors[i];
			input.uvrect[i] = *(DirectX::XMFLOAT4*) & datas[i]->_textureUV;
		}

		for (size_t i = count; i < 4; i++)
		{
			input.color[i] = ff::GetColorNone();
			input.uvrect[i] = ff::GetColorNone();
			input.textureIndex[i] = ff::INVALID_DWORD;
		}

		GetWorldMatrixAndTextureIndexes(textures, input.textureIndex, count, input.matrixIndex);
		input.rect = *(DirectX::XMFLOAT4*) & datas[0]->_worldRect;
		input.pos.x = pos.x;
		input.pos.y = pos.y;
		input.pos.z = depth;
		input.scale = *(DirectX::XMFLOAT2*) & scale;
		input.rotate = rotate;
	}
}

void Renderer11::DrawSprite(ff::ISprite* sprite, ff::PointFloat pos, ff::PointFloat scale, const float rotate, const DirectX::XMFLOAT4& color)
{
	return DrawSprite(sprite, pos, scale, rotate, color, _forceNoOverlap ? LastDepthType::SpriteNoOverlap : LastDepthType::Sprite);
}

void Renderer11::DrawMultiSprite(ff::ISprite** sprites, const DirectX::XMFLOAT4* colors, size_t count, ff::PointFloat pos, ff::PointFloat scale, const float rotate)
{
	return DrawMultiSprite(sprites, colors, count, pos, scale, rotate, _forceNoOverlap ? LastDepthType::SpriteNoOverlap : LastDepthType::Sprite);
}

void Renderer11::DrawFont(ff::ISprite* sprite, ff::PointFloat pos, ff::PointFloat scale, const DirectX::XMFLOAT4& color)
{
	return DrawSprite(sprite, pos, scale, 0, color, LastDepthType::FontNoOverlap);
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
	thickness = std::abs(thickness);

	ff::LineGeometryInput input;
	input.matrixIndex = GetWorldMatrixIndex();
	input.depth = NudgeDepth(pixelThickness
		? (_forceNoOverlap ? LastDepthType::LineScreenNoOverlap : LastDepthType::LineScreen)
		: (_forceNoOverlap ? LastDepthType::LineNoOverlap : LastDepthType::Line));
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
			GeometryBucketType bucketType = pixelThickness
				? ((alphaType == AlphaType::Transparent) ? GeometryBucketType::LinesScreenAlpha : GeometryBucketType::LinesScreen)
				: ((alphaType == AlphaType::Transparent) ? GeometryBucketType::LinesAlpha : GeometryBucketType::Lines);

			AddGeometry(input, bucketType, input.depth);
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
	input.matrixIndex = GetWorldMatrixIndex();
	input.depth = NudgeDepth(_forceNoOverlap ? LastDepthType::TriangleNoOverlap : LastDepthType::Triangle);

	for (size_t i = 0; i < count; i++, points += 3, colors += 3)
	{
		::memcpy(input.pos, points, sizeof(input.pos));
		::memcpy(input.color, colors, sizeof(input.color));

		AlphaType alphaType = ::GetAlphaType(colors, 3, _forceOpaque);
		if (alphaType != AlphaType::Invisible)
		{
			GeometryBucketType bucketType = (alphaType == AlphaType::Transparent) ? GeometryBucketType::TrianglesAlpha : GeometryBucketType::Triangles;
			AddGeometry(input, bucketType, input.depth);
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
	input.matrixIndex = GetWorldMatrixIndex();
	input.pos.x = center.x;
	input.pos.y = center.y;
	input.pos.z = NudgeDepth(pixelThickness
		? (_forceNoOverlap ? LastDepthType::CircleScreenNoOverlap : LastDepthType::CircleScreen)
		: (_forceNoOverlap ? LastDepthType::CircleNoOverlap : LastDepthType::Circle));
	input.radius = std::abs(radius);
	input.thickness = !pixelThickness ? std::min(thickness, input.radius) : thickness;
	input.insideColor = insideColor;
	input.outsideColor = outsideColor;

	AlphaType alphaType = ::GetAlphaType(&input.insideColor, 2, _forceOpaque);
	if (alphaType != AlphaType::Invisible)
	{
		GeometryBucketType bucketType = pixelThickness
			? ((alphaType == AlphaType::Transparent) ? GeometryBucketType::CircleScreenAlpha : GeometryBucketType::CircleScreen)
			: ((alphaType == AlphaType::Transparent) ? GeometryBucketType::CircleAlpha : GeometryBucketType::Circle);

		AddGeometry(input, bucketType, input.pos.z);
	}
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
