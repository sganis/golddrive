using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace golddrive
{
    public class SettingsViewModel : BaseViewModel
    {
        public ICommand SaveCommand { get; set; }
        public ICommand CancelCommand { get; set; }
        public ICommand NewCommand { get; set; }
        public ICommand DeleteCommand { get; set; }
        
        public SettingsViewModel(
            MainWindowViewModel mainViewModel,
            IDriver driver)
        {
            Driver = driver;
            _mainViewModel = mainViewModel;
            SaveCommand = new BaseCommand(Save);
            CancelCommand = new BaseCommand(Cancel);
            DeleteCommand = new BaseCommand(Delete);
           
        }

        

        private void Save(object obj)
        {
            _mainViewModel.ShowConnectCommand.Execute(null);
        }

        private void Cancel(object obj)
        {
            _mainViewModel.ShowConnectCommand.Execute(null);
        }
        private void Delete(object obj)
        {
            Driver.Unmount(Driver.SelectedDrive);
            Driver.Drives.Remove(Driver.SelectedDrive);
            Driver.SaveSettingsDrives(Driver.Drives.ToList());
            if (Driver.Drives.Count > 0)
                Driver.SelectedDrive = Driver.Drives[0];
        }
    }
}
