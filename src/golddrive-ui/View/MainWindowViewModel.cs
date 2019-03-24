using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace golddrive
{
    public class MainWindowViewModel : Observable
    {
        #region Properties

        public ICommand ShowMainCommand { get; set; }
        public ICommand ShowLoginCommand { get; set; }
        public ICommand ShowSettingsCommand { get; set; }
        public ICommand ShowAboutCommand { get; set; }
        public ICommand LoadedCommand { get; set; }
        public ICommand ClosingCommand { get; set; }
        public ICommand ConnectCommand { get; set; }
        public ICommand ConnectHostCommand { get; set; }
        public ICommand CancelHostCommand { get; set; }
        public ICommand CheckDriveStatusCommand { get; set; }

        public ICommand SaveSettingsCommand { get; set; }
        public ICommand CancelDriveNewCommand { get; set; }

        private MountService _mountService;

        private Page _currentPage;
        public Page CurrentPage
        {
            get { return _currentPage; }
            set { _currentPage = value; NotifyPropertyChanged();  }
        }
                
        
        private ObservableCollection<Drive> _drives;
        public ObservableCollection<Drive> Drives
        {
            get { return _drives; }
            set { _drives = value; NotifyPropertyChanged(); }
        }

        private ObservableCollection<Drive> _freeDrives;
        public ObservableCollection<Drive> FreeDrives
        {
            get { return _freeDrives; }
            set { _freeDrives = value; NotifyPropertyChanged(); }
        }

        private Drive _selectedDrive;
        public Drive SelectedDrive
        {
            get { return _selectedDrive; }
            set
            {
                _selectedDrive = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("HasDrives");
                NotifyPropertyChanged("HasNoDrives");

            }
        }
        private Drive _selectedFreeDrive;
        public Drive SelectedFreeDrive
        {
            get { return _selectedFreeDrive; }
            set {  _selectedFreeDrive = value;  NotifyPropertyChanged();  }
        }
        public bool HasDrives { get { return Drives != null && Drives.Count > 0; } }
        public bool HasNoDrives { get { return !HasDrives; } }

        //public bool CanConnect { get { return IsDriveSelected && !IsWorking; } }


        private bool _isWorking;
        public bool IsWorking
        {
            get { return _isWorking; }
            set
            {
                _isWorking = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("CanConnect");
            }
        }

        private string _connectButtonText;
        public string ConnectButtonText
        {
            get { return _connectButtonText; }
            set { _connectButtonText = value; NotifyPropertyChanged(); }
        }

        private string message;
        public string Message
        {
            get { return message; }
            set { message = value; NotifyPropertyChanged(); }
        }
        private string _newMountPoint;
        public string NewMountPoint
        {
            get { return _newMountPoint; }
            set { _newMountPoint = value; NotifyPropertyChanged(); }
        }
        

        public bool IsDriveSelected { get { return SelectedDrive != null; } }
        
        #endregion


        public MainWindowViewModel()
        {
            _mountService = new MountService();            
            //Messenger.Default.Register<string>(this, OnShowView);
            ShowMainCommand = new BaseCommand(OnShowMain);
            ShowLoginCommand = new BaseCommand(OnShowLogin);
            ShowSettingsCommand = new BaseCommand(OnShowSettings);
            ShowAboutCommand = new BaseCommand(OnShowAbout);
            SaveSettingsCommand = new BaseCommand(OnSaveSettings);
            LoadedCommand = new BaseCommand(Loaded);
            ClosingCommand = new BaseCommand(Closing);
            ConnectCommand = new BaseCommand(OnConnect);
            ConnectHostCommand = new BaseCommand(OnConnectHost);
            CancelHostCommand = new BaseCommand(OnCancelHost);
            CheckDriveStatusCommand = new BaseCommand(OnCheckDriveStatus);
            ConnectButtonText = "Connect";
            Message = "";
            CurrentPage = Page.Main;
        }

        public void LoadDrives()
        {
            List<Drive> drives = new List<Drive>();
            List<Drive> freeDrives =  new List<Drive>();
            Task.Factory.StartNew(() =>
            {
                drives = _mountService.GetGoldDrives();
               freeDrives = _mountService.GetFreeDrives();

            }).ContinueWith(t =>
            {
                Drives = new ObservableCollection<Drive>();
                FreeDrives = new ObservableCollection<Drive>();
                foreach (var d in drives)
                    Drives.Add(d);
                foreach (var d in freeDrives)
                    FreeDrives.Add(d);
                if (FreeDrives.Count > 0)
                {
                    SelectedFreeDrive = FreeDrives[freeDrives.Count-1];
                }
                if (Drives.Count > 0)
                {
                    SelectedDrive = Drives[0];
                }
                else
                {
                    CurrentPage = Page.Host;
                }
            }, TaskScheduler.FromCurrentSynchronizationContext());

            
        }
        private void Delete(object obj)
        {
            _mountService.Unmount(SelectedDrive);
            Drives.Remove(SelectedDrive);
            _mountService.SaveSettingsDrives(Drives.ToList());
            if (Drives.Count > 0)
                SelectedDrive = Drives[0];
        }

        
        
        private void Connect()
        {
            Message = "Connecting...";
            IsWorking = true;            
            Task.Factory.StartNew(() =>  {
                return _mountService.Connect(SelectedDrive);
            }).ContinueWith(t => {
                ReturnBox r = (ReturnBox)t.Result;
                MountStatus status = (MountStatus)r.Object;
                switch (status)
                {
                    case MountStatus.BAD_DRIVE:
                        Message = r.Error;
                        break;
                    case MountStatus.BAD_HOST:
                        Message = r.Error;
                        break;
                    case MountStatus.BAD_LOGIN:
                        Message = r.Error;
                        break;
                    case MountStatus.BAD_MOUNT:
                        Message = r.Error;
                        break;
                    case MountStatus.BAD_WINFSP:
                        Message = r.Error;
                        break;
                    default:
                        Message = DriveStatus.CONNECTED.ToString();
                        ConnectButtonText = "Disconnect";
                        break;                       
                }                
            }, TaskScheduler.FromCurrentSynchronizationContext());
            IsWorking = false;
        }
        private void Disconnect()
        {
            IsWorking = true;
            Message = "Disconnecting...";
            Task.Factory.StartNew(() => {
                return _mountService.Unmount(SelectedDrive);
            }).ContinueWith(t => {
                ReturnBox r = (ReturnBox)t.Result;
                if (r.Success) {
                    ConnectButtonText = "Connect";
                    Message = DriveStatus.DISCONNECTED.ToString();
                }
                else {
                    Message = r.Error;
                }
            }, TaskScheduler.FromCurrentSynchronizationContext());
            IsWorking = false;
        }
        private void CheckDriveStatus()
        {
            IsWorking = true;
            Message = $"Checking status...";
            Task.Factory.StartNew(() => {
                return _mountService.CheckDriveStatus(SelectedDrive);
            }).ContinueWith(t => {
                DriveStatus status = (DriveStatus)t.Result;
                Message = status.ToString();
                if (status == DriveStatus.CONNECTED)
                {
                    ConnectButtonText = "Disconnect";
                }
                else if(status == DriveStatus.DISCONNECTED)
                {
                    ConnectButtonText = "Connect";
                }
                else
                {

                }
            }, TaskScheduler.FromCurrentSynchronizationContext());
            IsWorking = false;
        }

        private void OnCheckDriveStatus(object obj)
        {
            CheckDriveStatus();
        }
        private void OnConnect(object obj)
        {
            if (Drives.Count == 0)
            {
                CurrentPage = Page.Host;
            }
            else
            {
                if (ConnectButtonText == "Connect")
                {
                    Connect();
                }
                else 
                {
                    Disconnect();
                }
            }
        }
        private void OnConnectHost(object obj)
        {
            SelectedFreeDrive.MountPoint = NewMountPoint;
            Drives.Add(SelectedFreeDrive);
            SelectedDrive = SelectedFreeDrive;
            Message = "";
            CurrentPage = Page.Main;
            Connect();
        }
        private void OnCancelHost(object obj)
        {
            CurrentPage = Page.Main;
        }
        private void OnShowLogin(object obj)
        {
            CurrentPage = Page.Login;
        }
        private void OnShowMain(object obj)
        {
            CurrentPage = Page.Main;
        }
        private void OnShowSettings(object obj)
        {
            CurrentPage = Page.Settings;
        }
        private void OnShowAbout(object obj)
        {
            CurrentPage = Page.About;
        }
        private void OnSaveSettings(object obj)
        {
            CurrentPage = Page.Main;
        }
        private void Loaded(object obj)
        {
            LoadDrives();

        }
        private void Closing(object obj)
        {
           // _mountService.SaveSettingsDrives(Drives.ToList());
        }

        private async void Login(object obj)
        {
            IsWorking = true;
            string user = "sant";
            string password = "";
            int port = 22;
            string host = "192.168.100.201";
            string pkey = $@"C:\Users\sant\.ssh\id_rsa-{user}-golddrive";
            if (!_mountService.Connected)
            {
                var r = await Task.Run(() => _mountService.Connect(host, port, user, password, pkey));
                if (!r)
                {
                    Message = _mountService.Error;

                }
            }
            if (_mountService.Connected)
            {
                Message = await Task.Run(() => _mountService.GetUid(user));
                //await Task.Delay(2000);
                CurrentPage = Page.Main;
            }
            IsWorking = false;

        }
    }

    
}
