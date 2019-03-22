﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace golddrive
{
    public class BaseViewModel : Observable
    {
        protected IDriver _driver;
        protected MainWindowViewModel _mainViewModel;

        private bool _isWorking;
        public bool IsWorking
        {
            get { return _isWorking; }
            set
            {
                _isWorking = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("CanConnect");
            }
        }
    }
}