#include "pch.h"
#include "Globals/MetroGlobals.h"
#include "Graph/Anim/Transform.h"
#include "Graph/Font/SpriteFont.h"
#include "Graph/GraphDevice.h"
#include "Graph/Render/Renderer.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "State/DebugPageState.h"
#include "Input/InputMapping.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "Resource/ResourceValue.h"
#include "Types/Timer.h"

static ff::hash_t EVENT_TOGGLE_NUMBERS = ff::HashStaticString(L"toggleNumbers");
static ff::hash_t EVENT_NEXT_PAGE = ff::HashStaticString(L"nextPage");
static ff::hash_t EVENT_PREV_PAGE = ff::HashStaticString(L"prevPage");
static ff::hash_t EVENT_TOGGLE_0 = ff::HashStaticString(L"toggle0");
static ff::hash_t EVENT_TOGGLE_1 = ff::HashStaticString(L"toggle1");
static ff::hash_t EVENT_TOGGLE_2 = ff::HashStaticString(L"toggle2");
static ff::hash_t EVENT_TOGGLE_3 = ff::HashStaticString(L"toggle3");
static ff::hash_t EVENT_TOGGLE_4 = ff::HashStaticString(L"toggle4");
static ff::hash_t EVENT_TOGGLE_5 = ff::HashStaticString(L"toggle5");
static ff::hash_t EVENT_TOGGLE_6 = ff::HashStaticString(L"toggle6");
static ff::hash_t EVENT_TOGGLE_7 = ff::HashStaticString(L"toggle7");
static ff::hash_t EVENT_TOGGLE_8 = ff::HashStaticString(L"toggle8");
static ff::hash_t EVENT_TOGGLE_9 = ff::HashStaticString(L"toggle9");
static ff::hash_t EVENT_CUSTOM = ff::HashStaticString(L"customDebug");

static ff::StaticString DEBUG_TOGGLE_CHARTS(L"Show FPS graph");
static ff::StaticString DEBUG_PAGE_NAME_0(L"Frame perf");

ff::DebugPageState::DebugPageState(AppGlobals* globals)
	: _globals(globals)
	, _debugPage(0)
	, _enabledStats(false)
	, _enabledCharts(false)
	, _totalSeconds(0)
	, _oldSeconds(0)
	, _totalAdvanceCount(0)
	, _totalRenderCount(0)
	, _advanceCount(0)
	, _fastNumberCounter(0)
	, _apsCounter(0)
	, _rpsCounter(0)
	, _graphCounters{ 0 }
	, _lastAps(0)
	, _lastRps(0)
	, _advanceTimeTotal(0)
	, _advanceTimeAverage(0)
	, _renderTime(0)
	, _flipTime(0)
	, _bankTime(0)
	, _bankPercent(0)
	, _font(L"SmallMonoFont")
	, _input(L"DebugPageInput")
	, _render(globals->GetGraph()->CreateRenderer())
	, _memStats{ 0 }
{
	_inputDevices._keys.Push(globals->GetKeysDebug());
	_globals->AddDebugPage(this);
}

ff::DebugPageState::~DebugPageState()
{
	_globals->RemoveDebugPage(this);
}

std::shared_ptr<ff::State> ff::DebugPageState::Advance(AppGlobals* globals)
{
	_apsCounter++;

	return nullptr;
}

void ff::DebugPageState::Render(AppGlobals* globals, IRenderTarget* target, IRenderDepth* depth)
{
	_rpsCounter++;
}

void ff::DebugPageState::AdvanceDebugInput(AppGlobals* globals)
{
	noAssertRet(_input.HasObject() && _input->Advance(_inputDevices, _globals->GetGlobalTime()._secondsPerAdvance));

	if (_input->HasStartEvent(EVENT_CUSTOM) && !_input->HasStartEvent(EVENT_PREV_PAGE))
	{
		_customDebugEvent.Notify();
	}
	else if (_input->HasStartEvent(EVENT_TOGGLE_NUMBERS) && !_input->HasStartEvent(EVENT_NEXT_PAGE))
	{
		_enabledStats = !_enabledStats;
	}
	else if (_enabledStats)
	{
		if (_input->HasStartEvent(EVENT_PREV_PAGE))
		{
			const Vector<IDebugPages*>& pages = _globals->GetDebugPages();
			if (_debugPage == 0 && GetTotalPageCount() > 0)
			{
				_debugPage = GetTotalPageCount() - 1;
			}
			else if (_debugPage > 0)
			{
				_debugPage--;
			}
		}
		else if (_input->HasStartEvent(EVENT_NEXT_PAGE))
		{
			const Vector<IDebugPages*>& pages = _globals->GetDebugPages();
			if (_debugPage + 1 >= GetTotalPageCount())
			{
				_debugPage = 0;
			}
			else
			{
				_debugPage++;
			}
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_0))
		{
			Toggle(0);
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_1))
		{
			Toggle(1);
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_2))
		{
			Toggle(2);
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_3))
		{
			Toggle(3);
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_4))
		{
			Toggle(4);
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_5))
		{
			Toggle(5);
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_6))
		{
			Toggle(6);
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_7))
		{
			Toggle(7);
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_8))
		{
			Toggle(8);
		}
		else if (_input->HasStartEvent(EVENT_TOGGLE_9))
		{
			Toggle(9);
		}
	}
}

void ff::DebugPageState::OnFrameRendered(AppGlobals* globals, AdvanceType type, IRenderTarget* target, IRenderDepth* depth)
{
	switch (type)
	{
	case AdvanceType::Stopped:
	{
		ff::PointFloat targetSize = target->GetRotatedSize().ToType<float>();
		ff::RectFloat targetRect(targetSize);
		ff::RectFloat scaledTargetRect(targetSize / (float)target->GetDpiScale());

		ff::RendererActive render = _render->BeginRender(target, nullptr, targetRect, scaledTargetRect);
		if (render)
		{
			DirectX::XMFLOAT4 color = ff::GetColorMagenta();
			color.w = 0.375;
			render->DrawOutlineRectangle(scaledTargetRect, color, std::min<size_t>(_fastNumberCounter++, 16) / 2.0f, true);
		}
	}
	break;

	case AdvanceType::SingleStep:
		_fastNumberCounter = 0;
		break;
	}

	if (_enabledStats)
	{
		if (type != AdvanceType::Stopped)
		{
			UpdateStats(globals);
		}

		RenderText(globals, target, depth);

		if (_enabledCharts)
		{
			RenderCharts(globals, target);
		}
	}
}

void ff::DebugPageState::UpdateStats(AppGlobals* globals)
{
	const ff::FrameTime& ft = globals->GetFrameTime();
	const ff::GlobalTime& gt = globals->GetGlobalTime();
	bool updateFastNumbers = !(_fastNumberCounter++ % 8);

	INT64 advanceTimeTotalInt = 0;
	for (size_t i = 0; i < ft._advanceCount && i < ft._advanceTime.size(); i++)
	{
		advanceTimeTotalInt += ft._advanceTime[i];
	}

	_memStats = ff::GetMemoryAllocationStats();
	_totalAdvanceCount = gt._advanceCount;
	_totalRenderCount = gt._renderCount;
	_totalSeconds = gt._clockSeconds;

	if (std::floor(_oldSeconds) != std::floor(_totalSeconds))
	{
		_lastAps = _apsCounter / (_totalSeconds - _oldSeconds);
		_lastRps = _rpsCounter / (_totalSeconds - _oldSeconds);
		_oldSeconds = _totalSeconds;
		_apsCounter = 0;
		_rpsCounter = 0;
	}

	if (updateFastNumbers || !_advanceCount)
	{
		_advanceCount = ft._advanceCount;
		_advanceTimeTotal = advanceTimeTotalInt / ft._freqD;
		_advanceTimeAverage = ft._advanceCount ? _advanceTimeTotal / std::min(ft._advanceCount, ft._advanceTime.size()) : 0.0;
		_renderTime = ft._renderTime / ft._freqD;
		_flipTime = ft._flipTime / ft._freqD;
		_bankTime = gt._bankSeconds;
		_bankPercent = _bankTime / gt._secondsPerAdvance;
		_graphCounters = ft._graphCounters;
	}

	FrameInfo frameInfo;
	frameInfo.advanceTime = (float)(advanceTimeTotalInt / ft._freqD);
	frameInfo.renderTime = (float)(ft._renderTime / ft._freqD);
	frameInfo.totalTime = (float)((advanceTimeTotalInt + ft._renderTime + ft._flipTime) / ft._freqD);
	_frames.Push(frameInfo);

	while (_frames.Size() > MAX_QUEUE_SIZE)
	{
		_frames.DeleteFirst();
	}

	size_t pageIndex, subPageIndex;
	IDebugPages* page = ConvertPageToSubPage(_debugPage, pageIndex, subPageIndex);
	if (page)
	{
		page->DebugUpdateStats(globals, pageIndex, updateFastNumbers);
	}
}

ff::State::Status ff::DebugPageState::GetStatus()
{
	return State::Status::Ignore;
}

size_t ff::DebugPageState::GetDebugPageCount() const
{
	return 1;
}

void ff::DebugPageState::DebugUpdateStats(ff::AppGlobals* globals, size_t page, bool updateFastNumbers)
{
}

ff::String ff::DebugPageState::GetDebugName(size_t page) const
{
	return DEBUG_PAGE_NAME_0.GetString();
}

size_t ff::DebugPageState::GetDebugInfoCount(size_t page) const
{
#ifdef _DEBUG
	return 4;
#else
	return 3;
#endif
}

ff::String ff::DebugPageState::GetDebugInfo(size_t page, size_t index, DirectX::XMFLOAT4& color) const
{
	switch (index)
	{
	case 0:
		color = ff::GetColorMagenta();
		return String::format_new(L"Update:%.2fms*%Iu/%.fHz", _advanceTimeAverage * 1000.0, _advanceCount, _lastAps);

	case 1:
		color = ff::GetColorGreen();
		return String::format_new(L"Render:%.2fms/%.fHz, Clear:T%lu/D%lu, Draw:%lu", _renderTime * 1000.0, _lastRps, _graphCounters._clear, _graphCounters._depthClear, _graphCounters._draw);

	case 2:
		return String::format_new(L"Total:%.2fms\n", (_advanceTimeTotal + _renderTime + _flipTime) * 1000.0);

	case 3:
		color = DirectX::XMFLOAT4(.5, .5, .5, 1);
		return String::format_new(L"Memory:%.3f MB (#%lu)", _memStats.current / 1000000.0, _memStats.count);

	default:
		return ff::GetEmptyString();
	}
}

size_t ff::DebugPageState::GetDebugToggleCount(size_t page) const
{
	return 1;
}

ff::String ff::DebugPageState::GetDebugToggle(size_t page, size_t index, int& value) const
{
	switch (index)
	{
	case 0:
		value = _enabledCharts;
		return DEBUG_TOGGLE_CHARTS.GetString();

	default:
		return ff::GetEmptyString();
	}
}

void ff::DebugPageState::DebugToggle(size_t page, size_t index)
{
	switch (index)
	{
	case 0:
		_enabledCharts = !_enabledCharts;
		break;
	}
}

ff::Event<void>& ff::DebugPageState::CustomDebugEvent()
{
	return _customDebugEvent;
}

void ff::DebugPageState::RenderText(AppGlobals* globals, IRenderTarget* target, IRenderDepth* depth)
{
	ff::ISpriteFont* font = _font.GetObject();
	noAssertRet(font);

	size_t pageIndex, subPageIndex;
	ff::IDebugPages* page = ConvertPageToSubPage(_debugPage, pageIndex, subPageIndex);
	noAssertRet(page);

	ff::PointFloat targetSize = target->GetRotatedSize().ToType<float>();
	ff::RectFloat targetRect(targetSize);

	ff::RendererActive render = _render->BeginRender(target, depth, targetRect, targetRect / (float)target->GetDpiScale());
	if (render)
	{
		render->PushNoOverlap();

		ff::String introText = ff::String::format_new(
			L"<F8> Close debug info\n"
			L"<Ctrl-F8> Page %lu/%lu: %s\n"
			L"Time:%.2fs, FPS:%.1f",
			_debugPage + 1,
			GetTotalPageCount(),
			page ? page->GetDebugName(subPageIndex).c_str() : L"",
			_totalSeconds,
			_lastRps);

		ff::FontColorChange outlineColor{ 0, ff::GetColorBlack() };
		font->DrawText(render, introText, introText.size(), ff::Transform::Create(ff::PointFloat(8, 8)), nullptr, 0, &outlineColor, 1);

		size_t line = 3;
		float spacingY = font->GetLineSpacing();
		float startY = spacingY / 2.0f + 8.0f;

		for (size_t i = 0; i < page->GetDebugInfoCount(subPageIndex); i++, line++)
		{
			DirectX::XMFLOAT4 color = ff::GetColorWhite();
			ff::String str = page->GetDebugInfo(subPageIndex, i, color);

			font->DrawText(render, str, str.size(), ff::Transform::Create(ff::PointFloat(8, startY + spacingY * line), ff::PointFloat::Ones(), 0.0f, color), nullptr, 0, &outlineColor, 1);
		}

		line++;

		for (size_t i = 0; i < page->GetDebugToggleCount(subPageIndex); i++, line++)
		{
			int value = -1;
			ff::String str = page->GetDebugToggle(subPageIndex, i, value);
			const wchar_t* toggleText = (value != -1) ? (!value ? L" OFF:" : L" ON:") : L"";

			font->DrawText(render, ff::String::format_new(L"<Ctrl-%lu>%s %s", i, toggleText, str.c_str()), ff::INVALID_SIZE, ff::Transform::Create(ff::PointFloat(8, startY + spacingY * line)), nullptr, 0, &outlineColor, 1);
		}

		render->PopNoOverlap();
	}
}

void ff::DebugPageState::RenderCharts(AppGlobals* globals, IRenderTarget* target)
{
	const float viewFps = 60;
	const float viewSeconds = MAX_QUEUE_SIZE / viewFps;
	const float scale = 16;
	const float viewFpsInverse = 1 / viewFps;

	ff::PointFloat targetSize = target->GetRotatedSize().ToType<float>();
	ff::RectFloat targetRect(targetSize);
	ff::RectFloat worldRect = ff::RectFloat(-viewSeconds, 1, 0, 0);

	ff::RendererActive render = _render->BeginRender(target, nullptr, targetRect, worldRect);
	if (render)
	{
		ff::PointFloat advancePoints[MAX_QUEUE_SIZE];
		ff::PointFloat renderPoints[MAX_QUEUE_SIZE];
		ff::PointFloat totalPoints[MAX_QUEUE_SIZE];

		ff::PointFloat points[2] =
		{
			ff::PointFloat(0, viewFpsInverse * scale),
			ff::PointFloat(-viewSeconds, viewFpsInverse * scale),
		};

		render->DrawLineStrip(points, 2, DirectX::XMFLOAT4(1, 1, 0, 1), 1, true);

		if (_frames.Size() > 1)
		{
			size_t timeIndex = 0;
			for (auto i = _frames.rbegin(); i != _frames.rend(); i++, timeIndex++)
			{
				float x = (float)(timeIndex * -viewFpsInverse);
				advancePoints[timeIndex].SetPoint(x, i->advanceTime * scale);
				renderPoints[timeIndex].SetPoint(x, (i->renderTime + i->advanceTime) * scale);
				totalPoints[timeIndex].SetPoint(x, i->totalTime * scale);
			}

			render->DrawLineStrip(advancePoints, _frames.Size(), DirectX::XMFLOAT4(1, 0, 1, 1), 1, true);
			render->DrawLineStrip(renderPoints, _frames.Size(), DirectX::XMFLOAT4(0, 1, 0, 1), 1, true);
			render->DrawLineStrip(totalPoints, _frames.Size(), DirectX::XMFLOAT4(1, 1, 1, 1), 1, true);
		}
	}
}

void ff::DebugPageState::Toggle(size_t index)
{
	size_t pageIndex, subPageIndex;
	IDebugPages* page = ConvertPageToSubPage(_debugPage, pageIndex, subPageIndex);
	noAssertRet(page);

	if (index < page->GetDebugToggleCount(subPageIndex))
	{
		page->DebugToggle(subPageIndex, index);
	}
}

size_t ff::DebugPageState::GetTotalPageCount() const
{
	size_t count = 0;

	for (const IDebugPages* page : _globals->GetDebugPages())
	{
		count += page->GetDebugPageCount();
	}

	return count;
}

ff::IDebugPages* ff::DebugPageState::ConvertPageToSubPage(size_t debugPage, size_t& outPage, size_t& outSubPage) const
{
	outPage = 0;
	outSubPage = 0;

	for (IDebugPages* page : _globals->GetDebugPages())
	{
		if (debugPage < page->GetDebugPageCount())
		{
			outSubPage = debugPage;
			return page;
		}

		outPage++;
		debugPage -= page->GetDebugPageCount();
	}

	return nullptr;
}
