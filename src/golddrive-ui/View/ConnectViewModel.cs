using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace golddrive
{
    public class ConnectViewModel : BaseViewModel
    {
        public ICommand ConnectCommand { get; set; }
        public ICommand CheckDriveStatusCommand { get; set; }
        public ICommand SettingsCommand { get; set; }
        
        

        private string buttonText;
        public string ButtonText
        {
            get { return buttonText; }
            set { buttonText = value; NotifyPropertyChanged(); }
        }

        private string message;
        public string Message
        {
            get { return message; }
            set { message = value; NotifyPropertyChanged(); }
        }
        private string host;
        public string Host
        {
            get { return host; }
            set { host = value; NotifyPropertyChanged(); }
        }

        
        public bool IsDriveSelected { get { return Driver.SelectedDrive != null; } }
        
        public ConnectViewModel(
            MainWindowViewModel mainWindowViewModel,
            IDriver driver)
        {
            Driver = driver;
            _mainViewModel = mainWindowViewModel;
            ConnectCommand = new BaseCommand(Connect);
            CheckDriveStatusCommand = new BaseCommand(CheckDriveStatus);
            SettingsCommand = new BaseCommand(OpenSettings);
            IsWorking = false;
            ButtonText = "Connect";

             
        }
        
        private async void Connect(object obj)
        {
            IsWorking = true;
            _mainViewModel.IsWorking = true;
            if(Host != null)
                Driver.SelectedDrive.MountPoint = Host;
            ReturnBox r;
            if (ButtonText == "Connect")
            {
                Message = "Connecting...";
                await Task.Delay(500);
                r = await Task.Run(() => Driver.Connect(Driver.SelectedDrive));
            }                
            else if (ButtonText == "Disconnect")
            {
                Message = "Disconnecting...";
                await Task.Delay(500);
                r = await Task.Run(() => Driver.Unmount(Driver.SelectedDrive));
            }
            else
            {

            }
            // fixme
            Drive d = Driver.SelectedDrive;
            Driver.SelectedDrive = null;
            Driver.SelectedDrive = d;

            CheckDriveStatus(null);
            //_mainViewModel.ShowLoginCommand.Execute(null);
        }       

        private async void CheckDriveStatus(object obj)
        {
            IsWorking = true;
            _mainViewModel.IsWorking = true;
            Message = $"Checking status...";
            await Task.Delay(500);
            var r = await Task.Run(()=> Driver.CheckDriveStatus(Driver.SelectedDrive));
            if(r == DriveStatus.CONNECTED)
            {
                ButtonText = "Disconnect";
            } else if (r == DriveStatus.DISCONNECTED)
            {
                ButtonText = "Connect";
            }
            else
            {

            }
            Message = r.ToString();

            //_mainViewModel.ShowLoginCommand.Execute(null);
            _mainViewModel.IsWorking = false;
            IsWorking = false;

        }
        private void OpenSettings(object obj)
        {
            _mainViewModel.ShowSettingsCommand.Execute(null);
        }
    }
}
