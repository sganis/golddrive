using System;
using System.Linq;
using System.Windows.Input;
using System.Windows.Media;
using System.Threading.Tasks;
using System.Collections.ObjectModel;
using System.Text.RegularExpressions;

namespace golddrive
{
    public class MainWindowViewModel : Observable, IRequestFocus
    {
        #region Properties

        const string HostRegex = @"^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$";

        public event EventHandler<FocusRequestedEventArgs> FocusRequested;

        private MountService _mountService;

        public bool Loaded { get; set; }
        public bool SkipComboChanged { get; set; }

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
                if (_isDriveNew != value)
                {
                    _isDriveNew = value;
                    NotifyPropertyChanged();
                    NotifyPropertyChanged("IsSettingsDirty");
                }
            }
        }
        private bool _isDriveEdit;
        public bool IsDriveEdit
        {
            get { return _isDriveEdit; }
            set
            {
                if (_isDriveEdit != value)
                {
                    _isDriveEdit = value;
                    NotifyPropertyChanged();
                    NotifyPropertyChanged("IsSettingsDirty");
                }
            }
        }
        private Page _currentPage;
        public Page CurrentPage
        {
            get { return _currentPage; }
            set {
                Title = value.ToString();
                _currentPage = value;
                HasBack = _currentPage == Page.Settings || _currentPage == Page.About;
                NotifyPropertyChanged();
                NotifyPropertyChanged("HasBack");
                NotifyPropertyChanged("Title");
            }
        }
        public bool HasBack { get; set; }
        public string Title { get; set; }
        
        public ObservableCollection<Drive> FreeDriveList { get; set; } = new ObservableCollection<Drive>();
        public ObservableCollection<Drive> GoldDriveList { get; set; } = new ObservableCollection<Drive>();
        //public string Selected => SelectedDrive != null ? SelectedDrive.Name : "";

        public Drive OriginalDrive { get; set; }

        private Drive _selectedDrive;
        public Drive SelectedDrive
        {
            get { return _selectedDrive; }
            set
            {
                               
                if (_selectedDrive != value)
                {
                    if(_selectedDrive != null)
                        OriginalDrive = new Drive(_selectedDrive);
                    _selectedDrive = value;
                    //_selectedDrive.PropertyChanged -= SelectedDrive_PropertyChanged;
                    //_selectedDrive.PropertyChanged += SelectedDrive_PropertyChanged;
                    
                    NotifyPropertyChanged();
                    NotifyPropertyChanged("HasDrives");
                    
                }

            }
            
        }

        public bool HasDrives 
        { 
            get 
            { 
                return _mountService.GoldDrives != null && _mountService.GoldDrives.Count > 0; 
            }
        }

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
        
        public bool IsDriveSelected { get { return SelectedDrive != null; } }
        public bool IsSettingsDirty
        {
            get
            {
                if (SelectedDrive == null)
                    return false;
                return CurrentPage == Page.Settings &&
                        (IsDriveNew || (IsDriveEdit && SelectedDrive.IsDirty));
            }
        }


        #endregion

        #region Constructor

        public MainWindowViewModel(ReturnBox rb)
        {
            _mountService = new MountService();
            //Messenger.Default.Register<string>(this, OnShowView);
            
            CurrentPage = Page.Main;
            LoadDrivesAsync();
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
                    //SelectedDrive = r.Drive;
                    NotifyPropertyChanged("HasDrives");
                    break;
                default:
                    break;
            }
            IsWorking = false;

        }

        public async void LoadDrivesAsync()
        {
            Loaded = false;
            WorkStart("Exploring local drives...");
            await Task.Run(() =>
            {
                Settings settings = _mountService.LoadSettings();
                if (_selectedDrive == null && settings.SelectedDrive != null)
                    SelectedDrive = settings.SelectedDrive;
                _mountService.UpdateDrives(settings);
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
            Loaded = true;

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
                if (_mountService.GoldDrives.Count > 0)
                {
                    SelectedDrive = _mountService.GoldDrives.First();
                }
                else if (_mountService.FreeDrives.Count > 0)
                {
                    SelectedDrive = _mountService.FreeDrives.First();
                }
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
            SkipComboChanged = true;
            UpdateObservableDrives(SelectedDrive);
            SkipComboChanged = false;
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

        #region Command methods

        private async void OnConnect(object obj)
        {
            if (IsWorking)
                return;

            Message = "";

            if (GoldDriveList.Count == 0 || string.IsNullOrEmpty(SelectedDrive.Host))
            {
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
        
        private void OnSettingsShow(object obj)
        {
            //Settings = _mountService.LoadSettings();
            //NotifyPropertyChanged("CanEdit");
            SelectedDrive.IsDirty = false;
            IsDriveNew = false;
            
            
        }
        private async void OnSettingsSave(object obj)
        {
            SelectedDrive.Trim();
            if (string.IsNullOrEmpty(SelectedDrive.Host))
            {
                Message = "Server is required";
                OnFocusRequested("SelectedDrive.Host");
                return;
            }

            Regex hostRegex = new Regex(HostRegex);
            if (!hostRegex.Match(SelectedDrive.Host).Success 
                && !hostRegex.Match(SelectedDrive.Host).Success)
            {
                Message = "Invalid server name";
                OnFocusRequested("SelectedDrive.Host");
                return;
            }
            await Task.Run(() =>
            {
                Settings settings = _mountService.LoadSettings();
                settings.AddDrive(SelectedDrive);
                _mountService.SaveSettings(settings);
                _mountService.UpdateDrives(settings);
            });

            UpdateObservableDrives(SelectedDrive);
            SelectedDrive.IsDirty = false;
            Message = "";
            IsDriveNew = false;
            IsDriveEdit = false;

        }
        private void OnSettingsCancel(object obj)
        {
            if (!IsDriveNew)
            {
                SelectedDrive.Clone(OriginalDrive);
            }

            Message = "";
            IsDriveNew = false;
            IsDriveEdit = false;
            SelectedDrive.IsDirty = false;
            NotifyPropertyChanged("IsSettingsDirty");

        }

        private void OnSettingsNew(object obj)
        {
            IsDriveNew = true;
            SelectedDrive = FreeDriveList.First();
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
                settings.AddDrives(GoldDriveList);
                _mountService.SaveSettings(settings);
                _mountService.UpdateDrives(settings);
            });
            UpdateObservableDrives(SelectedDrive);
            if(GoldDriveList.Count == 0)
                IsDriveNew = true;
            if(SelectedDrive != null)
                SelectedDrive.IsDirty = false;

        }
        private void OnSettingsEdit(object obj)
        {
            IsDriveEdit = true;
        }
        public void Closing(object obj)
        {
            Settings settings = _mountService.LoadSettings();
            if (GoldDriveList != null)
            {
                settings.Selected = SelectedDrive != null ? SelectedDrive.Name : "";
                settings.AddDrives(GoldDriveList.ToList());
                _mountService.SaveSettings(settings);
            }
        }


        #endregion

        #region Commands

        //private ICommand _selectedDriveChangedCommand;
        //public ICommand SelectedDriveChangedCommand
        //{
        //    get
        //    {
        //        return _selectedDriveChangedCommand ?? (_selectedDriveChangedCommand = new RelayCommand(
        //           // action
        //           x =>
        //           {
        //               if (CurrentPage != Page.Main)
        //                   return;
        //               if (OldSelectedDrive == null)
        //                   OldSelectedDrive = SelectedDrive;
        //               if (SelectedDrive != OldSelectedDrive)
        //               {
        //                   OldSelectedDrive = SelectedDrive;
        //                   CheckDriveStatusAsync();
        //               }
        //           }));
        //    }
        //}
        private ICommand _connectCommand;
        public ICommand ConnectCommand
        {
            get
            {
                return _connectCommand ??
                    (_connectCommand = new RelayCommand(OnConnect));
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
        private ICommand _showPageCommand;
        public ICommand ShowPageCommand
        {
            get
            {
                return _showPageCommand ??
                    (_showPageCommand = new RelayCommand(
                        x =>
                        {
                            Message = "";                            
                            CurrentPage = (Page)x;
                            
                            if(SelectedDrive != null)
                                SelectedDrive.IsDirty = false;
                            if (CurrentPage == Page.Settings)
                            {
                                OnSettingsShow(x);
                            }
                            if (CurrentPage == Page.Main)
                            {
                                CheckDriveStatusAsync();
                            }
                        },
                        // can execute
                        x =>
                        {
                            return CurrentPage != (Page)x; 
                        }));
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
        private ICommand _settingsNewCommand;
        public ICommand SettingsNewCommand
        {
            get
            {
                return _settingsNewCommand ??
                    (_settingsNewCommand = new RelayCommand(OnSettingsNew));
            }
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
        private ICommand _settingsEditCommand;
        public ICommand SettingsEditCommand
        {
            get
            {
                return _settingsEditCommand ?? (_settingsEditCommand = new RelayCommand(
                   // action
                   x =>
                   {
                       OnSettingsEdit(x);
                   },
                   // can execute
                   x =>
                   {
                       return true;
                       //return GoldDriveList != null && GoldDriveList.Count > 0;
                   }));
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
        private void SelectedDrive_PropertyChanged(object sender,  System.ComponentModel.PropertyChangedEventArgs e)
        {
            // this will be trigger when any property in SelectedDrive changes
            if (CurrentPage == Page.Settings)
            {
                NotifyPropertyChanged("IsSettingsDirty");
            }
        }
        

        public void OnComboChanged()
        {
            
            if (!Loaded)
                return;
            if (SkipComboChanged)
                return;
            if (CurrentPage == Page.Settings)
            {
                NotifyPropertyChanged("IsSettingsDirty");
                return;
            }

            CheckDriveStatusAsync();
        }

        #endregion

    }
}

