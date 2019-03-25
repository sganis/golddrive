using System;
using System.Collections.Generic;

namespace golddrive
{
    [Serializable()]
    public class Drive
    {
        public DriveStatus Status { get; set; }
        public string Letter { get; set; }
        public string VolumeLabel { get; set; }
        public string Label { get; set; }
        public string Host { get; set; }        
        public string Path { get; set; }        
        public bool? IsGoldDrive { get; set; }

        public string Name { get { return $"{ Letter }:"; } }

        private string _mountpoint;
        public string MountPoint
        {
            get { return _mountpoint; }
            set
            {
                _mountpoint = value;
                Host = _mountpoint;
                if (string.IsNullOrEmpty(_mountpoint))
                    return;
                
                string s = _mountpoint;
                if (s.Contains("\\"))
                {
                    Host = s.Split('\\')[0];
                    Path = s.Substring(s.IndexOf("\\")).Replace("\\", "/");
                    s = Host;
                }
                if (s.Contains("!"))
                {
                    Host = s.Split('!')[0];
                    Port = Int32.Parse(s.Split('!')[1]);
                    s = Host;
                }
                if (s.Contains("@"))
                {
                    User = s.Split('@')[0];
                    Host = s.Split('@')[1];                    
                }
            }

        }
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
        public string UserProfile
        {
            get { return Environment.ExpandEnvironmentVariables("%USERPROFILE%"); }
        }
        public string AppKey
        {
            get { return $@"{UserProfile}\.ssh\id_rsa-{User}-golddrive"; }
        }
        public string AppPubKey
        {
            get { return $@"{AppKey}.pub"; }
        }

        public string RegistryMountPoint2
        {
            get { return Remote?.Replace("\\", "#"); }
        }        
        public string Remote
        {
            get { return $@"\\golddrive\{MountPoint}"; }
        }
        public string ComboDisplay
        {
            get
            {   
                //int maxLengh = 15;
                string display = $"{ Letter }: {Label}";
                return display;

                //if (string.IsNullOrEmpty(s))
                //    s = Remote;
                //if (s.Length > maxLengh)
                //    s = "..." + s.Substring(s.Length - maxLengh);
                //return $"{ Letter }: {s}";
            }
        }
    }

    public class DriveList : List<Drive>
    {
        public DriveList() { }
    }
}
