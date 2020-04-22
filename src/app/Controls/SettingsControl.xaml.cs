using System.Threading;
using System.Windows.Controls;

namespace golddrive
{
    public partial class SettingsControl : UserControl
    {
        public SettingsControl()
        {
            InitializeComponent();
        }

        private void UserControl_Loaded(object sender, System.Windows.RoutedEventArgs e)
        {

            IRequestFocus focus = (IRequestFocus)DataContext;
            focus.FocusRequested += Focus_FocusRequested;
            
        }

        private void Focus_FocusRequested(object sender, FocusRequestedEventArgs e)
        {
            var viewModel = (MainWindowViewModel)DataContext;


            switch (e.PropertyName)
            {
                case "SelectedFreeDrive.Host":
                    Dispatcher.BeginInvoke((ThreadStart)delegate
                    {
                        //txtSettingsSelectedFreeDriveHost.Focus();
                    });
                    break;
                case "SelectedDrive.Host":
                    Dispatcher.BeginInvoke((ThreadStart)delegate
                    {
                        txtSettingsSelectedDriveHost.Focus();
                    });
                    break;
            }
        }
    }
}
