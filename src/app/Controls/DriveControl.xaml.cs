using System.Windows.Controls;

namespace golddrive
{
    public partial class DriveControl : UserControl
    {
        public DriveControl()
        {
            InitializeComponent();
        }

        private void ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var vm = (MainWindowViewModel)DataContext;
            vm.OnComboChanged();
        }
    }
}
