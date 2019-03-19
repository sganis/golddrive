using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Renci.SshNet;
using System.IO;
using System.Text.RegularExpressions;

namespace golddrive_ui
{

    public class Controller
    {
        public SshClient Ssh { get; set; }
        public SftpClient Sftp { get; set; }
        public bool Connected { get; set; }
        public string Error { get; set; }
        public MainWindow MainWindow;

        public Controller(MainWindow main)
        {
            MainWindow = main;
        }

        public bool Connect(string host, int port, string user, string password, string pkey)
        {
            Connected = false;
            try
            {
                if (!String.IsNullOrEmpty(pkey))
                {
                    var pk = new PrivateKeyFile(pkey);
                    var keyFiles = new[] { pk };
                    Ssh = new SshClient(host, port, user, keyFiles);
                    Ssh.Connect();
                    Sftp = new SftpClient(host, port, user, keyFiles);
                    Sftp.Connect();
                }
                else
                {
                    Ssh = new SshClient(host, port, user, password);
                    Ssh.Connect();
                    Sftp = new SftpClient(host, port, user, password);
                    Sftp.Connect();
                }
                Connected = true;
            }
            catch (Renci.SshNet.Common.SshAuthenticationException ex)
            {
                // wrong password
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
            return RunLocal(@"C:\Windows\System32\cmd.exe", "/C " + cmd);
        }
        public ReturnBox RunLocal(string cmd, string args)
        {
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
        public ReturnBox RunRemote(string cmd)
        {
            ReturnBox r = new ReturnBox();
            if (Connected)
            {
                try
                {
                    SshCommand c = Ssh.RunCommand(cmd);
                    r.Output = c.Result;
                    r.Error = c.Error;
                    r.ExitCode = c.ExitStatus;
                    r.Success = r.ExitCode == 0 && String.IsNullOrEmpty(r.Error);
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

        public async Task<ReturnBox> GetGoldDrives()
        {
            //return await Task.Run(() => mController.RunLocal(@"C:\Windows\System32\net.exe use"));
            var r = await Task.Run(() => RunLocal("net.exe", "use"));
            string drives = "";
            foreach (var line in r.Output.Split('\n'))
            {
                Match match = Regex.Match(line, @"^([A-Za-z]+)?\s+([A-Z]:)\s+(\\\\[^ ]+)");
                if (match.Success)
                {
                    string status = match.Groups[1].Value;
                    string drive = match.Groups[2].Value;
                    string remote = match.Groups[3].Value;
                    drives += string.Format($"{drive} {remote}\n");
                }
            }
            r.Output = drives;
            return r;
        }
    }
}
