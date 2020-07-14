#pragma once

namespace ff
{
    class UTIL_API NotifyPropertyChangedBase : public Noesis::BaseComponent, public Noesis::INotifyPropertyChanged
    {
    public:
        virtual ~NotifyPropertyChangedBase();
        virtual Noesis::PropertyChangedEventHandler& PropertyChanged() override;

        NS_IMPLEMENT_INTERFACE_FIXUP;

    protected:
        virtual void OnPropertyChanged(const char* name);

    private:
        Noesis::PropertyChangedEventHandler _propertyChanged;

        NS_DECLARE_REFLECTION(NotifyPropertyChangedBase, Noesis::BaseComponent);
    };
}
