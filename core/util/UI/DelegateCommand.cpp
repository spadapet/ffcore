#include "pch.h"
#include "UI/DelegateCommand.h"

ff::DelegateCommand::DelegateCommand(ExecuteFunc&& execute)
    : _execute(std::move(execute))
{
}

ff::DelegateCommand::DelegateCommand(ExecuteFunc&& execute, CanExecuteFunc&& canExecute)
    : _execute(std::move(execute))
    , _canExecute(std::move(canExecute))
{
}

bool ff::DelegateCommand::CanExecute(Noesis::BaseComponent* param) const
{
    return !_canExecute || _canExecute(param);
}

void ff::DelegateCommand::Execute(Noesis::BaseComponent* param) const
{
    _execute(param);
}
