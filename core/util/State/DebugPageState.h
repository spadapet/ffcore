#pragma once

#include "Input/InputMapping.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "Input/Pointer/PointerDevice.h"
#include "Resource/ResourceValue.h"
#include "State/IDebugPages.h"
#include "State/State.h"

namespace ff
{
	class I2dEffectWithState;
	class I2dRenderer;
	class IGraphDevice;
	class IInputMapping;
	class IRenderer;
	class ISpriteFont;

	class DebugPageState : public ff::State, IDebugPages
	{
	public:
		UTIL_API DebugPageState(AppGlobals* globals);
		UTIL_API virtual ~DebugPageState();

		UTIL_API ff::Event<void>& CustomDebugEvent();

		// State
		virtual std::shared_ptr<State> Advance(AppGlobals* globals) override;
		virtual void Render(AppGlobals* globals, IRenderTarget* target, IRenderDepth* depth) override;
		virtual void AdvanceDebugInput(AppGlobals* globals) override;
		virtual void OnFrameRendered(AppGlobals* globals, AdvanceType type, IRenderTarget* target, IRenderDepth* depth) override;
		virtual Status GetStatus() override;

		// IDebugPages
		virtual size_t GetDebugPageCount() const override;
		virtual void DebugUpdateStats(AppGlobals* globals, size_t page, bool updateFastNumbers) override;
		virtual String GetDebugName(size_t page) const override;
		virtual size_t GetDebugInfoCount(size_t page) const override;
		virtual String GetDebugInfo(size_t page, size_t index, DirectX::XMFLOAT4& color) const override;
		virtual size_t GetDebugToggleCount(size_t page) const override;
		virtual String GetDebugToggle(size_t page, size_t index, int& value) const override;
		virtual void DebugToggle(size_t page, size_t index) override;

	private:
		void UpdateStats(AppGlobals* globals);
		void RenderText(AppGlobals* globals, IRenderTarget* target);
		void RenderCharts(AppGlobals* globals, IRenderTarget* target);
		void Toggle(size_t index);
		size_t GetTotalPageCount() const;
		IDebugPages* ConvertPageToSubPage(size_t debugPage, size_t& outPage, size_t& outSubPage) const;

		static const size_t MAX_QUEUE_SIZE = 60 * 6;

		bool _enabledStats;
		bool _enabledCharts;
		size_t _debugPage;
		ff::AppGlobals* _globals;
		ff::InputDevices _inputDevices;
		ff::Event<void> _customDebugEvent;
		ff::TypedResource<ff::ISpriteFont> _font;
		ff::TypedResource<ff::IInputMapping> _input;
		std::unique_ptr<ff::IRenderer> _render;

		size_t _totalAdvanceCount;
		size_t _totalRenderCount;
		size_t _advanceCount;
		size_t _fastNumberCounter;
		size_t _apsCounter;
		size_t _rpsCounter;
		size_t _drawCount;
		double _lastAps;
		double _lastRps;
		double _totalSeconds;
		double _oldSeconds;
		double _advanceTimeTotal;
		double _advanceTimeAverage;
		double _renderTime;
		double _flipTime;
		double _bankTime;
		double _bankPercent;
		ff::MemoryStats _memStats;

		struct FrameInfo
		{
			float advanceTime;
			float renderTime;
			float totalTime;
		};

		ff::List<FrameInfo> _frames;
	};
}
