using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace golddrive
{
    public class Drive
    {
        private DriveStatus status;
        public DriveStatus Status
        {
            get { return status; }
            set { status = value; }
        }
        private string netuse_status;

        public string NetUseStatus
        {
            get { return netuse_status; }
            set { netuse_status = value; }
        }

        private string remote;

        public string Remote
        {
            get { return remote; }
            set { remote = value; }
        }


        private string letter;
        public string Letter
        {
            get { return letter; }
            set { letter = value; }
        }
        private string label;
        public string Label
        {
            get { return label; }
            set { label = value; }
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
            get
            {
                string display = $"{ letter }:";
                string description = name;
                if (string.IsNullOrEmpty(description))
                    description = remote;
                return display + " " + description;
            }
        }
        public string DriveDisplay
        {
            get { return $"{ letter }:"; }
        }

    }
}
