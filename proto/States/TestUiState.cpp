#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Graph/RenderTarget/RenderTargetWindow.h"
#include "States/TestUiState.h"
#include "UI/XamlGlobalState.h"
#include "UI/XamlView.h"
#include "UI/XamlViewState.h"

TestUiState::TestUiState(ff::XamlGlobalState* globals)
	: _globals(globals)
{
	std::shared_ptr<ff::XamlView> view = globals->CreateView(ff::String::from_static(L"Overlay.xaml"));
	_view = std::make_shared<ff::XamlViewState>(view, globals->GetAppGlobals()->GetTarget(), globals->GetAppGlobals()->GetDepth());

	Noesis::FrameworkElement* content = _view->GetView()->GetContent();
	Noesis::Button* button = content->FindName<Noesis::Button>("button");
	Noesis::Storyboard* anim = content->FindResource<Noesis::Storyboard>("RotateAnim");
	if (button && anim)
	{
		button->Click() += [anim](Noesis::BaseComponent*, const Noesis::RoutedEventArgs&)
		{
			if (anim->IsPlaying())
			{
				anim->Stop();
			}
			else
			{
				anim->Begin();
			}
		};
	}
}

std::shared_ptr<ff::State> TestUiState::Advance(ff::AppGlobals* globals)
{
	std::shared_ptr<State> newState = _view->Advance(globals);
	return newState ? newState : _view;
}
