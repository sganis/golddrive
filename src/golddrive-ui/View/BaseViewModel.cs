using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace golddrive_ui
{
    public class BaseViewModel : Observable
    {
        private bool _isWorking;
        public bool IsWorking
        {
            get
            {
                return _isWorking;
            }

            set
            {
                _isWorking = value;
                NotifyPropertyChanged();
            }
        }
    }
}
