using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;

namespace WaveEditor.Dictionaries {
    public class MenuButton : Button {
        private Popup _popup;
        private ItemsControl _itemsHost;

        static MenuButton() {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(MenuButton), new FrameworkPropertyMetadata(typeof(MenuButton)));
        }

        public ObservableCollection<object> MenuItems { get; } = new ObservableCollection<object>();

        public override void OnApplyTemplate() {
            base.OnApplyTemplate();

            _popup = GetTemplateChild("PART_Popup") as Popup;
            _itemsHost = GetTemplateChild("PART_ItemsHost") as ItemsControl;

            if(_itemsHost != null) {
                _itemsHost.ItemsSource = MenuItems;
            }

            Click += OnButtonClick;
        }

        private void OnButtonClick(object sender, RoutedEventArgs e) {
            if(_popup != null) {
                _popup.IsOpen = !_popup.IsOpen;
            }
        }
        
        protected override void OnLostFocus(RoutedEventArgs e) {
            base.OnLostFocus(e);
            if(_popup != null) {
                _popup.IsOpen = false;
            }
        }
    }
}
