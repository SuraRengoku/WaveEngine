using Accessibility;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Dynamic;
using System.Linq;
using System.Net.Http.Headers;
using System.Runtime.Serialization;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using WaveEditor.DLLWrappers;
using WaveEditor.GameProject;
using WaveEditor.Utilities;

namespace WaveEditor.Components {
    [DataContract]
    [KnownType(typeof(Transform))]
    [KnownType(typeof(Script))]
    class GameEntity : ViewModelBase {
        private int _entityId = ID.INVALID_ID;
        public int EntityId {
            get => _entityId;
            set {
                if(_entityId != value) {
                    _entityId = value;
                    OnPropertyChanged(nameof(EntityId));
                }
            }
        }

        private bool _isActive;
        public bool IsActive {
            get => _isActive;
            set {
                if(_isActive != value) {
                    _isActive = value;
                    if(_isActive) {
                        EntityId = EngineAPI.EntityAPI.CreateGameEntity(this);
                        Debug.Assert(ID.IsValid(_entityId));
                    } else if(ID.IsValid(EntityId)) {
                        EngineAPI.EntityAPI.RemoveGameEntity(this);
                        EntityId = ID.INVALID_ID;
                    }
                    
                    OnPropertyChanged(nameof(IsActive));
                }
            }
        }

        private bool _isEnabled = true;
        [DataMember]
        public bool IsEnabled {
            get => _isEnabled;
            set {
                if(_isEnabled != value) {
                    _isEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }

        private string _name;
        [DataMember]
        public string Name {
            get => _name;
            set {
                if (_name != value) {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        [DataMember]
        public Scene ParentScene { get; private set; }

        [DataMember(Name = nameof(Components))]
        private readonly ObservableCollection<Component> _components = new ObservableCollection<Component>();
        public ReadOnlyObservableCollection<Component> Components { get; private set; }

        public Component GetComponent(Type type) => Components.FirstOrDefault(c => c.GetType() == type);
        public T GetComponent<T>() where T : Component => GetComponent(typeof(T)) as T;

        public bool AddComponent(Component component) {
            Debug.Assert(component != null);
            if(!Components.Any(x => x.GetType() == component.GetType())) {
                IsActive = false;
                _components.Add(component);
                IsActive = true;
                return true;
            }
            Logger.Log(MessageType.Warning, $"Entity {Name} already has a {component.GetType().Name} component");
            return false;
        }

        public void RemoveComponent(Component component) {
            Debug.Assert(component != null);
            if (component is Transform) return; // Transform component can't be removed

            if(_components.Contains(component)) {
                IsActive = false; // remove the entire game entity from the Engine
                _components.Remove(component);
                IsActive = true;
            }
        }


        //public ICommand RenameCommand { get; private set; }
        //public ICommand IsEnabledCommand { get; private set; }

        [OnDeserialized]
        void OnDeserialized(StreamingContext context) {
            if(_components != null) {
                Components = new ReadOnlyObservableCollection<Component>(_components);
                OnPropertyChanged(nameof(Components));
            }

            //RenameCommand = new RelayCommand<string>(x => {
            //    var oldName = _name;
            //    Name = x;

            //    Project.UndoRedo.Add(new UndoRedoAction(
            //        nameof(Name), this, oldName, x, $"Rename entity '{oldName}' to '{x}'"));
            //}, x => x != _name);

            //IsEnabledCommand = new RelayCommand<bool>(x => {
            //    var oldValue = _isEnabled;
            //    IsEnabled = x;

            //    Project.UndoRedo.Add(new UndoRedoAction(
            //        nameof(IsEnabled), this, oldValue, x, x ? $"Enable {Name}" : $"Disable {Name}"));
            //});
        }

        public GameEntity(Scene scene) {
            Debug.Assert(scene != null);
            ParentScene = scene;
            _components.Add(new Transform(this));
            OnDeserialized(new StreamingContext());
        }
    }

    abstract class MSEntity : ViewModelBase {
        // enable updates to selected entities
        private bool _enableUpdates = true;

        private bool? _isEnabled;
        public bool? IsEnabled {
            get => _isEnabled;
            set {
                if(_isEnabled != value) {
                    _isEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }

        private string _name;
        public string Name {
            get => _name;
            set {
                if(_name != value) {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        private readonly ObservableCollection<IMSComponent> _components = new ObservableCollection<IMSComponent>();
        public ReadOnlyObservableCollection<IMSComponent> Components { get; }

        public T GetMSComponent<T>() where T : IMSComponent {
            return (T)Components.FirstOrDefault(x => x.GetType() == typeof(T));
        }

        public List<GameEntity> SelectedEntities { get; }
        private void MakeComponentList() {
            _components.Clear();
            var firstEntity = SelectedEntities.FirstOrDefault();
            if (firstEntity == null) return;

            foreach(var component in firstEntity.Components) {
                var type = component.GetType();
                if(!SelectedEntities.Skip(1).Any(entity => entity.GetComponent(type) == null)) {
                    Debug.Assert(Components.FirstOrDefault(x => x.GetType() == type) == null);
                    _components.Add(component.GetMultiselectionComponent(this));
                }
            }
        }

        public static float? GetMixedValue<T>(List<T> objects, Func<T, float> getProperty) {
            var value = getProperty(objects.First());
            return objects.Skip(1).Any(x => !getProperty(x).IsTheSameAs(value)) ? (float?)null : value;
        }

        public static bool? GetMixedValue<T>(List<T> objects, Func<T, bool> getProperty) {
            var value = getProperty(objects.First());
            return objects.Skip(1).Any(x => value != getProperty(x)) ? (bool?)null : value;
        }

        public static string? GetMixedValue<T>(List<T> objects, Func<T, string> getProperty) {
            var value = getProperty(objects.First());
            return objects.Skip(1).Any(x => value != getProperty(x)) ? (string?)null : value;
        }

        protected virtual bool UpdateGameEntities(string propertyName) {
            switch (propertyName) {
                case nameof(IsEnabled): SelectedEntities.ForEach(x => x.IsEnabled = IsEnabled.Value); return true;
                case nameof(Name): SelectedEntities.ForEach(x => x.Name = Name); return true;
            }
            return false;
        }

        protected virtual bool UpdateGameEntity() {
            IsEnabled = GetMixedValue(SelectedEntities, new Func<GameEntity, bool>(x => x.IsEnabled));
            Name = GetMixedValue(SelectedEntities, new Func<GameEntity, string>(x => x.Name));

            return true;
        }

        public void Refresh() {
            _enableUpdates = false;
            UpdateGameEntity();
            MakeComponentList();
            _enableUpdates = true;
        }

        public MSEntity(List<GameEntity> entities) {
            Debug.Assert(entities?.Any() == true);
            Components = new ReadOnlyObservableCollection<IMSComponent>(_components);
            SelectedEntities = entities;
            PropertyChanged += (s, e) => { if(_enableUpdates) UpdateGameEntities(e.PropertyName); };
        }
    }

    class MSGameEntity : MSEntity {
        public MSGameEntity(List<GameEntity> entities) : base(entities) {
            Refresh();
        }
    }
}
