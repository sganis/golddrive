namespace golddrive
{
    public partial class MainWindow
    {
        public MainWindow(ReturnBox rb)
        {
            InitializeComponent();
            DataContext = new MainWindowViewModel(rb);
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            var vm = (MainWindowViewModel)DataContext;
            vm.Closing(null);
        }
    }
}