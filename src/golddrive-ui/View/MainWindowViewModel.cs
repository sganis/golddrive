using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace golddrive_ui
{
    public class MainWindowViewModel : BaseViewModel
    {
        public ICommand ShowLoginCommand { get; set; }
        public ICommand ShowConnectCommand { get; set; }

        private LoginViewModel _loginViewModel = new LoginViewModel();
        private ConnectViewModel _connectViewModel = new ConnectViewModel();        
        private BaseViewModel selectedViewModel;

        

        public BaseViewModel SelectedViewModel
        {
            get { return selectedViewModel; }
            set {
                selectedViewModel = value;
                NotifyPropertyChanged();
            }
        }


        public MainWindowViewModel()
        {
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

        //private void OnNav(string destination)
        //{
        //    switch (destination)
        //    {
        //        case "orderPrep":
        //            SelectedViewModel = _orderPrepViewModel;
        //            break;
        //        case "customers":
        //        default:
        //            SelectedViewModel = _customerListViewModel;
        //            break;
        //    }
        //}
    }

    
}
