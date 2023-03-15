using System.Windows;
using System.Windows.Controls;

namespace DotMatrixAnimationDesigner.Components
{
    public partial class CustomTitleBar : UserControl
    {
        #region Dependency properties
        public string Title
        {
            get => (string)GetValue(TitleProperty);
            set => SetValue(TitleProperty, value);
        }
        public static readonly DependencyProperty TitleProperty = DependencyProperty.Register(
            nameof(Title),
            typeof(string),
            typeof(CustomTitleBar),
            new FrameworkPropertyMetadata(string.Empty, FrameworkPropertyMetadataOptions.AffectsRender));

        public string ApplicationDescription
        {
            get => (string)GetValue(ApplicationDescriptionProperty);
            set => SetValue(ApplicationDescriptionProperty, value);
        }
        public static readonly DependencyProperty ApplicationDescriptionProperty = DependencyProperty.Register(
            nameof(ApplicationDescription),
            typeof(string),
            typeof(CustomTitleBar),
            new FrameworkPropertyMetadata(string.Empty, FrameworkPropertyMetadataOptions.AffectsRender));

        public bool ShowMaximizeButton
        {
            get => (bool)GetValue(ShowMaximizeButtonProperty);
            set => SetValue(ShowMaximizeButtonProperty, value);
        }
        public static readonly DependencyProperty ShowMaximizeButtonProperty = DependencyProperty.Register(
            nameof(ShowMaximizeButton),
            typeof(bool),
            typeof(CustomTitleBar),
            new FrameworkPropertyMetadata(true, FrameworkPropertyMetadataOptions.AffectsRender));

        public bool ShowMinimizeButton
        {
            get => (bool)GetValue(ShowMinimizeButtonProperty);
            set => SetValue(ShowMinimizeButtonProperty, value);
        }
        public static readonly DependencyProperty ShowMinimizeButtonProperty = DependencyProperty.Register(
            nameof(ShowMinimizeButton),
            typeof(bool),
            typeof(CustomTitleBar),
            new FrameworkPropertyMetadata(true, FrameworkPropertyMetadataOptions.AffectsRender));

        public WindowState CurrentWindowState
        {
            get => (WindowState)GetValue(CurrentWindowStateProperty);
            set => SetValue(CurrentWindowStateProperty, value);
        }
        public static readonly DependencyProperty CurrentWindowStateProperty = DependencyProperty.Register(
            nameof(CurrentWindowState),
            typeof(WindowState),
            typeof(CustomTitleBar),
            new FrameworkPropertyMetadata(WindowState.Normal, WindowStateChangedCallback));

        private static void WindowStateChangedCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is CustomTitleBar titleBar && e.NewValue is WindowState state)
                titleBar.SetMaximizeOrRestoreVisibility(state);
        }
        #endregion

        public event RoutedEventHandler TitleBarButtonPressed
        {
            add { AddHandler(TitleBarButtonPressedEvent, value); }
            remove { RemoveHandler(TitleBarButtonPressedEvent, value); }
        }

        public static readonly RoutedEvent TitleBarButtonPressedEvent = EventManager.RegisterRoutedEvent(
            nameof(TitleBarButtonPressed),
            RoutingStrategy.Bubble,
            typeof(RoutedEventHandler),
            typeof(CustomTitleBar));

        public CustomTitleBar()
        {
            InitializeComponent();
            SetMaximizeOrRestoreVisibility(CurrentWindowState);
        }

        private void SetMaximizeOrRestoreVisibility(WindowState state)
        {
            maximizeButton.Visibility = state == WindowState.Maximized ? Visibility.Collapsed : Visibility.Visible;
            restoreButton.Visibility = state == WindowState.Maximized ? Visibility.Visible : Visibility.Collapsed;
        }

        private void MinimizeButtonPressed(object sender, RoutedEventArgs e)
        {
            var eventArgs = new TitleBarButtonClickedEventArgs(TitleBarButtonPressedEvent, TitleBarButton.Minimize);
            RaiseEvent(eventArgs);
        }

        private void MaximizeButtonPressed(object sender, RoutedEventArgs e)
        {
            var eventArgs = new TitleBarButtonClickedEventArgs(TitleBarButtonPressedEvent, TitleBarButton.Maximize);
            RaiseEvent(eventArgs);
        }

        private void RestoreButtonPressed(object sender, RoutedEventArgs e)
        {
            var eventArgs = new TitleBarButtonClickedEventArgs(TitleBarButtonPressedEvent, TitleBarButton.Restore);
            RaiseEvent(eventArgs);
        }

        private void CloseButtonPressed(object sender, RoutedEventArgs e)
        {
            var eventArgs = new TitleBarButtonClickedEventArgs(TitleBarButtonPressedEvent, TitleBarButton.Close);
            RaiseEvent(eventArgs);
        }
    }
}
