using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Media;

namespace golddrive
{
    public class MainWindowViewModel : Observable, IRequestFocus
    {
        public event EventHandler<FocusRequestedEventArgs> FocusRequested;

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
            set
            {
                _isDriveNew = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("SettingsOkButtonText");
                
            }
        }
        private Settings Settings { get; set; }

        private Page _currentPage;
        public Page CurrentPage
        {
            get { return _currentPage; }
            set { _currentPage = value; NotifyPropertyChanged(); }
        }

        public ObservableCollection<Drive> FreeDriveList { get; set; } = new ObservableCollection<Drive>();
        public ObservableCollection<Drive> GoldDriveList { get; set; } = new ObservableCollection<Drive>();
        public string Selected => SelectedDrive != null ? SelectedDrive.Name : "";

        public Drive OriginalDrive { get; set; }

        private Drive _selectedDrive;
        public Drive SelectedDrive
        {
            get { return _selectedDrive; }
            set
            {
                _selectedDrive = value;
                if (_selectedDrive != null)
                {
                    _selectedDrive.PropertyChanged -= SelectedDrive_PropertyChanged;
                    _selectedDrive.PropertyChanged += SelectedDrive_PropertyChanged;
                    OriginalDrive = new Drive(_selectedDrive);
                }
                NotifyPropertyChanged();
                NotifyPropertyChanged("HasDrive");
                NotifyPropertyChanged("HasDrives");
                }
            
        }
        public Drive OldSelectedDrive { get; set; }

        //private Drive _selectedFreeDrive;
        //public Drive SelectedFreeDrive
        //{
        //    get { return _selectedFreeDrive; }
        //    set { _selectedFreeDrive = value; NotifyPropertyChanged(); }
        //}
        public bool HasDrive { get { return SelectedDrive != null; } }
        public bool HasDrives { get { return _mountService.GoldDrives != null && _mountService.GoldDrives.Count > 0; } }

        private DriveStatus driveStatus;
        public DriveStatus DriveStatus
        {
            get { return driveStatus; }
            set
            {
                driveStatus = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("ConnectButtonText");
                NotifyPropertyChanged("ConnectButtonColor");
            }
        }

        private MountStatus _mountStatus;
        public MountStatus MountStatus
        {
            get { return _mountStatus; }
            set
            {
                _mountStatus = value;
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
        public bool CanEdit
        {
            get 
            {
                return SelectedDrive != null && SelectedDrive.Status == DriveStatus.DISCONNECTED;
            }
        }
        public string ConnectButtonText => 
            (DriveStatus == DriveStatus.CONNECTED 
            || DriveStatus == DriveStatus.BROKEN ) ? "Disconnect" : "Connect";
        public string ConnectButtonColor => DriveStatus == DriveStatus.CONNECTED ? "#689F38" : "#607d8b";
        public bool ConnectButtonIsEnabled => true;
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
                return Brushes.Black;
                //return MountStatus == MountStatus.OK ? Brushes.Black : Brushes.Red;
            }
        }
        private string password;
        public string Password
        {
            get { return password; }
            set { password = value; NotifyPropertyChanged(); }
        }
        //private string _newMountPoint;
        //public string NewMountPoint
        //{
        //    get { return _newMountPoint; }
        //    set { _newMountPoint = value; NotifyPropertyChanged(); }
        //}

        //private string _host;
        //public string Host
        //{
        //    get { return _host; }
        //    set { _host = value; NotifyPropertyChanged(); }
        //}
        
        //private string _user;
        //public string User
        //{
        //    get { return _user; }
        //    set { _user = value; NotifyPropertyChanged(); }
        //}
        //public string CurrentUser
        //{
        //    get { return System.Environment.UserName; }
        //}
        //private string _port;
        //public string Port
        //{
        //    get { return _port; }
        //    set { _port = value; NotifyPropertyChanged(); }
        //}
        //private string _label;
        //public string Label
        //{
        //    get { return _label; }
        //    set { _label = value; NotifyPropertyChanged(); }
        //}

        public bool IsDriveSelected { get { return SelectedDrive != null; } }
        public bool IsSettingsDirty
        {
            get
            {
                try
                {
                    return CurrentPage == Page.Settings &&
                        (IsDriveNew || SelectedDrive.IsDirty);
                }
                catch
                {
                    return false;
                }
            }
        }
        public string SettingsOkButtonText 
        { 
            get 
            { 
                return IsSettingsDirty ? "Save" : "Ok"; 
            }
        }


        #endregion

        #region Constructor

        public MainWindowViewModel(ReturnBox rb, Drive drive)
        {
            _mountService = new MountService();
            SelectedDrive = drive;
            //_port = 22;
           // _user = System.Environment.UserName;
            //Messenger.Default.Register<string>(this, OnShowView);
            CurrentPage = Page.Main;
            LoadDrivesAsync(drive);
            GetVersionsAsync();
            if (rb != null)
                Message = rb.Error;

            
        }

        

        #endregion

        #region Async Methods

        private void WorkStart(string message)
        {
            Message = message;
            if (IsWorking)
                return;
            IsWorking = true;
        }
        private void WorkDone(ReturnBox r = null)
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
                    OnFocusRequested(nameof(SelectedDrive.Host));
                    break;
                case MountStatus.BAD_PASSWORD:
                case MountStatus.BAD_KEY:
                    CurrentPage = Page.Password;
                    OnFocusRequested(nameof(Password));
                    break;
                case MountStatus.OK:
                    CurrentPage = Page.Main;
                    Message = r.DriveStatus.ToString();
                    SelectedDrive = r.Drive;
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
                Settings = _mountService.LoadSettings();
                if (drive != null)
                    Settings.AddDrive(drive);
                if (_selectedDrive == null)
                    SelectedDrive = Settings.SelectedDrive;
                _mountService.UpdateDrives(Settings);
            });
            
            UpdateObservableDrives(SelectedDrive);

            if (_mountService.GoldDrives.Count == 0)
            {
                CurrentPage = Page.Host;
                IsDriveNew = true;
                OnFocusRequested(nameof(SelectedDrive.Host));
                SelectedDrive = FreeDriveList.First();
                WorkDone();
            }
            else
            {
                CheckDriveStatusAsync();
            }

        }

        private void UpdateObservableDrives(Drive selected)
        {
            GoldDriveList.Clear();
            FreeDriveList.Clear();
            _mountService.GoldDrives.ForEach(GoldDriveList.Add);
            _mountService.FreeDrives.ForEach(FreeDriveList.Add);

            if (selected != null)
            {
                var d1 = _mountService.GoldDrives.ToList().Find(x => x.Name == selected.Name);
                if (d1 != null)
                {
                    SelectedDrive = d1;
                }
            }
            else
            {
                if(_mountService.GoldDrives.Count > 0)
                    SelectedDrive = _mountService.GoldDrives.First();
                else if(_mountService.FreeDrives.Count > 0)
                    SelectedDrive = _mountService.FreeDrives.First();
            }

            NotifyPropertyChanged("FreeDriveList");
            NotifyPropertyChanged("GoldDriveList");
        }

        void ReportStatus(string message)
        {
            Message = message;
        }

        private async void ConnectAsync(Drive drive)
        {
            WorkStart("Connecting...");
            var status = new Progress<string>(ReportStatus);
            ReturnBox r = await Task.Run(() => _mountService.Connect(drive, status));
            UpdateObservableDrives(SelectedDrive);
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
                   x =>
                   {
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
            if (IsWorking)
                return;
            
            Message = "";

            if (GoldDriveList.Count == 0 || string.IsNullOrEmpty(SelectedDrive.Host))
            {
                //if (SelectedDrive != null)
                //{
                //    var drive = FreeDrives.ToList().Find(x => x.Name == SelectedDrive.Name);
                //    if (drive == null)
                //        FreeDrives.Add(SelectedDrive);
                //    //SelectedFreeDrive = SelectedDrive;
                //}
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
            if (string.IsNullOrEmpty(SelectedDrive.Host))
            {
                Message = "Server is required";
                OnFocusRequested(nameof(SelectedDrive.Host));
                return;
            }
            ConnectAsync(SelectedDrive);
        }
        private ICommand _showPageCommand;
        public ICommand ShowPageCommand
        {
            get
            {
                return _showPageCommand ??
                    (_showPageCommand = new RelayCommand(
                        x =>
                        {
                            CurrentPage = (Page)x;
                            Message = "";
                            if(SelectedDrive != null)
                                SelectedDrive.IsDirty = false;
                            if (CurrentPage == Page.Settings)
                            {
                                Settings = _mountService.LoadSettings();
                                NotifyPropertyChanged("CanEdit");
                            }
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
            //if (!HasDrive)
            //{
            //    d = SelectedFreeDrive;
            //}
            //CurrentPage = Page.Main;
            WorkStart("Connecting...");
            var status = new Progress<string>(ReportStatus);
            ReturnBox r = await Task.Run(() => _mountService.ConnectPassword(d, password, status));
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
                   x =>
                   {
                       OnSettingsSave(x);
                   },
                   // can execute
                   x =>
                   {
                       return true; // IsSettingsChanged;
                   }));
            }
        }
        private async void OnSettingsSave(object obj)
        {
            if (IsSettingsDirty)
            {
                SelectedDrive.Trim();
                if (string.IsNullOrEmpty(SelectedDrive.Host))
                {
                    Message = "Server is required";
                    OnFocusRequested("SelectedDrive.Host");
                    return;
                }
                Settings.AddDrive(SelectedDrive);
                
                //await Task.Run(() =>
                //{
                //    Settings settings = _mountService.LoadSettings();
                //    settings.AddDrive(SelectedDrive);
                //    _mountService.SaveSettings(settings);
                //    _mountService.UpdateDrives(settings);
                //    Message = "";
                //});
                //UpdateObservableDrives(SelectedDrive);
                IsDriveNew = false;
                SelectedDrive.IsDirty = false;

            }
            else 
            {
                await Task.Run(() =>
                {
                    //Settings settings = _mountService.LoadSettings();
                    //settings.AddDrives(GoldDriveList.ToList());
                    _mountService.SaveSettings(Settings);
                    _mountService.UpdateDrives(Settings);
                });
                UpdateObservableDrives(SelectedDrive);
                if (SelectedDrive != null)
                    SelectedDrive.IsDirty = false;
                CurrentPage = Page.Main;
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
            SelectedDrive = FreeDriveList.First();
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
            if (IsDriveNew)
            {
                Message = "";
                IsDriveNew = false;
            }
            else // IsSettingsDirty
            {
                SelectedDrive.Clone(OriginalDrive);
            }
            
            SelectedDrive.IsDirty = false;

        }

        private ICommand _settingsDeleteCommand;
        public ICommand SettingsDeleteCommand
        {
            get
            {
                return _settingsDeleteCommand ?? (_settingsDeleteCommand = new RelayCommand(
                   // action
                   x =>
                   {
                       OnSettingsDelete(x);
                   },
                   // can execute
                   x =>
                   {
                       return true;
                       //return GoldDriveList != null && GoldDriveList.Count > 0;
                   }));
            }
        }

        private async void OnSettingsDelete(object obj)
        {
            Drive d = SelectedDrive;
            if (GoldDriveList.Contains(d))
                GoldDriveList.Remove(d);
            await Task.Run(() =>
            {
                if (d.Status == DriveStatus.CONNECTED)
                    _mountService.Unmount(d);
                Settings settings = _mountService.LoadSettings();
                //settings.Args = GlobalArgs;
                settings.AddDrives(_mountService.GoldDrives);
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
            Settings settings = _mountService.LoadSettings();
            if (GoldDriveList != null)
            {
                settings.Selected = Selected;
                settings.AddDrives(GoldDriveList.ToList());
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

        #region events

        
        protected virtual void OnFocusRequested(string propertyName)
        {
            FocusRequested?.Invoke(this, new FocusRequestedEventArgs(propertyName));
        }
        private void SelectedDrive_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (CurrentPage == Page.Settings)
            {
                NotifyPropertyChanged("IsSettingsDirty");
                NotifyPropertyChanged("SettingsOkButtonText");
            }
        }
        #endregion

    }


}

