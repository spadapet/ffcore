#pragma once

namespace ff
{
    class DelegateCommand : public Noesis::BaseCommand
    {
    public:
        typedef std::function<void(Noesis::BaseComponent*)> ExecuteFunc;
        typedef std::function<bool(Noesis::BaseComponent*)> CanExecuteFunc;

        UTIL_API DelegateCommand(ExecuteFunc&& execute);
        UTIL_API DelegateCommand(ExecuteFunc&& execute, CanExecuteFunc&& canExecute);

        bool CanExecute(Noesis::BaseComponent* param) const override;
        void Execute(Noesis::BaseComponent* param) const override;

    private:
        ExecuteFunc _execute;
        CanExecuteFunc _canExecute;
    };
}
