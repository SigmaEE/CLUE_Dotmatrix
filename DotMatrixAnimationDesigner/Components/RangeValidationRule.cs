using System.Globalization;
using System.Windows.Controls;

namespace DotMatrixAnimationDesigner.Components
{
    internal class RangeValidationRule : ValidationRule
    {
        public int MinExclusive { get; set; }
        public int MaxInclusive { get; set; }

        public override ValidationResult Validate(object value, CultureInfo cultureInfo)
        {
            if (!int.TryParse(value as string, NumberStyles.Integer, cultureInfo, out var v))
                return new ValidationResult(false, $"'{value}' is not a valid integer");

            if ((v <= MinExclusive) || (v > MaxInclusive))
                return new ValidationResult(false, $"The value must be in the range: ({MinExclusive}-{MaxInclusive}]");
            return ValidationResult.ValidResult;
        }

    }
}
