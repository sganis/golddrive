using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Media;

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
        public ICommand ConnectPasswordCommand { get; set; }
        public ICommand CancelPasswordCommand { get; set; }
        public ICommand DriveChangedCommand { get; set; }
        public ICommand SaveSettingsCommand { get; set; }
        //public ICommand CancelDriveNewCommand { get; set; }

        private MountService _mountService;

        private Page _currentPage;
        public Page CurrentPage
        {
            get { return _currentPage; }
            set { _currentPage = value; NotifyPropertyChanged(); }
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
        public Drive OldSelectedDrive { get; set; }

        private Drive _selectedFreeDrive;
        public Drive SelectedFreeDrive
        {
            get { return _selectedFreeDrive; }
            set { _selectedFreeDrive = value; NotifyPropertyChanged(); }
        }
        public bool HasDrives { get { return Drives != null && Drives.Count > 0; } }
        public bool HasNoDrives { get { return !HasDrives; } }

        //public bool CanConnect { get { return IsDriveSelected && !IsWorking; } }

        public DriveStatus DriveStatus { get; set; }
        
        private MountStatus _mountStatus;
        public MountStatus MountStatus
        {
            get { return _mountStatus; }
            set { _mountStatus = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("MessageColor");
            }
        }

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
        public Brush MessageColor
        {
            get { return MountStatus == MountStatus.OK ?
                    Brushes.Black: Brushes.Red;  }
        }
        private string password;
        public string Password
        {
            get { return password; }
            set { password = value; NotifyPropertyChanged(); }
        }
        private string _newMountPoint;
        public string NewMountPoint
        {
            get { return _newMountPoint; }
            set { _newMountPoint = value; NotifyPropertyChanged(); }
        }


        public bool IsDriveSelected { get { return SelectedDrive != null; } }

        #endregion

        #region Constructor
        public MainWindowViewModel()
        {
            _mountService = new MountService();
            //Messenger.Default.Register<string>(this, OnShowView);
            InitCommands();
            ConnectButtonText = "Connect";
            Message = "";
            CurrentPage = Page.Main;
        }

        private void InitCommands()
        {
            ShowMainCommand = new BaseCommand(OnShowMain);
            ShowLoginCommand = new BaseCommand(OnShowLogin);
            ShowSettingsCommand = new BaseCommand(OnShowSettings);
            ShowAboutCommand = new BaseCommand(OnShowAbout);
            SaveSettingsCommand = new BaseCommand(OnSaveSettings);
            LoadedCommand = new BaseCommand(Loaded);
            ClosingCommand = new BaseCommand(Closing);
            ConnectCommand = new BaseCommand(OnConnect);
            ConnectHostCommand = new BaseCommand(OnConnectHost);
            ConnectPasswordCommand = new BaseCommand(OnConnectPassword);
            CancelHostCommand = new BaseCommand(OnCancelHost);
            CancelPasswordCommand = new BaseCommand(OnCancelPassword);
            DriveChangedCommand = new BaseCommand(OnDriveChanged);
        }

        #endregion

        #region Async Methods

        public async void LoadDrivesAsync()
        {
            List<Drive> drives = new List<Drive>();
            List<Drive> freeDrives = new List<Drive>();
            await Task.Run(() => {
                drives = _mountService.GetGoldDrives();
                freeDrives = _mountService.GetFreeDrives();
            });
            Drives = new ObservableCollection<Drive>(drives);
            FreeDrives = new ObservableCollection<Drive>(freeDrives);
            if (FreeDrives.Count > 0)
                SelectedFreeDrive = FreeDrives[freeDrives.Count - 1];
            if (Drives.Count > 0)
                SelectedDrive = Drives[0];
            else
                CurrentPage = Page.Host;
        }
        private void Delete(object obj)
        {
            _mountService.Unmount(SelectedDrive);
            Drives.Remove(SelectedDrive);
            _mountService.SaveSettingsDrives(Drives.ToList());
            if (Drives.Count > 0)
                SelectedDrive = Drives[0];
        }

        private void WorkStart(string message)
        {
            Message = message;
            IsWorking = true;
        }
        private void WorkDone(ReturnBox r)
        {
            Message = r.Error;
            MountStatus = r.MountStatus;
            switch (r.MountStatus)
            {
                case MountStatus.BAD_DRIVE:
                case MountStatus.BAD_HOST:
                    CurrentPage = Page.Host;
                    Message = r.Error;
                    break;
                case MountStatus.BAD_LOGIN:
                    CurrentPage = Page.Login;
                    Message = r.Error;
                    break;
                case MountStatus.BAD_WINFSP:
                    Message = "Winfsp is not installed\n";
                    break;
                case MountStatus.OK:
                    CurrentPage = Page.Main;
                    Message = r.DriveStatus.ToString();
                    if (r.DriveStatus == DriveStatus.CONNECTED)
                    {
                        ConnectButtonText = "Disconnect";
                    }
                    else if (r.DriveStatus == DriveStatus.DISCONNECTED)
                    {
                        ConnectButtonText = "Connect";
                    }
                    else
                    {
                        // repair ?
                    }
                    break;
                default:
                    //Message = DriveStatus.UNKNOWN.ToString();
                    ConnectButtonText = "Connect";
                    break;
            }
            IsWorking = false;

        }
        private async void ConnectAsync()
        {
            WorkStart("Connecting");
            ReturnBox r = await Task.Run(() => _mountService.Connect(SelectedDrive));
            WorkDone(r);
        }

        private async void CheckDriveStatusAsync()
        {
            WorkStart("Checking status...");
            DriveStatus status = await Task.Run(() => _mountService.CheckDriveStatus(SelectedDrive));
            Message = status.ToString();
            if (status == DriveStatus.CONNECTED)
            {
                ConnectButtonText = "Disconnect";
            }
            else if (status == DriveStatus.DISCONNECTED)
            {
                ConnectButtonText = "Connect";
            }
            else
            {

            }
            IsWorking = false;
        }


        #endregion

        #region Command Methods

        private void OnDriveChanged(object obj)
        {
            if(SelectedDrive != OldSelectedDrive)
            {
                OldSelectedDrive = SelectedDrive;
                CheckDriveStatusAsync();
            }
                
        }
        private async void OnConnect(object obj)
        {
            if (Drives.Count == 0 || string.IsNullOrEmpty(SelectedDrive.Host))
            {
                CurrentPage = Page.Host;
                return;
            }
            if (ConnectButtonText == "Connect")
            {
                ConnectAsync();
            }
            else
            {
                WorkStart("Disconnecting...");
                ReturnBox r = await Task.Run(() => _mountService.Unmount(SelectedDrive));
                WorkDone(r);
            }

        }
        private void OnConnectHost(object obj)
        {
            SelectedFreeDrive.MountPoint = NewMountPoint;
            Drives.Add(SelectedFreeDrive);
            SelectedDrive = SelectedFreeDrive;
            if (string.IsNullOrEmpty(SelectedDrive.Host))
            {
                Message = "Enter host or mount point";
                return;
            }
            ConnectAsync();
        }
        private async void OnConnectPassword(object obj)
        {
            CurrentPage = Page.Main;
            WorkStart("Connecting");
            ReturnBox r = await Task.Run(() => _mountService.ConnectPassword(SelectedDrive, password));
            WorkDone(r);
        }
        private void OnCancelHost(object obj)
        {
            CurrentPage = Page.Main;
        }
        private void OnCancelPassword(object obj)
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

        #endregion

        #region Events
        private void Loaded(object obj)
        {
            //LoadDrives();
            LoadDrivesAsync();
        }
        private void Closing(object obj)
        {
            // _mountService.SaveSettingsDrives(Drives.ToList());
        }

        #endregion


    }


}

