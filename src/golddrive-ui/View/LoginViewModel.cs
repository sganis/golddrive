using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace golddrive
{
    public class LoginViewModel : BaseViewModel
    {
        #region Properties

        public ICommand LoginCommand { get; set; }
        public IMountManager MountManager { get; set; }
        public MainWindowViewModel MainWindowViewModel { get; set; }

        private string host;
        public string Host
        {
            get { return host; }
            set { host = value; NotifyPropertyChanged(); }
        }

        private string password;
        public string Password
        {
            get { return password; }
            set { password = value; NotifyPropertyChanged(); }
        }

        private string message;
        public string Message
        {
            get { return message; }
            set { message = value; NotifyPropertyChanged(); }
        }

        #endregion

        public LoginViewModel(MainWindowViewModel mainWindow, IMountManager mountManager)
        {
            LoginCommand = new BaseCommand(Login);
            IsWorking = false;
            Host = "192.168.100.201";
            MountManager = mountManager;
            MainWindowViewModel = mainWindow;
        }

        private async void Login(object obj)
        {
            IsWorking = true;
            //App.MainWindowViewModel.IsWorking = true;

            // parse host: user@host:port
            // get user
            // get pkey path
            string user = "sant";
            int port = 22;
            string pkey = $@"C:\Users\sant\.ssh\id_rsa-{user}-golddrive";
            if (!MountManager.Connected)
            {
                var r = await Task.Run(() => MountManager.Connect(host, port, user, password, pkey));
                if(!r)
                {
                    Message = MountManager.Error;

                }
            }
            if (MountManager.Connected)
            {
                var r = await MountManager.ls();
                Message = r.Output;
                await Task.Delay(2000);
                MainWindowViewModel.ShowConnectCommand.Execute(null);                
            }
            MainWindowViewModel.IsWorking = false;
            IsWorking = false;

        }
    }
}
