#pragma once

namespace ff
{
    class DelegateCommand : public Noesis::BaseCommand
    {
    public:
        typedef Noesis::Delegate<void(BaseComponent*)> ExecuteFunc;
        typedef Noesis::Delegate<bool(BaseComponent*)> CanExecuteFunc;

        UTIL_API DelegateCommand();
        UTIL_API DelegateCommand(ExecuteFunc&& execute);
        UTIL_API DelegateCommand(CanExecuteFunc&& canExecute, ExecuteFunc&& execute);

        bool CanExecute(Noesis::BaseComponent* param) const override;
        void Execute(Noesis::BaseComponent* param) const override;

    private:
        CanExecuteFunc _canExecute;
        ExecuteFunc _execute;

        NS_DECLARE_REFLECTION(DelegateCommand, BaseCommand)
    };
}
