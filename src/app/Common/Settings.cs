#pragma warning disable CS0168
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace golddrive
{
    [JsonObject(MemberSerialization.OptOut)]
    public class Settings
    {
        //public string Args { get; set; }
        //public string LogFile { get; set; }
        public string UsageUrl { get; set; }
        public string Selected { get; set; }
        public Dictionary<string, Drive> Drives { get; set; }

        public void AddDrives(List<Drive> drives)
        {
            Drives.Clear();
            foreach (var d in drives)
                Drives[d.Name] = d;
        }
        public void AddDrive(Drive drive)
        {
            if (Drives.ContainsKey(drive.Name))
            {
                var d = Drives[drive.Name];
                d.MountPoint = drive.MountPoint;
                d.Status = drive.Status;

            }
            else
            {
                Drives[drive.Name] = drive;
            }
        }

        [JsonIgnore]
        public Drive SelectedDrive { get; set; }

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
                //if (json.ContainsKey("Args"))
                //    Args = json["Args"].ToString();
                //LogFile = "C:\\Users\\Sant\\Desktop\\golddrive.log";
                
                //if (json.ContainsKey("LogFile"))
                //    LogFile = json["LogFile"].ToString();
                if (json.ContainsKey("UsageUrl"))
                    UsageUrl = json["UsageUrl"].ToString();
                else
                {
                    // testing url logging
                    //UsageUrl = "https://192.168.100.201:5000";
                    UsageUrl = "https://box.chaintrust.com:5000";
                }
                if (json.ContainsKey("Selected"))
                    Selected = json["Selected"].ToString();
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
                    if (data.ContainsKey("Args") && data["Args"] != null)
                        d.Args = data["Args"].ToString();
                    if (data.ContainsKey("Label") && data["Label"] != null)
                        d.Label = data["Label"].ToString();
                    if (data.ContainsKey("MountPoint") && data["MountPoint"] != null)
                        d.MountPoint = data["MountPoint"].ToString();
                    //if (data.ContainsKey("Hosts") && data["Hosts"] != null)
                    //    d.Hosts = JsonConvert.DeserializeObject<List<string>>(data["Hosts"].ToString());
                    Drives[d.Name] = d;
                }
                var selected = Drives.Values.ToList().Find(x => x.Name == Selected);
                if (selected != null)
                {
                    SelectedDrive = selected;
                }
            }
            catch (Exception ex)
            {
            }
        }
    }
}
