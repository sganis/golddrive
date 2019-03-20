using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace golddrive_ui
{
    public class Drive
    {
        private string letter;

        public string Letter
        {
            get { return letter; }
            set { letter = value; }
        }

        private string name;

        public string Name
        {
            get { return name; }
            set { name = value; }
        }
        private string host;

        public string Host
        {
            get { return host; }
            set { host = value; }
        }

        private string user;

        public string User
        {
            get { return user; }
            set { user = value; }
        }

        public string ComboDisplay
        {
            get { return $"{ letter }:   { name }"; }
        }
        public string DriveDisplay
        {
            get { return $"{ letter }:"; }
        }

    }
}
