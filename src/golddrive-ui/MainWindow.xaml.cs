using MahApps.Metro.Controls;
using Renci.SshNet;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace golddrive_ui
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : MetroWindow
    {
        Controller mController;

        public bool IsLoginOpen { get; set; }

        public MainWindow()
        {
            InitializeComponent();
            mController = new Controller();
            progressBar.Visibility = Visibility.Hidden;
        }
        
        void WorkStart(string msg = "")
        {
            btnConnect.IsEnabled = false;
            progressBar.Visibility = Visibility.Visible;
            txtStatus.Text = msg;
        }
        void WorkProgress(string message)
        {
            txtStatus.Text = message;
        }
        void WorkEnd(ReturnBox r)
        {
            btnConnect.IsEnabled = true;
            progressBar.Visibility = Visibility.Hidden;
            txtMessage.Text = r.Output;
            txtStatus.Text = "Done";
        }
        private async Task<bool> Connect()
        {
            var r = await Task.Run(() => {
                    return mController.Connect("192.168.100.201", "sant", "sant");
                });
            return r;
        }
        private async Task<ReturnBox> ls()
        {
            return await Task.Run(() => mController.RunRemote("ls -l"));            
        }
        private async Task<ReturnBox> GetGoldDrives()
        {
            return await Task.Run(() => mController.RunLocal("net use"));
        }

        private async void Connect_Click(object sender, RoutedEventArgs e)
        {
            WorkStart();
            txtStatus.Text = "Loging in...";
            bool connected = await Connect();
            if (connected)
                WorkProgress("connected");
            await Task.Delay(2000);
            var r1 = await ls();
            WorkEnd(r1);
            await Task.Delay(2000);
            var r2 = await GetGoldDrives();
            WorkEnd(r2);
        }

    }
}
