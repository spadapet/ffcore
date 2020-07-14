#include "pch.h"
#include "UI/Utility/DelegateCommand.h"

NS_IMPLEMENT_REFLECTION(ff::DelegateCommand)
{
}

ff::DelegateCommand::DelegateCommand(ExecuteFunc&& execute)
    : _execute(std::move(execute))
{
}

ff::DelegateCommand::DelegateCommand(CanExecuteFunc&& canExecute, ExecuteFunc&& execute)
    : _canExecute(std::move(canExecute))
    , _execute(std::move(execute))
{
}

bool ff::DelegateCommand::CanExecute(Noesis::BaseComponent* param) const
{
    return _canExecute.Empty() || _canExecute(param);
}

void ff::DelegateCommand::Execute(Noesis::BaseComponent* param) const
{
    _execute(param);
}
