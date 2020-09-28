#pragma once

#include <NsApp/TargetedTriggerAction.h>

namespace ff
{
    class SetPanelChildFocusAction : public NoesisApp::TargetedTriggerActionT<Noesis::UIElement>
    {
    protected:
        Noesis::Ptr<Noesis::Freezable> CreateInstanceCore() const override;
        void Invoke(Noesis::BaseComponent* parameter) override;

        NS_DECLARE_REFLECTION(SetPanelChildFocusAction, NoesisApp::TargetedTriggerActionT<Noesis::UIElement>)
    };
}
