using System.ComponentModel;
using System.IO;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using WaveEditor.GameProject;

namespace WaveEditor {
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window {
        public static string WavePath { get; private set; }
        public MainWindow() {
            InitializeComponent();
            Loaded += OnMainWindowLoaded;
            Closing += OnMainWindowClosing;
        }


        private void OnMainWindowLoaded(object sender, RoutedEventArgs e) {
            Loaded -= OnMainWindowLoaded;
            GetEnginePath();
            OpenProjectBrowserDialog();
        }

        private void GetEnginePath() {
            var wavePath = Environment.GetEnvironmentVariable("WAVE_ENGINE", EnvironmentVariableTarget.User);
            if (wavePath == null || !Directory.Exists(Path.Combine(wavePath, @"WaveEngine\EngineAPI"))) {
                var dlg = new EnginePathDialog();
                if(dlg.ShowDialog() == true) {
                    WavePath = dlg.WavePath;
                    Environment.SetEnvironmentVariable("WAVE_ENGINE", WavePath.ToUpper(), EnvironmentVariableTarget.User);
                } else {
                    Application.Current.Shutdown();
                }
            } else {
                WavePath = wavePath;
            }
        }

        private void OnMainWindowClosing(object sender, CancelEventArgs e) {
            Closing -= OnMainWindowClosing;
            Project.Current?.Unload();
        }

        private void OpenProjectBrowserDialog() {
            var projectBrowser = new ProjectBrowserDialog();
            if(projectBrowser.ShowDialog() == false || projectBrowser.DataContext == null) {
                Application.Current.Shutdown();
            } else {
                Project.Current?.Unload();
                DataContext = projectBrowser.DataContext;
            }
        }
    }
}