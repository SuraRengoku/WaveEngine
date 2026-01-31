using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using WaveEditor.ContentToolsAPIStructs;
using WaveEditor.DLLWrappers;
using WaveEditor.Editors;
using WaveEditor.GameProject;
using WaveEditor.Utilities.Controls;

namespace WaveEditor.Content {
    /// <summary>
    /// Interaction logic for PrimitiveMeshDialog.xaml
    /// </summary>
    public partial class PrimitiveMeshDialog : Window {

        private static readonly List<ImageBrush> _textures = new List<ImageBrush>();

        public PrimitiveMeshDialog() {
            InitializeComponent();
            Loaded += (s, e) => UpdatePrimitive(); // when load the dialog window, immediately generate a default mesh
        }

        private void OnPrimitiveType_ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e) => UpdatePrimitive();

        private void OnSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e) => UpdatePrimitive();

        private void OnScalarBox_ValueChanged(object sender, RoutedEventArgs e) => UpdatePrimitive();

        private float Value(ScalarBox scalarBox, float min) {
            float.TryParse(scalarBox.Value, out var result);
            return Math.Max(result, min);
        }

        private void UpdatePrimitive() {
            if (!IsInitialized) return;

            var primitiveType = (PrimitiveMeshType)PrimitiveTypeComboBox.SelectedItem;
            var info = new PrimitiveInitInfo() { Type = primitiveType };
            var smoothingAngle = 0;

            switch(primitiveType) {
                case PrimitiveMeshType.Plane: {
                        info.SegmentX = (int)xSliderPlane.Value;
                        info.SegmentZ = (int)zSliderPlane.Value;
                        info.Size.X = Value(widthScalarBoxPlane, 0.001f);
                        info.Size.Z = Value(heightScalarBoxPlane, 0.001f);
                        break;
                    }
                case PrimitiveMeshType.Cube: {
                        info.SegmentX = (int)xSliderCube.Value;
                        info.SegmentY = (int)ySliderCube.Value;
                        info.SegmentZ = (int)zSliderCube.Value;
                        info.Size.X = Value(XScalarBoxCube, 0.001f);
                        info.Size.Y = Value(YScalarBoxCube, 0.001f);
                        info.Size.Z = Value(ZScalarBoxCube, 0.001f);
                        break;
                    }
                case PrimitiveMeshType.UVSphere: {
                        info.SegmentX = (int)xSliderUVSphere.Value;
                        info.SegmentY = (int)ySliderUVSphere.Value;
                        info.Size.X = Value(XScalarBoxUVSphere, 0.001f);
                        info.Size.Y = Value(YScalarBoxUVSphere, 0.001f);
                        info.Size.Z = Value(ZScalarBoxUVSphere, 0.001f);
                        smoothingAngle = (int)angleSliderUVSphere.Value;
                        break;
                    }
                case PrimitiveMeshType.IcoSphere: {
                        return;
                    }
                case PrimitiveMeshType.Cylinder: {
                        return;
                    }
                case PrimitiveMeshType.Capsule: {
                        return;
                    }
                default:
                    break;
            }

            var geometry = new Geometry();
            geometry.ImportSettings.SmoothingAngle = smoothingAngle;
            ContentToolsAPI.CreatePrimitiveMesh(geometry, info); // get raw data
            (DataContext as GeometryEditor).SetAsset(geometry);
            OnTexture_CheckBox_Click(textureCheckBox, null);
        }

        private static void LoadTextures() {
            var uris = new List<Uri> {
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/awesomeface.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/UVcheckerMap.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/UVcheckerMap.png"),
            };

            _textures.Clear();
            foreach(var uri in uris) {
                var resource = Application.GetResourceStream(uri);
                using var reader = new BinaryReader(resource.Stream);
                var data = reader.ReadBytes((int)resource.Stream.Length);
                var imageSource = (BitmapSource)new ImageSourceConverter().ConvertFrom(data);
                imageSource.Freeze(); // now inmodifiable

                var brush = new ImageBrush(imageSource);
                brush.Transform = new ScaleTransform(1, -1, 0.5, 0.5); // flip v direction
                brush.ViewportUnits = BrushMappingMode.Absolute;
                brush.Freeze();
                _textures.Add(brush);
            }
        }

        static PrimitiveMeshDialog() {
            LoadTextures();
        }

        private void OnTexture_CheckBox_Click(object sender, RoutedEventArgs e) {
            Brush brush = Brushes.White;
            if((sender as CheckBox).IsChecked == true) {
                brush = _textures[(int)PrimitiveTypeComboBox.SelectedItem];
            }

            var vm = DataContext as GeometryEditor;
            foreach(var mesh in vm.MeshRenderer.Meshes) {
                mesh.Diffuse = brush;
            }
        }

        private void OnSave_Button_Click(object sender, RoutedEventArgs e) {
            var dlg = new SaveDialog();
            if(dlg.ShowDialog() == true) {
                Debug.Assert(!string.IsNullOrEmpty(dlg.SaveFilePath));
                var asset = (DataContext as IAssetEditor).Asset;
                Debug.Assert(asset != null);
                asset.Save(dlg.SaveFilePath);

                // Note: you can choose to close this window after saving
            }
        }
    }
}
