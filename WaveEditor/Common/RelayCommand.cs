using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace WaveEditor {
    class RelayCommand<T> : ICommand {
        private readonly Action<T> _execute; // void Method(T parameter)
        private readonly Predicate<T> _canExecute; // bool Method(T parameter)
        // private readonly Func<T, TResult> -> TResult Method(T paramenter)
      
        // auto call
        // when should we recheck whether command can be executed
        public event EventHandler CanExecuteChanged {
            add { CommandManager.RequerySuggested += value; }
            remove { CommandManager.RequerySuggested -= value; }
        }

        // auto call
        public bool CanExecute(object parameter) {
            if (parameter == null && typeof(T).IsValueType && Nullable.GetUnderlyingType(typeof(T)) == null)
                return _canExecute?.Invoke(default(T)) ?? true;
            return _canExecute?.Invoke((T)parameter) ?? true;
        }

        // auto call
        public void Execute(object parameter) {
            _execute?.Invoke((T)parameter);
        }

        public RelayCommand(Action<T> execute, Predicate<T> canExecute = null) { 
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

    }
}
