using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Data;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using WaveEditor.Utilities;

namespace WaveEditor.GameProject {
    [DataContract]
    public class ProjectData {
        [DataMember]
        public string ProjectName { get; set; }
        [DataMember]
        public string ProjectPath { get; set; }
        [DataMember]
        public DateTime Date { get; set; }
        public string FullPath { get => $"{ProjectPath}{ProjectName}{Project.Extension}"; }
        public byte[] Icon { get; set; }
        public byte[] ScreenShot { get; set; } 
    }

    [DataContract]
    public class ProjectDataList {
        [DataMember]
        public List<ProjectData> Projects { get; set; }
    }

    class OpenProject {
        private static readonly string _applicationDataPath = $@"{Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData)}\WaveEditor\";
        private static readonly string _projectDataDath;
        private static readonly ObservableCollection<ProjectData> _projects = new ObservableCollection<ProjectData>();
        public static ReadOnlyObservableCollection<ProjectData> Projects {
            get;
        }

        private static void ReadProjectData() {
            if (File.Exists(_projectDataDath)) {
                var projects = Serializer.FromFile<ProjectDataList>(_projectDataDath).Projects.OrderByDescending(x => x.Date);
                _projects.Clear();
                foreach (var project in projects) {
                    if(File.Exists(project.FullPath)) {
                        project.Icon = File.ReadAllBytes($@"{project.ProjectPath}\.Wave\Icon.png");
                        project.ScreenShot = File.ReadAllBytes($@"{project.ProjectPath}\.Wave\ScreenShot.png");
                        _projects.Add(project);
                    }
                }
            }
        }

        private static void WriteProjectData() {
            var projects = _projects.OrderBy(x => x.Date).ToList();
            Serializer.ToFile(new ProjectDataList() { Projects = projects }, _projectDataDath);
        }

        public static Project Open(ProjectData data) {
            ReadProjectData();
            var project = _projects.FirstOrDefault(x => x.FullPath == data.FullPath);
            if(project != null) {
                project.Date = DateTime.Now;
            } else {
                project = data;
                project.Date = DateTime.Now;
                _projects.Add(project);
            }

            WriteProjectData();

            return Project.Load(project.FullPath);
        }

        static OpenProject() {
            try {
                if(!Directory.Exists(_applicationDataPath)) Directory.CreateDirectory(_applicationDataPath);
                _projectDataDath = $@"{_applicationDataPath}ProjectData.xml";
                Projects = new ReadOnlyObservableCollection<ProjectData>(_projects);
                ReadProjectData();
            } catch (Exception ex) {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to read project data");
                throw;
            }
        }
    }
}
