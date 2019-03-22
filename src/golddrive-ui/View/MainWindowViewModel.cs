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

        private MountManager _mountManager;
        private LoginViewModel _loginViewModel;
        private ConnectViewModel _connectViewModel;        
        private BaseViewModel _selectedViewModel;        

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
            _mountManager = new MountManager();
            _loginViewModel = new LoginViewModel(this, _mountManager);
            _connectViewModel = new ConnectViewModel(this, _mountManager);

            //Messenger.Default.Register<string>(this, OnShowView);
            ShowLoginCommand = new BaseCommand(ShowLogin);
            ShowConnectCommand = new BaseCommand(ShowConnect);
            
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

    }

    
}
