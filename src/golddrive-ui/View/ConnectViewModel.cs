using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace golddrive_ui
{
    public class ConnectViewModel : BaseViewModel
    {
        public ICommand ConnectCommand { get; set; }
        public ICommand CheckDriveStatusCommand { get; set; }
        
        private string buttonText;
        public string ButtonText
        {
            get { return buttonText; }
            set { buttonText = value; NotifyPropertyChanged();  }
        }

        private string message;

        public string Message
        {
            get { return message; }
            set { message = value; NotifyPropertyChanged();  }
        }


        private ObservableCollection<Drive> _drives;
        public ObservableCollection<Drive> Drives
        {
            get { return _drives; }
            set { _drives = value; }
        }
        private Drive drive;
        public Drive Drive
        {
            get { return drive; }
            set { drive = value; }
        }

        public ConnectViewModel()
        {
            ConnectCommand = new BaseCommand(Connect);
            CheckDriveStatusCommand = new BaseCommand(CheckDriveStatus);
            IsWorking = false;
            ButtonText = "Connect";

            Drives = new ObservableCollection<Drive>()
            {
                new Drive() { Letter="S", Name="Simulation" },
                new Drive() { Letter="R", Name="Research" },
                new Drive() { Letter="T", Name="Training" }
            };
            Drive = Drives[0];
        }

        private async void Connect(object obj)
        {
            IsWorking = true;
            App.MainWindowViewModel.IsWorking = true;
            ButtonText = "Connecting...";
            // get connection params
            // try to connect
            await Task.Delay(1000);
            // if failure, show login            
            App.MainWindowViewModel.IsWorking = false;
            ButtonText = "Disconnect";
            IsWorking = false;
            App.MainWindowViewModel.ShowLoginCommand.Execute(null);
            
        }
        private async void CheckDriveStatus(object obj)
        {
            IsWorking = true;
            App.MainWindowViewModel.IsWorking = true;
            Message = $"Checking {drive.DriveDisplay}...";
            // get connection params
            // try to connect
            await Task.Delay(1000);
            Message = "CONNECTED";
            // if failure, show login            
            //App.MainWindowViewModel.ShowLoginCommand.Execute(null);
            App.MainWindowViewModel.IsWorking = false;
            //ButtonText = "Disconnect";
            IsWorking = false;

        }
    }
}
