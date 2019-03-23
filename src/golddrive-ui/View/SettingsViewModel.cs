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

        
        
        private ObservableCollection<Drive> _drives;
        public ObservableCollection<Drive> Drives
        {
            get { return _drives; }
            set { _drives = value; NotifyPropertyChanged(); }
        }
        private ObservableCollection<Drive> _freeDrives;
        public ObservableCollection<Drive> FreeDrives
        {
            get { return _freeDrives; }
            set { _freeDrives = value; NotifyPropertyChanged(); }
        }

        private Drive _selectedDrive;
        public Drive SelectedDrive
        {
            get { return _selectedDrive; }
            set
            {
                _selectedDrive = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("CanConnect");
                NotifyPropertyChanged("HasDrives");
                NotifyPropertyChanged("HasNoDrives");
            }
        }

        public SettingsViewModel(
            MainWindowViewModel mainViewModel,
            IDriver driver)
        {
            _driver = driver;
            _mainViewModel = mainViewModel;
            SaveCommand = new BaseCommand(Save);
            CancelCommand = new BaseCommand(Cancel);
            Drives = new ObservableCollection<Drive>();
            FreeDrives = new ObservableCollection<Drive>();
            LoadDrives();
        }

        private async void LoadDrives()
        {
            List<Drive> drives = await Task.Run(() => _driver.GetGoldDrives());
            List<Drive> freeDrives = await Task.Run(() => _driver.GetFreeDrives());

            foreach (var d in drives)
                Drives.Add(d);
            foreach (var d in freeDrives)
                FreeDrives.Add(d);

            if (Drives.Count > 0)
                SelectedDrive = Drives?[0];

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
