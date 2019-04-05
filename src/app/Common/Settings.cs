#pragma warning disable CS0168
using Newtonsoft.Json;
using System.Collections.Generic;
using System;
using System.IO;

namespace golddrive
{
    [JsonObject(MemberSerialization.OptOut)]
    public class Settings
    {
        public string Args { get; set; }
        public Dictionary<string,Drive> Drives { get; set; }

        public void AddDrives(List<Drive> drives)
        {
            Drives.Clear();
            foreach (var d in drives)
                Drives[d.Name] = d;            
        }
        [JsonIgnore]
        public string Filename { get; set; }

        public Settings()
        {
            Drives = new Dictionary<string, Drive>();
        }

        internal void Load()
        {
            if (!File.Exists(Filename))
                return;

            try
            {
                string args = "";
                var json = JsonConvert.DeserializeObject<Dictionary<string, object>>(File.ReadAllText(Filename));
                if (json.ContainsKey("Args"))
                    Args = json["Args"].ToString();
                if (!json.ContainsKey("Drives"))
                    return;
                var _drives = JsonConvert.DeserializeObject<Dictionary<string, object>>(json["Drives"].ToString());
                foreach (var _d in _drives.Keys)
                {
                    if (_d.Length < 2)
                        continue;
                    Drive d = new Drive
                    {
                        Letter = _d[0].ToString(),
                        Args = args
                    };
                    var data = JsonConvert.DeserializeObject<Dictionary<string, object>>(_drives[_d].ToString());
                    if (data.ContainsKey("Args"))
                        d.Args = data["Args"].ToString();
                    if (data.ContainsKey("Label"))
                        d.Label = data["Label"].ToString();
                    if (data.ContainsKey("MountPoint"))
                        d.MountPoint = data["MountPoint"].ToString();
                    if (data.ContainsKey("Hosts"))
                        d.Hosts = JsonConvert.DeserializeObject<List<string>>(data["Hosts"].ToString());
                    Drives[d.Name] = d;
                }
            }
            catch (Exception ex)
            {
            }
        }
    }
}
