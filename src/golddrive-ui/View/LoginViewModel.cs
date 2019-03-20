using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace golddrive_ui
{
    public class LoginViewModel : BaseViewModel
    {
        #region Properties

        public ICommand LoginCommand { get; set; }

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

        public LoginViewModel()
        {
            LoginCommand = new BaseCommand(Login);
            IsWorking = false;
            Host = "192.168.100.201";
        }

        private async void Login(object obj)
        {
            IsWorking = true;
            App.MainWindowViewModel.IsWorking = true;

            // parse host: user@host:port
            // get user
            // get pkey path
            string user = "sant";
            int port = 22;
            string pkey = $@"C:\Users\sant\.ssh\id_rsa-{user}-golddrive";
            if (!App.Controller.Connected)
            {
                var r = await Task.Run(() => App.Controller.Connect(host, port, user, password, pkey));
                if(!r)
                {
                    Message = App.Controller.Error;

                }
            }
            if (App.Controller.Connected)
            {
                var r = await App.Controller.ls();
                Message = r.Output;
                await Task.Delay(2000);
                App.MainWindowViewModel.ShowConnectCommand.Execute(null);                
            }
            App.MainWindowViewModel.IsWorking = false;
            IsWorking = false;

        }
    }
}
