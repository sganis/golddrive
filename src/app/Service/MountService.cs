﻿#pragma warning disable CS0168
using Microsoft.Win32;
using Newtonsoft.Json;
using Renci.SshNet;
using Renci.SshNet.Common;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Reflection;
using System.ServiceProcess;
using System.Text.RegularExpressions;

namespace golddrive
{

    public class MountService //, IDriver
    {
        #region Properties

        const int TIMEOUT = 20; // secs
        public SshClient Ssh { get; set; }
        public SftpClient Sftp { get; set; }
        public string Error { get; set; }
        public bool Connected { get { return Ssh != null && Ssh.IsConnected; } }
        public List<Drive> Drives { get; } = new List<Drive>();
        public List<Drive> GoldDrives
        {
            get
            {
                return Drives.Where(x => x.Status != DriveStatus.FREE && x.IsGoldDrive == true).ToList();
            }
        }

        public Drive GetDriveFromArgs(string args)
        {
            // args: Y: \\golddrive\user@host!port -uother...
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
                    localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + "\\golddrive";
                return localAppData;
            }
        }

        #endregion

        public MountService()
        {

        }

        #region Serialization


        public Settings LoadSettings()
        {
            Settings settings = new Settings() {
                Filename = LocalAppData + "\\config.json" 
            };
            settings.Load();
            return settings;
        }

        public void SaveSettings(Settings settings)
        {
            try
            {
                settings.Filename = LocalAppData + "\\config.json";
                
                using (var file = File.CreateText(settings.Filename))
                {
                    var json = JsonConvert.SerializeObject(
                        settings,
                        Newtonsoft.Json.Formatting.Indented,
                        new JsonSerializerSettings
                        {
                            NullValueHandling = NullValueHandling.Ignore
                        });
                    file.Write(json);
                }
            }
            catch (Exception ex)
            {
            }
        }

        #endregion

        #region Core Methods

        //public bool Connect(string host, int port, string user, string pkey)
        //{
        //    try
        //    {
        //        var pk = new PrivateKeyFile(pkey);
        //        var keyFiles = new[] { pk };
        //        Ssh = new SshClient(host, port, user, keyFiles);
        //        Ssh.ConnectionInfo.Timeout = TimeSpan.FromSeconds(5);
        //        Ssh.Connect();
        //        //Sftp = new SftpClient(host, port, user, keyFiles);
        //        //Sftp.ConnectionInfo.Timeout = TimeSpan.FromSeconds(5);
        //        //Sftp.Connect();
        //    }
        //    catch (Renci.SshNet.Common.SshAuthenticationException ex)
        //    {
        //        // bad key
        //        Error = ex.Message;
        //    }
        //    catch (Exception ex)
        //    {
        //        Error = ex.Message;
        //    }
        //    return Connected;
        //}

        public ReturnBox RunLocal(string cmd)
        {
            // 2 secs slower
            return RunLocal("cmd.exe", "/C " + cmd);
        }

        public ReturnBox RunLocal(string cmd, string args, 
            int timeout_secs = 30)
        {
            Logger.Log($"Running local command: {cmd} {args}");
            ReturnBox r = new ReturnBox();
            Process process = new Process();
            ProcessStartInfo startInfo = new ProcessStartInfo
            {
                WindowStyle = ProcessWindowStyle.Hidden,
                CreateNoWindow = true,
                RedirectStandardError = true,
                RedirectStandardOutput = true,
                UseShellExecute = false,
                FileName = cmd,
                Arguments = args
            };
            process.StartInfo = startInfo;
            process.Start();
            process.WaitForExit(timeout_secs * 1000);
            r.Output = process.StandardOutput.ReadToEnd();
            r.Error = process.StandardError.ReadToEnd();
            r.ExitCode = process.ExitCode;
            r.Success = r.ExitCode == 0;
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
                }
            }
            r.Success = r.ExitCode == 0 && String.IsNullOrEmpty(r.Error);
            return r;
        }

        //public ReturnBox DownloadFile(string src, string dst)
        //{
        //    ReturnBox r = new ReturnBox();
        //    if (Connected)
        //    {
        //        try
        //        {
        //            using (Stream fs = File.Create(dst))
        //            {
        //                Sftp.DownloadFile(src, fs);
        //            }
        //            r.Success = true;
        //        }
        //        catch (Exception ex)
        //        {
        //            r.Error = ex.Message;
        //        }
        //    }
        //    return r;
        //}

        //public ReturnBox UploadFile(string src, string dir, string filename)
        //{
        //    ReturnBox r = new ReturnBox();
        //    if (Connected)
        //    {
        //        try
        //        {
        //            using (var fs = new FileStream(src, FileMode.Open))
        //            {
        //                Sftp.BufferSize = 4 * 1024; // bypass Payload error large files
        //                Sftp.ChangeDirectory(dir);
        //                Sftp.UploadFile(fs, filename, true);
        //            }
        //            r.Success = true;
        //        }
        //        catch (Exception ex)
        //        {
        //            r.Error = ex.Message;
        //        }
        //    }
        //    return r;
        //}

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

            foreach (char c in letters) {
                //if (c == 'W')
                //    c.ToString();
                bool used = false;
                Drive d = new Drive { Letter = c.ToString() };

                for (int i = 0; i < drives.Length; i++) {
                    try {
                        DriveInfo dinfo = drives[i];
                        if (dinfo.Name[0] == c) {                          
                            d.Status = DriveStatus.UNKNOWN;
                            // this triggers IOException when drive is unavailable
                            d.IsGoldDrive = dinfo.DriveFormat == "FUSE-Golddrive";
                            used = true;
                            if (d.IsGoldDrive == true) {
                                d.MountPoint = dinfo.VolumeLabel.Replace("/", "\\");
                                d.Label = GetExplorerDriveLabel(d);
                                if (dinfo.IsReady)
                                    d.Status = DriveStatus.CONNECTED;
                                else
                                    d.Status = DriveStatus.BROKEN;
                                var d1 = settingsDrives.Find(x => x.Letter == d.Letter);
                                if (d1 != null) {
                                    //d.MountPoint = d1.MountPoint;
                                    d.Args = d1.Args;
                                    d.Label = d1.Label;
                                }
                            }
                            Drives.Add(d);
                            break;
                        }
                    } catch (IOException e) {

                    } catch (Exception ex) {

                    }
                }

                if (!used) {
                    var d0 = netUseDrives.Find(x => x.Letter == d.Letter);
                    if (d0 != null) {
                        d.IsGoldDrive = d0.IsGoldDrive;
                        d.Status = d0.Status;
                        if (d.IsGoldDrive==true) {
                            d.Status = DriveStatus.BROKEN;
                            d.MountPoint = d0.MountPoint;
                            d.Label = d0.Label;
                            var d1 = settingsDrives.Find(x1 => x1.Letter == d.Letter);
                            if (d1 != null) {
                                d.Args = d1.Args;
                                d.Label = d1.Label;
                            }
                        } else {
                            d.Status = DriveStatus.UNKNOWN;
                        }  
                    } else {
                        var d1 = settingsDrives.Find(x1 => x1.Letter == d.Letter);
                        if (d1 != null) {
                            d.Status = DriveStatus.DISCONNECTED;
                            d.MountPoint = d1.MountPoint;
                            d.Args = d1.Args;
                            d.Label = d1.Label;
                            d.IsGoldDrive = true;
                        } else {
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

            // get mounted drives using net use command
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

                        if (match.Groups[1].Value == "Unavailable")
                            d.Status = DriveStatus.BROKEN;
                        else
                            d.Status = DriveStatus.IN_USE;

                        if (d.IsGoldDrive == true)
                        {
                            //d.VolumeLabel = GetVolumeName(d.Letter);
                            //if (!String.IsNullOrEmpty(d.VolumeLabel) && d.VolumeLabel.Contains("@"))
                            //{
                            //    d.User = d.VolumeLabel.Split('@')[0];
                            //    d.Host = d.VolumeLabel.Split('@')[1];
                            //}
                            d.MountPoint = match.Groups[3].Value.Replace(@"\\golddrive\", "");
                            d.Label = GetExplorerDriveLabel(d);
                        }
                        drives.Add(d);
                    }
                    catch (Exception ex)
                    {

                    }
                }
            }
            return drives;
        }
     
        public List<Drive> GetFreeDrives()
        {
            string GOLDLETTERS = "GHIJKLMNOPQRSTUVWXYZ";
            List<char> letters = GOLDLETTERS.ToCharArray().ToList();
            List<Drive> freeDrives = new List<Drive>();
            DriveInfo[] drives = DriveInfo.GetDrives();

            for (int i = 0; i < drives.Length; i++)
                letters.Remove(drives[i].Name[0]);
            foreach (char c in letters)
            {
                Drive d = new Drive
                {
                    Letter = c.ToString()
                };
                freeDrives.Add(d);
            }
            freeDrives.Reverse();
            return freeDrives;
        }
        

        public ReturnBox CheckDriveStatus(Drive drive)
        {
            ReturnBox r = new ReturnBox
            {
                MountStatus = MountStatus.OK
            };

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
                                                && (x.Status != DriveStatus.DISCONNECTED
                                                    && x.Status != DriveStatus.FREE)) != null;

                if (pathUsed)
                {
                    r.MountStatus = MountStatus.BAD_DRIVE;
                    r.DriveStatus = DriveStatus.MOUNTPOINT_IN_USE;
                    r.Error = "Mount point in use";
                }
                else if (free)
                {
                    r.DriveStatus = DriveStatus.DISCONNECTED;
                }
                else if (disconnected)
                {
                    r.DriveStatus = DriveStatus.DISCONNECTED;
                }
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
                {
                    r.DriveStatus = DriveStatus.CONNECTED;
                }
            }
            r.Drive = drive;
            return r;
        }
        public bool CheckIfDriveWorks(Drive drive)
        {
            // Fist try DriveInfo
            var info = new DriveInfo(drive.Letter);
            bool ok = false;
            try {
                ok = info.AvailableFreeSpace >= 0;
            } catch (IOException) {
                return false;
            }
            return ok;
            
        }
        public string GetExplorerDriveLabel(Drive drive)
        {
            try
            {
                string key = $@"Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2\{drive.RegistryMountPoint2}";
                RegistryKey k = Registry.CurrentUser.OpenSubKey(key);
                if (k != null)
                    return k.GetValue("_LabelFromReg")?.ToString();
            }
            catch (Exception ex)
            {

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
                RegistryKey k = Registry.CurrentUser.CreateSubKey(key);
                if (k != null)
                    k.SetValue("_LabelFromReg", drive.Label, RegistryValueKind.String);
            }
            catch (Exception ex)
            {

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
            catch { }
        }
        public void SetDriveIcon(Drive drive, string icoPath)
        {
            try
            {
                string key = $@"Software\Classes\Applications\Explorer.exe\Drives\{drive.Letter}\DefaultIcon";
                RegistryKey k = Registry.CurrentUser.CreateSubKey(key);
                if (k != null)
                    k.SetValue("", icoPath, RegistryValueKind.String);
            }
            catch (Exception ex)
            {

            }
        }


        #endregion

        #region SSH Management

        public ReturnBox TestHost(Drive drive)
        {
            ReturnBox r = new ReturnBox();
            try
            {

                using (var client = new TcpClient())
                {
                    var result = client.BeginConnect(drive.Host, 
                        drive.CurrentPort, null, null);
                    var success = result.AsyncWaitHandle.WaitOne(TimeSpan.FromSeconds(5));
                    if (!success)
                    {
                        throw new Exception("Timeout. Server unknown or does not respond.");
                    }
                    else
                    {
                        if (client.Connected)
                        {
                            r.MountStatus = MountStatus.OK;
                            r.Success = true;
                        }
                    }
                    client.EndConnect(result);
                }
            }
            catch (Exception ex)
            {
                r.MountStatus = MountStatus.BAD_HOST;
                r.Error = ex.Message;
            }

            return r;
        }
        ReturnBox TestPassword(Drive drive, string password)
        {
            ReturnBox r = new ReturnBox();

            if (string.IsNullOrEmpty(password))
            {
                r.MountStatus = MountStatus.BAD_PASSWORD;
                r.Error = "Empty password";
                return r;
            }
            try
            {
                SshClient client = new SshClient(drive.Host, drive.CurrentPort, drive.CurrentUser, password);
                client.ConnectionInfo.Timeout = TimeSpan.FromSeconds(TIMEOUT);
                client.Connect();
                client.Disconnect();
                r.MountStatus = MountStatus.OK;
            }
            catch (Exception ex)
            {
                r.Error = string.Format($"Failed to connect to { drive.CurrentUser}@{drive.Host}:{drive.CurrentPort}.\nError: {ex.Message}" );
                if (ex is SshAuthenticationException)
                {
                    r.MountStatus = MountStatus.BAD_PASSWORD;
                }
                else if (ex is SocketException)
                {
                    r.MountStatus = MountStatus.BAD_HOST;
                }
                else
                {

                }

            }
            return r;
        }
        public ReturnBox TestSsh(Drive drive)
        {
            ReturnBox r = new ReturnBox();
            string appkey = string.IsNullOrEmpty(drive.AppKey) ? drive.DefaultAppKey : drive.AppKey;

            if (!File.Exists(appkey))
            {
                r.MountStatus = MountStatus.BAD_KEY;
                r.Error = String.Format($"Password is required to connnect to {drive.CurrentUser}@{drive.Host}:{drive.CurrentPort}.\nSSH keys will be generated and used in future conections.");
                return r;
            }
            try
            {
                r.MountStatus = MountStatus.UNKNOWN;
                var pk = new PrivateKeyFile(appkey);
                var keyFiles = new[] { pk };
                SshClient client = new SshClient(drive.Host, drive.CurrentPort, drive.CurrentUser, keyFiles);
                client.ConnectionInfo.Timeout = TimeSpan.FromSeconds(TIMEOUT);
                client.Connect();
                client.Disconnect();
                r.MountStatus = MountStatus.OK;
                r.Success = true;
            }
            catch (Exception ex)
            {
                if (ex is SocketException ||
                    ex is SshConnectionException ||
                    ex is InvalidOperationException ||
                    ex.Message.Contains("milliseconds"))
                {
                    r.Error = "Host does not respond";
                    r.MountStatus = MountStatus.BAD_HOST;
                }
                else
                {
                    // openssh keys not supported by ssh.net yet
                    string args = $"-i \"{appkey}\" -p {drive.CurrentPort} -oPasswordAuthentication=no -oStrictHostKeyChecking=no -oUserKnownHostsFile=/dev/null -oBatchMode=yes -oConnectTimeout={TIMEOUT} { drive.CurrentUser}@{drive.Host} \"echo ok\"";
                    var r1 = RunLocal("ssh.exe", args, TIMEOUT);
                    var ok = r1.Output.Trim() == "ok";
                    if (ok)
                    {
                        r.MountStatus = MountStatus.OK;
                        r.Error = "";
                        r.Success = true;
                    }
                    else
                    {
                        r.MountStatus = MountStatus.BAD_KEY;
                        r.Error = r1.Error;
                    }
                }
            }
            return r;
        }
        public ReturnBox SetupSsh(Drive drive, string password)
        {
            ReturnBox r = new ReturnBox();
            try
            {
                string pubkey = "";
                if (File.Exists(drive.DefaultAppKey) && File.Exists(drive.DefaultAppPubKey))
                {
                    pubkey = File.ReadAllText(drive.DefaultAppPubKey);
                }
                else
                {
                    pubkey = GenerateKeys(drive);
                }
                
                SshClient client = new SshClient(drive.Host, drive.CurrentPort, drive.CurrentUser, password);
                client.ConnectionInfo.Timeout = TimeSpan.FromSeconds(TIMEOUT);
                client.Connect();
                string cmd = "";
                bool linux = !client.ConnectionInfo.ServerVersion.ToLower().Contains("windows");
                if (linux)
                {
                    cmd = $"exec sh -c \"cd; mkdir -p .ssh; chmod 700 .ssh; touch .ssh/authorized_keys; chmod 744 .ssh/authorized_keys; echo '{pubkey}' >> .ssh/authorized_keys; chmod 644 .ssh/authorized_keys\"";
                }
                else
                {
                    ////cmd = "if not exists .ssh mkdir .ssh && ";
                    //cmd = $"echo {pubkey.Trim()} >> .ssh\\authorized_keys && ";
                    //cmd += $"icacls .ssh\\authorized_keys /inheritance:r && ";
                    //cmd += $"icacls .ssh\\authorized_keys /grant {drive.User}:f &&";
                    //cmd += $"icacls .ssh\\authorized_keys /grant SYSTEM:f";
                }
                SshCommand command = client.CreateCommand(cmd);
                command.CommandTimeout = TimeSpan.FromSeconds(TIMEOUT);
                r.Output = command.Execute();
                r.Error = command.Error;
                r.ExitCode = command.ExitStatus;

            }
            catch (Exception ex)
            {
                r.Error = ex.Message;
                return r;
            }

            r = TestSsh(drive);
            if (r.MountStatus != MountStatus.OK)
                return r;

            return r;
        }
         string GenerateKeys(Drive drive)
         {
            string pubkey = "";
            try
            {
                string dotssh = $@"{drive.UserProfile}\.ssh";
                if (!Directory.Exists(dotssh))
                    Directory.CreateDirectory(dotssh);
                if (!File.Exists(drive.AppKey))
                {
                    ReturnBox r = RunLocal($@"""{AppPath}\ssh-keygen.exe""", $@"-t rsa -b 4096 -m PEM -N """" -f ""{drive.DefaultAppKey}""");
                }
                if (File.Exists(drive.DefaultAppPubKey))
                {
                    pubkey = File.ReadAllText(drive.DefaultAppPubKey).Trim();
                }
                else
                {
                    ReturnBox r = RunLocal($@"""{AppPath}\ssh-keygen.exe""", $@"-y -f ""{drive.DefaultAppKey}""");
                    pubkey = r.Output;
                }

                // TODO: set permissons
            }
            catch (Exception ex)
            {
                Logger.Log("Error generating keys: " + ex.Message);
            }
            return pubkey;

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
            r = TestHost(drive);
            if (r.MountStatus != MountStatus.OK)
                return r;
            status?.Report("Authenticating...");
            r = TestSsh(drive);
            if (r.MountStatus != MountStatus.OK)
                return r;
            status?.Report("Mounting drive...");
            return Mount(drive);
        }
        public ReturnBox ConnectPassword(Drive drive, string password, IProgress<string> status)
        {
            status?.Report("Connecting...");
            ReturnBox r = TestPassword(drive, password);
            if (r.MountStatus != MountStatus.OK)
                return r;
            status?.Report("Generating ssh keys...");
            r = SetupSsh(drive, password);
            if (r.MountStatus != MountStatus.OK)
                return r;
            status?.Report("Mouting drive...");
            return Mount(drive);
        }
        private bool IsWinfspInstalled()
        {
            //string winfsp = Environment.ExpandEnvironmentVariables(@"%ProgramFiles(x86)%\WinFsp\bin\winfsp-x64.dll");
            //return File.Exists(winfsp);

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
                RegistryKey k = Registry.LocalMachine.OpenSubKey(key);
                if (k != null)
                    return k.GetValue("Executable")?.ToString();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"ERROR: cannot get registry path: {ex}");
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

            }
            return "n/a";
        }

        public ReturnBox Mount(Drive drive)
        {
            ReturnBox r = RunLocal("net.exe", $"use { drive.Name } { drive.Remote } /persistent:yes");
            if (!r.Success)
            {
                r.MountStatus = MountStatus.UNKNOWN;
                r.Drive = drive;
                return r;
            }
            SetExplorerDriveLabel(drive);
            SetDriveIcon(drive, $@"{ AppPath }\golddrive.ico");
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
                r.Error = r.Error;
                r.Drive = drive;
                return r;
            }
            // TODO: clenup drive name and registry
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

