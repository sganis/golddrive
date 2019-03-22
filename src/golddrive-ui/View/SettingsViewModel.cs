using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace golddrive
{
    public class SettingsViewModel: BaseViewModel
    {
        public ICommand SaveCommand { get; set; }
        public ICommand CancelCommand { get; set; }

        public SettingsViewModel(
            MainWindowViewModel mainViewModel,
            IDriver driver)
        {
            _driver = driver;
            _mainViewModel = mainViewModel;
            SaveCommand = new BaseCommand(Save);
            CancelCommand = new BaseCommand(Cancel);
        }
        
        private void Save(object obj)
        {
            _mainViewModel.ShowConnectCommand.Execute(null);
        }

        private void Cancel(object obj)
        {
            _mainViewModel.ShowConnectCommand.Execute(null);
        }

    }
}
