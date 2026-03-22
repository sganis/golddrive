// src/app/Common/Drive.cs
using Newtonsoft.Json;
using System;
using System.Collections.Generic;

namespace golddrive
{
    [JsonObject(MemberSerialization.OptIn)]
    public class Drive : Observable
    {
        public Drive()
        {
        }
        public Drive(Drive d)
        {
            Clone(d);
        }
        public void Clone(Drive d)
        {
            if (d == null)
                return;
            Host = d.Host;
            Port = d.Port;
            User = d.User;
            Letter = d.Letter;
            Label = d.Label;
            AppKey = d.AppKey;
            Args = d.Args;
        }
        public DriveStatus Status { get; set; }

        public bool? IsGoldDrive { get; set; }

        public string Path { get; set; }

        private string _host;
        public string Host
        {
            get { return _host; }
            set
            {
                if (_host != value)
                {
                    _host = value;
                    NotifyPropertyChanged();
                }
            }
        }
        public string Letter { get; set; }

        public string Name { get { return $"{ Letter }:"; } }

        private string _args;

        [JsonProperty]
        public string Args
        {
            get { return string.IsNullOrEmpty(_args) ? null : _args; }
            set
            {
                if (_args != value)
                {
                    _args = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private string _label;
        [JsonProperty]
        public string Label
        {
            get { return _label; }
            set
            {
                if (_label != value)
                {
                    _label = value;
                    NotifyPropertyChanged();
                    NotifyPropertyChanged("ComboDisplay");
                }
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
                if (!string.IsNullOrEmpty(Port) && Port != "22")
                    s = string.Format($"{Host}!{Port}");
                if (!string.IsNullOrEmpty(User) && User != EnvironmentUser)
                    s = string.Format($"{CurrentUser}@{s}");
                if (!string.IsNullOrEmpty(Path))
                    s = string.Format($"{s}{Path}");
                return s;
            }
            set
            {
                User = "";
                Host = "";
                Port = "";
                Path = "";
                NotifyPropertyChanged("User");
                NotifyPropertyChanged("Host");
                NotifyPropertyChanged("Port");
                NotifyPropertyChanged("Path");

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
                        Port = p.ToString();
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
            get { return user; }
            set
            {
                if (user != value)
                {
                    user = value;
                    NotifyPropertyChanged();
                }
            }
        }
        public string CurrentUser
        {
            get
            {
                return string.IsNullOrEmpty(User) ? EnvironmentUser : User;
            }
        }
        public string EnvironmentUser
        {
            get { return Environment.UserName.ToLower(); }
        }

        private string port;
        public string Port
        {
            get { return port; }
            set
            {
                if (port != value)
                {
                    port = value;
                    NotifyPropertyChanged();
                }
            }
        }
        public int CurrentPort
        {
            get
            {
                if (int.TryParse(Port, out int p) && p > 0 && p <= 65535)
                    return p;
                return 22;
            }
        }

        public string UserProfile
        {
            get
            {
                return Environment.ExpandEnvironmentVariables("%USERPROFILE%");
            }
        }

        private string appkey;

        [JsonProperty]
        public string AppKey
        {
            get { return appkey; }
            set
            {
                if (AppKey != value)
                {
                    appkey = value;
                    NotifyPropertyChanged();
                }
            }
        }
        public string DefaultAppKey
        {
            get { return $@"{UserProfile}\.ssh\id_rsa"; }
        }
        public string DefaultAppPubKey
        {
            get { return $@"{DefaultAppKey}.pub"; }
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
            get { return $"{ Name } {Label}"; }
        }

        public void Trim()
        {
            Host = Host?.Trim();
            Port = Port?.Trim();
            Label = Label?.Trim();
            User = User?.Trim().ToLower();
            Args = Args?.Trim();
            if (Port == "22")
                Port = "";
            if (User == EnvironmentUser)
                User = "";
        }

        public override string ToString()
        {
            return $"{ Name } {MountPoint}";
        }
    }

    public class DriveList : List<Drive>
    {
        public DriveList() { }
    }
}
