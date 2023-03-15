using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace DotMatrixAnimationDesigner.Converters
{
    public sealed class BooleanToVisibleOrCollapsedConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value == null)
                return Visibility.Collapsed;

            if (value is bool opt)
            {
                if (parameter is string s)
                {
                    if (s.Equals("inverted", StringComparison.CurrentCultureIgnoreCase))
                        opt = !opt;
                }
                return opt ? Visibility.Visible : Visibility.Collapsed;
            }
            throw new InvalidOperationException("Value must be a boolean");
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
