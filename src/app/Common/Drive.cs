using Newtonsoft.Json;
using System;
using System.Collections.Generic;

namespace golddrive
{
    [JsonObject(MemberSerialization.OptIn)]
    public class Drive : Observable
    {
        public DriveStatus Status { get; set; }

        public bool? IsGoldDrive { get; set; }

        public string ServerOS { get; set; }

        public string Path { get; set; }

        public string Host { get; set; }

        public string Letter { get; set; }

        public string Name { get { return $"{ Letter }:"; } }

        private string _args;

        [JsonProperty]
        public string Args
        {
            get { return string.IsNullOrEmpty(_args) ? null : _args; }
            set
            {
                _args = value;
                NotifyPropertyChanged();
            }
        }

        private List<string> _hosts;
        [JsonProperty]
        public List<string> Hosts
        {
            get
            {
                return _hosts ?? new List<string>();
            }
            set
            {
                _hosts = value;
            }
        }

        private string _label;
        [JsonProperty]
        public string Label
        {
            get { return _label; }
            set
            {
                _label = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("ComboDisplay");
            }
        }

        [JsonProperty]
        public string MountPoint
        {
            get 
            {
                string s = Host;
                if (string.IsNullOrEmpty(s))
                    return s;
                if (Port != 22)
                    s = string.Format($"{Host}!{Port}");
                if (User != Environment.UserName)
                    s = string.Format($"{User}@{s}");
                if (!string.IsNullOrEmpty(Path))
                    s = string.Format($"{s}{Path}");
                return s;  
            }
            set
            {
                string s = value;
                Host = s;
                if (string.IsNullOrEmpty(s))
                    return;

                
                if (s.Contains("\\"))
                {
                    Host = s.Split('\\')[0];
                    Path = s.Substring(s.IndexOf("\\")).Replace("\\", "/");
                    s = Host;
                    NotifyPropertyChanged("Path");
                }
                if (s.Contains("!"))
                {
                    Host = s.Split('!')[0];
                    int.TryParse(s.Split('!')[1], out int p);
                    if (p > 0)
                        Port = p;
                    s = Host;
                    NotifyPropertyChanged("Port");
                }
                if (s.Contains("@"))
                {
                    User = s.Split('@')[0];
                    Host = s.Split('@')[1];
                    NotifyPropertyChanged("User");
                }
                NotifyPropertyChanged();
                NotifyPropertyChanged("Host");
            }

        }

        private string user;
        public string User
        {
            get { return user == null ? Environment.UserName.ToLower() : user; }
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
            get
            {
                var prof = Environment.ExpandEnvironmentVariables("%USERPROFILE%");
                //if(!prof.Contains(User))
                //    prof = Regex.Replace(prof, @"(?i)([a-z]:\\Users\\)([a-z0-9_\.]+)", $@"$1{User}");
                return prof;
            }
        }
        public string AppKey
        {
            get { return $@"{UserProfile}\.ssh\id_golddrive_{User}"; }
            //get { return $@"{UserProfile}\.ssh\id_rsa"; }
        }
        //public string UserKey
        //{
        //    get { return $@"{UserProfile}\.ssh\id_rsa"; }
        //}
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
                string display = $"{ Name } {Label}";
                return display;

                //if (string.IsNullOrEmpty(s))
                //    s = Remote;
                //if (s.Length > maxLengh)
                //    s = "..." + s.Substring(s.Length - maxLengh);
                //return $"{ Letter }: {s}";
            }
        }

        public override string ToString()
        {
            return $"{ Name } {MountPoint}"; ;
        }
    }

    public class DriveList : List<Drive>
    {
        public DriveList() { }
    }
}
