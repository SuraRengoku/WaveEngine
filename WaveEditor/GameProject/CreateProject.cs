using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Web;
using System.Windows.Documents;
using WaveEditor.Utilities;

namespace WaveEditor.GameProject {
    [DataContract]
    public class ProjectTemplate {
        [DataMember]
        public string ProjectType { get; set; } 
        [DataMember]
        public string ProjectFile { get; set; }
        [DataMember]
        public List<string> Folders { get; set; }

        public byte[] Icon { get; set; }
        public byte[] ScreenShot { get; set; }
        public string IconFilePath { get; set; }
        public string ScreenShotFilePath { get; set; }
        public string ProjectFilePath {  get; set; }
        public string TemplatePath { get; set; }
    }
    class CreateProject : ViewModelBase {
        // TODO: get the path from the installation location
        private readonly string _templatePath = @"..\..\WaveEditor\ProjectTemplates\";
        private string _projectName = "NewProject";
        public string ProjectName {
            get { return _projectName; }
            set {
                if (_projectName != value) {
                    _projectName = value;
                    ValidateProjectPath();
                    OnPropertyChanged(nameof(ProjectName));
                }
            }
        }

        private string _projectPath = $@"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\WaveProjects\";
        public string ProjectPath {
            get { return _projectPath; }
            set {
                if (_projectPath != value) {
                    _projectPath = value;
                    ValidateProjectPath();
                    OnPropertyChanged(nameof(ProjectPath));
                }
            }
        }

        private bool _isValid;
        public bool IsValid {
            get => _isValid;
            set { 
                if(_isValid != value) {
                    _isValid = value;
                    OnPropertyChanged(nameof(IsValid));
                }
            }
        }

        private string _errorMsg;
        public string ErrorMsg {
            get => _errorMsg;
            set {
                if (_errorMsg != value) {
                    _errorMsg = value;
                    OnPropertyChanged(nameof(ErrorMsg));
                }
            }
        }

        private readonly ObservableCollection<ProjectTemplate> _projectTempaltes = new ObservableCollection<ProjectTemplate>();
        public ReadOnlyObservableCollection<ProjectTemplate> ProjectTemplates { get; }

        private bool ValidateProjectPath() {
            var path = ProjectPath;
            if (!Path.EndsInDirectorySeparator(path)) {
                path += @"\";
            }

            path += $@"{ProjectName}\";
            var nameRegex = new Regex(@"^[A-Za-z_][A-Za-z0-9_]*$");

            IsValid = false;
            if (string.IsNullOrWhiteSpace(ProjectName.Trim())) {
                ErrorMsg = "Type in a project name.";
            } else if (!nameRegex.IsMatch(ProjectName)) {
                ErrorMsg = "Invalid character(s) used in project name.";
            } else if (string.IsNullOrWhiteSpace(ProjectPath.Trim())) {
                ErrorMsg = "Select a valid project folder.";
            } else if(ProjectPath.IndexOfAny(Path.GetInvalidPathChars()) != -1) {
                ErrorMsg = "Invalid character(s) used in project path.";
            } else if(Directory.Exists(path) && Directory.EnumerateFileSystemEntries(path).Any()) {
                ErrorMsg = "Selected project folder already exists and is not empty";
            } else {
                IsValid = true;
                ErrorMsg = string.Empty;
            }

            return IsValid;
        }

        public string Create(ProjectTemplate template) {
            ValidateProjectPath();
            if(!IsValid) { return string.Empty; }

            if(!Path.EndsInDirectorySeparator(ProjectPath)) {
                ProjectPath += @"\";
            }
            var path = $@"{ProjectPath}{ProjectName}\";

            try {
                if (!Directory.Exists(path)) Directory.CreateDirectory(path);
                foreach(var folder in template.Folders) {
                    Directory.CreateDirectory(Path.GetFullPath(Path.Combine(Path.GetDirectoryName(path), folder)));
                }
                var dirInfo = new DirectoryInfo(path + @".Wave\");
                dirInfo.Attributes |= FileAttributes.Hidden;
                File.Copy(template.IconFilePath, Path.GetFullPath(Path.Combine(dirInfo.FullName, "Icon.png")));
                File.Copy(template.ScreenShotFilePath, Path.GetFullPath(Path.Combine(dirInfo.FullName, "ScreenShot.png")));

                //var project = new Project(ProjectName, path);
                //Serializer.ToFile(project, path + $@"{ProjectName}" + Project.Extension);
                var projectXml = File.ReadAllText(template.ProjectFilePath);
                projectXml = string.Format(projectXml, ProjectName, path);
                var projectPath = Path.GetFullPath(Path.Combine(path, $"{ProjectName}{Project.Extension}"));
                File.WriteAllText(projectPath, projectXml);

                CreateMSVCSolution(template, path);

                return path;

            } catch (Exception ex) {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to create {ProjectName}");
                throw;
            }
        }

        private void CreateMSVCSolution(ProjectTemplate template, string projectPath) {
            Debug.Assert(File.Exists(Path.Combine(template.TemplatePath, "MSVCSolution")));
            Debug.Assert(File.Exists(Path.Combine(template.TemplatePath, "MSVCProject")));

            var engineAPIPath = Path.Combine(MainWindow.WavePath, @"WaveEngine\EngineAPI\");
            Debug.Assert(Directory.Exists(engineAPIPath));

            var _0 = ProjectName;
            var _1 = "{" + Guid.NewGuid().ToString().ToUpper() + "}"; // project guid
            var _2 = "{" + Guid.NewGuid().ToString().ToUpper() + "}"; // Solution guid

            var solution = File.ReadAllText(Path.Combine(template.TemplatePath, "MSVCSolution"));
            solution = string.Format(solution, _0, _1, _2);
            File.WriteAllText(Path.GetFullPath(Path.Combine(projectPath, $"{_0}.sln")), solution);

            _2 = engineAPIPath;
            var _3 = MainWindow.WavePath;

            var project = File.ReadAllText(Path.Combine(template.TemplatePath, "MSVCProject"));
            project = string.Format(project, _0, _1, _2, _3);
            File.WriteAllText(Path.GetFullPath(Path.Combine(projectPath, $@"GameCode\{_0}.vcxproj")), project);
        }

        public CreateProject() {
            ProjectTemplates = new ReadOnlyObservableCollection<ProjectTemplate>(_projectTempaltes); 
            try {
                var templateFiles = Directory.GetFiles(_templatePath, "template.xml", SearchOption.AllDirectories);
                Debug.Assert(templateFiles.Any());
                foreach (var file in templateFiles) {
                    //var template = new ProjectTemplate() {
                    //    ProjectType = "Empty Project",
                    //    ProjectFile = "project.wave",
                    //    Folders = new List<string>() { ".Wave", "Content", "GameCode"}
                    //};

                    //Serializer.ToFile(template, file);

                    var template = Serializer.FromFile<ProjectTemplate>(file);
                    template.TemplatePath = Path.GetDirectoryName(file);
                    template.IconFilePath = Path.GetFullPath(Path.Combine(template.TemplatePath, "Icon.png"));
                    template.Icon = File.ReadAllBytes(template.IconFilePath);
                    template.ScreenShotFilePath = Path.GetFullPath(Path.Combine(template.TemplatePath, "ScreenShot.png"));
                    template.ScreenShot = File.ReadAllBytes(template.ScreenShotFilePath);
                    template.ProjectFilePath = Path.GetFullPath(Path.Combine(template.TemplatePath, template.ProjectFile));

                    _projectTempaltes.Add(template);
                }
                ValidateProjectPath();
            } catch(Exception e) {
                Debug.WriteLine(e.Message);
                Logger.Log(MessageType.Error, $"Failed to read project templates");
                throw;
            }
        }
    }

}
