using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Windows.Media;
using System.Windows.Media.Media3D;
using System.Collections.ObjectModel;
using WaveEditor.Content;
using System.IO;
using System.Windows;

namespace WaveEditor.Editors {

    class MeshRendererVertexData : ViewModelBase {
        private Brush _specular = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#ff111111"));
        public Brush Specular {
            get => _specular;
            set {
                if(_specular != value) {
                    _specular = value;
                    OnPropertyChanged(nameof(Specular));
                }
            }
        }

        private Brush _diffuse = Brushes.White;
        public Brush Diffuse {
            get => _diffuse;
            set {
                if (_diffuse != value) {
                    _diffuse = value;
                    OnPropertyChanged(nameof(Diffuse));
                }
            }
        }


        public Point3DCollection Positions { get; } = new Point3DCollection();
        public Vector3DCollection Normals { get; } = new Vector3DCollection();
        public Vector3DCollection Tangents { get; } = new Vector3DCollection();
        public PointCollection UVs { get; } = new PointCollection();
        public Int32Collection Indices { get; } = new Int32Collection();
    }

    class MeshRenderer : ViewModelBase {

        public ObservableCollection<MeshRendererVertexData> Meshes { get; } = new ObservableCollection<MeshRendererVertexData>();
        private Vector3D _cameraDirection = new Vector3D(0, 0, -10);
        public Vector3D CameraDirection {
            get => _cameraDirection;
            set {
                if(_cameraDirection != value) {
                    _cameraDirection = value;
                    OnPropertyChanged(nameof(CameraDirection));
                }
            }
        }

        private Point3D _cameraPosition = new Point3D(0, 0, 10);
        public Point3D CameraPosition {
            get => _cameraPosition;
            set {
                if(_cameraPosition != value) {
                    _cameraPosition = value;
                    CameraDirection = new Vector3D(-value.X, -value.Y, -value.Z);
                    OnPropertyChanged(nameof(OffsetCameraPosition));
                    OnPropertyChanged(nameof(CameraPosition));
                }
            }
        }

        private Point3D _cameraTarget = new Point3D(0, 0, 0);
        public Point3D CameraTarget {
            get => _cameraTarget;
            set {
                if(_cameraTarget != value) {
                    _cameraTarget = value;
                    OnPropertyChanged(nameof(OffsetCameraPosition));
                    OnPropertyChanged(nameof(CameraTarget));
                }
            }
        }

        public Point3D OffsetCameraPosition => 
            new Point3D(CameraPosition.X + CameraTarget.X, CameraPosition.Y + CameraTarget.Y, CameraPosition.Z + CameraTarget.Z);


        private Color _keyLight = (Color)ColorConverter.ConvertFromString("#ffaeaeae");
        public Color KeyLight {
            get => _keyLight;
            set {
                if(_keyLight != value) {
                    _keyLight = value;
                    OnPropertyChanged(nameof(KeyLight));
                }
            }
        }

        private Color _skyLight = (Color)ColorConverter.ConvertFromString("#ff113b30");
        public Color SkyLight {
            get => _skyLight;
            set {
                if (_skyLight != value) {
                    _skyLight = value;
                    OnPropertyChanged(nameof(SkyLight));
                }
            }
        }

        private Color _groundLight = (Color)ColorConverter.ConvertFromString("#ff3f2f1e");
        public Color GroundLight {
            get => _groundLight;
            set {
                if(_groundLight != value) {
                    _groundLight = value;
                    OnPropertyChanged(nameof(GroundLight));
                }
            }
        }

        private Color _ambientLight = (Color)ColorConverter.ConvertFromString("#ff3b3b3b");
        public Color AmbientLight {
            get => _ambientLight;
            set {
                if (_ambientLight != value) {
                    _ambientLight = value;
                    OnPropertyChanged(nameof(AmbientLight));
                }
            }
        }

        //struct vertex_static {
        //    MATH::v3 position;
        //    u8 reserved[3];
        //    u8 t_sign; // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1)
        //    u16 normal[2]; // we can use x and y component to calculate the z component
        //    u16 tangent[2]; // same as normal
        //    MATH::v2 uv;
        //};
        public MeshRenderer(MeshLOD lod, MeshRenderer old) {
            Debug.Assert(lod?.Meshes.Any() == true);
            // Calculate vertex size minus the position and normal vectors
            var offset = lod.Meshes[0].VertexSize - 3 * sizeof(float) - sizeof(int) - 2 * sizeof(short); // skip position, reserved, t_sign and normal

            // set up camera
            double minX, minY, minZ;
            minX = minY = minZ = double.MaxValue;
            double maxX, maxY, maxZ;
            maxX = maxY = maxZ = double.MinValue;
            Vector3D avgNormal = new Vector3D();

            // unpack the packed normals
            var intervals = 2.0f / ((1 << 16) - 1); // normalizing interval = 2.0 / 65535
            foreach(var mesh in lod.Meshes) {
                var vertexData = new MeshRendererVertexData();
                // unpack all vertices
                using (var reader = new BinaryReader(new MemoryStream(mesh.Vertices))) 
                    for(int i = 0; i < mesh.VertexCount; ++i) {
                        // read positions
                        var posX = reader.ReadSingle(); // float: 4bytes
                        var posY = reader.ReadSingle(); // float: 4bytes
                        var posZ = reader.ReadSingle(); // float: 4bytes
                        var signs = (reader.ReadUInt32() >> 24) & 0x000000ff; // 3 * u8 + u8 => int : 4bytes
                        vertexData.Positions.Add(new Point3D(posX, posY, posZ));

                        // update the bounding box
                        minX = Math.Min(minX, posX); maxX = Math.Max(maxX, posX);
                        minY = Math.Min(minY, posY); maxY = Math.Max(maxY, posY);
                        minZ = Math.Min(minZ, posZ); maxZ = Math.Max(maxZ, posZ);

                        // read normals [0, 65535] -> [-1, 1]
                        var nrmX = reader.ReadUInt16() * intervals - 1.0f; // u16: 2bytes
                        var nrmY = reader.ReadUInt16() * intervals - 1.0f; // u16: 2bytes
                        // calculate nrmZ by using x^2 + y^2 + z^2 = 1, sign of nrmZ determined by signs
                        var nrmZ = Math.Sqrt(Math.Clamp(1f - (nrmX * nrmX + nrmY * nrmY), 0f, 1f)) * ((signs & 0x2) - 1f);
                        var normal = new Vector3D(nrmX, nrmY, nrmZ); 
                        normal.Normalize();
                        vertexData.Normals.Add(normal);
                        avgNormal += normal; 

                        // read uvs (skip tangent and joint data)
                        reader.BaseStream.Position += (offset - 2 * sizeof(float));
                        var u = reader.ReadSingle(); // float: 4bytes
                        var v = reader.ReadSingle(); // float: 4bytes
                        vertexData.UVs.Add(new Point(u, v));
                    }

                using (var reader = new BinaryReader(new MemoryStream(mesh.Indices)))
                    if (mesh.IndexSize == sizeof(short))
                        for (int i = 0; i < mesh.IndexCount; ++i) vertexData.Indices.Add(reader.ReadUInt16()); // up to 65535 vertices
                    else
                        for (int i = 0; i < mesh.IndexCount; ++i) vertexData.Indices.Add(reader.ReadInt32()); // up to 4294967295 vertices

                vertexData.Positions.Freeze();
                vertexData.Normals.Freeze();
                // TODO: vertexData.Tangents
                vertexData.UVs.Freeze();
                vertexData.Indices.Freeze();
                Meshes.Add(vertexData);
            }

            if(old != null) { // if we have old renderer, inherit camera settings
                CameraTarget = old.CameraTarget;
                CameraPosition = old.CameraPosition;
            } else {
                // compute bounding box dimensions
                var width = maxX - minX;
                var height = maxY - minY;
                var depth = maxZ - minZ;
                var radius = new Vector3D(height, width, depth).Length * 1.2;
                // if the mesh shows a obvious normal direction, put the camera along the normal vector
                if(avgNormal.Length > 0.8) {
                    avgNormal.Normalize();
                    avgNormal *= radius;
                    CameraPosition = new Point3D(avgNormal.X, avgNormal.Y, avgNormal.Z);
                } else {
                    CameraPosition = new Point3D(width, height * 0.5, radius);
                }

                // set the center of bounding box as the camera target
                CameraTarget = new Point3D(minX + width * 0.5, minY + height * 0.5, minZ + depth * 0.5);
            }
        }
    }

    class GeometryEditor : ViewModelBase, IAssetEditor {

        Asset IAssetEditor.Asset => Geometry;

        private Content.Geometry _geometry;
        public Content.Geometry Geometry {
            get => _geometry;
            set {
                if(_geometry != value) {
                    _geometry = value;
                    OnPropertyChanged(nameof(Geometry));
                }
            }
        }

        private MeshRenderer _meshRenderer;
        public MeshRenderer MeshRenderer {
            get => _meshRenderer;
            set {
                if(_meshRenderer != value) {
                    _meshRenderer = value;
                    OnPropertyChanged(nameof(MeshRenderer));
                    var lods = Geometry.GetLODGroup().LODs;
                    MaxLODIndex = (lods.Count > 0) ? lods.Count - 1 : 0;
                    OnPropertyChanged(nameof(MaxLODIndex));
                    if(lods.Count > 1) {
                        MeshRenderer.PropertyChanged += (s, e) => {
                            if (e.PropertyName == nameof(MeshRenderer.OffsetCameraPosition) && AutoLOD) {
                                ComputeLOD(lods);
                            }
                        };

                        ComputeLOD(lods);
                    }
                }
            }
        }


        private bool _autoLOD = true;
        public bool AutoLOD {
            get => _autoLOD;
            set {
                if(_autoLOD != value) {
                    _autoLOD = value;
                    OnPropertyChanged(nameof(AutoLOD));
                }
            }
        }

        public int MaxLODIndex { get; private set; }

        private int _lodIndex;
        public int LODIndex {
            get => _lodIndex;
            set {
                var lods = Geometry.GetLODGroup().LODs;
                value = Math.Clamp(value, 0, lods.Count - 1);
                if(_lodIndex != value) {
                    _lodIndex = value;
                    OnPropertyChanged(nameof(LODIndex));
                    MeshRenderer = new MeshRenderer(lods[value], MeshRenderer);
                }
            }
        }

        private void ComputeLOD(IList<MeshLOD> lods) {
            if (!AutoLOD) return;

            var p = MeshRenderer.OffsetCameraPosition;
            var distance = new Vector3D(p.X, p.Y, p.Z).Length;
            for(int i = MaxLODIndex; i >= 0; ++i) {
                if (lods[i].LodThreshold < distance) {
                    LODIndex = i;
                    break;
                }
            }
        }

        public void SetAsset(Asset asset) {
            Debug.Assert(asset is Content.Geometry);
            if(asset is Content.Geometry geometry) {
                Geometry = geometry;
                var numLods = geometry.GetLODGroup().LODs.Count;
                if(LODIndex >= numLods) {
                    LODIndex = numLods - 1;
                } else {
                    MeshRenderer = new MeshRenderer(Geometry.GetLODGroup().LODs[0], MeshRenderer);
                }
            }
        }

        public async void SetAsset(AssetInfo info) {
            try {
                Debug.Assert(info != null && File.Exists(info.FullPath));
                var geometry = new Content.Geometry();

                await Task.Run(() => {
                    geometry.Load(info.FullPath);
                });
                SetAsset(geometry);
            } catch(Exception ex) {
                Debug.WriteLine(ex.Message);
            }
        }
    }
}
