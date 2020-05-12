#include "pch.h"
#include "Graph/DirectXUtil.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteList.h"
#include "Graph/Sprite/SpriteOptimizer.h"
#include "Graph/Sprite/SpriteType.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"

// STATIC_DATA (pod)
static const int TEXTURE_SIZE_MAX = 1024;
static const int TEXTURE_SIZE_MIN = 128;
static const int TEXTURE_GRID_SIZE = 8;

// Info about where each sprite came from and where it's going
struct OptimizedSpriteInfo
{
	OptimizedSpriteInfo();
	OptimizedSpriteInfo(ff::ISprite* sprite, size_t spriteIndex);

	bool operator<(const OptimizedSpriteInfo& rhs) const;

	ff::SpriteData _spriteData;
	ff::RectInt _sourceRect;
	ff::RectInt _destRect;
	size_t _destTexture;
	size_t _spriteIndex;
};

OptimizedSpriteInfo::OptimizedSpriteInfo()
{
	assert(false);
	ff::ZeroObject(*this);
}

OptimizedSpriteInfo::OptimizedSpriteInfo(ff::ISprite* sprite, size_t spriteIndex)
	: _spriteData(sprite->GetSpriteData())
	, _sourceRect(sprite->GetSpriteData().GetTextureRect())
	, _destRect(ff::RectInt::Zeros())
	, _destTexture(ff::INVALID_SIZE)
	, _spriteIndex(spriteIndex)
{
}

bool OptimizedSpriteInfo::operator<(const OptimizedSpriteInfo& rhs) const
{
	if (_sourceRect.Height() == rhs._sourceRect.Height())
	{
		if (_sourceRect.Width() == rhs._sourceRect.Width())
		{
			if (_sourceRect.top == rhs._sourceRect.top)
			{
				if (_sourceRect.left == rhs._sourceRect.left)
				{
					return _spriteData._textureView < rhs._spriteData._textureView;
				}

				return _sourceRect.left < rhs._sourceRect.left;
			}

			return _sourceRect.top < rhs._sourceRect.top;
		}

		// larger comes first
		return _sourceRect.Width() > rhs._sourceRect.Width();
	}

	// larger comes first
	return _sourceRect.Height() > rhs._sourceRect.Height();
}

// Cached RGBA original texture
struct OriginalTextureInfo
{
	OriginalTextureInfo();
	OriginalTextureInfo(const OriginalTextureInfo& rhs);
	OriginalTextureInfo(OriginalTextureInfo&& rhs);

	OriginalTextureInfo& operator=(const OriginalTextureInfo& rhs);

	ff::ComPtr<ff::ITexture> _rgbTexture;
	const DirectX::ScratchImage* _rgbScratch;
	DirectX::ScratchImage _rgbScratchData;
};

OriginalTextureInfo::OriginalTextureInfo()
	: _rgbScratch(nullptr)
{
}

OriginalTextureInfo::OriginalTextureInfo(const OriginalTextureInfo& rhs)
	: _rgbScratch(nullptr)
{
	*this = rhs;
}

OriginalTextureInfo::OriginalTextureInfo(OriginalTextureInfo&& rhs)
	: _rgbTexture(std::move(rhs._rgbTexture))
	, _rgbScratch(rhs._rgbScratch)
	, _rgbScratchData(std::move(_rgbScratchData))
{
	if (rhs._rgbScratch == &rhs._rgbScratchData)
	{
		_rgbScratch = &_rgbScratchData;
	}
}

OriginalTextureInfo& OriginalTextureInfo::operator=(const OriginalTextureInfo& rhs)
{
	assert(false);
	return *this;
}

// Destination texture RGBA (_texture) and final converted texture (_finalTexture)
struct OptimizedTextureInfo
{
	OptimizedTextureInfo(ff::PointInt size);
	OptimizedTextureInfo(const OptimizedTextureInfo& rhs);
	OptimizedTextureInfo(OptimizedTextureInfo&& rhs);

	ff::RectInt FindPlacement(ff::PointInt size);
	bool PlaceRect(ff::RectInt rect);

	ff::PointInt _size;
	DirectX::ScratchImage _texture;
	ff::ComPtr<ff::ITexture> _finalTexture;

private:
	BYTE _rowLeft[TEXTURE_SIZE_MAX / TEXTURE_GRID_SIZE];
	BYTE _rowRight[TEXTURE_SIZE_MAX / TEXTURE_GRID_SIZE];
};

OptimizedTextureInfo::OptimizedTextureInfo(ff::PointInt size)
	: _size(size)
{
	// Column indexes must fit within a byte (even one beyond the last column)
	assert(TEXTURE_SIZE_MAX / TEXTURE_GRID_SIZE < 256 && _countof(_rowLeft) == _countof(_rowRight));

	ff::ZeroObject(_rowLeft);
	ff::ZeroObject(_rowRight);
}

OptimizedTextureInfo::OptimizedTextureInfo(const OptimizedTextureInfo& rhs)
	: _size(rhs._size)
	, _finalTexture(rhs._finalTexture)
{
	assert(false);

	std::memcpy(_rowLeft, rhs._rowLeft, sizeof(_rowLeft));
	std::memcpy(_rowRight, rhs._rowRight, sizeof(_rowLeft));
}

OptimizedTextureInfo::OptimizedTextureInfo(OptimizedTextureInfo&& rhs)
	: _size(rhs._size)
	, _texture(std::move(rhs._texture))
	, _finalTexture(std::move(rhs._finalTexture))
{
	std::memcpy(_rowLeft, rhs._rowLeft, sizeof(_rowLeft));
	std::memcpy(_rowRight, rhs._rowRight, sizeof(_rowLeft));
}

ff::RectInt OptimizedTextureInfo::FindPlacement(ff::PointInt size)
{
	ff::PointInt destPos(-1, -1);

	if (size.x > 0 && size.x <= _size.x && size.y > 0 && size.y <= _size.y)
	{
		ff::PointInt cellSize(
			(size.x + TEXTURE_GRID_SIZE - 1) / TEXTURE_GRID_SIZE,
			(size.y + TEXTURE_GRID_SIZE - 1) / TEXTURE_GRID_SIZE);

		for (int y = 0; y + cellSize.y <= _size.y / TEXTURE_GRID_SIZE; y++)
		{
			for (int attempt = 0; attempt < 2; attempt++)
			{
				// Try to put the sprite as far left as possible in each row
				int x;

				if (attempt)
				{
					x = _rowRight[y];

					if (x)
					{
						// don't touch the previous sprite
						x++;
					}
				}
				else
				{
					x = _rowLeft[y];
					x -= cellSize.x + 1;
				}

				if (x >= 0 && x + cellSize.x <= _size.x / TEXTURE_GRID_SIZE)
				{
					bool found = true;

					// Look for intersection with another sprite
					for (int checkY = y + cellSize.y; checkY >= y - 1; checkY--)
					{
						if (checkY >= 0 &&
							checkY < _size.y / TEXTURE_GRID_SIZE &&
							checkY < _countof(_rowRight) &&
							_rowRight[checkY] &&
							_rowRight[checkY] + 1 > x &&
							x + cellSize.x + 1 > _rowLeft[checkY])
						{
							found = false;
							break;
						}
					}

					// Prefer positions further to the left
					if (found && (destPos.x == -1 || destPos.x > x * TEXTURE_GRID_SIZE))
					{
						destPos.SetPoint(x * TEXTURE_GRID_SIZE, y * TEXTURE_GRID_SIZE);
					}
				}
			}
		}
	}

	return (destPos.x == -1)
		? ff::RectInt(0, 0, 0, 0)
		: ff::RectInt(destPos.x, destPos.y, destPos.x + size.x, destPos.y + size.y);
}

bool OptimizedTextureInfo::PlaceRect(ff::RectInt rect)
{
	ff::RectInt rectCells(
		rect.left / TEXTURE_GRID_SIZE,
		rect.top / TEXTURE_GRID_SIZE,
		(rect.right + TEXTURE_GRID_SIZE - 1) / TEXTURE_GRID_SIZE,
		(rect.bottom + TEXTURE_GRID_SIZE - 1) / TEXTURE_GRID_SIZE);

	assertRetVal(
		rect.left >= 0 && rect.right <= _size.x &&
		rect.top >= 0 && rect.bottom <= _size.y &&
		rect.Width() > 0 &&
		rect.Height() > 0, false);

	// Validate that the sprite doesn't overlap anything

	for (int y = rectCells.top - 1; y <= rectCells.bottom; y++)
	{
		if (y >= 0 && y < _size.y / TEXTURE_GRID_SIZE && y < _countof(_rowRight))
		{
			// Must be a one cell gap between sprites
			assertRetVal(!_rowRight[y] ||
				_rowRight[y] + 1 <= rectCells.left ||
				rectCells.right + 1 <= _rowLeft[y], false);
		}
	}

	// Invalidate the space taken up by the new sprite

	for (int y = rectCells.top; y < rectCells.bottom; y++)
	{
		if (y >= 0 && y < _size.y / TEXTURE_GRID_SIZE && y < _countof(_rowRight))
		{
			if (_rowRight[y])
			{
				_rowLeft[y] = std::min<BYTE>(_rowLeft[y], rectCells.left);
				_rowRight[y] = std::max<BYTE>(_rowRight[y], rectCells.right);
			}
			else
			{
				_rowLeft[y] = rectCells.left;
				_rowRight[y] = rectCells.right;
			}
		}
	}

	return true;
}

// Returns true when all done (sprites will still be placed when false is returned)
static bool PlaceSprites(ff::Vector<OptimizedSpriteInfo>& sprites, ff::Vector<OptimizedTextureInfo>& textureInfos, size_t startTexture)
{
	size_t nSpritesDone = 0;

	for (size_t i = 0; i < sprites.Size(); i++)
	{
		OptimizedSpriteInfo& sprite = sprites[i];
		bool bReusePrevious = (i > 0) && !(sprite < sprites[i - 1]) && !(sprites[i - 1] < sprite);

		if (bReusePrevious)
		{
			sprite._destTexture = sprites[i - 1]._destTexture;
			sprite._destRect = sprites[i - 1]._destRect;
		}
		else
		{
			if (sprite._destTexture == ff::INVALID_SIZE)
			{
				if (sprite._sourceRect.Width() > TEXTURE_SIZE_MAX ||
					sprite._sourceRect.Height() > TEXTURE_SIZE_MAX)
				{
					ff::PointInt size = sprite._sourceRect.Size();

					// Oversized, so make a new unshared texture
					sprite._destTexture = textureInfos.Size();
					sprite._destRect.SetRect(ff::PointInt(0, 0), size);

					// texture sizes should be powers of 2 to support compression and mipmaps
					size.x = (int)ff::NearestPowerOfTwo((size_t)size.x);
					size.y = (int)ff::NearestPowerOfTwo((size_t)size.y);

					OptimizedTextureInfo newTexture(size);
					textureInfos.Push(newTexture);
				}
			}

			if (sprite._destTexture == ff::INVALID_SIZE)
			{
				// Look for empty space in an existing texture
				for (size_t h = startTexture; h < textureInfos.Size(); h++)
				{
					OptimizedTextureInfo& texture = textureInfos[h];

					if (texture._size.x <= TEXTURE_SIZE_MAX &&
						texture._size.y <= TEXTURE_SIZE_MAX)
					{
						sprite._destRect = texture.FindPlacement(sprite._sourceRect.Size());

						if (!sprite._destRect.IsZeros())
						{
							verify(texture.PlaceRect(sprite._destRect));
							sprite._destTexture = h;

							break;
						}
					}
				}
			}
		}

		if (sprite._destTexture != ff::INVALID_SIZE)
		{
			nSpritesDone++;
		}
	}

	return nSpritesDone == sprites.Size();
}

static ff::Vector<OptimizedSpriteInfo> CreateSpriteInfos(ff::ISpriteList* originalSprites)
{
	ff::Vector<OptimizedSpriteInfo> spriteInfos;
	spriteInfos.Reserve(originalSprites->GetCount());

	for (size_t i = 0; i < originalSprites->GetCount(); i++)
	{
		spriteInfos.Push(OptimizedSpriteInfo(originalSprites->Get(i), i));
	}

	return spriteInfos;
}

static bool CreateOriginalTextures(const ff::Vector<OptimizedSpriteInfo>& spriteInfos, ff::Map<ff::ITexture*, OriginalTextureInfo>& originalTextures)
{
	for (const OptimizedSpriteInfo& spriteInfo : spriteInfos)
	{
		ff::ITexture* texture = spriteInfo._spriteData._textureView->GetTexture();
		if (!originalTextures.KeyExists(texture))
		{
			OriginalTextureInfo textureInfo;
			textureInfo._rgbTexture = texture->Convert(ff::TextureFormat::RGBA32, 1);
			assertRetVal(textureInfo._rgbTexture, false);

			textureInfo._rgbScratch = textureInfo._rgbTexture->AsTextureDxgi()->Capture(textureInfo._rgbScratchData);
			assertRetVal(textureInfo._rgbScratch, false);

			originalTextures.SetKey(std::move(texture), std::move(textureInfo));
		}
	}

	return true;
}

static bool ComputeOptimizedSprites(ff::Vector<OptimizedSpriteInfo>& sprites, ff::Vector<OptimizedTextureInfo>& textureInfos)
{
	for (bool bDone = false; !bDone && sprites.Size(); )
	{
		// Add a new texture, start with the smallest size and work up
		for (ff::PointInt size(TEXTURE_SIZE_MIN, TEXTURE_SIZE_MIN); !bDone && size.x <= TEXTURE_SIZE_MAX; size *= 2)
		{
			size_t nTextureInfo = textureInfos.Size();
			textureInfos.Push(OptimizedTextureInfo(size));

			bDone = ::PlaceSprites(sprites, textureInfos, textureInfos.Size() - 1);

			if (!bDone && size.x < TEXTURE_SIZE_MAX)
			{
				// Remove this texture and use a bigger one instead
				textureInfos.Delete(nTextureInfo);

				for (OptimizedSpriteInfo& sprite : sprites)
				{
					if (sprite._destTexture != ff::INVALID_SIZE)
					{
						if (sprite._destTexture == nTextureInfo)
						{
							sprite._destTexture = ff::INVALID_SIZE;
							sprite._destRect.SetRect(0, 0, 0, 0);
						}
						else if (sprite._destTexture > nTextureInfo)
						{
							sprite._destTexture--;
						}
					}
				}
			}
		}
	}

	return true;
}

static bool CreateOptimizedTextures(ff::TextureFormat format, ff::Vector<OptimizedTextureInfo>& textureInfos)
{
	format = (format == ff::TextureFormat::A8) ? format : ff::TextureFormat::RGBA32;

	for (OptimizedTextureInfo& texture : textureInfos)
	{
		assertHrRetVal(texture._texture.Initialize2D(ff::ConvertTextureFormat(format), texture._size.x, texture._size.y, 1, 1), false);
		::ZeroMemory(texture._texture.GetPixels(), texture._texture.GetPixelsSize());
	}

	return true;
}

static bool CopyOptimizedSprites(
	ff::Vector<OptimizedSpriteInfo>& spriteInfos,
	ff::Map<ff::ITexture*, OriginalTextureInfo>& originalTextures,
	ff::Vector<OptimizedTextureInfo>& textureInfos)
{
	for (OptimizedSpriteInfo& sprite : spriteInfos)
	{
		const ff::SpriteData& data = sprite._spriteData;
		assertRetVal(sprite._destTexture < textureInfos.Size(), false);
		OptimizedTextureInfo& destTexture = textureInfos[sprite._destTexture];

		auto iter = originalTextures.GetKey(sprite._spriteData._textureView->GetTexture());
		assertRetVal(iter, false);
		OriginalTextureInfo& originalInfo = iter->GetEditableValue();
		sprite._spriteData._type = ff::GetSpriteTypeForImage(*originalInfo._rgbScratch, &sprite._sourceRect.ToType<size_t>());

		DirectX::CopyRectangle(
			*originalInfo._rgbScratch->GetImages(),
			DirectX::Rect(
				sprite._sourceRect.left,
				sprite._sourceRect.top,
				sprite._sourceRect.Width(),
				sprite._sourceRect.Height()),
			*destTexture._texture.GetImages(),
			DirectX::TEX_FILTER_DEFAULT,
			sprite._destRect.left,
			sprite._destRect.top);
	}

	return true;
}

static bool ConvertFinalTextures(ff::IGraphDevice* device, ff::TextureFormat format, size_t mipMapLevels, ff::Vector<OptimizedTextureInfo>& textureInfos)
{
	for (OptimizedTextureInfo& textureInfo : textureInfos)
	{
		ff::ComPtr<ff::ITexture> rgbTexture = device->AsGraphDeviceInternal()->CreateTexture(std::move(textureInfo._texture));
		assertRetVal(rgbTexture, false);

		textureInfo._finalTexture = rgbTexture->Convert(format, mipMapLevels);
		assertRetVal(textureInfo._finalTexture, false);
	}

	return true;
}

static bool CreateFinalSprites(const ff::Vector<OptimizedSpriteInfo>& spriteInfos, ff::Vector<OptimizedTextureInfo>& textureInfos, ff::ISpriteList* newSprites)
{
	for (const OptimizedSpriteInfo& spriteInfo : spriteInfos)
	{
		assertRetVal(newSprites->Add(
			textureInfos[spriteInfo._destTexture]._finalTexture->AsTextureView(),
			spriteInfo._spriteData._name,
			spriteInfo._destRect.ToType<float>(),
			spriteInfo._spriteData.GetHandle(),
			spriteInfo._spriteData.GetScale(),
			spriteInfo._spriteData._type), false);
	}

	return true;
}

static ff::ComPtr<ff::ISpriteList> CreateOutputSprites(ff::IGraphDevice* device, ff::ISpriteList* suggestedResult)
{
	ff::ComPtr<ff::ISpriteList> newSprites = suggestedResult;
	if (!newSprites)
	{
		assertRetVal(ff::CreateSpriteList(device, &newSprites), nullptr);
	}

	return newSprites;
}

bool ff::OptimizeSprites(ISpriteList* originalSprites, TextureFormat format, size_t mipMapLevels, ISpriteList** outSprites)
{
	assertRetVal(originalSprites && outSprites, false);
	ComPtr<ISpriteList> newSprites = ::CreateOutputSprites(originalSprites->GetDevice(), *outSprites);

	Vector<OptimizedSpriteInfo> spriteInfos = ::CreateSpriteInfos(originalSprites);
	std::sort(spriteInfos.begin(), spriteInfos.end());

	Map<ITexture*, OriginalTextureInfo> originalTextures;
	assertRetVal(::CreateOriginalTextures(spriteInfos, originalTextures), false);

	Vector<OptimizedTextureInfo> textureInfos;
	assertRetVal(::ComputeOptimizedSprites(spriteInfos, textureInfos), false);
	assertRetVal(::CreateOptimizedTextures(format, textureInfos), false);

	// Go back to the original order
	std::sort(spriteInfos.begin(), spriteInfos.end(), [](const OptimizedSpriteInfo& info1, const OptimizedSpriteInfo& info2)
		{
			return info1._spriteIndex < info2._spriteIndex;
		});

	assertRetVal(::CopyOptimizedSprites(spriteInfos, originalTextures, textureInfos), false);
	assertRetVal(::ConvertFinalTextures(originalSprites->GetDevice(), format, mipMapLevels, textureInfos), false);
	assertRetVal(::CreateFinalSprites(spriteInfos, textureInfos, newSprites), false);

	*outSprites = *outSprites ? *outSprites : newSprites.Detach();
	return true;
}

static bool CreateOutlineSprites(
	ff::Vector<OptimizedSpriteInfo>& spriteInfos,
	const ff::Map<ff::ITexture*, OriginalTextureInfo>& originalTextures,
	ff::Vector<ff::ComPtr<ff::ITexture>>& outlineTextures,
	ff::ISpriteList* outlineSpriteList)
{
	assertRetVal(outlineSpriteList, false);

	for (OptimizedSpriteInfo& spriteInfo : spriteInfos)
	{
		const ff::SpriteData& spriteData = spriteInfo._spriteData;
		ff::ITexture* originalTexture = spriteData._textureView->GetTexture();
		auto iter = originalTextures.GetKey(originalTexture);
		assertRetVal(iter, false);

		const OriginalTextureInfo& originalTextureInfo = iter->GetValue();
		const ff::RectInt& srcRect = spriteInfo._sourceRect;

		DirectX::ScratchImage outlineScratch;
		assertHrRetVal(outlineScratch.Initialize2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			srcRect.Width() + 2,
			srcRect.Height() + 2,
			1, 1), false);
		::ZeroMemory(outlineScratch.GetPixels(), outlineScratch.GetPixelsSize());

		const DirectX::Image& srcImage = *originalTextureInfo._rgbScratch->GetImages();
		const DirectX::Image& destImage = *outlineScratch.GetImages();

		for (ff::PointInt xy(srcRect.left, srcRect.top), destXY = ff::PointInt::Zeros(); xy.y < srcRect.bottom; xy.y++, destXY.y++)
		{
			const uint8_t* srcRow = srcImage.pixels + srcImage.rowPitch * xy.y + srcRect.left * 4;

			for (xy.x = srcRect.left, destXY.x = 0; xy.x < srcRect.right; xy.x++, destXY.x++, srcRow += 4)
			{
				if (srcRow[3]) // alpha is set
				{
					const uint8_t* dest = destImage.pixels + destImage.rowPitch * destXY.y + destXY.x * 4;
					((DWORD*)dest)[0] = 0xFFFFFFFF;
					((DWORD*)dest)[1] = 0xFFFFFFFF;
					((DWORD*)dest)[2] = 0xFFFFFFFF;

					dest += destImage.rowPitch;
					((DWORD*)dest)[0] = 0xFFFFFFFF;
					((DWORD*)dest)[1] = 0xFFFFFFFF;
					((DWORD*)dest)[2] = 0xFFFFFFFF;

					dest += destImage.rowPitch;
					((DWORD*)dest)[0] = 0xFFFFFFFF;
					((DWORD*)dest)[1] = 0xFFFFFFFF;
					((DWORD*)dest)[2] = 0xFFFFFFFF;
				}
			}
		}

		ff::ComPtr<ff::ITexture> outlineTexture = originalTexture->GetDevice()->AsGraphDeviceInternal()->CreateTexture(std::move(outlineScratch));
		assertRetVal(outlineTexture, false);
		outlineTextures.Push(outlineTexture);

		assertRetVal(outlineSpriteList->Add(
			outlineTexture->AsTextureView(),
			spriteData._name,
			ff::RectInt(outlineTexture->GetSize()).ToType<float>(),
			spriteData.GetHandle() + ff::PointFloat(1, 1),
			spriteData.GetScale()), false);
	}

	return true;
}

bool ff::CreateOutlineSprites(ISpriteList* originalSprites, TextureFormat format, size_t mipMapLevels, ISpriteList** outSprites)
{
	assertRetVal(originalSprites && outSprites, false);
	ComPtr<ISpriteList> newSprites = ::CreateOutputSprites(originalSprites->GetDevice(), nullptr);
	Vector<OptimizedSpriteInfo> spriteInfos = ::CreateSpriteInfos(originalSprites);

	Map<ITexture*, OriginalTextureInfo> originalTextures;
	assertRetVal(::CreateOriginalTextures(spriteInfos, originalTextures), false);

	Vector<ComPtr<ITexture>> outlineTextures;
	assertRetVal(::CreateOutlineSprites(spriteInfos, originalTextures, outlineTextures, newSprites), false);

	return ff::OptimizeSprites(newSprites, format, mipMapLevels, outSprites);
}
