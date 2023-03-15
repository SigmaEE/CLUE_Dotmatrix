using CommunityToolkit.Mvvm.ComponentModel;

namespace DotMatrixAnimationDesigner
{
    internal class DotModel : ObservableObject
    {
        private bool _isChecked;
        public bool IsChecked
        {
            get => _isChecked;
            set => SetProperty(ref _isChecked, value);
        }

        public DotModel(bool initalState = false)
        {
            IsChecked = initalState;
        }
    }
}
