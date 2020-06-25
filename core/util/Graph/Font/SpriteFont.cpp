#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Dict/Dict.h"
#include "Globals/AppGlobals.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/Anim/Transform.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteList.h"
#include "Graph/Sprite/SpriteOptimizer.h"
#include "Graph/Sprite/SpriteType.h"
#include "Graph/Font/FontData.h"
#include "Graph/Font/SpriteFont.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Resource/ResourceValue.h"
#include "String/StringUtil.h"
#include "Types/Timer.h"
#include "Value/Values.h"

static ff::StaticString PROP_AA(L"aa");
static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_GLYPHS(L"glyphs");
static ff::StaticString PROP_OUTLINE(L"outline");
static ff::StaticString PROP_SIZE(L"size");
static ff::StaticString PROP_SPRITES(L"sprites");
static ff::StaticString PROP_OUTLINE_SPRITES(L"outlineSprites");

static const DWRITE_GLYPH_OFFSET s_zeroOffset{ 0, 0 };
static const DWRITE_MATRIX s_identityTransform{ 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };

class __declspec(uuid("c87f399b-5a75-4e0a-8b80-cebc58e2eaa2"))
	SpriteFont
	: public ff::ComBase
	, public ff::ISpriteFont
	, public ff::IResourcePersist
	, public ff::IResourceFinishLoading
{
public:
	DECLARE_HEADER(SpriteFont);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;

	// IGraphDeviceChild functions
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// ISpriteFont functions
	virtual ff::PointFloat DrawText(ff::IRendererActive* render, ff::StringRef text, const ff::Transform& transform, const DirectX::XMFLOAT4& outlineColor) override;
	virtual ff::PointFloat MeasureText(ff::StringRef text, ff::PointFloat scale) override;
	virtual float GetLineSpacing() override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

	// IResourceFinishLoading
	virtual ff::Vector<ff::SharedResourceValue> GetResourceDependencies() override;
	virtual bool FinishLoadFromSource() override;

private:
	bool InitSprites();
	ff::PointFloat InternalDrawText(ff::IRendererActive* render, ff::ISpriteList* sprites, ff::StringRef text, const ff::Transform& transform);

	static const size_t MAX_GLYPH_COUNT = 0x10000;

	struct CharAndGlyphInfo
	{
		UINT16 _glyphToSprite;
		UINT16 _charToGlyph;
		float _glyphWidth;
	};

	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<ff::ISpriteList> _sprites;
	ff::ComPtr<ff::ISpriteList> _outlineSprites;
	ff::TypedResource<ff::IFontData> _data;
	std::array<CharAndGlyphInfo, MAX_GLYPH_COUNT> _glyphs;
	float _size;
	int _outlineThickness;
	bool _antiAlias;
};

BEGIN_INTERFACES(SpriteFont)
	HAS_INTERFACE(ff::ISpriteFont)
	HAS_INTERFACE(ff::IResourcePersist)
	HAS_INTERFACE(ff::IResourceFinishLoading)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<SpriteFont>(L"font");
	});

SpriteFont::SpriteFont()
	: _size(0)
	, _outlineThickness(0)
	, _antiAlias(false)
{
	ff::ZeroObject(_glyphs);
}

SpriteFont::~SpriteFont()
{
	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT SpriteFont::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

ff::IGraphDevice* SpriteFont::GetDevice() const
{
	return _device;
}

bool SpriteFont::Reset()
{
	return true;
}

bool SpriteFont::InitSprites()
{
	noAssertRetVal(!_sprites && !_outlineSprites, true);
	assertRetVal(_size > 0 && _size < 200, false);

	ff::IGraphicFactoryDWrite* dwrite = ff::ProcessGlobals::Get()->GetGraphicFactory()->AsGraphicFactoryDWrite();
	assertRetVal(dwrite, false);

	IDWriteFactoryX* factory = dwrite->GetWriteFactory();
	assertRetVal(factory, false);

	ff::IFontData* data = _data.Flush();
	assertRetVal(data, false);

	IDWriteFontFaceX* fontFace = data->GetFontFace();
	assertRetVal(fontFace, false);

	DWRITE_FONT_METRICS1 fontMetrics;
	fontFace->GetMetrics(&fontMetrics);
	float designUnitSize = _size / fontMetrics.designUnitsPerEm;

	struct SpriteInfo
	{
		size_t _texture;
		ff::RectFloat _pos;
		ff::PointFloat _handle;
	};

	ff::Vector<SpriteInfo> spriteInfos;
	spriteInfos.Reserve(fontFace->GetGlyphCount());

	std::array<bool, MAX_GLYPH_COUNT> hasGlyph;
	ff::ZeroObject(hasGlyph);

	// Map unicode characters to glyphs
	{
		UINT32 unicodeRangeCount;
		assertRetVal(fontFace->GetUnicodeRanges(0, nullptr, &unicodeRangeCount) == E_NOT_SUFFICIENT_BUFFER, false);

		ff::Vector<DWRITE_UNICODE_RANGE> unicodeRanges;
		unicodeRanges.Resize(unicodeRangeCount);
		assertHrRetVal(fontFace->GetUnicodeRanges(unicodeRangeCount, unicodeRanges.Data(), &unicodeRangeCount), false);

		ff::Vector<UINT16> unicodeToGlyph;
		unicodeToGlyph.Reserve(MAX_GLYPH_COUNT);
		for (const DWRITE_UNICODE_RANGE& ur : unicodeRanges)
		{
			for (UINT32 ch = ur.first; ch < MAX_GLYPH_COUNT && ch <= ur.last; ch++)
			{
				UINT16 glyph;
				if (SUCCEEDED(fontFace->GetGlyphIndices(&ch, 1, &glyph)))
				{
					_glyphs[ch]._charToGlyph = glyph;
					hasGlyph[glyph] = true;
				}
			}
		}
	}

	ff::Vector<DirectX::ScratchImage> stagingScratches;
	const ff::PointInt stagingTextureSize(1024, 1024);
	ff::PointInt stagingPos(0, 0);
	int stagingRowHeight = 0;

	DirectX::ScratchImage stagingScratch;
	assertHrRetVal(stagingScratch.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, stagingTextureSize.x, stagingTextureSize.y, 1, 1), false);
	::ZeroMemory(stagingScratch.GetPixels(), stagingScratch.GetPixelsSize());

	float glyphAdvances = 0;
	ff::Vector<BYTE> glyphBytes;
	ff::Map<ff::hash_t, UINT16, ff::NonHasher<ff::hash_t>> hashToSprite;
	DWRITE_TEXTURE_TYPE glyphTextureType = _antiAlias ? DWRITE_TEXTURE_CLEARTYPE_3x1 : DWRITE_TEXTURE_ALIASED_1x1;

	UINT16 glyphId = 0;
	DWRITE_GLYPH_RUN gr;
	ff::ZeroObject(gr);
	gr.fontEmSize = _size;
	gr.fontFace = fontFace;
	gr.glyphAdvances = &glyphAdvances;
	gr.glyphCount = 1;
	gr.glyphIndices = &glyphId;
	gr.glyphOffsets = &s_zeroOffset;

	for (size_t i = 0; i < MAX_GLYPH_COUNT; i++)
	{
		if (!hasGlyph[i])
		{
			continue;
		}

		glyphId = (UINT16)i;

		DWRITE_GLYPH_METRICS gm;
		if (FAILED(fontFace->GetDesignGlyphMetrics(&glyphId, 1, &gm)))
		{
			continue;
		}

		_glyphs[i]._glyphWidth = gm.advanceWidth * designUnitSize;

		ff::ComPtr<IDWriteGlyphRunAnalysis> gra;
		if (FAILED(factory->CreateGlyphRunAnalysis(
			&gr,
			&s_identityTransform,
			_antiAlias ? DWRITE_RENDERING_MODE1_NATURAL : DWRITE_RENDERING_MODE1_ALIASED,
			DWRITE_MEASURING_MODE_NATURAL,
			DWRITE_GRID_FIT_MODE_DEFAULT,
			DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE,
			0, 0,
			&gra)))
		{
			continue;
		}

		RECT bounds;
		if (FAILED(gra->GetAlphaTextureBounds(glyphTextureType, &bounds)))
		{
			continue;
		}

		ff::RectInt blackBox(bounds.left, bounds.top, bounds.right, bounds.bottom);
		if (blackBox.IsEmpty())
		{
			continue;
		}

		glyphBytes.Resize(blackBox.Area() * (_antiAlias ? 3 : 1));
		if (FAILED(gra->CreateAlphaTexture(glyphTextureType, &bounds, glyphBytes.Data(), (UINT32)glyphBytes.Size())))
		{
			continue;
		}

		ff::hash_t glyphBytesHash = ff::HashBytes(glyphBytes.ConstData(), glyphBytes.ByteSize());
		auto iter = hashToSprite.GetKey(glyphBytesHash);
		if (!iter)
		{
			if (stagingPos.x + blackBox.Width() > stagingTextureSize.x)
			{
				// Move down to the next row

				stagingPos.x = 0;
				stagingPos.y += stagingRowHeight + 1;
				stagingRowHeight = 0;
			}

			if (stagingPos.y + blackBox.Height() > stagingTextureSize.y)
			{
				// Filled up this texture, make a new one

				stagingPos = ff::PointInt::Zeros();
				stagingRowHeight = 0;

				stagingScratches.Push(std::move(stagingScratch));
				assertHrRetVal(stagingScratch.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, stagingTextureSize.x, stagingTextureSize.y, 1, 1), false);
				::ZeroMemory(stagingScratch.GetPixels(), stagingScratch.GetPixelsSize());
			}

			// Copy bits to the texture

			size_t pixelStride = _antiAlias ? 3 : 1;
			for (int y = 0; y < blackBox.Height(); y++)
			{
				LPBYTE pAlpha = &glyphBytes[y * blackBox.Width() * pixelStride];
				LPBYTE pData = stagingScratch.GetImages()->pixels + (stagingPos.y + y) * stagingScratch.GetImages()->rowPitch + stagingPos.x * 4;

				for (int x = 0; x < blackBox.Width(); x++)
				{
					pData[x * 4 + 0] = 255;
					pData[x * 4 + 1] = 255;
					pData[x * 4 + 2] = 255;

					if (_antiAlias)
					{
						float value =
							(float)pAlpha[x * pixelStride + 0] +
							(float)pAlpha[x * pixelStride + 1] +
							(float)pAlpha[x * pixelStride + 2];
						pData[x * 4 + 3] = (BYTE)((size_t)(value / 3) & 0xFF);
					}
					else
					{
						pData[x * 4 + 3] = pAlpha[x * pixelStride];
					}
				}
			}

			// Add sprite to staging texture

			spriteInfos.Push(SpriteInfo
				{
					stagingScratches.Size(),
					ff::RectInt(stagingPos, stagingPos + blackBox.Size()).ToType<float>(),
					ff::PointFloat(-gm.leftSideBearing * designUnitSize, blackBox.Height() + gm.bottomSideBearing * designUnitSize),
				});

			stagingPos.x += blackBox.Width() + 1;
			stagingRowHeight = std::max(stagingRowHeight, blackBox.Height());

			iter = hashToSprite.SetKey(glyphBytesHash, (UINT16)(spriteInfos.Size() - 1));
		}

		_glyphs[i]._glyphToSprite = iter->GetValue();
	}

	stagingScratches.Push(std::move(stagingScratch));

	ff::Vector<ff::ComPtr<ff::ITextureView>> textures;
	for (DirectX::ScratchImage& scratch : stagingScratches)
	{
		ff::ComPtr<ff::ITexture> texture = _device->AsGraphDeviceInternal()->CreateTexture(std::move(scratch), nullptr);
		assertRetVal(texture && texture->AsTextureView(), false);
		textures.Push(texture->AsTextureView());
	}

	ff::ComPtr<ff::ISpriteList> sprites;
	assertRetVal(ff::CreateSpriteList(_device, &sprites), false);

	for (const SpriteInfo& info : spriteInfos)
	{
		assertRetVal(sprites->Add(textures[info._texture], ff::GetEmptyString(), info._pos, info._handle), false);
	}

	// Create optimized sprite list
	assertRetVal(ff::OptimizeSprites(sprites, ff::TextureFormat::BC2, 1, &_sprites), false);

	if (_outlineThickness)
	{
		assertRetVal(ff::CreateOutlineSprites(sprites, ff::TextureFormat::BC2, 1, &_outlineSprites), false);
	}

	return true;
}

ff::PointFloat SpriteFont::InternalDrawText(ff::IRendererActive* render, ff::ISpriteList* sprites, ff::StringRef text, const ff::Transform& transform)
{
	noAssertRetVal(text.size() && transform._scale.x != 0 && transform._scale.y != 0 && transform._color.w > 0, ff::PointFloat::Zeros());

	ff::IFontData* data = _data.Flush();
	assertRetVal(data, ff::PointFloat::Zeros());

	IDWriteFontFaceX* fontFace = data->GetFontFace();
	bool hasKerning = fontFace->HasKerningPairs();

	DWRITE_FONT_METRICS1 fm;
	fontFace->GetMetrics(&fm);

	float designUnitSize = _size / fm.designUnitsPerEm;
	ff::PointFloat scaledDesignUnitSize = transform._scale * designUnitSize;
	ff::Transform basePos = transform;
	basePos._position.y += fm.ascent * scaledDesignUnitSize.y;
	ff::PointFloat maxPos(transform._position.x, transform._position.y + (fm.ascent + fm.descent) * scaledDesignUnitSize.y);
	float lineSpacing = (fm.ascent + fm.descent + fm.lineGap) * scaledDesignUnitSize.y;

	if (render)
	{
		render->PushNoOverlap();
	}

	for (const wchar_t* ch = text.c_str(), *chEnd = ch + text.size(); ch != chEnd; )
	{
		if (*ch == '\r' || *ch == '\n')
		{
			ch += (*ch == '\r' && ch + 1 != chEnd && ch[1] == '\n') ? 2 : 1;
			basePos._position.SetPoint(transform._position.x, basePos._position.y + lineSpacing);
			maxPos.y += lineSpacing;
			continue;
		}
		else if (*ch >= (wchar_t)ff::SpriteFontControl::NoOp && *ch <= (wchar_t)ff::SpriteFontControl::AfterLast)
		{
			ff::SpriteFontControl control = (ff::SpriteFontControl) * ch++;
			switch (control)
			{
			case ff::SpriteFontControl::SetOutlineColor:
			case ff::SpriteFontControl::SetTextColor:
				if ((control == ff::SpriteFontControl::SetOutlineColor && sprites == _outlineSprites) ||
					(control == ff::SpriteFontControl::SetTextColor && sprites == _sprites))
				{
					basePos._color.x = ((ch != chEnd) ? (int)*ch++ : 0) / 255.0f;
					basePos._color.y = ((ch != chEnd) ? (int)*ch++ : 0) / 255.0f;
					basePos._color.z = ((ch != chEnd) ? (int)*ch++ : 0) / 255.0f;
					basePos._color.w = ((ch != chEnd) ? (int)*ch++ : 0) / 255.0f;
				}
				break;

			case ff::SpriteFontControl::SetOutlinePaletteColor:
			case ff::SpriteFontControl::SetTextPaletteColor:
				if ((control == ff::SpriteFontControl::SetOutlinePaletteColor && sprites == _outlineSprites) ||
					(control == ff::SpriteFontControl::SetTextPaletteColor && sprites == _sprites))
				{
					ff::PaletteIndexToColor((ch != chEnd) ? (int)*ch++ : 0, basePos._color);
				}
				break;
			}
		}
		else
		{
			const CharAndGlyphInfo& glyph = _glyphs[_glyphs[*ch]._charToGlyph];

			if (render && sprites && glyph._glyphToSprite && glyph._glyphToSprite < sprites->GetCount())
			{
				render->DrawSprite(sprites->Get(glyph._glyphToSprite), basePos);
			}

			basePos._position.x += glyph._glyphWidth * basePos._scale.x;
			maxPos.x = std::max(maxPos.x, basePos._position.x);

			if (hasKerning && ch + 1 != chEnd)
			{
				UINT16 twoGlyphs[2] = { ch[0], ch[1] };
				int twoDesignKerns[2];
				if (SUCCEEDED(fontFace->GetKerningPairAdjustments(2, twoGlyphs, twoDesignKerns)))
				{
					basePos._position.x += twoDesignKerns[0] * scaledDesignUnitSize.x;
				}
			}

			ch++;
		}
	}

	if (render)
	{
		render->PopNoOverlap();
	}

	return maxPos - transform._position;
}

ff::PointFloat SpriteFont::DrawText(ff::IRendererActive* render, ff::StringRef text, const ff::Transform& transform, const DirectX::XMFLOAT4& outlineColor)
{
	if (outlineColor.w > 0 && _outlineSprites)
	{
		ff::Transform outlineTransform = transform;
		outlineTransform._color = outlineColor;
		InternalDrawText(render, _outlineSprites, text, outlineTransform);
		render->NudgeDepth();
	}

	return InternalDrawText(render, _sprites, text, transform);
}

ff::PointFloat SpriteFont::MeasureText(ff::StringRef text, ff::PointFloat scale)
{
	return InternalDrawText(nullptr, nullptr, text, ff::Transform::Identity());
}

float SpriteFont::GetLineSpacing()
{
	ff::IFontData* data = _data.Flush();
	assertRetVal(data, 0);

	IDWriteFontFaceX* fontFace = data->GetFontFace();
	assertRetVal(fontFace, 0);

	DWRITE_FONT_METRICS1 fm;
	fontFace->GetMetrics(&fm);

	float line = (fm.ascent + fm.descent + fm.lineGap) * _size / fm.designUnitsPerEm;
	return line;
}

bool SpriteFont::LoadFromSource(const ff::Dict& dict)
{
	_data.Init(dict.Get<ff::SharedResourceWrapperValue>(PROP_DATA));
	_size = dict.Get<ff::FloatValue>(PROP_SIZE);
	_outlineThickness = dict.Get<ff::IntValue>(PROP_OUTLINE);
	_antiAlias = dict.Get<ff::BoolValue>(PROP_AA);

	return true;
}

ff::Vector<ff::SharedResourceValue> SpriteFont::GetResourceDependencies()
{
	ff::Vector<ff::SharedResourceValue> values;
	assertRetVal(_data.DidInit(), values);
	values.Push(_data.GetResourceValue());

	return values;
}

bool SpriteFont::FinishLoadFromSource()
{
	assertRetVal(InitSprites(), false);
	return true;
}

bool SpriteFont::LoadFromCache(const ff::Dict& dict)
{
	_data.Init(dict.Get<ff::SharedResourceWrapperValue>(PROP_DATA));
	_size = dict.Get<ff::FloatValue>(PROP_SIZE);
	_outlineThickness = dict.Get<ff::IntValue>(PROP_OUTLINE);
	_antiAlias = dict.Get<ff::BoolValue>(PROP_AA);

	ff::ComPtr<ff::IData> glyphsData = dict.Get<ff::DataValue>(PROP_GLYPHS);
	assertRetVal(glyphsData && glyphsData->GetSize() == sizeof(_glyphs), false);
	std::memcpy(_glyphs.data(), glyphsData->GetMem(), glyphsData->GetSize());

	ff::ValuePtr spritesValue = dict.GetValue(PROP_SPRITES);
	assertRetVal(spritesValue && spritesValue->IsType<ff::ObjectValue>(), false);
	assertRetVal(_sprites.QueryFrom(spritesValue->GetValue<ff::ObjectValue>()), false);

	ff::ValuePtr outlineSpritesValue = dict.GetValue(PROP_OUTLINE_SPRITES);
	if (outlineSpritesValue)
	{
		assertRetVal(outlineSpritesValue->IsType<ff::ObjectValue>(), false);
		assertRetVal(_outlineSprites.QueryFrom(outlineSpritesValue->GetValue<ff::ObjectValue>()), false);
	}

	return true;
}

bool SpriteFont::SaveToCache(ff::Dict& dict)
{
	assertRetVal(_sprites, false);

	dict.Set<ff::SharedResourceWrapperValue>(PROP_DATA, _data.GetResourceValue());
	dict.Set<ff::FloatValue>(PROP_SIZE, _size);
	dict.Set<ff::IntValue>(PROP_OUTLINE, _outlineThickness);
	dict.Set<ff::BoolValue>(PROP_AA, _antiAlias);
	dict.Set<ff::DataValue>(PROP_GLYPHS, _glyphs.data(), sizeof(_glyphs));

	ff::Dict spritesDict;
	assertRetVal(ff::SaveResourceToCache(_sprites, spritesDict), false);
	dict.Set<ff::DictValue>(PROP_SPRITES, std::move(spritesDict));

	if (_outlineSprites)
	{
		ff::Dict outlineSpritesDict;
		assertRetVal(ff::SaveResourceToCache(_outlineSprites, outlineSpritesDict), false);
		dict.Set<ff::DictValue>(PROP_OUTLINE_SPRITES, std::move(outlineSpritesDict));
	}

	return true;
}
