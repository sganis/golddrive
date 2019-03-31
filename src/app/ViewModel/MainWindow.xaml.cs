using System.Windows;

namespace golddrive
{
    public partial class MainWindow 
    {
        public MainWindow()
        {
            InitializeComponent();
            DataContext = new MainWindowViewModel();            
        }
    }
}