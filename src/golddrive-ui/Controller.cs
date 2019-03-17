using Renci.SshNet;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace golddrive_ui
{
    
    class Controller
    {
        public SshClient Ssh { get; set; }
        public bool Connected { get; set; }
        public string Error { get; set; }

        public Controller()
        {

        }

        public bool Connect(string host, string user, string password)
        {
            Connected = false;
            try
            {
                Ssh = new SshClient(host, user, password);
                Ssh.Connect();
                Connected = true;
            }
            catch (Renci.SshNet.Common.SshAuthenticationException ex)
            {
                Error = ex.Message;
            }
            return Connected;
        }


        public ReturnBox RunLocal(string cmd)
        {
            ReturnBox r = new ReturnBox();
            Process process = new Process();
            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.WindowStyle = ProcessWindowStyle.Hidden;
            startInfo.CreateNoWindow = true;
            startInfo.RedirectStandardError = true;
            startInfo.RedirectStandardOutput = true;
            startInfo.UseShellExecute = false;
            startInfo.FileName = "cmd.exe";
            startInfo.Arguments = "/C " + cmd;
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
            SshCommand c = Ssh.RunCommand(cmd);
            r.Output = c.Result;
            r.Error = c.Error;
            r.ExitCode = c.ExitStatus;
            r.Success = r.ExitCode == 0 && String.IsNullOrEmpty(r.Error);
            return r;
        }

    }
}
