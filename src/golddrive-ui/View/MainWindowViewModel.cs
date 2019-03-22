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
        public ICommand WindowClosingCommand { get; set; }


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


        public MainWindowViewModel()
        {
            _driver = new Driver();
            _loginViewModel = new LoginViewModel(this, _driver);
            _connectViewModel = new ConnectViewModel(this, _driver);
            _settingsViewModel = new SettingsViewModel(this, _driver);

            //Messenger.Default.Register<string>(this, OnShowView);
            ShowLoginCommand = new BaseCommand(ShowLogin);
            ShowConnectCommand = new BaseCommand(ShowConnect);
            ShowSettingsCommand = new BaseCommand(ShowSettings);
            WindowClosingCommand = new BaseCommand(WindowClosing);

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
        private void WindowClosing(object obj)
        {
            _driver.SaveSettingsDrives(_connectViewModel.Drives.ToList());
        }
    }

    
}
