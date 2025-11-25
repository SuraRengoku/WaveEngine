using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace WaveEditor.GameProject {
    /// <summary>
    /// Interaction logic for ProjectBrowserDialog.xaml
    /// </summary>
    public partial class ProjectBrowserDialog : Window {
        private readonly CubicEase _easing = new CubicEase() { EasingMode = EasingMode.EaseInOut };

        public ProjectBrowserDialog() {
            InitializeComponent();
            Loaded += OnProjectBrowserDialogLoaded;
        }

        private void OnProjectBrowserDialogLoaded(object sender, RoutedEventArgs e) {
            Loaded -= OnProjectBrowserDialogLoaded;
            if(!OpenProject.Projects.Any()) {
                openProjectButton.IsEnabled = false;
                openProjectView.Visibility = Visibility.Hidden;
                OnToggleButton_Click(createProjectButton, new RoutedEventArgs());
            }
        }

        private void AnimateToOpenProject() {
            var highlightAnimation = new DoubleAnimation(400, 200, new Duration(TimeSpan.FromSeconds(0.1)));
            highlightAnimation.EasingFunction = _easing;
            highlightAnimation.Completed += (s, e) => {
                var animation = new ThicknessAnimation(new Thickness(-1600, 0, 0, 0), new Thickness(0), new Duration(TimeSpan.FromSeconds(0.3)));
                animation.EasingFunction = _easing;
                browserContent.BeginAnimation(MarginProperty, animation);
            };
            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnimation);
        }

        private void AnimateToCreateProject() {
            var highlightAnimation = new DoubleAnimation(200, 400, new Duration(TimeSpan.FromSeconds(0.1)));
            highlightAnimation.EasingFunction = _easing;
            highlightAnimation.Completed += (s, e) => {
                var animation = new ThicknessAnimation(new Thickness(0), new Thickness(-1600, 0, 0, 0), new Duration(TimeSpan.FromSeconds(0.3)));
                animation.EasingFunction = _easing;
                browserContent.BeginAnimation(MarginProperty, animation);
            };
            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnimation);
        }

        private void OnToggleButton_Click(object sender, RoutedEventArgs e) {
            if(sender == openProjectButton) {
                if(createProjectButton.IsChecked == true) {
                    createProjectButton.IsChecked = false;
                    AnimateToOpenProject();
                    openProjectView.IsEnabled = true;
                    createProjectView.IsEnabled = false;
                }
                openProjectButton.IsChecked = true;
            } else {
                if(openProjectButton.IsChecked == true) {
                    openProjectButton.IsChecked = false;
                    AnimateToCreateProject();
                    createProjectView.IsEnabled = true;
                    openProjectView.IsEnabled = false;
                }
                createProjectButton.IsChecked = true;
            }
        }
    }
}
