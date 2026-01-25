using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using WaveEditor.Editors;

namespace WaveEditor.Editors {
    public static class MenuBarBehavior {
        public static readonly DependencyProperty IsEnabledProperty =
            DependencyProperty.RegisterAttached(
                "IsEnabled",
                typeof(bool),
                typeof(MenuBarBehavior),
                new PropertyMetadata(false, OnIsEnabledChanged));

        public static bool GetIsEnabled(DependencyObject obj) {
            return (bool)obj.GetValue(IsEnabledProperty);
        }

        public static void SetIsEnabled(DependencyObject obj, bool value) {
            obj.SetValue(IsEnabledProperty, value);
        }

        private static void OnIsEnabledChanged(DependencyObject d, DependencyPropertyChangedEventArgs e) {
            if (d is Button button && (bool)e.NewValue) {
                button.Click -= OnMenuButtonClick;
                button.Click += OnMenuButtonClick;
            }
        }

        private static Popup _currentOpenPopup;

        private static void OnMenuButtonClick(object sender, RoutedEventArgs e) {
            var button = sender as Button;
            if (button == null) return;

            ItemsControl menu = button.Name switch {
                "File" => MenuFactory.CreateFileMenu(CloseCurrentPopup),
                "Edit" => MenuFactory.CreateEditMenu(CloseCurrentPopup),
                "View" => MenuFactory.CreateViewMenu(CloseCurrentPopup),
                "Project" => MenuFactory.CreateProjectMenu(CloseCurrentPopup),
                "Build" => MenuFactory.CreateBuildMenu(CloseCurrentPopup),
                "Tools" => MenuFactory.CreateToolsMenu(CloseCurrentPopup),
                "Analyze" => MenuFactory.CreateAnalyzeMenu(CloseCurrentPopup),
                "Help" => MenuFactory.CreateHelpMenu(CloseCurrentPopup),
                _ => null
            };

            ShowMenu(button, menu);
        }

        private static void ShowMenu(Button button, ItemsControl menu) {
            if (button == null || menu == null) return;

            if (_currentOpenPopup != null && _currentOpenPopup.IsOpen) {
                _currentOpenPopup.IsOpen = false;
            }

            var popup = new Popup {
                PlacementTarget = button,
                Placement = PlacementMode.Bottom,
                StaysOpen = false,
                AllowsTransparency = true,
                Child = new Border {
                    Background = (System.Windows.Media.Brush)Application.Current.Resources["Editor.Window.GrayBrush2"],
                    BorderBrush = (System.Windows.Media.Brush)Application.Current.Resources["Editor.Window.GrayBrush5"],
                    BorderThickness = new Thickness(1),
                    Margin = new Thickness(0, 2, 0, 0),
                    Child = menu,
                    MinWidth = 200
                }
            };

            popup.Closed += (s, args) => _currentOpenPopup = null;
            popup.IsOpen = true;
            _currentOpenPopup = popup;
        }

        private static void CloseCurrentPopup() {
            if (_currentOpenPopup != null) {
                _currentOpenPopup.IsOpen = false;
            }
        }
    }
}