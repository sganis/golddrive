using MahApps.Metro.Controls;
using Renci.SshNet;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.RegularExpressions;
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
    public partial class MainWindow : MetroWindow, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private void NotifyPropertyChanged([CallerMemberName] String propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        bool mIsLoginOpen;
        public bool IsLoginOpen
        {
            get { return mIsLoginOpen; }
            set
            {
                if (value != this.mIsLoginOpen)
                {
                    mIsLoginOpen = value;
                    NotifyPropertyChanged();
                }
            }
        }

        // constructor
        public MainWindow()
        {
            InitializeComponent();
            App.Controller = new Controller(this);
            progressBar.Visibility = Visibility.Hidden;
            DataContext = this;
        }

        void WorkStart(string msg = "")
        {
            btnConnect.IsEnabled = false;
            progressBar.Visibility = Visibility.Visible;
            txtMessage.Text = msg;
        }
        void WorkProgress(string message)
        {
            txtMessage.Text = message;
        }
        void WorkEnd(ReturnBox r)
        {
            btnConnect.IsEnabled = true;
            progressBar.Visibility = Visibility.Hidden;
            txtMessage.Text = r.Output;
            //txtStatus.Text = "Done";
        }

        private async Task<ReturnBox> ls()
        {
            return await Task.Run(() => App.Controller.RunRemote("ls -l"));
        }


        private async void Connect_Click(object sender, RoutedEventArgs e)
        {
            WorkStart();
            //if (!App.Controller.Connected)
            //{
            //    txtStatus.Text = "Loging in...";

            //}
            if (App.Controller.Connected)
            {
                WorkProgress("connected");
                var r1 = await ls();
                WorkEnd(r1);
                await Task.Delay(1000);
            }

            var t = new Stopwatch();
            t.Start();
            var r2 = await App.Controller.GetGoldDrives();
            //WorkEnd(r2);
            txtMessage.Text = t.ElapsedMilliseconds.ToString();
            WorkEnd(r2);
        }

        private void Settings_Click(object sender, RoutedEventArgs e)
        {
            // setup ssh
            //GenerateKeys();
            TransferPublicKey();
            //TestSsh();
        }

        private void TestSsh()
        {
            throw new NotImplementedException();
        }

        private void TransferPublicKey()
        {
            App.Controller.UploadFile(
                @"D:\Users\ganissa\.ssh\id_rsa-ganissa-golddrive.pub",
                ".ssh",
                "id_rsa-ganissa-golddrive.pub");
        }

        private void Login_Click(object sender, RoutedEventArgs e)
        {
            IsLoginOpen = true;
        }
    }
}