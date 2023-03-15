using System.Windows;

namespace DotMatrixAnimationDesigner.Components
{
    internal enum TitleBarButton
    {
        Minimize,
        Maximize,
        Restore,
        Close
    }

    internal class TitleBarButtonClickedEventArgs : RoutedEventArgs
    {
        public TitleBarButton ButtonClicked { get; }

        public TitleBarButtonClickedEventArgs(RoutedEvent routedEvent, TitleBarButton buttonClicked) : base(routedEvent)
            => ButtonClicked = buttonClicked;
    }
}
