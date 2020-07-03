#pragma once

#include "State/State.h"
#include "Types/Event.h"

namespace ff
{
	class XamlView;

	// Renders and resizes a view automatically as part of the app state pipeline
	class XamlViewState : public ff::State
	{
	public:
		UTIL_API XamlViewState(std::shared_ptr<XamlView> view, ff::IRenderTarget* target, ff::IRenderDepth* depth);
		UTIL_API virtual ~XamlViewState() override;

		UTIL_API XamlView* GetView() const;

		virtual std::shared_ptr<ff::State> Advance(ff::AppGlobals* globals) override;
		virtual void OnFrameRendering(ff::AppGlobals* globals, ff::AdvanceType type) override;
		virtual void Render(ff::AppGlobals* globals, ff::IRenderTarget* target, ff::IRenderDepth* depth) override;

	private:
		std::shared_ptr<XamlView> _view;
		ff::ComPtr<ff::IRenderTarget> _target;
		ff::ComPtr<ff::IRenderDepth> _depth;
		ff::EventCookie _sizeChangedCookie;
	};
}
