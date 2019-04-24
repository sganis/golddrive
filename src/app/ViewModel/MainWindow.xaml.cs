using System.Windows;

namespace golddrive
{
    public partial class MainWindow 
    {
        public MainWindow(ReturnBox rb)
        {
            InitializeComponent();
            DataContext = new MainWindowViewModel(rb);            
        }
    }
}