#include "pch.h"
#include "UI/Utility/SetPanelChildFocusAction.h"

NS_IMPLEMENT_REFLECTION_(ff::SetPanelChildFocusAction, "ff.SetPanelChildFocusAction")

Noesis::Ptr<Noesis::Freezable> ff::SetPanelChildFocusAction::CreateInstanceCore() const
{
    return *new SetPanelChildFocusAction();
}

void ff::SetPanelChildFocusAction::Invoke(Noesis::BaseComponent*)
{
    Noesis::UIElement* element = GetTarget();

    if (element)
    {
        Noesis::Panel* panel = Noesis::DynamicCast<Noesis::Panel*>(element->GetUIParent());
        if (panel && panel->GetChildren())
        {
            for (int i = 0; i < panel->GetChildren()->Count(); i++)
            {
                Noesis::UIElement* child = panel->GetChildren()->Get((unsigned int)i);
                if (child->GetIsFocused())
                {
                    // No need to change focus
                    element = nullptr;
                    break;
                }
            }
        }
    }

    if (element)
    {
        element->Focus();
    }
}
