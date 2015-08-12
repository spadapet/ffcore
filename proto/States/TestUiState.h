#pragma once

#include "State/State.h"

namespace ff
{
	class XamlGlobalState;
	class XamlViewState;
}

class TestUiState : public ff::State
{
public:
	TestUiState(ff::XamlGlobalState* globals);

	virtual std::shared_ptr<ff::State> Advance(ff::AppGlobals *globals) override;

private:
	ff::XamlGlobalState* _globals;
	std::shared_ptr<ff::XamlViewState> _view;
};
