namespace golddrive
{
    public partial class MainWindow
    {
        public MainWindow(ReturnBox rb, Drive drive)
        {
            InitializeComponent();
            DataContext = new MainWindowViewModel(rb, drive);
        }

    }
}