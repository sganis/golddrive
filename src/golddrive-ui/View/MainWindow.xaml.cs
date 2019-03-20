using MahApps.Metro.Controls;

namespace golddrive_ui
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : MetroWindow
    {
        // constructor
        public MainWindow()
        {
            InitializeComponent();
            App.Controller = new Controller();
            App.MainWindowViewModel = new MainWindowViewModel();
            DataContext = App.MainWindowViewModel;            
        }

        //private async void Connect_Click(object sender, RoutedEventArgs e)
        //{
        //    var t = new Stopwatch();
        //    t.Start();
        //    var r2 = await App.Controller.GetGoldDrives();
        //    //WorkEnd(r2);
        //    txtMessage.Text = t.ElapsedMilliseconds.ToString();
        //    WorkEnd(r2);
        //}

        //private void Settings_Click(object sender, RoutedEventArgs e)
        //{
        //    // setup ssh
        //    //GenerateKeys();
        //    TransferPublicKey();
        //    //TestSsh();
        //}

        //private void TestSsh()
        //{
        //    throw new NotImplementedException();
        //}

        //private void TransferPublicKey()
        //{
        //    App.Controller.UploadFile(
        //        @"D:\Users\ganissa\.ssh\id_rsa-ganissa-golddrive.pub",
        //        ".ssh",
        //        "id_rsa-ganissa-golddrive.pub");
        //}
    }
}