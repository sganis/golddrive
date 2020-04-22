using System.Threading;
using System.Windows.Controls;
using System.Windows.Input;

namespace golddrive
{
    public partial class HostControl : UserControl
    {
        public HostControl()
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

            switch(e.PropertyName)
            {
                case nameof(viewModel.SelectedDrive.Host):
                    Dispatcher.BeginInvoke((ThreadStart)delegate
                    {
                        txtHost.Focus();
                    });
                    break;
            }
        }
    }
}
