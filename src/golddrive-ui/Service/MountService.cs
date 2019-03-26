using System;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using Renci.SshNet;
using System.Linq;
using Microsoft.Win32;
using System.Reflection;
using System.Runtime.Serialization.Formatters.Binary;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Net.Sockets;
using Renci.SshNet.Common;

namespace golddrive
{

    public class MountService //, IDriver
    {
        #region Properties

        public SshClient Ssh { get; set; }
        public SftpClient Sftp { get; set; }
        public string Error { get; set; }
        public bool Connected { get { return Ssh != null && Ssh.IsConnected; } }

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


        #endregion

        public MountService()
        {

        }

        #region Serialization

        public void SaveSettingsDrives(List<Drive> drives)
        {
            try
            {
                using (MemoryStream ms = new MemoryStream())
                {
                    BinaryFormatter bf = new BinaryFormatter();
                    bf.Serialize(ms, drives);
                    ms.Position = 0;
                    byte[] buffer = new byte[(int)ms.Length];
                    ms.Read(buffer, 0, buffer.Length);
                    Properties.Settings.Default.Drives = Convert.ToBase64String(buffer);
                    Properties.Settings.Default.Save();
                }
            }
            catch { }
        }

        private List<Drive> LoadSettingsDrives()
        {
            List<Drive> drives = new List<Drive>();
            try
            {
                using (MemoryStream ms = new MemoryStream(
                    Convert.FromBase64String(Properties.Settings.Default.Drives)))
                {
                    BinaryFormatter bf = new BinaryFormatter();
                    if (ms.Length > 0)
                        drives = (List<Drive>)bf.Deserialize(ms);
                }
            }
            catch (Exception ex)
            {
            }
            return drives;
        }

        #endregion

        #region Core Methods

        public bool Connect(string host, int port, string user, string pkey)
        {
            try
            {
                var pk = new PrivateKeyFile(pkey);
                var keyFiles = new[] { pk };
                Ssh = new SshClient(host, port, user, keyFiles);
                Ssh.ConnectionInfo.Timeout = TimeSpan.FromSeconds(5);
                Ssh.Connect();
                Sftp = new SftpClient(host, port, user, keyFiles);
                Sftp.ConnectionInfo.Timeout = TimeSpan.FromSeconds(5);
                Sftp.Connect();
            }
            catch (Renci.SshNet.Common.SshAuthenticationException ex)
            {
                // bad key
                Error = ex.Message;
            }
            catch (Exception ex)
            {
                Error = ex.Message;
            }
            return Connected;
        }

        public ReturnBox RunLocal(string cmd)
        {
            // 2 secs slower
            return RunLocal("cmd.exe", "/C " + cmd);
        }

        public ReturnBox RunLocal(string cmd, string args)
        {
            Logger.Log($"Running local command: {cmd} {args}");
            ReturnBox r = new ReturnBox();
            Process process = new Process();
            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.WindowStyle = ProcessWindowStyle.Hidden;
            startInfo.CreateNoWindow = true;
            startInfo.RedirectStandardError = true;
            startInfo.RedirectStandardOutput = true;
            startInfo.UseShellExecute = false;
            startInfo.FileName = cmd;
            startInfo.Arguments = args;
            process.StartInfo = startInfo;
            process.Start();
            process.WaitForExit();
            r.Output = process.StandardOutput.ReadToEnd();
            r.Error = process.StandardError.ReadToEnd();
            r.ExitCode = process.ExitCode;
            r.Success = r.ExitCode == 0 && String.IsNullOrEmpty(r.Error);
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

        public ReturnBox DownloadFile(string src, string dst)
        {
            ReturnBox r = new ReturnBox();
            if (Connected)
            {
                try
                {
                    using (Stream fs = File.Create(dst))
                    {
                        Sftp.DownloadFile(src, fs);
                    }
                    r.Success = true;
                }
                catch (Exception ex)
                {
                    r.Error = ex.Message;
                }
            }
            return r;
        }

        public ReturnBox UploadFile(string src, string dir, string filename)
        {
            ReturnBox r = new ReturnBox();
            if (Connected)
            {
                try
                {
                    using (var fs = new FileStream(src, FileMode.Open))
                    {
                        Sftp.BufferSize = 4 * 1024; // bypass Payload error large files
                        Sftp.ChangeDirectory(dir);
                        Sftp.UploadFile(fs, filename, true);
                    }
                    r.Success = true;
                }
                catch (Exception ex)
                {
                    r.Error = ex.Message;
                }
            }
            return r;
        }

        #endregion

        #region Local Drive Management




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
                        Drive d = new Drive();
                        //d.NetUseStatus = match.Groups[1].Value;
                        d.Letter = match.Groups[2].Value[0].ToString();
                        d.IsGoldDrive = match.Groups[3].Value.Contains(@"\\golddrive\");
                        if (d.IsGoldDrive == true)
                        {
                            d.VolumeLabel = GetVolumeName(d.Letter);
                            if (!String.IsNullOrEmpty(d.VolumeLabel) && d.VolumeLabel.Contains("@"))
                            {
                                d.User = d.VolumeLabel.Split('@')[0];
                                d.Host = d.VolumeLabel.Split('@')[1];
                            }
                            d.MountPoint = match.Groups[3].Value.Replace(@"\\golddrive\", "");
                            d.Label = GetDriveLabel(d);
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

        public List<Drive> GetGoldDrives()
        {
            List<Drive> drives = GetUsedDrives().Where(x => x.IsGoldDrive == true).ToList();
            List<Drive> savedDrives = LoadSettingsDrives();
            foreach (Drive d in savedDrives)
                if (drives.Find(x => x.Letter == d.Letter) == null)
                    drives.Add(d);
            return drives;
        }

        public string GetVolumeName(string letter)
        {
            var r = RunLocal($"vol {letter}:");
            foreach (var line in r.Output.Split('\n'))
            {
                try
                {
                    Match match = Regex.Match(line, $@"^\s*Volume in drive {letter} is ([^ ]+)");
                    if (match.Success)
                        return match.Groups[1].Value.Trim();
                }
                catch (Exception ex)
                {

                }
            }
            return "";
        }

        public ReturnBox CheckDriveStatus(Drive drive)
        {
            ReturnBox r = new ReturnBox();
            r.MountStatus = MountStatus.OK;

            if (drive == null ||
                (drive.Letter.ToCharArray()[0] < 'G' && drive.Letter.ToCharArray()[0] > 'Z'))
            {
                r.DriveStatus = DriveStatus.NOT_SUPPORTED;
            }
            else
            {
                var freeDrives = GetFreeDrives();
                var drives = GetUsedDrives();
                var inUse = freeDrives.Find(x => x.Letter == drive.Letter) == null;
                var isGold = drives.Find(x => x.Letter == drive.Letter && x.IsGoldDrive == true) != null;
                var pathUsed = drives.Find(x => x.Letter != drive.Letter && x.MountPoint == drive.MountPoint) != null;
                if (!inUse)
                    r.DriveStatus = DriveStatus.DISCONNECTED;
                else if (pathUsed)
                    r.DriveStatus = DriveStatus.PATH_IN_USE;
                else if (!isGold)
                    r.DriveStatus = DriveStatus.IN_USE;
                else if (!CheckIfDriveWorks(drive))
                    r.DriveStatus = DriveStatus.BROKEN;
                else
                    r.DriveStatus = DriveStatus.CONNECTED;
            }
            return r;
        }
        public bool CheckIfDriveWorks(Drive drive)
        {
            int epoch = (int)(DateTime.Now - new DateTime(1970, 1, 1, 0, 0, 0, 0)).TotalSeconds;
            string tempfile = $@"{ drive.Name }\tmp\{drive.User}@{drive.Host}.{epoch}";
            var r = RunLocal("type nul > " + tempfile);
            if (r.ExitCode == 0)
            {
                RunLocal("del " + tempfile);
                return true;
            }
            return false;
        }
        public string GetDriveLabel(Drive drive)
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
        public void SetDriveLabel(Drive drive)
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
                Drive d = new Drive();
                d.Letter = c.ToString();
                freeDrives.Add(d);
            }
            freeDrives.Reverse();
            return freeDrives;
        }
        #endregion

        #region SSH Management

        ReturnBox TestHost(Drive drive)
        {
            ReturnBox r = new ReturnBox();
            try
            {
                using (var client = new TcpClient())
                {
                    var result = client.BeginConnect(drive.Host, drive.Port, null, null);
                    var success = result.AsyncWaitHandle.WaitOne(5000);
                    if (!success)
                    {
                        r.MountStatus = MountStatus.BAD_HOST;
                    }
                    client.EndConnect(result);
                }
            }
            catch (Exception ex)
            {
                r.MountStatus = MountStatus.BAD_HOST;
                r.Error = ex.Message;
            }
            r.MountStatus = MountStatus.OK;
            return r;
        }
        ReturnBox TestPassword(Drive drive, string password)
        {
            ReturnBox r = new ReturnBox();

            if (string.IsNullOrEmpty(password))
            {
                r.MountStatus = MountStatus.BAD_LOGIN;
                r.Error = "Empty password";
                return r;
            }
            try
            {
                SshClient client = new SshClient(drive.Host, drive.Port, drive.User, password);
                client.ConnectionInfo.Timeout = TimeSpan.FromSeconds(5);
                client.Connect();
                client.Disconnect();
                r.MountStatus = MountStatus.OK;
            }
            catch (Exception ex)
            {
                r.Error = ex.Message;
                if (ex is SshAuthenticationException)
                {
                    r.MountStatus = MountStatus.BAD_LOGIN;
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
        ReturnBox TestSsh(Drive drive)
        {
            ReturnBox r = new ReturnBox();

            //r = TestHost(drive);
            //if (r.MountStatus == MountStatus.BAD_HOST)
            //    return r;

            if (!File.Exists(drive.AppKey))
            {
                r.MountStatus = MountStatus.BAD_LOGIN;
                r.Error = "No ssh key";
                return r;
            }
            try
            {
                r.MountStatus = MountStatus.UNKNOWN;
                var pk = new PrivateKeyFile(drive.AppKey);
                var keyFiles = new[] { pk };
                SshClient client = new SshClient(drive.Host, drive.Port, drive.User, keyFiles);
                client.ConnectionInfo.Timeout = TimeSpan.FromSeconds(5);
                client.Connect();
                client.Disconnect();
                r.MountStatus = MountStatus.OK;
            }
            catch (Exception ex)
            {
                r.Error = ex.Message;
                if (ex is SshAuthenticationException)
                {
                    r.MountStatus = MountStatus.BAD_LOGIN;
                }
                else if (ex is SocketException)
                {
                    r.MountStatus = MountStatus.BAD_HOST;
                }
                else
                {
                    Console.Write(ex.ToString());
                    r.MountStatus = MountStatus.UNKNOWN;
                }
            }            
            return r;
        }
        ReturnBox SetupSsh(Drive drive, string password)
        {
            ReturnBox r = new ReturnBox();
            try
            {
                string pubkey = "";
                if (File.Exists(drive.AppKey) && File.Exists(drive.AppPubKey))
                {
                    pubkey = File.ReadAllText(drive.AppPubKey);
                }
                else
                {
                    pubkey = GenerateKeys(drive);
                }
                SshClient client = new SshClient(drive.Host, drive.Port, drive.User, password);
                client.ConnectionInfo.Timeout = TimeSpan.FromSeconds(5);
                client.Connect();
                string cmd = $"exec sh -c \"cd; umask 077; mkdir -p .ssh; echo '{pubkey}' >> .ssh/authorized_keys\"";
                SshCommand command = client.CreateCommand(cmd);
                command.CommandTimeout = TimeSpan.FromSeconds(5);
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
                ReturnBox r = RunLocal($@"D:\Users\ganissa\AppData\Local\Programs\Git\usr\bin\ssh-keygen.exe -m PEM -t rsa -N """" -f {drive.AppKey}");
                if (File.Exists(drive.AppPubKey))
                    pubkey = File.ReadAllText(drive.AppPubKey).Trim();
            }
            catch (Exception ex)
            {
                Logger.Log("Error generating keys: " + ex.Message);
            }
            return pubkey;

        }

        #endregion

        #region Mount Management

        public ReturnBox Connect(Drive drive)
        {
            ReturnBox r = new ReturnBox();
            if (!IsWinfspInstalled())
            {
                r.MountStatus = MountStatus.BAD_WINFSP;
                return r;
            }
            r = CheckDriveStatus(drive);
            if (r.DriveStatus != DriveStatus.DISCONNECTED)
            {
                r.MountStatus = MountStatus.BAD_DRIVE;
                r.Error = r.DriveStatus.ToString();
                return r;
            }
            r = TestSsh(drive);
            if (r.MountStatus != MountStatus.OK)
                return r;

            return Mount(drive);
        }
        public ReturnBox ConnectPassword(Drive drive, string password)
        {
            ReturnBox r = TestPassword(drive, password);
            if (r.MountStatus != MountStatus.OK)
                return r;

            r = SetupSsh(drive, password);
            if (r.MountStatus != MountStatus.OK)
                return r;

            return Mount(drive);
        }
        private bool IsWinfspInstalled()
        {
            return true;
        }

        private ReturnBox Mount(Drive drive)
        {
            ReturnBox r = RunLocal("net.exe", $"use { drive.Name } { drive.Remote }");
            if (!r.Success)
            {
                r.Error = r.Error;
                r.MountStatus = MountStatus.UNKNOWN;
                return r;
            }
            SetDriveLabel(drive);
            SetDriveIcon(drive, $@"{ AppPath }\golddrive.ico");
            r.MountStatus = MountStatus.OK;
            r.DriveStatus = DriveStatus.CONNECTED;
            return r;
        }

        public ReturnBox Unmount(Drive drive)
        {
            ReturnBox r = RunLocal("net.exe", "use /d " + drive.Name);
            if (!r.Success)
            {
                r.Error = r.Error;
                return r;
            }
            // TODO: clenup drive name and registry
            r.MountStatus = MountStatus.OK;
            r.DriveStatus = DriveStatus.DISCONNECTED;
            return r;

        }

        public ReturnBox UnmountAll()
        {
            ReturnBox rb = new ReturnBox();

            return rb;

        }




        #endregion

        public string GetUid(string user)
        {
            return RunRemote($"id -u {user}").Output;
        }


    }
}

