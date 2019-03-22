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
        public IMountManager MountManager { get; set; }
        public MainWindowViewModel MainWindowViewModel { get; set; }


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

        public ConnectViewModel(
            MainWindowViewModel mainWindowViewModel,
            IMountManager mountManager)
        {
            ConnectCommand = new BaseCommand(Connect);
            CheckDriveStatusCommand = new BaseCommand(CheckDriveStatus);
            IsWorking = false;
            ButtonText = "Connect";
            MountManager = mountManager;
            MainWindowViewModel = mainWindowViewModel;

            LoadDrives();
        }
        private void LoadDrives()
        {
            List<Drive> drives = MountManager.GetGoldDrives();
            Drives = new ObservableCollection<Drive>();
            foreach (var d in drives)
                Drives.Add(d);
            if (Drives.Count > 0)
                Drive = Drives[0];
        }
        private async void Connect(object obj)
        {
            IsWorking = true;
            MainWindowViewModel.IsWorking = true;
            ButtonText = "Connecting...";
            // get connection params
            // try to connect
            await Task.Delay(1000);
            // if failure, show login            
            MainWindowViewModel.IsWorking = false;
            ButtonText = "Disconnect";
            IsWorking = false;
            MainWindowViewModel.ShowLoginCommand.Execute(null);
            
        }
        private async void CheckDriveStatus(object obj)
        {
            IsWorking = true;
            MainWindowViewModel.IsWorking = true;
            Message = $"Checking {drive.DriveDisplay}...";
            // get connection params
            // try to connect
            await Task.Delay(1000);
            Message = "CONNECTED";
            // if failure, show login            
            MainWindowViewModel.ShowLoginCommand.Execute(null);
            MainWindowViewModel.IsWorking = false;
            //ButtonText = "Disconnect";
            IsWorking = false;

        }
    }
}
