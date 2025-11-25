using WaveEditor.Content;

namespace WaveEditor.Editors {
    interface IAssetEditor {
        Asset Asset { get; }

        void SetAsset(Asset asset);
    }
}