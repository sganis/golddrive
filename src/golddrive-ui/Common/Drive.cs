using System;

namespace golddrive
{
    [Serializable()]
    public class Drive
    {
        public DriveStatus Status { get; set; }
        public string NetUseStatus { get; set; }

        private string remote;
        public string Remote {
            get { return remote; }
            set
            {
                remote = value;
                string hostport = remote;
                if (remote.Contains("\\"))
                {
                    hostport = remote.Split('\\')[0];
                    Path = remote.Substring(remote.IndexOf("\\")).Replace("\\", "/");
                }
                if (hostport.Contains("!"))
                {
                    hostport = remote.Split('\\')[0];
                    Port = Int32.Parse(hostport.Split('!')[1]);
                }
            }

        }
        public string Letter { get; set; }
        public string Label { get; set; }
        public string Name { get; set; }
        public string Host { get; set; }
        
        public string Path { get; set; }
        
        public bool? IsGoldDrive { get; set; }

        private string user;
        public string User
        {
            get { return user == null ? Environment.UserName : user; }
            set { user = value; }
        }

        private int port;
        public int Port
        {
            get { return port == 0 ? 22 : port; }
            set { port = value; }
        }

        public string MountPoint2
        {
            get { return MountPoint?.Replace("\\", "#"); }
        }        
        public string MountPoint
        {
            get { return $@"\\golddrive\{Remote}"; }
        }
        public string ComboDisplay
        {
            get
            {
                
                //int maxLengh = 15;
                string display = $"{ Letter }: {Name}";
                return display;

                //if (string.IsNullOrEmpty(s))
                //    s = Remote;
                //if (s.Length > maxLengh)
                //    s = "..." + s.Substring(s.Length - maxLengh);
                //return $"{ Letter }: {s}";
            }
        }
        public string LetterColon
        {
            get { return $"{ Letter }:"; }
        }

    }
}
