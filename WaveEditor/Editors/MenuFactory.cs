using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using WaveEditor.GameProject;

namespace WaveEditor.Editors {
    internal static class MenuFactory {
        private static Style MenuItemStyle => (Style)Application.Current.Resources["MenuItemStyle"];
        private static Style MenuSeparatorStyle => (Style)Application.Current.Resources["MenuSeparatorStyle"];

        public static ItemsControl CreateFileMenu(Action onMenuClose) {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("New Project...", OnNewProject, "Ctrl+Shift+N", onMenuClose));
            menu.Items.Add(CreateMenuItem("Open Project...", OnOpenProject, "Ctrl+Shift+O", onMenuClose));
            menu.Items.Add(CreateSeparator());
            menu.Items.Add(CreateMenuItem("Save", OnSave, "Ctrl+S", onMenuClose));
            menu.Items.Add(CreateMenuItem("Save All", OnSaveAll, "Ctrl+Shift+S", onMenuClose));
            menu.Items.Add(CreateSeparator());
            menu.Items.Add(CreateMenuItem("Close Solution", null, null, onMenuClose));
            menu.Items.Add(CreateMenuItem("Exit", OnExit, "Alt+F4", onMenuClose));

            return menu;
        }

        public static ItemsControl CreateEditMenu(Action onMenuClose) {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Undo", null, "Ctrl+Z", onMenuClose));
            menu.Items.Add(CreateMenuItem("Redo", null, "Ctrl+Y", onMenuClose));
            menu.Items.Add(CreateSeparator());
            menu.Items.Add(CreateMenuItem("Cut", null, "Ctrl+X", onMenuClose));
            menu.Items.Add(CreateMenuItem("Copy", null, "Ctrl+C", onMenuClose));
            menu.Items.Add(CreateMenuItem("Paste", null, "Ctrl+V", onMenuClose));
            menu.Items.Add(CreateMenuItem("Delete", null, "Del", onMenuClose));

            return menu;
        }

        public static ItemsControl CreateViewMenu(Action onMenuClose) {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Solution Explorer", null, "Ctrl+Alt+L", onMenuClose));
            menu.Items.Add(CreateMenuItem("Properties", null, "F4", onMenuClose));
            menu.Items.Add(CreateMenuItem("Output", null, "Ctrl+Alt+O", onMenuClose));
            menu.Items.Add(CreateMenuItem("Error List", null, "Ctrl+\\, E", onMenuClose));

            return menu;
        }

        public static ItemsControl CreateProjectMenu(Action onMenuClose) {
            var menu = new ItemsControl();
            menu.Items.Add(CreateMenuItem("New Scene", null, "Ctrl+Shift+S", onMenuClose));
            return menu;
        }

        public static ItemsControl CreateBuildMenu(Action onMenuClose) {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Build Solution", null, "F7", onMenuClose));
            menu.Items.Add(CreateMenuItem("Rebuild Solution", null, "Ctrl+Shift+B", onMenuClose));
            menu.Items.Add(CreateMenuItem("Clean Solution", null, null, onMenuClose));

            return menu;
        }

        public static ItemsControl CreateToolsMenu(Action onMenuClose) {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Options...", null, null, onMenuClose));
            menu.Items.Add(CreateMenuItem("Customize...", null, null, onMenuClose));

            return menu;
        }

        public static ItemsControl CreateAnalyzeMenu(Action onMenuClose) {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("Run Code Analysis", null, null, onMenuClose));
            menu.Items.Add(CreateMenuItem("Configure Code Analysis", null, null, onMenuClose));

            return menu;
        }

        public static ItemsControl CreateHelpMenu(Action onMenuClose) {
            var menu = new ItemsControl();

            menu.Items.Add(CreateMenuItem("View Help", null, "F1", onMenuClose));
            menu.Items.Add(CreateSeparator());
            menu.Items.Add(CreateMenuItem("About", null, null, onMenuClose));

            return menu;
        }

        private static MenuItem CreateMenuItem(string header, RoutedEventHandler clickHandler, string inputGesture, Action OnMenuClose) {
            var menuItem = new MenuItem {
                Header = header,
                Style = MenuItemStyle
            };

            if(!string.IsNullOrEmpty(inputGesture)) {
                menuItem.InputGestureText = inputGesture;
            }

            if(clickHandler != null) {
                menuItem.Click += clickHandler;
                menuItem.Click += (s, e) => OnMenuClose?.Invoke();
            }

            return menuItem;
        }

        private static Separator CreateSeparator() {
            return new Separator {
                Style = MenuSeparatorStyle
            };
        }

        #region Menu Item Handlers

        private static void OnNewProject(object sender, RoutedEventArgs e) {
            ProjectBrowserDialog.GotoNewProjectTab = true;
            GameProject.Project.Current?.Unload();
            Application.Current.MainWindow.DataContext = null;
            Application.Current.MainWindow.Close();
        }

        private static void OnOpenProject(object sender, RoutedEventArgs e) {
            GameProject.Project.Current?.Unload();
            Application.Current.MainWindow.DataContext = null;
            Application.Current.MainWindow.Close();
        }

        private static void OnSave(object sender, RoutedEventArgs e) {
            var window = Application.Current.MainWindow;
            if(window?.DataContext is GameProject.Project project) {
                // TODO
                Project.Save(project);
            }
        }

        private static void OnSaveAll(object sender, RoutedEventArgs e) {
            OnSave(sender, e);
        }

        private static void OnExit(object sender, RoutedEventArgs e) {
            Application.Current.MainWindow.Close();
        }

        #endregion
    }
}
