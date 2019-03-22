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
        public bool HasDrives
        {
            get { return Drives != null && Drives.Count > 0; }
        }
        public bool HasNoDrives
        {
            get { return !HasDrives; }
        }

        private ObservableCollection<Drive> _drives;
        public ObservableCollection<Drive> Drives
        {
            get { return _drives; }
            set {  _drives = value;  NotifyPropertyChanged();    }
        }
        private ObservableCollection<Drive> _freeDrives;
        public ObservableCollection<Drive> FreeDrives
        {
            get { return _freeDrives; }
            set { _freeDrives = value; NotifyPropertyChanged(); }
        }

        private Drive drive;
        public Drive Drive
        {
            get { return drive; }
            set {
                drive = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("CanConnect");
                NotifyPropertyChanged("HasDrives");
                NotifyPropertyChanged("HasNoDrives");
            }
        }
        public bool IsDriveSelected { get { return drive != null; } }
        public bool CanConnect { get { return IsDriveSelected && !IsWorking; } }

        public ConnectViewModel(
            MainWindowViewModel mainWindowViewModel,
            IDriver driver)
        {
            _driver = driver;
            _mainViewModel = mainWindowViewModel;
            ConnectCommand = new BaseCommand(Connect);
            CheckDriveStatusCommand = new BaseCommand(CheckDriveStatus);
            SettingsCommand = new BaseCommand(OpenSettings);
            Drives = new ObservableCollection<Drive>();
            FreeDrives = new ObservableCollection<Drive>();
            IsWorking = false;
            ButtonText = "Connect";

            LoadDrives();
        }
        private async void LoadDrives()
        {
            List<Drive> drives = await Task.Run(() => _driver.GetGoldDrives());
            List<Drive> freeDrives = await Task.Run(() => _driver.GetFreeDrives());

            foreach (var d in drives)
                Drives.Add(d);
            foreach (var d in freeDrives)
                FreeDrives.Add(d);

            if (Drives.Count > 0)
                Drive = Drives?[0];

        }
        private async void Connect(object obj)
        {
            IsWorking = true;
            _mainViewModel.IsWorking = true;
            if(Host != null)
                drive.Remote = Host;
            ReturnBox r;
            if (ButtonText == "Connect")
            {
                Message = "Connecting...";
                await Task.Delay(500);
                r = await Task.Run(() => _driver.Connect(Drive));
            }                
            else if (ButtonText == "Disconnect")
            {
                Message = "Disconnecting...";
                await Task.Delay(500);
                r = await Task.Run(() => _driver.Unmount(Drive));
            }
            else
            {

            }

            CheckDriveStatus(null);
            //_mainViewModel.ShowLoginCommand.Execute(null);
        }       

        private async void CheckDriveStatus(object obj)
        {
            IsWorking = true;
            _mainViewModel.IsWorking = true;
            Message = $"Checking status...";
            await Task.Delay(500);
            var r = await Task.Run(()=>_driver.CheckDriveStatus(drive));
            if(r == DriveStatus.CONNECTED)
            {
                Message = r.ToString();
                ButtonText = "Disconnect";
            } else if (r == DriveStatus.DISCONNECTED)
            {
                Message = r.ToString();
                ButtonText = "Connect";
            }
            else
            {

            }
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
