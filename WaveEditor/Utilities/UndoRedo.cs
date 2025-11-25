using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Ink;

namespace WaveEditor.Utilities {
    public interface IUndoRedo {
        string Name { get; }
        void Undo();
        void Redo();
    }

    public class UndoRedoAction : IUndoRedo {
        private Action _undoAction;
        private Action _redoAction;
        public string Name { get; }
        public void Undo() => _undoAction();
        public void Redo() => _redoAction();

        public UndoRedoAction(string name) {
            Name = name;
        }

        public UndoRedoAction(Action undo, Action redo, string name)
            : this(name) {
            Debug.Assert(undo != null && redo != null);
            _undoAction = undo;
            _redoAction = redo;
        }

        public UndoRedoAction(string property, object instance, object undoValue, object redoValue, string name)
               : this(
                    () => instance.GetType().GetProperty(property).SetValue(instance, undoValue),
                    () => instance.GetType().GetProperty(property).SetValue(instance, redoValue),
                    name) { 
        }
    }

    public class UndoRedo {
        private bool _enableAdd = true;
        private readonly ObservableCollection<IUndoRedo> _redolist = new ObservableCollection<IUndoRedo>();
        private readonly ObservableCollection<IUndoRedo> _undolist = new ObservableCollection<IUndoRedo>(); 
        public ReadOnlyObservableCollection<IUndoRedo> RedoList { get; }
        public ReadOnlyObservableCollection<IUndoRedo> UndoList { get; }

        public void Reset() {
            _redolist.Clear();
            _undolist.Clear();
        }

        public void Add(IUndoRedo cmd) {
            if(_enableAdd) {
                _undolist.Add(cmd);
                _redolist.Clear();
            }
        }

        public void Undo() {
            if(_undolist.Any()) {
                var cmd = _undolist.Last();
                _undolist.RemoveAt(_undolist.Count - 1);
                _enableAdd = false;
                cmd.Undo();
                _enableAdd = true;
                _redolist.Insert(0, cmd);
            }
        }
        public void Redo() {
            if (_redolist.Any()) {
                var cmd = _redolist.First();
                _redolist.RemoveAt(0);
                _enableAdd = false;
                cmd.Redo();
                _enableAdd = true;
                _undolist.Add(cmd);
            }
        }

        public UndoRedo() {
            RedoList = new ReadOnlyObservableCollection<IUndoRedo>(_redolist);
            UndoList = new ReadOnlyObservableCollection<IUndoRedo>(_undolist);
        }
    }
}
