using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Media;

namespace golddrive
{
    public class MainWindowViewModel : Observable
    {
        #region Properties

        private MountService _mountService;

        private string _version;

        public string Version
        {
            get { return _version; }
            set { _version = value; NotifyPropertyChanged(); }
        }
        
        private bool _isDriveNew;
        public bool IsDriveNew
        {
            get { return _isDriveNew; }
            set { _isDriveNew = value; NotifyPropertyChanged(); }
        }

        private Page _currentPage;
        public Page CurrentPage
        {
            get { return _currentPage; }
            set { _currentPage = value; NotifyPropertyChanged(); }
        }

        private string _globalArgs;

        public string GlobalArgs
        {
            get { return _globalArgs; }
            set { _globalArgs = value; NotifyPropertyChanged(); }
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
            CurrentPage = Page.Main;
            LoadDrivesAsync();
            GetVersions();
        }

        #endregion

        #region Async Methods

        private void WorkStart(string message)
        {
            Message = message;
            IsWorking = true;
        }
        private void WorkDone(ReturnBox r=null)
        {
            IsWorking = false;
            if (r == null)
            {
                Message = "";
                return;
            }
            MountStatus = r.MountStatus;
            switch (r.MountStatus)
            {
                case MountStatus.BAD_DRIVE:
                case MountStatus.BAD_HOST:
                    CurrentPage = Page.Host;
                    Message = r.Error;
                    break;
                case MountStatus.BAD_LOGIN:
                    CurrentPage = Page.Password;
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
        public async void LoadDrivesAsync()
        {
            ConnectButtonText = "Connect";
            WorkStart("Exploring local drives...");
            List<Drive> drives = new List<Drive>();
            List<Drive> freeDrives = new List<Drive>();
            await Task.Run(() =>
            {
                Settings settings = _mountService.LoadSettings();
                GlobalArgs = settings.Args;
                drives = _mountService.GetGoldDrives(settings.Drives.Values.ToList());
                freeDrives = _mountService.GetFreeDrives();
            });
            Drives = new ObservableCollection<Drive>(drives);
            FreeDrives = new ObservableCollection<Drive>(freeDrives);
            if (FreeDrives.Count > 0)
                SelectedFreeDrive = FreeDrives[0];
            if (Drives.Count > 0)
            {
                SelectedDrive = Drives[0];
                CheckDriveStatusAsync();
            }
            else
            {
                CurrentPage = Page.Host;
                WorkDone();
            }
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
            ReturnBox r = await Task.Run(() => _mountService.CheckDriveStatus(SelectedDrive));
            WorkDone(r);
        }

        private async void GetVersions()
        {
            Version = await Task.Run(() => _mountService.GetVersions());            
        }

        #endregion

        #region Commands

        //private ICommand _cmd;
        //public ICommand Cmd
        //{
        //    get
        //    {
        //        if (_cmd == null)
        //        {
        //            _cmd = new RelayCommand(
        //                // action
        //                x =>
        //                {

        //                },
        //                // can execute
        //                x =>
        //                {
        //                    return true;
        //                });
        //        }
        //        return _cmd;
        //    }
        //}




        private ICommand _selectedDriveChangedCommand;
        public ICommand SelectedDriveChangedCommand
        {
            get
            {
                return _selectedDriveChangedCommand ?? (_selectedDriveChangedCommand = new RelayCommand(
                   // action
                   x => {
                       if (CurrentPage != Page.Main)
                           return;
                       if (OldSelectedDrive == null)
                           OldSelectedDrive = SelectedDrive;
                       if (SelectedDrive != OldSelectedDrive)
                       {
                           OldSelectedDrive = SelectedDrive;
                           CheckDriveStatusAsync();
                       }
                   }));
            }
        }
        private ICommand _connectCommand;
        public ICommand ConnectCommand
        {
            get
            {
                return _connectCommand ?? 
                    (_connectCommand = new RelayCommand(OnConnect));
            }
        }
        private async void OnConnect(object obj)
        {
            if (Drives.Count == 0 || string.IsNullOrEmpty(SelectedDrive.Host))
            {
                if (SelectedDrive != null)
                {
                    FreeDrives.Add(SelectedDrive);
                    SelectedFreeDrive = SelectedDrive;
                }
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

        private ICommand _connectHostCommand;
        public ICommand ConnectHostCommand
        {
            get
            {
                return _connectHostCommand ??
                    (_connectHostCommand = new RelayCommand(OnConnectHost));
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
        private ICommand _showPageCommand;
        public ICommand ShowPageCommand
        {
            get
            {
                return _showPageCommand ??
                    (_showPageCommand = new RelayCommand(
                        x => {
                            CurrentPage = (Page)x;
                        }
                        ));
            }
        }
        private ICommand _connectPasswordCommand;
        public ICommand ConnectPasswordCommand
        {
            get
            {
                return _connectPasswordCommand ??
                    (_connectPasswordCommand = new RelayCommand(OnConnectPassword));
            }
        }
        private async void OnConnectPassword(object obj)
        {
            CurrentPage = Page.Main;
            WorkStart("Connecting");
            ReturnBox r = await Task.Run(() => _mountService.ConnectPassword(SelectedDrive, password));
            WorkDone(r);
        }
       
        private ICommand _showPasswordCommand;
        public ICommand ShowLoginCommand
        {
            get
            {
                return _showPasswordCommand ??
                    (_showPasswordCommand = new RelayCommand(
                        x => { CurrentPage = Page.Password; }));
            }
        }
 
        private ICommand _settingsOkCommand;
        public ICommand SettingsOkCommand
        {
            get
            {
                return _settingsOkCommand ??
                    (_settingsOkCommand = new RelayCommand(OnSettingsSave));
            }
        }
        
        private void OnSettingsSave(object obj)
        {            
            if (IsDriveNew)
            {
                Drives.Add(SelectedFreeDrive);
                SelectedDrive = SelectedFreeDrive;
                FreeDrives.Remove(SelectedFreeDrive);
                if (FreeDrives.Count > 0)
                    SelectedFreeDrive = FreeDrives[0];
                IsDriveNew = false;
            }
            else
            {
                CurrentPage = Page.Main;
                CheckDriveStatusAsync();
                GlobalArgs.ToString();
            }
        }

        private ICommand _settingsNewCommand;
        public ICommand SettingsNewCommand
        {
            get
            {
                return _settingsNewCommand ??
                    (_settingsNewCommand = new RelayCommand(OnSettingsNew));
            }
        }
        private void OnSettingsNew(object obj)
        {
            IsDriveNew = true;
        }

        private ICommand _settingsCancelCommand;
        public ICommand SettingsCancelCommand
        {
            get
            {
                return _settingsCancelCommand ??
                    (_settingsCancelCommand = new RelayCommand(OnSettingsCancel));
            }
        }
        private void OnSettingsCancel(object obj)
        {
            IsDriveNew = false;
        }

        private ICommand _settingsDeleteCommand;
        public ICommand SettingsDeleteCommand
        {
            get
            {
                return _settingsDeleteCommand ?? (_settingsDeleteCommand = new RelayCommand(
                   // action
                   x => {
                       if (Drives.Contains(SelectedDrive))
                           Drives.Remove(SelectedDrive);
                       if (Drives.Count > 0)
                           SelectedDrive = Drives[0];
                   },
                   // can execute
                   x => {
                       return Drives != null && Drives.Count > 0;
                   }));
            }
        }


        private ICommand _closingCommand;
        public ICommand ClosingCommand
        {
            get
            {
                return _closingCommand ??
                    (_closingCommand = new RelayCommand(Closing));
            }
        }
        private void Closing(object obj)
        {
            if (Drives != null)
            {
                Settings settings = new Settings
                {
                    Args = GlobalArgs
                };
                settings.AddDrives(Drives.ToList());
                _mountService.SaveSettingsDrives(settings);
            }
        }

        private ICommand _githubCommand;
        public ICommand GithubCommand
        {
            get
            {
                return _githubCommand ??
                    (_githubCommand = new RelayCommand(
                        url => System.Diagnostics.Process.Start(url.ToString())));
            }
        }      

        private ICommand _runTerminalCommand;
        public ICommand RunTerminalCommand
        {
            get
            {
                return _runTerminalCommand ??
                    (_runTerminalCommand = new RelayCommand(
                        url => System.Diagnostics.Process.Start("cmd.exe")));
            }
        }

        #endregion

    }


}

