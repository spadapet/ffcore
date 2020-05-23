#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/DataPersist.h"
#include "Dict/Dict.h"
#include "Graph/Anim/AnimKeys.h"
#include "Graph/Anim/KeyFrames.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteAnimation.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Value/Values.h"

static ff::StaticString PROP_FPS(L"fps");
static ff::StaticString PROP_FRAME(L"frame");
static ff::StaticString PROP_LAST_FRAME(L"lastFrame");
static ff::StaticString PROP_PARTS(L"parts");
static ff::StaticString PROP_KEYS(L"keys");

static ff::StaticString PROP_SPRITE(L"sprite");
static ff::StaticString PROP_COLOR(L"color");
static ff::StaticString PROP_OFFSET(L"offset");
static ff::StaticString PROP_SCALE(L"scale");
static ff::StaticString PROP_ROTATE(L"rotate");
static ff::StaticString PROP_HIT_BOX(L"hitbox");

class __declspec(uuid("07643649-dd11-42d9-9e56-614844600ca5"))
	SpriteAnimation
	: public ff::ComBase
	, public ff::ISpriteAnimation
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(SpriteAnimation);

	// ISpriteAnimation
	virtual void Clear() override;
	virtual void Render(
		ff::IRendererActive* render,
		ff::AnimTweenType type,
		float frame,
		ff::PointFloat pos,
		ff::PointFloat scale,
		float rotate,
		const DirectX::XMFLOAT4& color) override;

	virtual void SetSprite(float frame, size_t nPart, size_t nSprite, ff::ISprite* pSprite) override;
	virtual void SetColor(float frame, size_t nPart, size_t nSprite, const DirectX::XMFLOAT4& color) override;
	virtual void SetOffset(float frame, size_t nPart, ff::PointFloat offset) override;
	virtual void SetScale(float frame, size_t nPart, ff::PointFloat scale) override;
	virtual void SetRotate(float frame, size_t nPart, float rotate) override;
	virtual void SetHitBox(float frame, size_t nPart, ff::RectFloat hitBox) override;
	virtual ff::RectFloat GetHitBox(float frame, size_t nPart, ff::AnimTweenType type) override;

	virtual void SetLastFrame(float frame) override;
	virtual float GetLastFrame() const override;

	virtual void SetFPS(float fps) override;
	virtual float GetFPS() const override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	struct PartKeys
	{
		PartKeys()
		{
			_scale.SetIdentityValue(ff::VectorKey::IdentityScale()._value);
		}

		std::array<ff::KeyFrames<ff::SpriteKey>, 4> _sprites;
		std::array<ff::KeyFrames<ff::VectorKey>, 4> _colors;
		ff::KeyFrames<ff::VectorKey> _offset;
		ff::KeyFrames<ff::VectorKey> _scale;
		ff::KeyFrames<ff::FloatKey> _rotate;
		ff::KeyFrames<ff::VectorKey> _hitBox;
	};

	void UpdateKeys();
	PartKeys& EnsurePartKeys(size_t nPart, float frame);

	ff::Vector<PartKeys*> _parts;
	float _lastFrame;
	float _fps;
	bool _keysChanged;
};

BEGIN_INTERFACES(SpriteAnimation)
	HAS_INTERFACE(ff::ISpriteAnimation)
	HAS_INTERFACE(ff::IRenderAnimation)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<SpriteAnimation>(L"spriteanim");
	});

bool ff::CreateSpriteAnimation(ISpriteAnimation** ppAnim)
{
	return SUCCEEDED(ComAllocator<SpriteAnimation>::CreateInstance(
		nullptr, GUID_NULL, __uuidof(ISpriteAnimation), (void**)ppAnim));
}

SpriteAnimation::SpriteAnimation()
	: _lastFrame(0)
	, _fps(1)
	, _keysChanged(false)
{
}

SpriteAnimation::~SpriteAnimation()
{
	Clear();
}

void SpriteAnimation::Clear()
{
	for (size_t i = 0; i < _parts.Size(); i++)
	{
		delete _parts[i];
	}

	_parts.Clear();
}

void SpriteAnimation::Render(
	ff::IRendererActive* render,
	ff::AnimTweenType type,
	float frame,
	ff::PointFloat pos,
	ff::PointFloat scale,
	float rotate,
	const DirectX::XMFLOAT4& color)
{
	UpdateKeys();

	for (size_t i = 0; i < _parts.Size(); i++)
	{
		if (!_parts[i])
		{
			continue;
		}

		PartKeys& part = *_parts[i];
		ff::ISprite* pSprites[4] = { nullptr, nullptr, nullptr, nullptr };
		DirectX::XMFLOAT4 realColors[4];
		size_t nSprites = 0;

		// Sneaky way to avoid constructor/destructor
		// (don't want them to show up in profiles anymore, this function is called A LOT)
		ff::ComPtr<ff::ISprite>* pSpritePtrs = (ff::ComPtr<ff::ISprite>*)pSprites;

		for (; nSprites < part._sprites.size(); nSprites++)
		{
			part._sprites[nSprites].Get(frame, type, pSpritePtrs[nSprites]);

			if (!pSprites[nSprites])
			{
				break;
			}
		}

		for (size_t h = 0; h < nSprites; h++)
		{
			part._colors[h].Get(frame, type, realColors[h]);
		}

		if (nSprites)
		{
			DirectX::XMFLOAT4 vectorPos;
			DirectX::XMFLOAT4 vectorScale;
			float realRotate;

			part._offset.Get(frame, type, vectorPos);
			part._scale.Get(frame, type, vectorScale);
			part._rotate.Get(frame, type, realRotate);

			realRotate += rotate;

			DirectX::XMStoreFloat4(&vectorPos,
				DirectX::XMVectorAdd(
					DirectX::XMLoadFloat4(&vectorPos),
					DirectX::XMVectorSet(pos.x, pos.y, 0, 0)));

			DirectX::XMStoreFloat4(&vectorScale,
				DirectX::XMVectorMultiply(
					DirectX::XMLoadFloat4(&vectorScale),
					DirectX::XMVectorSet(scale.x, scale.y, 1, 1)));

			for (size_t h = 0; h < nSprites; h++)
			{
				DirectX::XMStoreFloat4(&realColors[h],
					DirectX::XMColorModulate(
						DirectX::XMLoadFloat4(&realColors[h]),
						DirectX::XMLoadFloat4(&color)));
			}

			ff::PointFloat realPos(vectorPos.x, vectorPos.y);
			ff::PointFloat realScale(vectorScale.x, vectorScale.y);

			for (size_t h = 0; h < nSprites; h++)
			{
				render->DrawSprite(pSprites[h], realPos, realScale, realRotate, realColors[h]);
				pSprites[h]->Release();
			}
		}
	}
}

void SpriteAnimation::SetSprite(float frame, size_t nPart, size_t nSprite, ff::ISprite* pSprite)
{
	assertRet(nSprite >= 0 && nSprite < 4);

	PartKeys& keys = EnsurePartKeys(nPart, frame);

	keys._sprites[nSprite].Set(frame, pSprite);
}

void SpriteAnimation::SetColor(float frame, size_t nPart, size_t nSprite, const DirectX::XMFLOAT4& color)
{
	assertRet(nSprite >= 0 && nSprite < 4);

	PartKeys& keys = EnsurePartKeys(nPart, frame);

	keys._colors[nSprite].Set(frame, color);
}

void SpriteAnimation::SetOffset(float frame, size_t nPart, ff::PointFloat offset)
{
	PartKeys& keys = EnsurePartKeys(nPart, frame);

	keys._offset.Set(frame, DirectX::XMFLOAT4(offset.x, offset.y, 0, 0));
}

void SpriteAnimation::SetScale(float frame, size_t nPart, ff::PointFloat scale)
{
	PartKeys& keys = EnsurePartKeys(nPart, frame);

	keys._scale.Set(frame, DirectX::XMFLOAT4(scale.x, scale.y, 0, 0));
}

void SpriteAnimation::SetRotate(float frame, size_t nPart, float rotate)
{
	PartKeys& keys = EnsurePartKeys(nPart, frame);

	keys._rotate.Set(frame, rotate);
}

void SpriteAnimation::SetHitBox(float frame, size_t nPart, ff::RectFloat hitBox)
{
	PartKeys& keys = EnsurePartKeys(nPart, frame);

	keys._hitBox.Set(frame, *(const DirectX::XMFLOAT4*) & hitBox);
}

ff::RectFloat SpriteAnimation::GetHitBox(float frame, size_t nPart, ff::AnimTweenType type)
{
	UpdateKeys();

	ff::RectFloat hitBox(0, 0, 0, 0);
	assertRetVal(nPart >= 0 && nPart < _parts.Size(), hitBox);

	_parts[nPart]->_hitBox.Get(frame, type, *(DirectX::XMFLOAT4*) & hitBox);

	return hitBox;
}

void SpriteAnimation::SetLastFrame(float frame)
{
	_lastFrame = std::max<float>(0, frame);
	_keysChanged = true;
}

float SpriteAnimation::GetLastFrame() const
{
	return _lastFrame;
}

void SpriteAnimation::SetFPS(float fps)
{
	_fps = std::abs(fps);
}

float SpriteAnimation::GetFPS() const
{
	return _fps;
}

void SpriteAnimation::UpdateKeys()
{
	if (_keysChanged)
	{
		_keysChanged = false;

		for (size_t i = 0; i < _parts.Size(); i++)
		{
			if (_parts[i])
			{
				PartKeys& part = *_parts[i];

				part._sprites[0].SetLastFrame(_lastFrame);
				part._sprites[1].SetLastFrame(_lastFrame);
				part._sprites[2].SetLastFrame(_lastFrame);
				part._sprites[3].SetLastFrame(_lastFrame);

				part._colors[0].SetLastFrame(_lastFrame);
				part._colors[1].SetLastFrame(_lastFrame);
				part._colors[2].SetLastFrame(_lastFrame);
				part._colors[3].SetLastFrame(_lastFrame);

				part._offset.SetLastFrame(_lastFrame);
				part._scale.SetLastFrame(_lastFrame);
				part._rotate.SetLastFrame(_lastFrame);
				part._hitBox.SetLastFrame(_lastFrame);
			}
		}
	}
}

SpriteAnimation::PartKeys& SpriteAnimation::EnsurePartKeys(size_t nPart, float frame)
{
	while (nPart >= _parts.Size())
	{
		_parts.Push(nullptr);
	}

	if (!_parts[nPart])
	{
		_parts[nPart] = new PartKeys();

		_parts[nPart]->_scale.SetIdentityValue(ff::VectorKey::IdentityScale()._value);

		_parts[nPart]->_colors[0].SetIdentityValue(ff::VectorKey::IdentityWhite()._value);
		_parts[nPart]->_colors[1].SetIdentityValue(ff::VectorKey::IdentityWhite()._value);
		_parts[nPart]->_colors[2].SetIdentityValue(ff::VectorKey::IdentityWhite()._value);
		_parts[nPart]->_colors[3].SetIdentityValue(ff::VectorKey::IdentityWhite()._value);
	}

	_lastFrame = std::max(_lastFrame, frame);
	_keysChanged = true;

	return *_parts[nPart];
}

bool SpriteAnimation::LoadFromSource(const ff::Dict& dict)
{
	return LoadFromCache(dict);
}

bool SpriteAnimation::LoadFromCache(const ff::Dict& dict)
{
	ff::ValuePtr partsValue = dict.GetValue(PROP_PARTS);
	if (partsValue && partsValue->IsType<ff::ValueVectorValue>())
	{
		for (size_t i = 0; i < partsValue->GetValue<ff::ValueVectorValue>().Size(); i++)
		{
			ff::ValuePtr partValue = partsValue->GetValue<ff::ValueVectorValue>().GetAt(i);

			ff::ValuePtr partDictValue = partValue->Convert<ff::DictValue>();
			if (!partDictValue)
			{
				continue;
			}

			const ff::Dict& partDict = partDictValue->GetValue<ff::DictValue>();
			ff::ValuePtr keysValue = partDict.GetValue(PROP_KEYS);
			if (!keysValue || !keysValue->IsType<ff::ValueVectorValue>())
			{
				continue;
			}

			for (size_t h = 0; h < keysValue->GetValue<ff::ValueVectorValue>().Size(); h++)
			{
				ff::ValuePtr keyValue = keysValue->GetValue<ff::ValueVectorValue>().GetAt(h);

				ff::ValuePtr keyDictValue = keyValue->Convert<ff::DictValue>();
				if (!keyDictValue)
				{
					continue;
				}

				const ff::Dict& keyDict = keyDictValue->GetValue<ff::DictValue>();
				float frame = keyDict.Get<ff::FloatValue>(PROP_FRAME);
				ff::ValuePtr spriteValue = keyDict.GetValue(PROP_SPRITE);
				ff::ValuePtr colorValue = keyDict.GetValue(PROP_COLOR);
				ff::ValuePtr offsetValue = keyDict.GetValue(PROP_OFFSET);
				ff::ValuePtr scaleValue = keyDict.GetValue(PROP_SCALE);
				ff::ValuePtr rotateValue = keyDict.GetValue(PROP_ROTATE);
				ff::ValuePtr hitboxValue = keyDict.GetValue(PROP_HIT_BOX);

				if (spriteValue && spriteValue->IsType<ff::ValueVectorValue>())
				{
					for (size_t j = 0; j < 4 && j < spriteValue->GetValue<ff::ValueVectorValue>().Size(); j++)
					{
						ff::ValuePtr resVal = spriteValue->GetValue<ff::ValueVectorValue>().GetAt(j)->Convert<ff::SharedResourceWrapperValue>();
						ff::ComPtr<ff::ISprite> sprite;
						if (resVal && ff::CreateSpriteResource(resVal->GetValue<ff::SharedResourceWrapperValue>(), &sprite))
						{
							SetSprite(frame, i, j, sprite);
						}
					}
				}

				if (colorValue && colorValue->IsType<ff::ValueVectorValue>())
				{
					for (size_t j = 0; j < 4 && j < colorValue->GetValue<ff::ValueVectorValue>().Size(); j++)
					{
						ff::ValuePtr rectVal = colorValue->GetValue<ff::ValueVectorValue>().GetAt(j)->Convert<ff::RectFloatValue>();
						if (rectVal)
						{
							SetColor(frame, i, j, *(const DirectX::XMFLOAT4*) & rectVal->GetValue<ff::RectFloatValue>());
						}
					}
				}

				ff::ValuePtr offsetValue2 = offsetValue->Convert<ff::PointFloatValue>();
				if (offsetValue2)
				{
					SetOffset(frame, i, offsetValue2->GetValue<ff::PointFloatValue>());
				}

				ff::ValuePtr scaleValue2 = scaleValue->Convert<ff::PointFloatValue>();
				if (scaleValue2)
				{
					SetScale(frame, i, scaleValue2->GetValue<ff::PointFloatValue>());
				}

				ff::ValuePtr rotateValue2 = rotateValue->Convert<ff::FloatValue>();
				if (rotateValue2)
				{
					SetRotate(frame, i, rotateValue2->GetValue<ff::FloatValue>() * ff::DEG_TO_RAD_F);
				}

				ff::ValuePtr hitboxValue2 = hitboxValue->Convert<ff::RectFloatValue>();
				if (hitboxValue2)
				{
					SetHitBox(frame, i, hitboxValue2->GetValue<ff::RectFloatValue>());
				}
			}
		}
	}

	_lastFrame = dict.Get<ff::FloatValue>(PROP_LAST_FRAME);
	_fps = dict.Get<ff::FloatValue>(PROP_FPS, 1.0f);
	_keysChanged = true;

	return true;
}

bool SpriteAnimation::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::FloatValue>(PROP_LAST_FRAME, _lastFrame);
	dict.Set<ff::FloatValue>(PROP_FPS, _fps);

	ff::Vector<ff::ValuePtr> parts;
	for (const PartKeys* part : _parts)
	{
		if (!part)
		{
			parts.Push(ff::Value::New<ff::NullValue>());
			continue;
		}

		enum class KeyType
		{
			Sprite,
			Color,
			Point,
			Vector,
			Radians,
		};

		struct KeyInfo
		{
			KeyType type;
			ff::String name;
			const ff::KeyFramesBase& keys;
		};

		std::array<KeyInfo, 6> infos =
		{
			KeyInfo{ KeyType::Sprite, PROP_SPRITE, part->_sprites[0] },
			KeyInfo{ KeyType::Color, PROP_COLOR, part->_colors[0] },
			KeyInfo{ KeyType::Point, PROP_OFFSET, part->_offset },
			KeyInfo{ KeyType::Point, PROP_SCALE, part->_scale },
			KeyInfo{ KeyType::Radians, PROP_ROTATE, part->_rotate },
			KeyInfo{ KeyType::Vector, PROP_HIT_BOX, part->_hitBox },
		};

		ff::Set<float> keyFrames;
		for (const KeyInfo& info : infos)
		{
			for (size_t i = 0; i < info.keys.GetKeyCount(); i++)
			{
				keyFrames.SetKey(info.keys.GetKeyFrame(i));
			}
		}

		ff::Vector<ff::ValuePtr> keys;
		for (float frame : keyFrames)
		{
			ff::Dict keyDict;
			keyDict.Set<ff::FloatValue>(PROP_FRAME, frame);

			for (const KeyInfo& info : infos)
			{
				size_t keyIndex = info.keys.FindKey(frame);
				if (keyIndex == ff::INVALID_SIZE)
				{
					// ignore
				}
				else if (info.type == KeyType::Sprite)
				{
					const ff::KeyFrames<ff::SpriteKey>* keys = (const ff::KeyFrames<ff::SpriteKey>*) & info.keys;
					ff::Vector<ff::ValuePtr> spriteVec;

					for (size_t i = 0; i < 4; i++)
					{
						keyIndex = keys[i].FindKey(frame);
						if (keyIndex != ff::INVALID_SIZE)
						{
							const ff::SpriteKey& spriteKey = keys[i].GetKey(keyIndex);

							ff::ComPtr<ff::ISpriteResource> res;
							assertRetVal(res.QueryFrom(spriteKey._value), false);

							spriteVec.Push(ff::Value::New<ff::SharedResourceWrapperValue>(res->GetSourceResource()));
						}
					}

					keyDict.SetValue(info.name, ff::Value::New<ff::ValueVectorValue>(std::move(spriteVec)));
				}
				else if (info.type == KeyType::Color)
				{
					const ff::KeyFrames<ff::VectorKey>* keys = (const ff::KeyFrames<ff::VectorKey>*) & info.keys;
					ff::Vector<ff::ValuePtr> colorVec;

					for (size_t i = 0; i < 4; i++)
					{
						keyIndex = keys[i].FindKey(frame);
						if (keyIndex != ff::INVALID_SIZE)
						{
							const ff::VectorKey& vecKey = keys[i].GetKey(keyIndex);

							ff::Vector<float> vec;
							vec.Push(vecKey._value.x);
							vec.Push(vecKey._value.y);
							vec.Push(vecKey._value.z);
							vec.Push(vecKey._value.w);

							colorVec.Push(ff::Value::New<ff::FloatVectorValue>(std::move(vec)));
						}
					}

					keyDict.SetValue(info.name, ff::Value::New<ff::ValueVectorValue>(std::move(colorVec)));
				}
				else if (info.type == KeyType::Point)
				{
					const ff::KeyFrames<ff::VectorKey>& keys = (const ff::KeyFrames<ff::VectorKey>&)info.keys;
					const ff::VectorKey& vecKey = keys.GetKey(keyIndex);
					keyDict.Set<ff::PointFloatValue>(info.name, ff::PointFloat(vecKey._value.x, vecKey._value.y));
				}
				else if (info.type == KeyType::Vector)
				{
					const ff::KeyFrames<ff::VectorKey>& keys = (const ff::KeyFrames<ff::VectorKey>&)info.keys;
					const ff::VectorKey& vecKey = keys.GetKey(keyIndex);
					keyDict.Set<ff::RectFloatValue>(info.name, ff::RectFloat(vecKey._value.x, vecKey._value.y, vecKey._value.z, vecKey._value.w));
				}
				else if (info.type == KeyType::Radians)
				{
					const ff::KeyFrames<ff::FloatKey>& keys = (const ff::KeyFrames<ff::FloatKey>&)info.keys;
					const ff::FloatKey& floatKey = keys.GetKey(keyIndex);
					keyDict.Set<ff::FloatValue>(info.name, floatKey._value * ff::RAD_TO_DEG_F);
				}
			}

			keys.Push(ff::Value::New<ff::DictValue>(std::move(keyDict)));
		}

		ff::Dict partDict;

		partDict.SetValue(PROP_KEYS, ff::Value::New<ff::ValueVectorValue>(std::move(keys)));
		parts.Push(ff::Value::New<ff::DictValue>(std::move(partDict)));
	}

	dict.SetValue(PROP_PARTS, ff::Value::New<ff::ValueVectorValue>(std::move(parts)));

	return true;
}
