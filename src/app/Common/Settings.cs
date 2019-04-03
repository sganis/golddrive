using Newtonsoft.Json;
using System.Collections.Generic;
using System;

namespace golddrive
{
    [JsonObject(MemberSerialization.OptOut)]
    public class Settings
    {
        private string _args;

        public string Args
        {
            get { return _args?? (_args = ""); }
            set { _args = value; }
        }

        public Dictionary<string,Drive> Drives { get; set; }

        public void AddDrives(List<Drive> drives)
        {
            Drives = new Dictionary<string, Drive>();
            foreach (var d in drives)
            {
                Drives[d.Name] = d;
            }
        }
    }
}
