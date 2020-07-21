#pragma once

#include <NsCore/Noesis.h>
#include <NsApp/InteractivityApi.h>
#include <NsApp/TargetedTriggerAction.h>
#include <NsGui/UIElement.h>

namespace ff
{
    class SetFocusAction2 : public NoesisApp::TargetedTriggerActionT<Noesis::UIElement>
    {
    public:
        SetFocusAction2();

    protected:
        Noesis::Ptr<Noesis::Freezable> CreateInstanceCore() const override;
        void Invoke(Noesis::BaseComponent* parameter) override;

        NS_DECLARE_REFLECTION(SetFocusAction2, NoesisApp::TargetedTriggerActionT<Noesis::UIElement>)

    private:
        bool _uniqueInPanel;
    };
}
