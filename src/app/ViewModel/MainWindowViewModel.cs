// src/app/ViewModel/MainWindowViewModel.cs
using System;
using System.Linq;
using System.Windows;
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

        const string HostRegex = @"^((([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])|(\d{1,3}\.){3}\d{1,3})$";

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
        public bool IsEditMode { get { return IsDriveNew || IsDriveEdit; } }

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
                    NotifyPropertyChanged(nameof(IsEditMode));
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
                    NotifyPropertyChanged(nameof(IsEditMode));
                }
            }
        }
        private Page _currentPage;
        public Page CurrentPage
        {
            get { return _currentPage; }
            set
            {
                Title = value.ToString();
                _currentPage = value;
                HasBack = _currentPage == Page.Settings || _currentPage == Page.About;
                NotifyPropertyChanged();
                NotifyPropertyChanged(nameof(HasBack));
                NotifyPropertyChanged(nameof(Title));
            }
        }
        public bool HasBack { get; set; }
        public string Title { get; set; }

        public ObservableCollection<Drive> FreeDriveList { get; set; } = new ObservableCollection<Drive>();
        public ObservableCollection<Drive> GoldDriveList { get; set; } = new ObservableCollection<Drive>();

        public Drive OriginalDrive { get; set; }

        private Drive _selectedDrive;
        public Drive SelectedDrive
        {
            get { return _selectedDrive; }
            set
            {
                if (_selectedDrive != value)
                {
                    _selectedDrive = value;
                    NotifyPropertyChanged();
                    NotifyPropertyChanged(nameof(HasDrives));
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
                NotifyPropertyChanged(nameof(ConnectButtonText));
                NotifyPropertyChanged(nameof(ConnectButtonColor));
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
                NotifyPropertyChanged(nameof(MessageColor));
                NotifyPropertyChanged(nameof(ConnectButtonIsEnabled));
            }
        }

        private bool _isWorking;
        public bool IsWorking
        {
            get { return _isWorking; }
            set { _isWorking = value; NotifyPropertyChanged(); }
        }

        public string ConnectButtonText =>
            (DriveStatus == DriveStatus.CONNECTED
            || DriveStatus == DriveStatus.BROKEN) ? "Disconnect" : "Connect";
        public string ConnectButtonColor => DriveStatus == DriveStatus.CONNECTED ? "#43A047" : "#5C6BC0";
        public bool ConnectButtonIsEnabled => true;
        public bool IsSettingsChanged { get; set; }

        private string message;
        public string Message
        {
            get { return message; }
            set { message = value; NotifyPropertyChanged(); }
        }
        public Brush MessageColor { get { return Brushes.Black; } }

        private string password;
        public string Password
        {
            get { return password; }
            set { password = value; NotifyPropertyChanged(); }
        }

        #endregion

        #region Constructor

        public MainWindowViewModel(ReturnBox rb)
        {
            _mountService = new MountService();
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
                    NotifyPropertyChanged(nameof(HasDrives));
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

            UpdateObservableDrives();

            if (_mountService.GoldDrives.Count == 0)
            {
                CurrentPage = Page.Host;
                IsDriveNew = true;
                OnFocusRequested(nameof(SelectedDrive.Host));
                SelectedDrive = FreeDriveList.FirstOrDefault();
                WorkDone();
            }
            else
            {
                CheckDriveStatusAsync();
            }
            Loaded = true;
        }

        private void UpdateObservableDrives()
        {
            var action = new Action(() =>
            {
                Drive old = SelectedDrive;
                GoldDriveList.Clear();
                FreeDriveList.Clear();
                _mountService.GoldDrives.ForEach(GoldDriveList.Add);
                _mountService.FreeDrives.ForEach(FreeDriveList.Add);
                if (old != null && SelectedDrive == null)
                    SelectedDrive = old;

                if (SelectedDrive != null)
                {
                    var d1 = _mountService.GoldDrives.ToList().Find(x => x.Name == SelectedDrive.Name);
                    if (d1 != null)
                    {
                        d1.Clone(SelectedDrive);
                        SelectedDrive = d1;
                    }
                    else
                    {
                        var d2 = _mountService.FreeDrives.ToList().Find(x => x.Name == SelectedDrive.Name);
                        if (d2 != null)
                        {
                            d2.Clone(SelectedDrive);
                            SelectedDrive = d2;
                        }
                    }
                }
                else
                {
                    if (_mountService.GoldDrives.Count > 0)
                        SelectedDrive = _mountService.GoldDrives.First();
                    else if (_mountService.FreeDrives.Count > 0)
                        SelectedDrive = _mountService.FreeDrives.First();
                }

                NotifyPropertyChanged(nameof(FreeDriveList));
                NotifyPropertyChanged(nameof(GoldDriveList));
            });

            if (Application.Current?.Dispatcher?.CheckAccess() == false)
                Application.Current.Dispatcher.Invoke(action);
            else
                action();
        }

        private async void ConnectAsync(Drive drive)
        {
            WorkStart("Connecting...");
            try
            {
                var status = new Progress<string>(s => Message = s);
                ReturnBox r = await Task.Run(() => _mountService.Connect(drive, status));
                SkipComboChanged = true;
                UpdateObservableDrives();
                SkipComboChanged = false;
                WorkDone(r);
            }
            catch (Exception ex)
            {
                Logger.Log($"Connect error: {ex.Message}");
                WorkDone(new ReturnBox { Error = ex.Message, MountStatus = MountStatus.UNKNOWN });
            }
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
            if (IsWorking) return;
            Message = "";

            if (GoldDriveList.Count == 0 || string.IsNullOrEmpty(SelectedDrive.Host))
            {
                IsDriveNew = true;
                SelectedDrive = FreeDriveList.FirstOrDefault();
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
            if (SelectedDrive == null) { Message = "Invalid drive"; return; }
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
            if (SelectedDrive == null) { Message = "Invalid drive"; return; }

            WorkStart("Connecting...");
            try
            {
                var status = new Progress<string>(s => Message = s);
                string pwd = password;
                Password = null;
                ReturnBox r = await Task.Run(() => _mountService.ConnectPassword(SelectedDrive, pwd, status));
                SkipComboChanged = true;
                UpdateObservableDrives();
                SkipComboChanged = false;
                WorkDone(r);
            }
            catch (Exception ex)
            {
                Logger.Log($"ConnectPassword error: {ex.Message}");
                WorkDone(new ReturnBox { Error = ex.Message, MountStatus = MountStatus.UNKNOWN });
            }
        }

        private async void OnSettingsSave(object obj)
        {
            if (SelectedDrive == null) { Message = "Invalid drive"; return; }

            SelectedDrive.Trim();
            if (string.IsNullOrEmpty(SelectedDrive.Host))
            {
                Message = "Server is required";
                OnFocusRequested(nameof(SelectedDrive.Host));
                return;
            }

            Regex hostRegex = new Regex(HostRegex);
            if (!hostRegex.Match(SelectedDrive.Host).Success)
            {
                Message = "Invalid server name";
                OnFocusRequested(nameof(SelectedDrive.Host));
                return;
            }
            await Task.Run(() =>
            {
                Settings settings = _mountService.LoadSettings();
                settings.AddDrive(SelectedDrive);
                _mountService.SaveSettings(settings);
                _mountService.UpdateDrives(settings);
            });

            UpdateObservableDrives();
            Message = "";
            IsDriveNew = false;
            IsDriveEdit = false;
        }

        private void OnSettingsCancel(object obj)
        {
            if (SelectedDrive == null) { Message = "Invalid drive"; return; }
            SelectedDrive.Clone(OriginalDrive);
            Message = "";
            IsDriveNew = false;
            IsDriveEdit = false;
        }

        private async void OnSettingsDelete(object obj)
        {
            if (SelectedDrive == null) { Message = "Invalid drive"; return; }

            Drive d = SelectedDrive;
            if (GoldDriveList.Contains(d))
                GoldDriveList.Remove(d);
            try
            {
                await Task.Run(() =>
                {
                    if (d.Status == DriveStatus.CONNECTED)
                    {
                        var ur = _mountService.Unmount(d);
                        if (!ur.Success)
                            Logger.Log($"Failed to unmount {d.Name}: {ur.Error}");
                    }
                    Settings settings = _mountService.LoadSettings();
                    settings.AddDrives(GoldDriveList);
                    _mountService.SaveSettings(settings);
                    _mountService.UpdateDrives(settings);
                });
            }
            catch (Exception ex)
            {
                Logger.Log($"Delete error: {ex.Message}");
            }
            UpdateObservableDrives();
            if (GoldDriveList.Count == 0)
                IsDriveNew = true;
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

        private ICommand _connectCommand;
        public ICommand ConnectCommand => _connectCommand ?? (_connectCommand = new RelayCommand(OnConnect));

        private ICommand _connectHostCommand;
        public ICommand ConnectHostCommand => _connectHostCommand ?? (_connectHostCommand = new RelayCommand(OnConnectHost));

        private ICommand _showPageCommand;
        public ICommand ShowPageCommand => _showPageCommand ?? (_showPageCommand = new RelayCommand(
            x =>
            {
                Message = "";
                CurrentPage = (Page)x;
                if (CurrentPage == Page.Settings)
                {
                    IsDriveNew = GoldDriveList.Count == 0;
                }
                if (CurrentPage == Page.Main)
                {
                    CheckDriveStatusAsync();
                }
            },
            x => CurrentPage != (Page)x));

        private ICommand _connectPasswordCommand;
        public ICommand ConnectPasswordCommand => _connectPasswordCommand ?? (_connectPasswordCommand = new RelayCommand(OnConnectPassword));

        private ICommand _showPasswordCommand;
        public ICommand ShowLoginCommand => _showPasswordCommand ?? (_showPasswordCommand = new RelayCommand(x => { CurrentPage = Page.Password; }));

        private ICommand _settingsOkCommand;
        public ICommand SettingsOkCommand => _settingsOkCommand ?? (_settingsOkCommand = new RelayCommand(OnSettingsSave));

        private ICommand _settingsNewCommand;
        public ICommand SettingsNewCommand => _settingsNewCommand ?? (_settingsNewCommand = new RelayCommand(x =>
        {
            OriginalDrive = new Drive(SelectedDrive);
            SelectedDrive = FreeDriveList.FirstOrDefault();
            IsDriveNew = true;
        }));

        private ICommand _settingsCancelCommand;
        public ICommand SettingsCancelCommand => _settingsCancelCommand ?? (_settingsCancelCommand = new RelayCommand(OnSettingsCancel));

        private ICommand _settingsDeleteCommand;
        public ICommand SettingsDeleteCommand => _settingsDeleteCommand ?? (_settingsDeleteCommand = new RelayCommand(OnSettingsDelete));

        private ICommand _settingsEditCommand;
        public ICommand SettingsEditCommand => _settingsEditCommand ?? (_settingsEditCommand = new RelayCommand(x =>
        {
            OriginalDrive = new Drive(SelectedDrive);
            IsDriveEdit = true;
        }));

        private ICommand _githubCommand;
        public ICommand GithubCommand => _githubCommand ?? (_githubCommand = new RelayCommand(
            url => System.Diagnostics.Process.Start(url.ToString())));

        private ICommand _cleanMountsCommand;
        public ICommand CleanMountsCommand => _cleanMountsCommand ?? (_cleanMountsCommand = new RelayCommand(OnCleanMounts));

        private ICommand _restartExplorerCommand;
        public ICommand RestartExplorerCommand => _restartExplorerCommand ?? (_restartExplorerCommand = new RelayCommand(OnRestartExplorer));

        private ICommand _runTerminalCommand;
        public ICommand RunTerminalCommand => _runTerminalCommand ?? (_runTerminalCommand = new RelayCommand(
            url => System.Diagnostics.Process.Start("cmd.exe")));

        private ICommand _openLogsFolderCommand;
        public ICommand OpenLogsFolderCommand => _openLogsFolderCommand ?? (_openLogsFolderCommand = new RelayCommand(
            url => System.Diagnostics.Process.Start("explorer.exe", _mountService.LocalAppData)));

        private ICommand _openHelpCommand;
        public ICommand OpenHelpCommand => _openHelpCommand ?? (_openHelpCommand = new RelayCommand(
            x => System.Diagnostics.Process.Start("notepad.exe", _mountService.AppPath + "\\help.md")));

        #endregion

        #region Events

        protected virtual void OnFocusRequested(string propertyName)
        {
            FocusRequested?.Invoke(this, new FocusRequestedEventArgs(propertyName));
        }

        private async void OnCleanMounts(object obj)
        {
            if (IsWorking) return;
            WorkStart("Cleaning old mounts...");
            int count = await Task.Run(() => _mountService.CleanMounts());
            Message = count > 0 ? $"Cleaned {count} old mount(s)" : "No old mounts found";
            IsWorking = false;
        }

        private void OnRestartExplorer(object obj)
        {
            try
            {
                foreach (var p in System.Diagnostics.Process.GetProcessesByName("explorer"))
                    p.Kill();
                System.Diagnostics.Process.Start("explorer.exe");
                Message = "Explorer restarted";
            }
            catch (Exception ex)
            {
                Logger.Log($"RestartExplorer error: {ex.Message}");
                Message = $"Failed to restart Explorer: {ex.Message}";
            }
        }

        public void OnComboChanged()
        {
            if (!Loaded || SkipComboChanged || CurrentPage == Page.Settings)
                return;
            CheckDriveStatusAsync();
        }

        #endregion
    }
}
