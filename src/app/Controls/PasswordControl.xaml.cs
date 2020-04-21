using System.Threading;
using System.Windows.Controls;

namespace golddrive
{
    public partial class PasswordControl : UserControl
    {
        public PasswordControl()
        {
            InitializeComponent();
        }

        private void ucPassword_Loaded(object sender, System.Windows.RoutedEventArgs e)
        {

            IRequestFocus focus = (IRequestFocus)DataContext;
            focus.FocusRequested += Focus_FocusRequested;
        }

        private void Focus_FocusRequested(object sender, FocusRequestedEventArgs e)
        {
            var viewModel = (MainWindowViewModel)DataContext;

            
            switch (e.PropertyName)
            {
                case nameof(viewModel.Password):
                    Dispatcher.BeginInvoke((ThreadStart)delegate
                    {
                        txtPassword.Focus();
                    });
                    break;
            }
        }

    }
}
