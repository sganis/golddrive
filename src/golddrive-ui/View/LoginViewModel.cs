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
        public ICommand CancelCommand { get; set; }

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

        public LoginViewModel(MainWindowViewModel mainWindow, IDriver driver)
        {
            LoginCommand = new BaseCommand(Login);
            CancelCommand = new BaseCommand(Cancel);
            IsWorking = false;
            Host = "192.168.100.201";
            _driver = driver;
            _mainViewModel = mainWindow;
        }

        private async void Login(object obj)
        {
            IsWorking = true;
            _mainViewModel.IsWorking = true;

            // parse host: user@host:port
            // get user
            // get pkey path
            string user = "sant";
            int port = 22;
            string pkey = $@"C:\Users\sant\.ssh\id_rsa-{user}-golddrive";
            if (!_driver.Connected)
            {
                var r = await Task.Run(() => _driver.Connect(host, port, user, password, pkey));
                if (!r)
                {
                    Message = _driver.Error;

                }
            }
            if (_driver.Connected)
            {
                Message = await Task.Run(() => _driver.GetUid(user));
                //await Task.Delay(2000);
                _mainViewModel.ShowConnectCommand.Execute(null);
            }
            _mainViewModel.IsWorking = false;
            IsWorking = false;

        }
        private void Cancel(object obj)
        {
            _mainViewModel.ShowConnectCommand.Execute(null);
        }
    }
}
