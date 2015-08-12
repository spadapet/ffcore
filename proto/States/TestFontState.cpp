#include "pch.h"
#include "Dict/Dict.h"
#include "Globals/AppGlobals.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/Font/SpriteFont.h"
#include "Graph/GraphDevice.h"
#include "Graph/Render/Renderer.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Resource/ResourcePersist.h"
#include "Resource/Resources.h"
#include "States/TestFontState.h"
#include "String/StringUtil.h"
#include "Thread/ThreadPool.h"

TestFontState::TestFontState()
	: _createdSpriteFont(false)
{
	ff::IGraphDevice *graph = ff::AppGlobals::Get()->GetGraph();
	_render = graph->CreateRenderer();

}

TestFontState::~TestFontState()
{
}

std::shared_ptr<ff::State> TestFontState::Advance(ff::AppGlobals *context)
{
	CreateSpriteFont();

	return nullptr;
}

void TestFontState::Render(ff::AppGlobals *context, ff::IRenderTarget *target, ff::IRenderDepth* depth)
{
	ff::ISpriteFont *font = _font.GetObject();
	ff::ISpriteFont *font2 = _font2.GetObject();
	noAssertRet(font && font2);

	ff::PointFloat view = target->GetRotatedSize().ToType<float>();
	ff::PointFloat world = target->GetRotatedSize().ToType<float>() / (float)target->GetDpiScale();
	ff::RendererActive render = _render->BeginRender(target, depth, ff::RectFloat(view), ff::RectFloat(world));

	if (render)
	{
		render->DrawLine(ff::PointFloat(20, 20), ff::PointFloat(800, 20), ff::GetColorRed(), 1, true);
		render->DrawLine(ff::PointFloat(20, 40), ff::PointFloat(800, 40), ff::GetColorRed(), 1, true);
		render->DrawLine(ff::PointFloat(20, 60), ff::PointFloat(800, 60), ff::GetColorRed(), 1, true);
		render->DrawLine(ff::PointFloat(20, 80), ff::PointFloat(800, 80), ff::GetColorRed(), 1, true);

		ff::StaticString str(L"Hello Testing!\nLine 2. 12345... 'AaBbYy'");

		ff::PointFloat size = font->DrawText(
			render,
			str,
			ff::PointFloat(20, 20),
			ff::PointFloat(4, 2),
			ff::GetColorGreen(),
			ff::GetColorMagenta());

		ff::PointFloat size2 = font2->DrawText(
			render,
			str,
			ff::PointFloat(20, 200),
			ff::PointFloat::Ones(),
			ff::GetColorMagenta(),
			ff::GetColorGreen());

		render->DrawOutlineRectangle(ff::RectFloat(20, 20, 20 + size.x, 20 + size.y), ff::GetColorYellow(), 1, true);
		render->DrawOutlineRectangle(ff::RectFloat(20, 200, 20 + size2.x, 200 + size2.y), ff::GetColorYellow(), 1, true);
	}
}

void TestFontState::CreateSpriteFont()
{
	noAssertRet(!_createdSpriteFont);
	_createdSpriteFont = true;

	ff::GetThreadPool()->AddTask([this]()
	{
		ff::String jsonFile = ff::GetExecutableDirectory();
		ff::AppendPathTail(jsonFile, ff::String(L"Assets\\TestAssets.res.json"));

		ff::Vector<ff::String> errors;
		ff::Dict dict = ff::LoadResourcesFromFileCached(jsonFile, false, errors);
		assertRet(dict.Size() && errors.IsEmpty());
		assertRet(ff::CreateResources(nullptr, dict, &_resources));
	},
	[this]()
	{
		_font.Init(_resources, ff::String(L"SansFont"));
		_font2.Init(_resources, ff::String(L"SmallMonoFont"));
	});
}
