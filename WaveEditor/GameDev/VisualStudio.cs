using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WaveEditor.Utilities;
using System.Runtime.InteropServices.ComTypes;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.IO;
using WaveEditor.GameProject;
using System.Diagnostics.Eventing.Reader;
using System.Security.Cryptography;
using Microsoft.Windows.Themes;

namespace WaveEditor.GameDev {
    static class VisualStudio {
        // ref:https://learn.microsoft.com/en-us/dotnet/api/envdte80?view=visualstudiosdk-2022
        // ref:https://learn.microsoft.com/en-us/dotnet/api/envdte?view=visualstudiosdk-2022
        private static EnvDTE80.DTE2 _vsInstance = null;
        private static readonly string _progID = "VisualStudio.DTE.17.0"; // VS2022

        public static bool BuildSucceeded { get; private set; } = true;
        public static bool BuildDone { get; private set; } = true;

        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(uint reserved, out IBindCtx ppbc);

        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);
        public static void OpenVisualStudio(string solutionPath) {
            // ref:https://learn.microsoft.com/en-us/windows/win32/api/objidl/nn-objidl-irunningobjecttable
            IRunningObjectTable rot = null;
            IEnumMoniker monikerTable = null;
            IBindCtx bindCtx = null;
            try {
                if (_vsInstance == null) {
                    // Find and open VS 
                    var hResult = GetRunningObjectTable(0, out rot);
                    if (hResult < 0 || rot == null) throw new COMException($"GetRunningObjectTable() returned HERSULT: {hResult: X8}");

                    // VS is already running
                    rot.EnumRunning(out monikerTable);
                    monikerTable.Reset();

                    hResult = CreateBindCtx(0, out bindCtx);
                    if (hResult < 0 || bindCtx == null) throw new COMException($"CreateBindCtx() returned HRESULT: {hResult: X8}");

                    IMoniker[] currentMoniker = new IMoniker[1];
                    while(monikerTable.Next(1, currentMoniker, IntPtr.Zero) == 0) {
                        string name = string.Empty;
                        currentMoniker[0]?.GetDisplayName(bindCtx, null, out name);
                        if (name.Contains(_progID)) {
                            hResult = rot.GetObject(currentMoniker[0], out object obj);
                            if (hResult < 0 || obj == null) throw new COMException($"Running object table's GetObject() returned HRESULT: {hResult: X8}");

                            EnvDTE80.DTE2 dte = obj as EnvDTE80.DTE2;
                            var solutionName = dte.Solution.FullName;
                            if(solutionName == solutionPath) {
                                _vsInstance = dte;
                                break;
                            }
                        }
                    }


                    // after all effort, the VS is sill not opened
                    if (_vsInstance == null) {
                        Type visualStudioType = Type.GetTypeFromProgID(_progID, true);
                        _vsInstance = Activator.CreateInstance(visualStudioType) as EnvDTE80.DTE2;
                    }
                }
            } catch(Exception ex) {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, "Failed to open Visual Studio");
            }
            finally {
                if (monikerTable != null) Marshal.ReleaseComObject(monikerTable);
                if (rot != null) Marshal.ReleaseComObject(rot);
                if (bindCtx != null) Marshal.ReleaseComObject(bindCtx);
            }
        }

        public static void CloseVisualStudio() {
            if(_vsInstance?.Solution.IsOpen == true) {
                _vsInstance.ExecuteCommand("File.SaveAll");
                _vsInstance.Solution.Close(true);
            }
            _vsInstance?.Quit();
        }

        internal static bool AddFilesToSolution(string solution, string projectName, string[] files) {
            Debug.Assert(files.Length > 0);
            OpenVisualStudio(solution);
            try {
                if(_vsInstance != null) {
                    if (!_vsInstance.Solution.IsOpen) _vsInstance.Solution.Open(solution);
                    else _vsInstance.ExecuteCommand("File.SaveAll");

                    foreach(EnvDTE.Project project in _vsInstance.Solution.Projects) {
                        if(project.UniqueName.Contains(projectName)) {
                            foreach(var file in files) {
                                project.ProjectItems.AddFromFile(file);
                            }
                        }
                    }

                    var cpp = files.FirstOrDefault(x => Path.GetExtension(x) == ".cpp"); // find cpp file
                    if(!string.IsNullOrEmpty(cpp)) {
                        _vsInstance.ItemOperations.OpenFile(cpp, EnvDTE.Constants.vsViewKindTextView).Visible = true; // open cpp file in the Editor
                    }
                    _vsInstance.MainWindow.Activate();
                    _vsInstance.MainWindow.Visible = true;
                }
            } catch(Exception ex) {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine("failed to add files to Visual Studio project.");
                return false;
            }
            return true;
        }

        private static void OnBuildSolutionDone(string project, string projectConfig, string platform, string solutionConfig, bool success) {
            if (BuildDone) return;

            if (success) {
                Logger.Log(MessageType.Info, $"Building {projectConfig} configuration succeeded");
            } else {
                Logger.Log(MessageType.Error, $"Building {projectConfig} configuration failed");
            }

            BuildDone = true;
            BuildSucceeded = success;
        }

        private static void OnBuildSolutionBegin(string project, string projectConfig, string platform, string solutionConfig) {
            Logger.Log(MessageType.Info, $"Building {project}, {projectConfig}, {platform}, {solutionConfig}");
        }

        public static bool IsDebugging() {
            bool result = false;
            bool tryAgain = true;

            for(int i = 0; i < 3 && tryAgain; ++i) {
                try {
                    result = _vsInstance != null &&
                        (_vsInstance.Debugger.CurrentProgram != null || _vsInstance.Debugger.CurrentMode == EnvDTE.dbgDebugMode.dbgRunMode);
                    tryAgain = false;
                } catch (Exception ex) {
                    Debug.WriteLine(ex.Message);
                    System.Threading.Thread.Sleep(1000);
                }
            }
            return result;
        }

        public static bool BuildSolution(Project project, string configName, bool showWindow = true) {
            // make sure vs is not running the code or anything that will interfere
            if(IsDebugging()) {
                Logger.Log(MessageType.Error, "Visual Studio is currently running a process.");
                return false;
            }
            OpenVisualStudio(project.Solution);

            if (_vsInstance == null) {
                Logger.Log(MessageType.Error, "Failed to connect to Visual Studio");
                return false;
            }

            BuildSucceeded = false;
            BuildDone = false;

            for(int i = 0; i < 3 && !BuildDone; ++i) {
                try {
                    if (!_vsInstance.Solution.IsOpen) {
                        _vsInstance.Solution.Open(project.Solution);
                    }
                    _vsInstance.MainWindow.Visible = showWindow;

                    // only register once
                    UnregisterBuildEvents();
                    RegisterBuildEvents();

                    try {
                        foreach (var pdbFile in Directory.GetFiles(Path.Combine($"{project.Path}", $@"x64\{configName}"), "*.pdb")) {
                            File.Delete(pdbFile);
                        }
                    } catch(Exception ex) {
                        Debug.WriteLine(ex.Message);
                    }

                    // TODO: do we really have to run a debug/release version to generate the exe file?
                    _vsInstance.Solution.SolutionBuild.SolutionConfigurations.Item(configName).Activate();
                    _vsInstance.ExecuteCommand("Build.BuildSolution");

                    // TODO: break if build succeed
                    if (_vsInstance.Solution.SolutionBuild.LastBuildInfo == 0) {
                        BuildSucceeded = true;
                        BuildDone = true;
                        UnregisterBuildEvents();
                        break;
                    }
                } catch (Exception ex) {
                    Debug.WriteLine(ex.Message);
                    Debug.WriteLine($"Attempt {i}: failed to build {project.Name}");
                    System.Threading.Thread.Sleep(1000);
                } finally {
                    UnregisterBuildEvents();
                }
            }
            BuildDone = true;
            return BuildSucceeded;
        }
       

        private static void RegisterBuildEvents() {
            if(_vsInstance?.Events?.BuildEvents != null) {
                _vsInstance.Events.BuildEvents.OnBuildProjConfigBegin += OnBuildSolutionBegin;
                _vsInstance.Events.BuildEvents.OnBuildProjConfigDone += OnBuildSolutionDone;
            }
        }

        private static void UnregisterBuildEvents() {
            if (_vsInstance?.Events?.BuildEvents != null) {
                _vsInstance.Events.BuildEvents.OnBuildProjConfigBegin -= OnBuildSolutionBegin;
                _vsInstance.Events.BuildEvents.OnBuildProjConfigDone -= OnBuildSolutionDone;
            }
        }

        public static void Run(Project project, string configName, bool debug) {
            if(_vsInstance != null && !IsDebugging() && BuildDone && BuildSucceeded) {
                _vsInstance.ExecuteCommand(debug ? "Debug.Start" : "Debug.StartWithoutDebugging");
            }
        }

        public static void Stop() {
            if (_vsInstance != null && IsDebugging()) {
                _vsInstance.ExecuteCommand("Debug.StopDebugging");
            }
        }
    }
}
