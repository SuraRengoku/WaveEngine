using Microsoft.VisualStudio.CommandBars;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;

namespace WaveEditor.Dictionaries {
    public partial class ControlTemplates : ResourceDictionary {
        private Popup _currentOpenPopup;

        private void OnTextBox_KeyDown(object sender, System.Windows.Input.KeyEventArgs e) {
            var textBox = sender as TextBox;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if (exp == null)
                return;
            if(e.Key == Key.Enter) {
                if(textBox.Tag is ICommand command && command.CanExecute(textBox.Text)) {
                    command.Execute(textBox.Text);
                } else {
                    exp.UpdateSource();
                }
                Keyboard.ClearFocus();
                e.Handled = true;
            } else if(e.Key == Key.Escape) {
                exp.UpdateTarget();
                Keyboard.ClearFocus();
            }
        }

        private void OnTextBoxRename_KeyDown(object sender, KeyEventArgs e) {
            var textBox = sender as TextBox;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if (exp == null)
                return;
            if(e.Key == Key.Enter) {
                if(textBox.Tag is ICommand command && command.CanExecute(textBox.Text)) {
                    command.Execute(textBox.Text);
                } else {
                    exp.UpdateSource();
                }
                textBox.Visibility = Visibility.Collapsed;
                e.Handled = true;
            } else if(e.Key == Key.Escape) {
                exp.UpdateTarget();
                textBox.Visibility = Visibility.Collapsed;
            }
        }

        private void OnTextBoxRename_LostFocus(object sender, RoutedEventArgs e) {
            var textBox = sender as TextBox;
            if (!textBox.IsVisible) return;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if(exp != null) {
                exp.UpdateTarget();
                //textBox.MoveFocus(new TraversalRequest(FocusNavigationDirection.Previous));
                textBox.Visibility = Visibility.Collapsed;
            }
        }

        private void OnClose_Button_Click(object sender, RoutedEventArgs e) {
            var window = (Window)((FrameworkElement)sender).TemplatedParent;
            window.Close();
        }

        private void OnMaximieRestore_Button_Click(object sender, RoutedEventArgs e) {
            var window = (Window)((FrameworkElement)sender).TemplatedParent;
            window.WindowState = (window.WindowState == WindowState.Normal) ? WindowState.Maximized : WindowState.Normal;
        }

        private void OnMinimize_Button_Click(object sender, RoutedEventArgs e) {
            var window = (Window)((FrameworkElement)sender).TemplatedParent;
            window.WindowState = WindowState.Minimized;
        }

        #region Menu Handlers

        private void OnMenu_Button_Click(object sender, RoutedEventArgs e) {
            var button = sender as Button;
            if (button == null) return;

            ItemsControl menu = button.Name switch {
                "File" => CreateFileMenu(),
                "Edit" => CreateEditMenu(),
                "View" => CreateViewMenu(),
                "Project" => CreateProjectMenu(),
                "Build" => CreateBuildMenu(),
                "Tools" => CreateToolsMenu(),
                "Analyze" => CreateAnalyzeMenu(),
                "Help" => CreateHelpMenu(),
                _ => null
            };

            ShowMenu(button, menu);
        }

        private void ShowMenu(Button button, ItemsControl menu) {
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
                    Background = (System.Windows.Media.Brush)this["Editor.Window.GrayBrush2"],
                    BorderBrush = (System.Windows.Media.Brush)this["Editor.Window.GrayBrush5"],
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

        private ItemsControl CreateFileMenu() {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("New Project...", null, "Ctrl+Shift+N"));
            menu.Items.Add(CreateMenuItem("Open Project...", null, "Ctrl+Shift+O"));
            menu.Items.Add(CreateSeparator());
            menu.Items.Add(CreateMenuItem("Save", OnSave_Click, "Ctrl+S"));
            menu.Items.Add(CreateMenuItem("Save All", OnSaveAll_Click, "Ctrl+Shift+S"));
            menu.Items.Add(CreateSeparator());
            menu.Items.Add(CreateMenuItem("Close Solution", null));
            menu.Items.Add(CreateSeparator());
            menu.Items.Add(CreateMenuItem("Exit", OnExit_Click, "Alt+F4"));

            return menu;
        }
        private ItemsControl CreateEditMenu() {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Undo", null, "Ctrl+Z"));
            menu.Items.Add(CreateMenuItem("Redo", null, "Ctrl+Y"));
            menu.Items.Add(CreateSeparator());
            menu.Items.Add(CreateMenuItem("Cut", null, "Ctrl+X"));
            menu.Items.Add(CreateMenuItem("Copy", null, "Ctrl+C"));
            menu.Items.Add(CreateMenuItem("Paste", null, "Ctrl+V"));
            menu.Items.Add(CreateMenuItem("Delete", null, "Del"));

            return menu;
        }

        private ItemsControl CreateViewMenu() {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Solution Explorer", null, "Ctrl+Alt+L"));
            menu.Items.Add(CreateMenuItem("Properties", null, "F4"));
            menu.Items.Add(CreateMenuItem("Output", null, "Ctrl+Alt+O"));
            menu.Items.Add(CreateMenuItem("Error List", null, "Ctrl+\\, E"));

            return menu;
        }

        private ItemsControl CreateProjectMenu() {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("New Scene", null, "Ctrl+Shift+S"));

            return menu;
        }

        private ItemsControl CreateBuildMenu() {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Build Solution", null, "F7"));
            menu.Items.Add(CreateMenuItem("Rebuild Solution", null, "Ctrl+Shift+B"));
            menu.Items.Add(CreateMenuItem("Clean Solution", null));

            return menu;
        }

        private ItemsControl CreateToolsMenu() {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Options...", null));
            menu.Items.Add(CreateMenuItem("Customize...", null));

            return menu;
        }

        private ItemsControl CreateAnalyzeMenu() {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Run Code Analysis", null));
            menu.Items.Add(CreateMenuItem("Configure Code Analysis", null));

            return menu;
        }

        private ItemsControl CreateHelpMenu() {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("View Help", null, "F1"));
            menu.Items.Add(CreateSeparator());
            menu.Items.Add(CreateMenuItem("About", null));

            return menu;
        }

        private MenuItem CreateMenuItem(string header, RoutedEventHandler clickHandler, string inputGesture = null) {
            var menuItem = new MenuItem {
                Header = header,
                Style = (Style)this["MenuItemStyle"]
            };

            if(!string.IsNullOrEmpty(inputGesture)) {
                menuItem.InputGestureText = inputGesture;
            }

            if(clickHandler != null) {
                menuItem.Click += clickHandler;
                menuItem.Click += (s, e) => { if (_currentOpenPopup != null) _currentOpenPopup.IsOpen = false; };
            }

            return menuItem;
        }

        private Separator CreateSeparator() {
            return new Separator {
                Style = (Style)this["MenuSeparatorStyle"]
            };
        }

        #endregion

        #region Menu Item Handlers

        private void OnSave_Click(object sender, RoutedEventArgs e) {
            // TODO
            var window = Application.Current.MainWindow;
            if (window?.DataContext is GameProject.Project project) {
                //project.Save();
            }
        }

        private void OnSaveAll_Click(object sender, RoutedEventArgs e) {
            // TODO
            OnSave_Click(sender, e);
        }

        private void OnExit_Click(object sender, RoutedEventArgs e) {
            Application.Current.Shutdown();
        }

        #endregion


    }
}
