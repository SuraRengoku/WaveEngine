using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using WaveEditor.Components;
using WaveEditor.EngineAPIStructs;
using WaveEditor.GameProject;
using WaveEditor.Utilities;


namespace WaveEditor.EngineAPIStructs {
    // use [StructLayout(LayoutKind.Sequential)] to make sure .NET will not rearrange the memory layout of class / struct
    // Position offset 0
    // Rotatoin offset 12 (0 + 3 * 4)
    // Scale offset 24 (12 + 3 * 4)
    // you will find the same layout in EngineAPI.cpp
    [StructLayout(LayoutKind.Sequential)] 
    class TransformComponent {
        public Vector3 Position;
        public Vector3 Rotation;
        public Vector3 Scale = new Vector3(1, 1, 1);
    }

    [StructLayout(LayoutKind.Sequential)]
    class ScriptComponent {
        public IntPtr ScriptCreator;
    }

    [StructLayout(LayoutKind.Sequential)]
    class GameEntityDescriptor {
        public TransformComponent Transform = new TransformComponent();
        public ScriptComponent Script = new ScriptComponent();
    }
}


namespace WaveEditor.DLLWrappers {
    static class EngineAPI {
        private const string _engineDll = "EngineDll.dll"; // windows is not case-sensitive

        [DllImport(_engineDll, CharSet = CharSet.Ansi)]
        public static extern int LoadGameCodeDll(string dllPath);
        [DllImport(_engineDll)]
        public static extern int UnloadGameCodeDll();
        [DllImport(_engineDll)]
        public static extern IntPtr GetScriptCreator(string name);
        [DllImport(_engineDll)]
        [return: MarshalAs(UnmanagedType.SafeArray)]
        public static extern string[] GetScriptNames();

        [DllImport(_engineDll)]
        public static extern int CreateRenderSurface(IntPtr host, int width, int height);
        [DllImport(_engineDll)]
        public static extern void RemoveRenderSurface(int surfaceId);
        [DllImport(_engineDll)]
        public static extern IntPtr GetWindowHandle(int surfaceId);
        [DllImport(_engineDll)]
        public static extern void ResizeRenderSurface(int surfaceId); 

        internal static class EntityAPI {

            [DllImport(_engineDll)]
            private static extern int CreateGameEntity(GameEntityDescriptor desc);
            public static int CreateGameEntity(GameEntity entity) {
                GameEntityDescriptor desc = new GameEntityDescriptor();

                // transform component
                {
                    var c = entity.GetComponent<Transform>();
                    desc.Transform.Position = c.Position;
                    desc.Transform.Rotation = c.Rotation;
                    desc.Transform.Scale = c.Scale;
                }

                // script component
                {
                    //  NOTE:   here we check if current project is not null, so we can tell whether the game code DLL has been loaded or not.
                    //          This way, creation of entities with a script component is deferred until the DLL has been loaded.             
                    var c = entity.GetComponent<Script>();
                    if(c != null && Project.Current != null) {
                        if(Project.Current.AvailableScripts.Contains(c.Name)) {
                            desc.Script.ScriptCreator = GetScriptCreator(c.Name);
                        } else {
                            Logger.Log(MessageType.Error, $"Unable to find script with name {c.Name}. Game entity will be created without script component.");
                        }
                    }
                }

                return CreateGameEntity(desc);
            }

            [DllImport(_engineDll)]
            private static extern void RemoveGameEntity(int id);
            public static void RemoveGameEntity(GameEntity entity) {
                RemoveGameEntity(entity.EntityId);
            }
        }

    }
}
