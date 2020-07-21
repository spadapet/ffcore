#include "pch.h"
#include "UI/Utility/SetFocusAction2.h"

NS_IMPLEMENT_REFLECTION(ff::SetFocusAction2, "ff.SetFocusAction2")
{
    NsProp("UniqueInPanel", &ff::SetFocusAction2::_uniqueInPanel);
}

ff::SetFocusAction2::SetFocusAction2()
    : _uniqueInPanel(false)
{
}

Noesis::Ptr<Noesis::Freezable> ff::SetFocusAction2::CreateInstanceCore() const
{
    return *new SetFocusAction2();
}

void ff::SetFocusAction2::Invoke(Noesis::BaseComponent*)
{
    Noesis::UIElement* element = GetTarget();

    if (element && _uniqueInPanel)
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
