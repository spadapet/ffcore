#include "pch.h"
#include "UI/NotifyPropertyChangedBase.h"

NS_IMPLEMENT_REFLECTION(ff::NotifyPropertyChangedBase)
{
    NsImpl<Noesis::INotifyPropertyChanged>();
}

ff::NotifyPropertyChangedBase::~NotifyPropertyChangedBase()
{
}

Noesis::PropertyChangedEventHandler& ff::NotifyPropertyChangedBase::PropertyChanged()
{
    return _propertyChanged;
}

void ff::NotifyPropertyChangedBase::OnPropertyChanged(const char* name)
{
    if (_propertyChanged)
    {
        _propertyChanged.Invoke(this, Noesis::PropertyChangedEventArgs(Noesis::Symbol(name)));
    }
}
