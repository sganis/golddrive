﻿using System.Globalization;
using System.Windows.Controls;

namespace golddrive
{
    public class NotEmptyValidationRule : ValidationRule
    {
        public override ValidationResult Validate(object value, CultureInfo cultureInfo)
        {
            return string.IsNullOrWhiteSpace((value ?? "").ToString())
                ? new ValidationResult(false, " Field is required.")
                : ValidationResult.ValidResult;
        }
    }
}