#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Graph/Font/SpriteFont.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/Render/Renderer.h"
#include "Graph/Render/RendererActive.h"
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
	, _spriteResource(L"TestSprites.Player")
	, _fontResource(L"TestFont2")
{
}

std::shared_ptr<ff::State> TestEntityState::Advance(ff::AppGlobals* globals)
{
	if (globals->GetKeys()->GetKeyPressCount(VK_DELETE))
	{
		ff::Timer timer;
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

	ff::ISprite* sprite = _spriteResource.GetObject();
	if (sprite)
	{
		for (const RenderSystemEntry& entry : _renderEntityBucket->GetEntries())
		{
			render->DrawSprite(sprite,
				entry.positionComponent->position,
				entry.visualComponent->scale,
				entry.visualComponent->rotate,
				entry.visualComponent->color);
		}
	}

	ff::ISpriteFont* font = _fontResource.GetObject();
	if (font)
	{
		ff::String text = ff::String::format_new(L"Entities:%lu", _renderEntityBucket->GetEntries().Size());
		font->DrawText(render, text, ff::PointFloat(20, 1040), ff::PointFloat::Ones(), ff::GetColorWhite(), ff::GetColorBlack());
	}
}

void TestEntityState::AddEntities()
{
	for (int i = 0; i < 5000; i++)
	{
		ff::Entity entity = _domain.CreateEntity();
		PositionComponent* positionComponent = entity->AddComponent<PositionComponent>();
		VelocityComponent* velocityComponent = entity->AddComponent<VelocityComponent>();
		VisualComponent* visualComponent = entity->AddComponent<VisualComponent>();

		positionComponent->position = ff::PointFloat((float)(std::rand() % 1920), (float)(std::rand() % 1080));
		velocityComponent->velocity = ff::PointFloat((std::rand() % 21 - 10) / 2.0f, (std::rand() % 21 - 10) / 2.0f);
		visualComponent->scale = ff::PointFloat((std::rand() % 16) / 10.0f + 0.5f, (std::rand() % 16) / 10.0f + 0.5f);
		visualComponent->color = DirectX::XMFLOAT4((std::rand() % 65) / 64.0f, (std::rand() % 65) / 64.0f, (std::rand() % 65) / 64.0f, 1.0f);
		visualComponent->rotate = (std::rand() % 360) * ff::DEG_TO_RAD_F;

		entity->Activate();
	}
}
