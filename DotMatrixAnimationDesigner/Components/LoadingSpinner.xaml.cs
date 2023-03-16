using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;

namespace DotMatrixAnimationDesigner.Components
{
    public partial class LoadingSpinner : UserControl
    {
        #region Dependency properties
        public double ControlSize
        {
            get => (double)GetValue(ControlSizeProperty);
            set => SetValue(ControlSizeProperty, value);
        }
        public static readonly DependencyProperty ControlSizeProperty = DependencyProperty.Register(
            nameof(ControlSize),
            typeof(double),
            typeof(LoadingSpinner),
            new FrameworkPropertyMetadata(20.0, UpdateControlPropertiesCallback));

        public bool UseShrinkAnimation
        {
            get => (bool)GetValue(UseShrinkAnimationProperty);
            set => SetValue(UseShrinkAnimationProperty, value);
        }
        public static readonly DependencyProperty UseShrinkAnimationProperty = DependencyProperty.Register(
            nameof(UseShrinkAnimation),
            typeof(bool),
            typeof(LoadingSpinner),
            new FrameworkPropertyMetadata(false, UpdateControlPropertiesCallback));

        public Color ElementColor
        {
            get => (Color)GetValue(ElementColorProperty);
            set => SetValue(ElementColorProperty, value);
        }
        public static readonly DependencyProperty ElementColorProperty = DependencyProperty.Register(
            nameof(ElementColor),
            typeof(Color),
            typeof(LoadingSpinner),
            new FrameworkPropertyMetadata(Color.FromArgb(255, 155, 155, 155), UpdateControlPropertiesCallback));

        private static void UpdateControlPropertiesCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is LoadingSpinner spinner)
            {
                var controlSize = e.Property == ControlSizeProperty ? (e.NewValue is double newSize ? newSize : spinner.ControlSize) : spinner.ControlSize;
                var useShrinkAnimation = e.Property == UseShrinkAnimationProperty ? (e.NewValue is bool newShrinkSetting ? newShrinkSetting : spinner.UseShrinkAnimation) : spinner.UseShrinkAnimation;
                var elementColor = e.Property == ElementColorProperty ? (e.NewValue is Color newColor ? newColor : spinner.ElementColor) : spinner.ElementColor;

                spinner.RecalculateControlParameters(controlSize, useShrinkAnimation, elementColor);
            }
        }

        #endregion

        private readonly List<Ellipse> _components;

        public LoadingSpinner()
        {
            InitializeComponent();
            _components = new List<Ellipse>
            {
                Circle0,
                Circle1,
                Circle2,
                Circle3,
                Circle4,
                Circle5,
                Circle6,
                Circle7
            };

            Loaded += ControlLoadedEvent;
        }

        private void ControlLoadedEvent(object sender, RoutedEventArgs e)
        {
            Loaded -= ControlLoadedEvent;
            RecalculateControlParameters(ControlSize, UseShrinkAnimation, ElementColor);
        }

        private void RecalculateControlParameters(double controlSize, bool useShrinkAnimation, Color elementColor)
        {
            var circleDiameter = controlSize * 0.8;
            var mainCircleRadius = circleDiameter * 2;

            ContainingCanvas.Width = controlSize;
            ContainingCanvas.Height = controlSize;

            var transparentColor = elementColor;
            transparentColor.A = 0;

            var start = new SplineColorKeyFrame(elementColor, KeyTime.FromTimeSpan(TimeSpan.FromMilliseconds(0)));
            var end = new SplineColorKeyFrame(transparentColor, KeyTime.FromTimeSpan(TimeSpan.FromMilliseconds(1200)));

            var shrinkingStart = new SplineDoubleKeyFrame(circleDiameter, KeyTime.FromTimeSpan(TimeSpan.FromMilliseconds(0)));
            var shrinkingMiddle = new SplineDoubleKeyFrame(0.9 * circleDiameter, KeyTime.FromTimeSpan(TimeSpan.FromMilliseconds(800)));
            var shrinkingEnd = new SplineDoubleKeyFrame(0, KeyTime.FromTimeSpan(TimeSpan.FromMilliseconds(1200)));

            var fillProperty = new PropertyPath("(Shape.Fill).(SolidColorBrush.Color)");
            var widthProperty = new PropertyPath("Width");
            var heightProperty = new PropertyPath("Height");

            var storyboard = new Storyboard();

            foreach (var (c, idx) in _components.Select((x, i) => (x, i)))
            {
                var centerPointX = mainCircleRadius * Math.Cos(Math.PI / 4 * idx);
                var centerPointY = mainCircleRadius * Math.Sin(Math.PI / 4 * idx);

                Canvas.SetLeft(c, centerPointX - circleDiameter / 2);
                Canvas.SetTop(c, centerPointY - circleDiameter / 2);
                c.Width = circleDiameter;
                c.Height = circleDiameter;

                var fadingAnimation = new ColorAnimationUsingKeyFrames();
                var shrinkingAnimation = new DoubleAnimationUsingKeyFrames();

                fadingAnimation.KeyFrames.Add(start);
                fadingAnimation.KeyFrames.Add(end);
                fadingAnimation.BeginTime = TimeSpan.FromMilliseconds(150 * idx);
                fadingAnimation.RepeatBehavior = RepeatBehavior.Forever;

                Storyboard.SetTarget(fadingAnimation, c);
                Storyboard.SetTargetProperty(fadingAnimation, fillProperty);
                storyboard.Children.Add(fadingAnimation);

                if (useShrinkAnimation)
                {
                    shrinkingAnimation.KeyFrames.Add(shrinkingStart);
                    shrinkingAnimation.KeyFrames.Add(shrinkingMiddle);
                    shrinkingAnimation.KeyFrames.Add(shrinkingEnd);
                    shrinkingAnimation.BeginTime = TimeSpan.FromMilliseconds(150 * idx);
                    shrinkingAnimation.RepeatBehavior = RepeatBehavior.Forever;

                    Storyboard.SetTarget(shrinkingAnimation, c);
                    Storyboard.SetTargetProperty(shrinkingAnimation, widthProperty);
                    Storyboard.SetTargetProperty(shrinkingAnimation, heightProperty);

                    storyboard.Children.Add(shrinkingAnimation);
                }
            }
            storyboard.Begin(this);
        }
    }
}
