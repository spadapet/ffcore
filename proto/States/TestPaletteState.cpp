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
#include "Graph/Texture/Texture.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "States/TestPaletteState.h"
#include "Types/Timer.h"

namespace TestPalette
{
	static const ff::RectFloat FULL_WORLD_RECT(0, 0, 1920, 1080);
	static const ff::RectFloat SMALL_WORLD_RECT(0, 0, 1920 / 4, 1080 / 4);

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
}

TestPaletteState::TestPaletteState(ff::AppGlobals* globals)
	: _render(globals->GetGraph()->CreateRenderer())
	, _viewport(TestPalette::FULL_WORLD_RECT.Size())
	, _updateEntityBucket(_domain.GetBucket<TestPalette::UpdateSystemEntry>())
	, _renderEntityBucket(_domain.GetBucket<TestPalette::RenderSystemEntry>())
	, _paletteSpritesResource(L"TestPaletteSprites")
	, _fontResource(L"TestFont2")
{
	ff::IGraphDevice* graph = globals->GetGraph();
	ff::TypedResource<ff::IPaletteData> paletteResource(L"TestPalette");
	_palette = paletteResource->CreatePalette(graph);

	_paletteTextureSmall = graph->CreateTexture(TestPalette::SMALL_WORLD_RECT.Size().ToType<int>(), ff::TextureFormat::R8_UINT);
	_paletteTargetSmall = graph->CreateRenderTargetTexture(_paletteTextureSmall);
	_paletteDepthSmall = graph->CreateRenderDepth(_paletteTextureSmall->GetSize());
	_paletteTextureFull = graph->CreateTexture(TestPalette::FULL_WORLD_RECT.Size().ToType<int>(), ff::TextureFormat::RGBA32);
	_paletteTargetFull = graph->CreateRenderTargetTexture(_paletteTextureFull);
}

std::shared_ptr<ff::State> TestPaletteState::Advance(ff::AppGlobals* globals)
{
	if (globals->GetKeys()->GetKeyPressCount(VK_DELETE))
	{
		_domain.DeleteEntities();
	}

	_domain.Advance();

	// Update Position
	for (const TestPalette::UpdateSystemEntry& entry : _updateEntityBucket->GetEntries())
	{
		ff::PointFloat& pos = entry.positionComponent->position;
		ff::PointFloat& vel = entry.velocityComponent->velocity;

		pos += vel;

		if (pos.x < TestPalette::SMALL_WORLD_RECT.left || pos.x >= TestPalette::SMALL_WORLD_RECT.right)
		{
			vel.x = -vel.x;
		}

		if (pos.y < TestPalette::SMALL_WORLD_RECT.top || pos.y >= TestPalette::SMALL_WORLD_RECT.bottom)
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

void TestPaletteState::Render(ff::AppGlobals* globals, ff::IRenderTarget* target, ff::IRenderDepth* depth)
{
	ff::RendererActive render = _render->BeginRender(_paletteTargetSmall, _paletteDepthSmall, TestPalette::SMALL_WORLD_RECT, TestPalette::SMALL_WORLD_RECT);

	for (const TestPalette::RenderSystemEntry& entry : _renderEntityBucket->GetEntries())
	{
		render->DrawSprite(
			entry.visualComponent->sprite,
			ff::Transform::Create(
				entry.positionComponent->position,
				entry.visualComponent->scale,
				entry.visualComponent->rotate));
	}

	std::array<int, 4> c4{ 224, 224, 255, 255 };
	render->DrawPaletteFilledRectangle(ff::RectFloat(4, 4, 68, 68), c4.data());
	render->DrawPaletteFilledCircle(ff::PointFloat(128, 128), 64, 224, 255);

	ff::ISpriteFont* font = _fontResource.Flush();
	ff::String text = ff::String::format_new(L"Entities:%lu", _renderEntityBucket->GetEntries().Size());
	font->DrawPaletteText(render, text, ff::PointFloat(5, 240), ff::PointFloat(0.5, 0.5), 245, 224);

	render = _render->BeginRender(_paletteTargetFull, nullptr, TestPalette::FULL_WORLD_RECT, TestPalette::SMALL_WORLD_RECT);
	render->PushPalette(_palette);
	render->DrawSprite(_paletteTextureSmall->AsSprite(), ff::Transform::Identity());

	ff::RectFloat view = _viewport.GetView(target);
	render = _render->BeginRender(target, nullptr, view, TestPalette::FULL_WORLD_RECT);
	render->AsRendererActive11()->PushTextureSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	render->DrawSprite(_paletteTextureFull->AsSprite(), ff::Transform::Identity());
}

void TestPaletteState::AddEntities()
{
	ff::ISpriteList* paletteSprites = _paletteSpritesResource.Flush();

	for (int i = 0; i < 5000; i++)
	{
		ff::Entity entity = _domain.CreateEntity();
		TestPalette::PositionComponent* positionComponent = entity->AddComponent<TestPalette::PositionComponent>();
		TestPalette::VelocityComponent* velocityComponent = entity->AddComponent<TestPalette::VelocityComponent>();
		TestPalette::VisualComponent* visualComponent = entity->AddComponent<TestPalette::VisualComponent>();
		size_t sprite = (size_t)std::rand() % paletteSprites->GetCount();

		positionComponent->position = ff::PointFloat((float)(std::rand() % 1920), (float)(std::rand() % 1080)) / 4.0f;
		velocityComponent->velocity = ff::PointFloat((std::rand() % 21 - 10) / 2.0f, (std::rand() % 21 - 10) / 2.0f) / 4.0f;
		visualComponent->scale = ff::PointFloat((std::rand() % 16) / 10.0f + 0.5f, (std::rand() % 16) / 10.0f + 0.5f);
		visualComponent->rotate = (std::rand() % 360) * ff::DEG_TO_RAD_F;
		visualComponent->sprite = paletteSprites->Get(sprite);

		entity->Activate();
	}
}
