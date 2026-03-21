// src/app/Service/SshService.cs
using Renci.SshNet;
using Renci.SshNet.Common;
using System;
using System.IO;
using System.Net.Sockets;

namespace golddrive
{
    public class SshService
    {
        const int TIMEOUT = 20;
        private readonly string appPath;

        public SshService(string appPath)
        {
            this.appPath = appPath;
        }

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

        public ReturnBox TestPassword(Drive drive, string password)
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
                using (SshClient client = new SshClient(drive.Host, drive.CurrentPort, drive.CurrentUser, password))
                {
                    client.ConnectionInfo.Timeout = TimeSpan.FromSeconds(TIMEOUT);
                    client.Connect();
                    client.Disconnect();
                }
                r.MountStatus = MountStatus.OK;
            }
            catch (Exception ex)
            {
                r.Error = $"Failed to connect to {drive.CurrentUser}@{drive.Host}:{drive.CurrentPort}.\nError: {ex.Message}";
                if (ex is SshAuthenticationException)
                    r.MountStatus = MountStatus.BAD_PASSWORD;
                else if (ex is SocketException)
                    r.MountStatus = MountStatus.BAD_HOST;
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
                r.Error = $"Password is required to connect to {drive.CurrentUser}@{drive.Host}:{drive.CurrentPort}.\nSSH keys will be generated and used in future connections.";
                return r;
            }
            try
            {
                r.MountStatus = MountStatus.UNKNOWN;
                var pk = new PrivateKeyFile(appkey);
                var keyFiles = new[] { pk };
                using (SshClient client = new SshClient(drive.Host, drive.CurrentPort, drive.CurrentUser, keyFiles))
                {
                    client.ConnectionInfo.Timeout = TimeSpan.FromSeconds(TIMEOUT);
                    client.Connect();
                    client.Disconnect();
                }
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
                    string args = $"-i \"{appkey}\" -p {drive.CurrentPort} -oPasswordAuthentication=no -oStrictHostKeyChecking=no -oUserKnownHostsFile=/dev/null -oBatchMode=yes -oConnectTimeout={TIMEOUT} {drive.CurrentUser}@{drive.Host} \"echo ok\"";
                    var r1 = RunLocalProcess("ssh.exe", args, TIMEOUT);
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

                using (SshClient client = new SshClient(drive.Host, drive.CurrentPort, drive.CurrentUser, password))
                {
                    client.ConnectionInfo.Timeout = TimeSpan.FromSeconds(TIMEOUT);
                    client.Connect();
                    string cmd = "";
                    bool linux = !client.ConnectionInfo.ServerVersion.ToLower().Contains("windows");
                    if (linux)
                    {
                        cmd = $"exec sh -c \"cd; mkdir -p .ssh; chmod 700 .ssh; touch .ssh/authorized_keys; chmod 744 .ssh/authorized_keys; echo '{pubkey}' >> .ssh/authorized_keys; chmod 644 .ssh/authorized_keys\"";
                    }
                    SshCommand command = client.CreateCommand(cmd);
                    command.CommandTimeout = TimeSpan.FromSeconds(TIMEOUT);
                    r.Output = command.Execute();
                    r.Error = command.Error;
                    r.ExitCode = command.ExitStatus;
                }
            }
            catch (Exception ex)
            {
                r.Error = ex.Message;
                return r;
            }

            r = TestSsh(drive);
            return r;
        }

        public string GenerateKeys(Drive drive)
        {
            string pubkey = "";
            try
            {
                string dotssh = Path.Combine(drive.UserProfile, ".ssh");
                if (!Directory.Exists(dotssh))
                    Directory.CreateDirectory(dotssh);
                if (!File.Exists(drive.AppKey))
                {
                    RunLocalProcess($@"""{appPath}\ssh-keygen.exe""", $@"-t rsa -b 4096 -m PEM -N """" -f ""{drive.DefaultAppKey}""");
                }
                if (File.Exists(drive.DefaultAppPubKey))
                {
                    pubkey = File.ReadAllText(drive.DefaultAppPubKey).Trim();
                }
                else
                {
                    ReturnBox r = RunLocalProcess($@"""{appPath}\ssh-keygen.exe""", $@"-y -f ""{drive.DefaultAppKey}""");
                    pubkey = r.Output;
                }
            }
            catch (Exception ex)
            {
                Logger.Log("Error generating keys: " + ex.Message);
            }
            return pubkey;
        }

        private ReturnBox RunLocalProcess(string cmd, string args, int timeout_secs = 30)
        {
            ReturnBox r = new ReturnBox();
            using (var process = new System.Diagnostics.Process())
            {
                process.StartInfo = new System.Diagnostics.ProcessStartInfo
                {
                    WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden,
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
    }
}
