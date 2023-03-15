using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;

namespace DotMatrixAnimationDesigner.Components
{
    public partial class Dot : UserControl
    {

        #region Dependency properties
        public bool DotIsChecked
        {
            get => (bool)GetValue(DotIsCheckedProperty);
            set => SetValue(DotIsCheckedProperty, value);
        }
        public static readonly DependencyProperty DotIsCheckedProperty = DependencyProperty.Register(
            nameof(DotIsChecked),
            typeof(bool),
            typeof(Dot),
            new FrameworkPropertyMetadata(false, FrameworkPropertyMetadataOptions.AffectsRender));
        #endregion

        public Dot()
        {
            InitializeComponent();
        }
    }
}
