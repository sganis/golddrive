// src/app/Common/Settings.cs
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
        public string UsageUrl { get; set; }
        public string Selected { get; set; }
        public Dictionary<string, Drive> Drives { get; set; }

        public void AddDrives(IEnumerable<Drive> drives)
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
                d.Label = drive.Label;
                d.Args = drive.Args;
                d.AppKey = drive.AppKey;
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

        public void Load()
        {
            if (!File.Exists(Filename))
                return;

            try
            {
                string args = "";
                var json = JsonConvert.DeserializeObject<Dictionary<string, object>>(File.ReadAllText(Filename));
                if (json.ContainsKey("UsageUrl"))
                {
                    UsageUrl = json["UsageUrl"].ToString();
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
                    if (data.ContainsKey("AppKey") && data["AppKey"] != null)
                        d.AppKey = data["AppKey"].ToString();
                    if (data.ContainsKey("Label") && data["Label"] != null)
                        d.Label = data["Label"].ToString();
                    if (data.ContainsKey("MountPoint") && data["MountPoint"] != null)
                        d.MountPoint = data["MountPoint"].ToString();
                    Drives[d.Name] = d;
                }
                var selected = Drives.Values.ToList().Find(x => x.Name == Selected);
                if (selected != null)
                {
                    SelectedDrive = selected;
                }
            }
            catch (JsonException ex)
            {
                Logger.Log($"Error parsing settings JSON ({Filename}): {ex.Message}");
                // preserve corrupted file for diagnosis
                try
                {
                    string backup = Filename + ".corrupt";
                    if (!File.Exists(backup))
                        File.Copy(Filename, backup);
                }
                catch { }
            }
            catch (Exception ex)
            {
                Logger.Log($"Error loading settings ({Filename}): {ex.Message}");
            }
        }
    }
}
