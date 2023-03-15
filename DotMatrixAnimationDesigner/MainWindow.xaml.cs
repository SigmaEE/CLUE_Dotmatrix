using System.Windows;
using DotMatrixAnimationDesigner.Components;

namespace DotMatrixAnimationDesigner
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void TitleBarButtonPressed(object sender, RoutedEventArgs e)
        {
            if (e is TitleBarButtonClickedEventArgs eventArgs)
            {
                if (eventArgs.ButtonClicked == TitleBarButton.Close)
                    Close();
                else if (eventArgs.ButtonClicked == TitleBarButton.Maximize)
                    WindowState = WindowState.Maximized;
                else if (eventArgs.ButtonClicked == TitleBarButton.Minimize)
                    WindowState = WindowState.Minimized;
            }
        }
    }
}
