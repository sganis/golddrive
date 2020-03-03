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
        //private int _selecteTabIndex;
        //public int SelectedTabIndex
        //{
        //    get { return _selecteTabIndex; }
        //    set
        //    {
        //        _selecteTabIndex = value;
        //        NotifyPropertyChanged();
        //        NotifyPropertyChanged("IsDrivesTabSelected");
        //    }
        //}
        //public bool IsDrivesTabSelected => SelectedTabIndex == 0;

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
            set {
                _isDriveNew = value; 
                NotifyPropertyChanged();
                NotifyPropertyChanged("SettingsOkButtonText");
            }
        }

        private Page _currentPage;
        public Page CurrentPage
        {
            get { return _currentPage; }
            set { _currentPage = value; NotifyPropertyChanged(); }
        }

        //private string _globalArgs;

        //public string GlobalArgs
        //{
        //    get { return _globalArgs; }
        //    set { _globalArgs = value; NotifyPropertyChanged(); }
        //}
        public ObservableCollection<Drive> GoldDrives { get; set; } = new ObservableCollection<Drive>();
        public ObservableCollection<Drive> FreeDrives { get; set; } = new ObservableCollection<Drive>();

        public string Selected => SelectedDrive != null ? SelectedDrive.Name : "";
        private Drive _selectedDrive;
        public Drive SelectedDrive
        {
            get { return _selectedDrive; }
            set
            {
                _selectedDrive = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("HasDrive");
                NotifyPropertyChanged("HasDrives");
            }
        }
        public Drive OldSelectedDrive { get; set; }

        private Drive _selectedFreeDrive;
        public Drive SelectedFreeDrive
        {
            get { return _selectedFreeDrive; }
            set { _selectedFreeDrive = value; NotifyPropertyChanged(); }
        }
        public bool HasDrive { get { return SelectedDrive != null; } }
        public bool HasDrives { get { return GoldDrives != null && GoldDrives.Count > 0; } }
        
        private DriveStatus driveStatus;
        public DriveStatus DriveStatus
        {
            get { return driveStatus; }
            set { driveStatus = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("ConnectButtonText");
                NotifyPropertyChanged("ConnectButtonColor");
            }
        }

        private MountStatus _mountStatus;
        public MountStatus MountStatus
        {
            get { return _mountStatus; }
            set { _mountStatus = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("MessageColor");
                NotifyPropertyChanged("ConnectButtonIsEnabled");

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
            }
        }

        public string ConnectButtonText => DriveStatus == DriveStatus.CONNECTED ? "Disconnect" : "Connect";
        public string ConnectButtonColor => DriveStatus == DriveStatus.CONNECTED ? "#689F38" : "#607d8b";
        public bool ConnectButtonIsEnabled => MountStatus == MountStatus.OK;
        public bool IsSettingsChanged { get; set; }

        private string message;
        public string Message
        {
            get { return message; }
            set { message = value; NotifyPropertyChanged(); }
        }
        public Brush MessageColor
        {
            get
            {
                //return Brushes.Black;
                return MountStatus == MountStatus.OK ?  Brushes.Black: Brushes.Red;
            }
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

        public string SettingsOkButtonText { get { return IsDriveNew ? "Save": "Back"; } }


        #endregion

        #region Constructor

        public MainWindowViewModel(ReturnBox rb, Drive drive)
        {
            _mountService = new MountService();
            SelectedDrive = drive;
            //Messenger.Default.Register<string>(this, OnShowView);
            CurrentPage = Page.Main;
            LoadDrivesAsync(drive);
            GetVersionsAsync();
            if(rb != null)
                Message = rb.Error;
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
            DriveStatus = r.DriveStatus;
            MountStatus = r.MountStatus;
            Message = r.Error;

            switch (r.MountStatus)
            {
                case MountStatus.BAD_CLI:
                case MountStatus.BAD_WINFSP:
                case MountStatus.BAD_DRIVE:
                    CurrentPage = Page.Main;
                    break;
                case MountStatus.BAD_HOST:
                    CurrentPage = Page.Host;
                    break;
                case MountStatus.BAD_PASSWORD:
                case MountStatus.BAD_KEY:
                    CurrentPage = Page.Password;
                    break;
                case MountStatus.OK:
                    CurrentPage = Page.Main;
                    Message = r.DriveStatus.ToString();
                    UpdateObservableDrives(r.Drive);
                    NotifyPropertyChanged("HasDrives");
                    break;
                default:
                    break;
            }
            IsWorking = false;

        }
       
        public async void LoadDrivesAsync(Drive drive)
        {
            WorkStart("Exploring local drives...");
            await Task.Run(() =>
            {
                Settings settings = _mountService.LoadSettings();
                if (drive != null)
                    settings.AddDrive(drive);
                //GlobalArgs = settings.Args;
                if(_selectedDrive == null)
                    SelectedDrive = settings.SelectedDrive;
                _mountService.UpdateDrives(settings);
            });

            UpdateObservableDrives(SelectedDrive);

            if (GoldDrives.Count == 0)
            {
                CurrentPage = Page.Host;
                WorkDone();
            }
            else
            {
                CheckDriveStatusAsync();
            }
            
        }

        private void UpdateObservableDrives(Drive selected)
        {
            GoldDrives.Clear();
            FreeDrives.Clear();
            _mountService.GoldDrives.ForEach(GoldDrives.Add);
            _mountService.FreeDrives.ForEach(FreeDrives.Add);
            if (selected != null)
            {
                var d1 = GoldDrives.ToList().Find(x => x.Name == selected.Name);
                if (d1 != null)
                {
                    SelectedDrive = d1;
                }
            }
            else
            {
                if (GoldDrives.Count > 0)
                    SelectedDrive = GoldDrives[0];
            }
            if (FreeDrives.Count > 0)
                SelectedFreeDrive = FreeDrives[0];

        }

        private async void ConnectAsync(Drive drive)
        {
            WorkStart("Connecting...");
            ReturnBox r = await Task.Run(() => _mountService.Connect(drive));
            WorkDone(r);
        }

        private async void CheckDriveStatusAsync()
        {
            if (SelectedDrive != null)
            {
                WorkStart("Checking status...");
                ReturnBox r = await Task.Run(() => _mountService.CheckDriveStatus(SelectedDrive));
                WorkDone(r);
            } 
        }

        private async void GetVersionsAsync()
        {
            Version = await Task.Run(() => _mountService.GetVersions());            
        }

        #endregion

        #region Commands

        


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
            if (GoldDrives.Count == 0 || string.IsNullOrEmpty(SelectedDrive.Host))
            {
                if (SelectedDrive != null)
                {
                    var drive = FreeDrives.ToList().Find(x => x.Name == SelectedDrive.Name);
                    if(drive == null)
                        FreeDrives.Add(SelectedDrive);
                    SelectedFreeDrive = SelectedDrive;
                }
                CurrentPage = Page.Host;
                return;
            }
            if (ConnectButtonText == "Connect")
            {
                ConnectAsync(SelectedDrive);
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
            Drive d;
            if (HasDrive)
            {
                SelectedDrive.MountPoint = NewMountPoint;
                d = SelectedDrive;
            }
            else
            {
                SelectedFreeDrive.MountPoint = NewMountPoint;
                d = SelectedFreeDrive;
            }
            if (string.IsNullOrEmpty(d.Host))
            {
                Message = "Host is required";
                return;
            }
            ConnectAsync(d);
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
            Drive d = SelectedDrive;
            if (!HasDrive)
            {
                d = SelectedFreeDrive;
            }
            CurrentPage = Page.Main;
            WorkStart("Connecting...");
            ReturnBox r = await Task.Run(() => _mountService.ConnectPassword(d, password));
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
                return _settingsOkCommand ?? (_settingsOkCommand = new RelayCommand(
                   // action
                   x => {
                       OnSettingsSave(x);
                   },
                   // can execute
                   x => {
                       return true; // IsSettingsChanged;
                   }));
            }
        }
        
        private async void OnSettingsSave(object obj)
        {            
            if (IsDriveNew)
            {
                Drive d = SelectedFreeDrive;
                await Task.Run(() => {
                    Settings settings = _mountService.LoadSettings();
                    //settings.Args = GlobalArgs;
                    settings.AddDrive(d);
                    _mountService.SaveSettings(settings);
                    _mountService.UpdateDrives(settings);
                });
                UpdateObservableDrives(d);
                IsDriveNew = false;
            }
            else
            {
                CurrentPage = Page.Main;
                Drive d = SelectedDrive;
                await Task.Run(() => {
                    Settings settings = _mountService.LoadSettings();
                    //settings.Args = GlobalArgs;
                    settings.AddDrives(GoldDrives.ToList());
                    _mountService.SaveSettings(settings);
                    _mountService.UpdateDrives(settings);
                });
                UpdateObservableDrives(d);                
                CheckDriveStatusAsync();
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
                       OnSettingsDelete(x);
                   },
                   // can execute
                   x => {
                       return GoldDrives != null && GoldDrives.Count > 0;
                   }));
            }
        }

        private async void OnSettingsDelete(object obj)
        {
            Drive d = SelectedDrive;
            if (GoldDrives.Contains(d))
                GoldDrives.Remove(d);
            await Task.Run(() => {
                if (d.Status == DriveStatus.CONNECTED)
                    _mountService.Unmount(d);
                Settings settings = _mountService.LoadSettings();
                //settings.Args = GlobalArgs;
                settings.AddDrives(GoldDrives.ToList());
                _mountService.SaveSettings(settings);
                _mountService.UpdateDrives(settings);
            });
            UpdateObservableDrives(SelectedDrive);
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
            if (GoldDrives != null)
            {
                Settings settings = new Settings
                {
                    //Args = GlobalArgs,
                    Selected = Selected,
                };
                settings.AddDrives(GoldDrives.ToList());
                _mountService.SaveSettings(settings);
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
        private ICommand _openLogsFolderCommand;
        public ICommand OpenLogsFolderCommand
        {
            get
            {
                return _openLogsFolderCommand ??
                    (_openLogsFolderCommand = new RelayCommand(
                        url => System.Diagnostics.Process.Start("explorer.exe", _mountService.LocalAppData)));
            }
        }
        #endregion

    }


}

