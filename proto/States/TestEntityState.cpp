#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Graph/Anim/Transform.h"
#include "Graph/Font/SpriteFont.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/Render/Renderer.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteList.h"
#include "Graph/Texture/Palette.h"
#include "Graph/Texture/PaletteData.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "States/TestEntityState.h"
#include "Types/Timer.h"

static const ff::RectFloat WORLD_RECT(0, 0, 1920, 1080);

struct PositionComponent : public ff::Component
{
	ff::PointFloat position;
};

struct VelocityComponent : public ff::Component
{
	ff::PointFloat velocity;
};

struct VisualComponent : public ff::Component
{
	ff::ISprite* sprite;
	DirectX::XMFLOAT4 color;
	ff::PointFloat scale;
	float rotate;
};

// Defines what components the "Update System" requires on each entity that it cares about
struct UpdateSystemEntry : public ff::EntityBucketEntry
{
	DECLARE_ENTRY_COMPONENTS()

	PositionComponent* positionComponent;
	VelocityComponent* velocityComponent;
	VisualComponent* visualComponent;
};

BEGIN_ENTRY_COMPONENTS(UpdateSystemEntry)
	HAS_COMPONENT(PositionComponent)
	HAS_COMPONENT(VelocityComponent)
	HAS_COMPONENT(VisualComponent)
END_ENTRY_COMPONENTS(UpdateSystemEntry)

// Defines what components the "Render System" requires on each entity that it cares about
struct RenderSystemEntry : public ff::EntityBucketEntry
{
	DECLARE_ENTRY_COMPONENTS()

	PositionComponent const* positionComponent;
	VisualComponent const* visualComponent;
};

BEGIN_ENTRY_COMPONENTS(RenderSystemEntry)
	HAS_COMPONENT(PositionComponent)
	HAS_COMPONENT(VisualComponent)
END_ENTRY_COMPONENTS(RenderSystemEntry)

TestEntityState::TestEntityState(ff::AppGlobals* globals)
	: _render(globals->GetGraph()->CreateRenderer())
	, _viewport(WORLD_RECT.Size())
	, _updateEntityBucket(_domain.GetBucket<UpdateSystemEntry>())
	, _renderEntityBucket(_domain.GetBucket<RenderSystemEntry>())
	, _colorSpriteResource(L"TestSprites.Player")
	, _paletteSpritesResource(L"TestPaletteSprites")
	, _fontResource(L"TestFont2")
{
	ff::TypedResource<ff::IPaletteData> paletteData(L"TestPalette");
	_palette = paletteData->CreatePalette();
}

std::shared_ptr<ff::State> TestEntityState::Advance(ff::AppGlobals* globals)
{
	if (globals->GetKeys()->GetKeyPressCount(VK_DELETE))
	{
		_domain.DeleteEntities();
	}

	_domain.Advance();

	// Update Position
	for (const UpdateSystemEntry& entry : _updateEntityBucket->GetEntries())
	{
		ff::PointFloat& pos = entry.positionComponent->position;
		ff::PointFloat& vel = entry.velocityComponent->velocity;

		pos += vel;

		if (pos.x < WORLD_RECT.left || pos.x >= WORLD_RECT.right)
		{
			vel.x = -vel.x;
		}

		if (pos.y < WORLD_RECT.top || pos.y >= WORLD_RECT.bottom)
		{
			vel.y = -vel.y;
		}
	}

	if (globals->GetKeys()->GetKeyPressCount(VK_SPACE))
	{
		AddEntities();
	}

	return nullptr;
}

void TestEntityState::Render(ff::AppGlobals* globals, ff::IRenderTarget* target, ff::IRenderDepth* depth)
{
	ff::RectFloat view = _viewport.GetView(target);
	ff::RendererActive render = _render->BeginRender(target, depth, view, WORLD_RECT);
	render->PushPalette(_palette);

	for (const RenderSystemEntry& entry : _renderEntityBucket->GetEntries())
	{
		render->DrawSprite(
			entry.visualComponent->sprite,
			ff::Transform::Create(
				entry.positionComponent->position,
				entry.visualComponent->scale,
				entry.visualComponent->rotate,
				entry.visualComponent->color));
	}

	ff::ISpriteFont* font = _fontResource.Flush();
	ff::String text = ff::String::format_new(L"Entities:%lu", _renderEntityBucket->GetEntries().Size());
	font->DrawText(render, text, ff::Transform::Create(ff::PointFloat(20, 1040)), ff::GetColorBlack());
}

void TestEntityState::AddEntities()
{
	ff::ISprite* colorSprite = _colorSpriteResource.Flush();
	ff::ISpriteList* paletteSprites = _paletteSpritesResource.Flush();

	for (int i = 0; i < 5000; i++)
	{
		ff::Entity entity = _domain.CreateEntity();
		PositionComponent* positionComponent = entity->AddComponent<PositionComponent>();
		VelocityComponent* velocityComponent = entity->AddComponent<VelocityComponent>();
		VisualComponent* visualComponent = entity->AddComponent<VisualComponent>();
		size_t sprite = (size_t)std::rand() % (paletteSprites->GetCount() * 4);
		bool usePalette = sprite < paletteSprites->GetCount();

		positionComponent->position = ff::PointFloat((float)(std::rand() % 1920), (float)(std::rand() % 1080));
		velocityComponent->velocity = ff::PointFloat((std::rand() % 21 - 10) / 2.0f, (std::rand() % 21 - 10) / 2.0f);
		visualComponent->scale = usePalette ? ff::PointFloat(2, 2) : ff::PointFloat((std::rand() % 16) / 10.0f + 0.5f, (std::rand() % 16) / 10.0f + 0.5f);
		visualComponent->color = usePalette ? ff::GetColorWhite() : DirectX::XMFLOAT4((std::rand() % 65) / 64.0f, (std::rand() % 65) / 64.0f, (std::rand() % 65) / 64.0f, 1.0f);
		visualComponent->rotate = (std::rand() % 360) * ff::DEG_TO_RAD_F;
		visualComponent->sprite = usePalette ? paletteSprites->Get(sprite) : colorSprite;

		entity->Activate();
	}
}
