using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace golddrive
{
    public interface IRequestFocus
    {
        event EventHandler<FocusRequestedEventArgs> FocusRequested;
    }
    public class FocusRequestedEventArgs : EventArgs
    {
        public FocusRequestedEventArgs(string propertyName)
        {
            PropertyName = propertyName;
        }

        public string PropertyName { get; private set; }
    }
}
