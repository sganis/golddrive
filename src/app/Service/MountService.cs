// src/app/Service/MountService.cs
using Microsoft.Win32;
using Newtonsoft.Json;
using Renci.SshNet;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.ServiceProcess;
using System.Text.RegularExpressions;

namespace golddrive
{
    public class MountService
    {
        #region Properties

        public SshClient Ssh { get; set; }
        public SftpClient Sftp { get; set; }
        public string Error { get; set; }
        public bool Connected { get { return Ssh != null && Ssh.IsConnected; } }
        public List<Drive> Drives { get; } = new List<Drive>();
        public SshService SshService { get; }

        public List<Drive> GoldDrives
        {
            get
            {
                return Drives.Where(x => x.Status != DriveStatus.FREE && x.IsGoldDrive == true).ToList();
            }
        }

        public Drive GetDriveFromArgs(string args)
        {
            Drive drive = new Drive();
            Match m = Regex.Match(args, @"([g-z]): \\\\golddrive\\([^ ]+)", RegexOptions.IgnoreCase);
            if (m.Success)
            {
                drive.Letter = m.Groups[1].Value;
                drive.MountPoint = m.Groups[2].Value;
            }
            return drive;
        }

        public List<Drive> FreeDrives
        {
            get
            {
                List<Drive> list = new List<Drive>(
                    Drives.Where(x => x.Status == DriveStatus.FREE).ToList());
                list.Reverse();
                return list;
            }
        }

        private string appPath;
        public string AppPath
        {
            get
            {
                if (appPath == null)
                {
                    string codeBase = Assembly.GetExecutingAssembly().CodeBase;
                    UriBuilder uri = new UriBuilder(codeBase);
                    string path = Uri.UnescapeDataString(uri.Path);
                    appPath = Path.GetDirectoryName(path);
                }
                return appPath;
            }
        }

        private string localAppData;
        public string LocalAppData
        {
            get
            {
                if (localAppData == null)
                    localAppData = Path.Combine(
                        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                        "golddrive");
                return localAppData;
            }
        }

        #endregion

        public MountService()
        {
            SshService = new SshService(AppPath);
        }

        #region Serialization

        public Settings LoadSettings()
        {
            Settings settings = new Settings()
            {
                Filename = Path.Combine(LocalAppData, "config.json")
            };
            settings.Load();
            return settings;
        }

        public void SaveSettings(Settings settings)
        {
            try
            {
                settings.Filename = Path.Combine(LocalAppData, "config.json");
                using (var file = File.CreateText(settings.Filename))
                {
                    var json = JsonConvert.SerializeObject(
                        settings,
                        Formatting.Indented,
                        new JsonSerializerSettings
                        {
                            NullValueHandling = NullValueHandling.Ignore
                        });
                    file.Write(json);
                }
            }
            catch (Exception ex)
            {
                Logger.Log($"Error saving settings: {ex.Message}");
            }
        }

        #endregion

        #region Core Methods

        public ReturnBox RunLocal(string cmd)
        {
            return RunLocal("cmd.exe", "/C " + cmd);
        }

        public ReturnBox RunLocal(string cmd, string args, int timeout_secs = 30)
        {
            Logger.Log($"Running local command: {cmd} {args}");
            ReturnBox r = new ReturnBox();
            using (Process process = new Process())
            {
                process.StartInfo = new ProcessStartInfo
                {
                    WindowStyle = ProcessWindowStyle.Hidden,
                    CreateNoWindow = true,
                    RedirectStandardError = true,
                    RedirectStandardOutput = true,
                    UseShellExecute = false,
                    FileName = cmd,
                    Arguments = args
                };
                process.Start();
                r.Output = process.StandardOutput.ReadToEnd();
                r.Error = process.StandardError.ReadToEnd();
                process.WaitForExit(timeout_secs * 1000);
                r.ExitCode = process.ExitCode;
                r.Success = r.ExitCode == 0;
            }
            return r;
        }

        public ReturnBox RunRemote(string cmd, int timeout_secs = 3600)
        {
            ReturnBox r = new ReturnBox();
            if (Connected)
            {
                try
                {
                    SshCommand command = Ssh.CreateCommand(cmd);
                    command.CommandTimeout = TimeSpan.FromSeconds(timeout_secs);
                    r.Output = command.Execute();
                    r.Error = command.Error;
                    r.ExitCode = command.ExitStatus;
                }
                catch (Exception ex)
                {
                    r.Error = ex.Message;
                    Logger.Log($"RunRemote error: {ex.Message}");
                }
            }
            r.Success = r.ExitCode == 0 && String.IsNullOrEmpty(r.Error);
            return r;
        }

        #endregion

        #region Local Drive Management

        public void UpdateDrives(Settings settings)
        {
            string GOLDLETTERS = "GHIJKLMNOPQRSTUVWXYZ";
            List<char> letters = GOLDLETTERS.ToCharArray().ToList();

            DriveInfo[] drives = DriveInfo.GetDrives();
            Drives.Clear();
            var settingsDrives = settings.Drives.Values.ToList();
            var netUseDrives = GetUsedDrives();

            foreach (char c in letters)
            {
                bool used = false;
                Drive d = new Drive { Letter = c.ToString() };

                for (int i = 0; i < drives.Length; i++)
                {
                    try
                    {
                        DriveInfo dinfo = drives[i];
                        if (dinfo.Name[0] == c)
                        {
                            d.Status = DriveStatus.UNKNOWN;
                            d.IsGoldDrive = dinfo.DriveFormat == "FUSE-Golddrive";
                            used = true;
                            if (d.IsGoldDrive == true)
                            {
                                d.MountPoint = dinfo.VolumeLabel.Replace("/", "\\");
                                d.Label = GetExplorerDriveLabel(d);
                                d.Status = dinfo.IsReady ? DriveStatus.CONNECTED : DriveStatus.BROKEN;
                                var d1 = settingsDrives.Find(x => x.Letter == d.Letter);
                                if (d1 != null)
                                {
                                    d.Args = d1.Args;
                                    d.Label = d1.Label;
                                }
                            }
                            Drives.Add(d);
                            break;
                        }
                    }
                    catch (IOException ex)
                    {
                        Logger.Log($"Drive {c}: IOException: {ex.Message}");
                    }
                    catch (Exception ex)
                    {
                        Logger.Log($"Drive {c}: Error: {ex.Message}");
                    }
                }

                if (!used)
                {
                    var d0 = netUseDrives.Find(x => x.Letter == d.Letter);
                    if (d0 != null)
                    {
                        d.IsGoldDrive = d0.IsGoldDrive;
                        d.Status = d0.Status;
                        if (d.IsGoldDrive == true)
                        {
                            d.Status = DriveStatus.BROKEN;
                            d.MountPoint = d0.MountPoint;
                            d.Label = d0.Label;
                            var d1 = settingsDrives.Find(x1 => x1.Letter == d.Letter);
                            if (d1 != null)
                            {
                                d.Args = d1.Args;
                                d.Label = d1.Label;
                            }
                        }
                        else
                        {
                            d.Status = DriveStatus.UNKNOWN;
                        }
                    }
                    else
                    {
                        var d1 = settingsDrives.Find(x1 => x1.Letter == d.Letter);
                        if (d1 != null)
                        {
                            d.Status = DriveStatus.DISCONNECTED;
                            d.MountPoint = d1.MountPoint;
                            d.Args = d1.Args;
                            d.Label = d1.Label;
                            d.IsGoldDrive = true;
                        }
                        else
                        {
                            d.Status = DriveStatus.FREE;
                        }
                    }
                    Drives.Add(d);
                }
            }
        }

        public List<Drive> GetUsedDrives()
        {
            List<Drive> drives = new List<Drive>();
            var r = RunLocal("net.exe", "use");
            foreach (var line in r.Output.Split('\n'))
            {
                Match match = Regex.Match(line, @"^([A-Za-z]+)?\s+([A-Z]:)\s+(\\\\[^ ]+)");
                if (match.Success)
                {
                    try
                    {
                        Drive d = new Drive
                        {
                            Letter = match.Groups[2].Value[0].ToString(),
                            IsGoldDrive = match.Groups[3].Value.Contains(@"\\golddrive\")
                        };
                        d.Status = match.Groups[1].Value == "Unavailable" ? DriveStatus.BROKEN : DriveStatus.IN_USE;
                        if (d.IsGoldDrive == true)
                        {
                            d.MountPoint = match.Groups[3].Value.Replace(@"\\golddrive\", "");
                            d.Label = GetExplorerDriveLabel(d);
                        }
                        drives.Add(d);
                    }
                    catch (Exception ex)
                    {
                        Logger.Log($"GetUsedDrives parse error: {ex.Message}");
                    }
                }
            }
            return drives;
        }

        public ReturnBox CheckDriveStatus(Drive drive)
        {
            ReturnBox r = new ReturnBox { MountStatus = MountStatus.OK };

            if (drive == null ||
                (drive.Letter.ToCharArray()[0] < 'G' || drive.Letter.ToCharArray()[0] > 'Z'))
            {
                r.DriveStatus = DriveStatus.NOT_SUPPORTED;
            }
            else
            {
                Settings settings = LoadSettings();
                UpdateDrives(settings);
                var free = FreeDrives.Find(x => x.Letter == drive.Letter) != null;
                var isGold = GoldDrives.Find(x => x.Letter == drive.Letter) != null;
                var disconnected = GoldDrives.Find(x => x.Letter == drive.Letter && x.Status == DriveStatus.DISCONNECTED) != null;
                var pathUsed = GoldDrives.Find(x => x.Letter != drive.Letter && x.MountPoint == drive.MountPoint
                                                && x.Status != DriveStatus.DISCONNECTED && x.Status != DriveStatus.FREE) != null;

                if (pathUsed)
                {
                    r.MountStatus = MountStatus.BAD_DRIVE;
                    r.DriveStatus = DriveStatus.MOUNTPOINT_IN_USE;
                    r.Error = "Mount point in use";
                }
                else if (free || disconnected)
                    r.DriveStatus = DriveStatus.DISCONNECTED;
                else if (!isGold)
                {
                    r.MountStatus = MountStatus.BAD_DRIVE;
                    r.DriveStatus = DriveStatus.IN_USE;
                    r.Error = "Drive in use";
                }
                else if (!CheckIfDriveWorks(drive))
                {
                    r.MountStatus = MountStatus.BAD_DRIVE;
                    r.DriveStatus = DriveStatus.BROKEN;
                    r.Error = "Drive is broken";
                }
                else
                    r.DriveStatus = DriveStatus.CONNECTED;
            }
            r.Drive = drive;
            return r;
        }

        public bool CheckIfDriveWorks(Drive drive)
        {
            var info = new DriveInfo(drive.Letter);
            try { return info.AvailableFreeSpace >= 0; }
            catch (IOException) { return false; }
        }

        #endregion

        #region Registry Helpers

        public string GetExplorerDriveLabel(Drive drive)
        {
            try
            {
                string key = $@"Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2\{drive.RegistryMountPoint2}";
                using (RegistryKey k = Registry.CurrentUser.OpenSubKey(key))
                {
                    if (k != null)
                        return k.GetValue("_LabelFromReg")?.ToString();
                }
            }
            catch (Exception ex)
            {
                Logger.Log($"GetExplorerDriveLabel error: {ex.Message}");
            }
            return "";
        }

        public void SetExplorerDriveLabel(Drive drive)
        {
            if (String.IsNullOrEmpty(drive.Label))
                return;
            try
            {
                string key = $@"Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2\{drive.RegistryMountPoint2}";
                using (RegistryKey k = Registry.CurrentUser.CreateSubKey(key))
                {
                    if (k != null)
                        k.SetValue("_LabelFromReg", drive.Label, RegistryValueKind.String);
                }
            }
            catch (Exception ex)
            {
                Logger.Log($"SetExplorerDriveLabel error: {ex.Message}");
            }
        }

        public void CleanExplorerDriveLabel(Drive drive)
        {
            if (String.IsNullOrEmpty(drive.RegistryMountPoint2))
                return;
            try
            {
                string key = $@"Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2\{drive.RegistryMountPoint2}";
                Registry.CurrentUser.DeleteSubKey(key);
            }
            catch (Exception ex)
            {
                Logger.Log($"CleanExplorerDriveLabel error: {ex.Message}");
            }
        }

        public void SetDriveIcon(Drive drive, string icoPath)
        {
            try
            {
                string key = $@"Software\Classes\Applications\Explorer.exe\Drives\{drive.Letter}\DefaultIcon";
                using (RegistryKey k = Registry.CurrentUser.CreateSubKey(key))
                {
                    if (k != null)
                        k.SetValue("", icoPath, RegistryValueKind.String);
                }
            }
            catch (Exception ex)
            {
                Logger.Log($"SetDriveIcon error: {ex.Message}");
            }
        }

        #endregion

        #region Mount Management

        public ReturnBox Connect(Drive drive, IProgress<string> status)
        {
            ReturnBox r = new ReturnBox();
            if (!IsWinfspInstalled())
            {
                r.MountStatus = MountStatus.BAD_WINFSP;
                r.Error = "Winfsp is not installed\n";
                return r;
            }
            if (!IsCliInstalled())
            {
                r.MountStatus = MountStatus.BAD_CLI;
                r.Error = "Goldrive CLI is not installed\n";
                return r;
            }
            r = CheckDriveStatus(drive);
            if (r.DriveStatus != DriveStatus.DISCONNECTED)
            {
                r.MountStatus = MountStatus.BAD_DRIVE;
                return r;
            }
            status?.Report("Checking server...");
            r = SshService.TestHost(drive);
            if (r.MountStatus != MountStatus.OK)
                return r;
            status?.Report("Authenticating...");
            r = SshService.TestSsh(drive);
            if (r.MountStatus != MountStatus.OK)
                return r;
            status?.Report("Mounting drive...");
            return Mount(drive);
        }

        public ReturnBox ConnectPassword(Drive drive, string password, IProgress<string> status)
        {
            status?.Report("Connecting...");
            ReturnBox r = SshService.TestPassword(drive, password);
            if (r.MountStatus != MountStatus.OK)
                return r;
            status?.Report("Generating ssh keys...");
            r = SshService.SetupSsh(drive, password);
            if (r.MountStatus != MountStatus.OK)
                return r;
            status?.Report("Mounting drive...");
            return Mount(drive);
        }

        private bool IsWinfspInstalled()
        {
            ServiceController[] services = ServiceController.GetServices();
            var service = services.FirstOrDefault(s => s.ServiceName == "WinFsp.Launcher");
            if (service != null)
                return service.Status == ServiceControllerStatus.Running;
            return false;
        }

        private bool IsCliInstalled()
        {
            return File.Exists(GetGolddriveCliPath());
        }

        private string GetGolddriveCliPath()
        {
            try
            {
                string key = $@"Software\WOW6432Node\WinFsp\Services\golddrive";
                using (RegistryKey k = Registry.LocalMachine.OpenSubKey(key))
                {
                    if (k != null)
                        return k.GetValue("Executable")?.ToString();
                }
            }
            catch (Exception ex)
            {
                Logger.Log($"Cannot get registry path: {ex.Message}");
            }
            return "";
        }

        public string GetVersions()
        {
            try
            {
                string app = Assembly.GetExecutingAssembly().GetName().Version.ToString();
                string cli = "n/a";
                string winfsp = "n/a";
                string golddrive_cli = GetGolddriveCliPath();
                string winfsp_dll = Environment.ExpandEnvironmentVariables(@"%ProgramFiles(x86)%\WinFsp\bin\winfsp-x64.dll");

                if (File.Exists(golddrive_cli))
                {
                    var r = RunLocal($@"""{golddrive_cli}"" --version");
                    cli = r.Error.Trim();
                }
                if (IsWinfspInstalled())
                {
                    FileVersionInfo info = FileVersionInfo.GetVersionInfo(winfsp_dll);
                    winfsp = $"{info.FileMajorPart}.{info.FileMinorPart}.{info.FileBuildPart}";
                }
                return $"App {app}\n{cli}\nWinFsp {winfsp}";
            }
            catch (Exception ex)
            {
                Logger.Log($"GetVersions error: {ex.Message}");
            }
            return "n/a";
        }

        public ReturnBox Mount(Drive drive)
        {
            ReturnBox r = RunLocal("net.exe", $"use {drive.Name} {drive.Remote} /persistent:yes");
            if (!r.Success)
            {
                r.MountStatus = MountStatus.UNKNOWN;
                r.Drive = drive;
                return r;
            }
            SetExplorerDriveLabel(drive);
            SetDriveIcon(drive, Path.Combine(AppPath, "golddrive.ico"));
            Settings settings = LoadSettings();
            settings.AddDrive(drive);
            SaveSettings(settings);
            UpdateDrives(settings);
            r.MountStatus = MountStatus.OK;
            r.DriveStatus = DriveStatus.CONNECTED;
            r.Drive = drive;
            return r;
        }

        public ReturnBox Unmount(Drive drive)
        {
            ReturnBox r = RunLocal("net.exe", "use /d " + drive.Name);
            if (!r.Success)
            {
                r.Drive = drive;
                return r;
            }
            CleanExplorerDriveLabel(drive);
            Settings settings = LoadSettings();
            SaveSettings(settings);
            UpdateDrives(settings);
            drive.Status = DriveStatus.DISCONNECTED;
            r.MountStatus = MountStatus.OK;
            r.DriveStatus = DriveStatus.DISCONNECTED;
            r.Drive = drive;
            return r;
        }

        #endregion

        public string GetUid(string user)
        {
            return RunRemote($"id -u {user}").Output;
        }
    }
}
