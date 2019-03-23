using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace golddrive
{
    public class MainWindowViewModel : BaseViewModel
    {
        public ICommand ShowLoginCommand { get; set; }
        public ICommand ShowConnectCommand { get; set; }
        public ICommand ShowSettingsCommand { get; set; }
        public ICommand LoadedCommand { get; set; }
        public ICommand ClosingCommand { get; set; }
        public ICommand AboutCommand { get; set; }


        private LoginViewModel _loginViewModel;
        private ConnectViewModel _connectViewModel;        
        private BaseViewModel _selectedViewModel;
        private SettingsViewModel _settingsViewModel;

        public BaseViewModel SelectedViewModel
        {
            get { return _selectedViewModel; }
            set {
                _selectedViewModel = value;
                NotifyPropertyChanged();
            }
        }

        public Task Taks { get; private set; }

        public MainWindowViewModel()
        {
            Driver = new Driver();
            _loginViewModel = new LoginViewModel(this, Driver);
            _connectViewModel = new ConnectViewModel(this, Driver);
            _settingsViewModel = new SettingsViewModel(this, Driver);

            //Messenger.Default.Register<string>(this, OnShowView);
            ShowLoginCommand = new BaseCommand(ShowLogin);
            ShowConnectCommand = new BaseCommand(ShowConnect);
            ShowSettingsCommand = new BaseCommand(ShowSettings);
            LoadedCommand = new BaseCommand(Loaded);
            ClosingCommand = new BaseCommand(Closing);
            AboutCommand = new BaseCommand(About);

            ShowConnectCommand.Execute(null);
        }


        private void ShowLogin(object obj)
        {
            SelectedViewModel = _loginViewModel;
        }
        private void ShowConnect(object obj)
        {
            SelectedViewModel = _connectViewModel;
        }
        private void ShowSettings(object obj)
        {
            SelectedViewModel = _settingsViewModel;
        }
        private void Loaded(object obj)
        {
            Driver.LoadDrives();

        }
        private void Closing(object obj)
        {
            Driver.SaveSettingsDrives(Driver.Drives.ToList());
        }
        private void About(object obj)
        {
            
        }
    }

    
}
