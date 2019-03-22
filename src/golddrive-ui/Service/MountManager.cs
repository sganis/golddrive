using System;
using System.Diagnostics;
using System.Threading.Tasks;
using System.IO;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using Renci.SshNet;

namespace golddrive
{

    public class MountManager : IMountManager
    {
        public SshClient Ssh { get; set; }
        public SftpClient Sftp { get; set; }
        public string Error { get; set; }

        public bool Connected { get { return Ssh != null && Ssh.IsConnected; } }

        public MountManager()
        {

        }

        public bool Connect(string host, int port, string user, string password, string pkey)
        {
            try
            {
                if (!String.IsNullOrEmpty(pkey))
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
                else
                {
                    Ssh = new SshClient(host, port, user, password);
                    Ssh.Connect();
                    Sftp = new SftpClient(host, port, user, password);
                    Sftp.Connect();
                }
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

        public List<Drive> GetGoldDrives()
        {
            List<Drive> drives = new List<Drive>();
            // get mounted drives
            var r = RunLocal("net.exe", "use");
            foreach (var line in r.Output.Split('\n'))
            {
                Match match = Regex.Match(line,
                    @"^([A-Za-z]+)?\s+([A-Z]:)\s+(\\\\golddrive\\[^ ]+)");
                if (match.Success)
                {
                    try
                    {
                        Drive d = new Drive();
                        d.NetUseStatus = match.Groups[1].Value;
                        d.Letter = match.Groups[2].Value[0].ToString();
                        d.Remote = match.Groups[3].Value;
                        d.Label = GetDriveLabel(d.Letter);
                        if(!String.IsNullOrEmpty(d.Label) && d.Label.Contains("@"))
                        {
                            d.User = d.Label.Split('@')[0];
                            d.Host = d.Label.Split('@')[1];
                        }
                        drives.Add(d);
                    }
                    catch(Exception ex)
                    {

                    }
                }
            }
            return drives;
        }
        private string GetDriveLabel(string letter)
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
                catch(Exception ex)
                {

                }
            }
            return "";
        }
        public async Task<ReturnBox> ls()
        {
            return await Task.Run(() => RunRemote("ls -l"));
        }


        #region Setup SSH
        
        ReturnBox TestHost(string user, string host, int port=22)
        {
            ReturnBox rb = new ReturnBox();

            //	try:
            //		client.connect(hostname=host, username=user, 
            //				password='', port=port, timeout=5)	
            //		rb.returncode = util.ReturnCode.OK
            //    except(paramiko.ssh_exception.AuthenticationException,
            //        paramiko.ssh_exception.BadAuthenticationType,
            //        paramiko.ssh_exception.PasswordRequiredException):

            //        rb.returncode = util.ReturnCode.OK
            //    except Exception as ex:
            //		rb.returncode = util.ReturnCode.BAD_HOST
            //        rb.error = str(ex)
            //	finally:
            //		client.close()
            return rb;
        }

        void TestLogin(string userhost, string password, int port=22)
        {
            //       def testlogin(userhost, password, port= 22):
            //'''
            //Test ssh password authentication
            //'''

            //   logger.info(f'Logging in with password for {userhost}...')
            //rb = util.ReturnBox()
            //if not password:

            //       rb.returncode =util.ReturnCode.BAD_LOGIN
            //       rb.error = 'Empty password'
            //	return rb

            //   user, host = userhost.split('@')

            //   client = paramiko.SSHClient()

            //   client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

            //try:
            //	client.connect(hostname=host, username=user, 
            //			password=password, port=port, timeout=10, 
            //			look_for_keys=False)
            //	rb.returncode = util.ReturnCode.OK
            //   except(paramiko.ssh_exception.AuthenticationException,
            //       paramiko.ssh_exception.BadAuthenticationType,
            //       paramiko.ssh_exception.PasswordRequiredException) as ex:
            //	rb.returncode = util.ReturnCode.BAD_LOGIN
            //       rb.error = str(ex)

            //   except Exception as ex:
            //	rb.returncode = util.ReturnCode.BAD_HOST
            //       rb.error = str(ex)
            //finally:
            //	client.close()
            //return rb
        }

        void TestSsh(string userhost, string pkey, int port=22)
        {
            //       def testssh(userhost, seckey, port= 22):
            //'''
            //Test ssh key authentication
            //'''

            //   logger.info(f'Testing ssh keys for {userhost} using key {seckey}...')

            //rb = testhost(userhost, port)
            //if rb.returncode == util.ReturnCode.BAD_HOST:
            //	return rb

            //if not os.path.exists(seckey):

            //       seckey_win = seckey.replace('/','\\')
            //	logger.error(f'Key does not exist: {seckey_win}')
            //	rb.returncode = util.ReturnCode.BAD_LOGIN
            //       rb.error = "No key"
            //	return rb

            //   cmd = f'''ssh.exe
            //	-i "{seckey}"
            //	-p { port} 
            //	-o PasswordAuthentication = no
            //       - o StrictHostKeyChecking=no 
            //	-o UserKnownHostsFile =/ dev / null
            //       - o BatchMode=yes 
            //	{userhost
            //   } "echo ok"'''
            //r = util.run(cmd, capture=True, timeout=10)

            //if r.stdout == 'ok':
            //	# success
            //	rb.returncode = util.ReturnCode.OK
            //   elif 'Permission denied' in r.stderr:
            //	# wrong user or key issue
            //	rb.returncode = util.ReturnCode.BAD_LOGIN
            //       rb.error = 'Access denied'
            //else:
            //	# wrong port: connection refused 
            //	# unknown host: connection timeout
            //	logger.error(r.stderr)

            //       rb.returncode = util.ReturnCode.BAD_HOST
            //       rb.error = r.stderr
            //return rb
        }

        void GenerateKeys(string userhost, string pkey)
        {
             //       def generate_keys(seckey, userhost):

             //   logger.info('Generating new ssh keys...')
	            //rb = util.ReturnBox()
	            //sk = paramiko.RSAKey.generate(2048)
	            //try:
	            //	sshdir = os.path.dirname(seckey)
	            //	if not os.path.exists(sshdir):

             //           os.makedirs(sshdir)
	            //		os.chmod(sshdir, 0o700)
	            //	sk.write_private_key_file(seckey)	
	            //except Exception as ex:
	            //	logger.error(f'{ex}, {seckey}')
	            //	rb.error = str(ex)
	            //	return rb

             //   pubkey = f'ssh-rsa {sk.get_base64()} {userhost}'
	
	            //# try:
	            //# 	with open(seckey + '.pub', 'wt') as w:
	            //# 		w.write(pubkey)
	            //# except Exception as ex:
	            //# 	logger.error(f'Could not save public key: {ex}')

	            //rb.output = pubkey
	            //return rb
        }

        bool HasAppKeys(string user)
        {
            //def has_app_keys(user):

            //    appkey = util.get_app_key(user)
            //	return os.path.exists(appkey)
            return false;
        }

        void SetKeyPermissions(string user, string pkey)
        {
            //       def set_key_permissions(user, pkey):


            //   logger.info('setting ssh key permissions...')
            //ssh_folder = os.path.dirname(pkey)
            //# Remove Inheritance ::
            //# subprocess.run(fr'icacls {ssh_folder} /c /t /inheritance:d')
            //util.run(fr'icacls {pkey} /c /t /inheritance:d', capture=True)

            //# Set Ownership to Owner and SYSTEM account
            //# subprocess.run(fr'icacls {ssh_folder} /c /t /grant %username%:F')
            //util.run(fr'icacls {pkey} /c /t /grant {user}:F', capture=True)
            //util.run(fr'icacls {pkey} /c /t /grant SYSTEM:F', capture=True)

            //# Remove All Users, except for Owner 
            //# subprocess.run(fr'icacls {ssh_folder} /c /t /remove Administrator BUILTIN\Administrators BUILTIN Everyone System Users')
            //util.run(fr'icacls {pkey} /c /t /remove Administrator BUILTIN\Administrators BUILTIN Everyone Users', capture=True)

            //# Verify 
            //# util.run(fr'icacls {pkey}')
        }

        void SetupSsh(string userhost, string password, int port=22)
        {
            //        def main(userhost, password, port= 22):
            //	'''
            //	Setup ssh keys, return ReturnBox
            //	'''
            //	logger.info(f'Setting up ssh keys for {userhost}...')
            //	rb = util.ReturnBox()

            //	# app key
            //	user, host = userhost.split('@')
            //	seckey = util.get_app_key(user)	

            //	# Check if keys need to be generated
            //	pubkey = ''
            //	if has_app_keys(user):

            //        logger.info('Private key already exists.')
            //		sk = paramiko.RSAKey.from_private_key_file(seckey)
            //		pubkey = f'ssh-rsa {sk.get_base64()} {userhost}'
            //	else:
            //		rbkey = generate_keys(seckey, userhost)
            //		if rbkey.error:
            //			rbkey.returncode = util.ReturnCode.BAD_SSH
            //			return rbkey
            //		else:
            //			pubkey = rbkey.output

            //# connect
            //    client = paramiko.SSHClient()

            //    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

            //	rb.error = ''
            //	try:
            //		logger.info('Connecting using password...')
            //		client.connect(hostname=host, username=user,
            //						password=password, port=port, timeout=10,
            //						look_for_keys=False)     
            //	except paramiko.ssh_exception.AuthenticationException:
            //		rb.error = f'User or password wrong'

            //        rb.returncode = 1

            //    except Exception as ex:

            //        rb.error = f'connection error: {ex}'

            //        rb.returncode = 2

            //	if rb.error:

            //        logger.error(rb.error)
            //		if 'getaddrinfo failed' in rb.error:
            //			rb.error = f'{host} not found'
            //		client.close()
            //		rb.returncode = util.ReturnCode.BAD_SSH
            //		return rb

            //    set_key_permissions(user, seckey)


            //    logger.info(f'Publising public key...')

            //	# Copy to the target machines.
            //	# cmd = f"exec bash -c \"cd; umask 077; mkdir -p .ssh && echo '{pubkey}' >> .ssh/authorized_keys || exit 1\" || exit 1"
            //	cmd = f"exec sh -c \"cd; umask 077; mkdir -p .ssh; echo '{pubkey}' >> .ssh/authorized_keys\""
            //	logger.info(cmd)
            //	ok = False

            //	try:
            //		stdin, stdout, stderr = client.exec_command(cmd, timeout=10)
            //		rc = stdout.channel.recv_exit_status()   
            //		if rc == 0:
            //			logger.info('Key transfer successful')
            //			rb.returncode = util.ReturnCode.OK
            //		else:
            //			logger.error(f'Error transfering public key: exit {rc}, error: {stderr}')
            //	except Exception as ex:
            //		logger.error(ex)
            //		rb.returncode = util.ReturnCode.BAD_SSH
            //        rb.error = f'error transfering public key: {ex}'
            //		return rb
            //	finally:

            //        client.close()


            //    err = stderr.read()
            //	if err:
            //		logger.error(err)
            //		rb.returncode = util.ReturnCode.BAD_SSH
            //        rb.error = f'error transfering public key, error: {err}'
            //		return rb

            //    rb = testssh(userhost, seckey, port)
            //	if rb.returncode == util.ReturnCode.OK:

            //        rb.output = "SSH setup successfull."

            //        logger.info(rb.output)
            //	else:
            //		message = 'SSH setup test failed'
            //		detail = ''
            //		if rb.returncode == util.ReturnCode.BAD_LOGIN:
            //			detail = ': authentication probem'
            //		else:
            //			message = ': connection problem'
            //		rb.error = message
            //        rb.returncode = util.ReturnCode.BAD_SSH
            //        logger.error(message + detail)
            //	return rb
        }





        //if __name__ == '__main__':

        //	import sys

        //    import os

        //    import getpass

        //    assert(len(sys.argv) > 1 and
        //			'@' in sys.argv[1]) # usage: prog user@host
        //	os.environ['PATH'] = f'{DIR}\\..\\client\\sshfs\\bin;' + os.environ['PATH']
        //    userhost = sys.argv[1]

        //    password = getpass.getpass('Linux password: ')
        //	port=22
        //	if ':' in userhost:
        //		userhost, port = userhost.split(':')                             
        //	logging.basicConfig(level=logging.INFO)


        //    main(userhost, password, port)




        #endregion

        #region Mount manager

        DriveStatus CheckDrive(string drive, string user, string host)
        {
            DriveStatus status = DriveStatus.UNKNOWN;



            return status;
        }

        ReturnBox Mount()
        {
            ReturnBox rb = new ReturnBox();

            return rb;
        }

        ReturnBox Unmount()
        {
            ReturnBox rb = new ReturnBox();

            return rb;
        }

        ReturnBox UnmountAll()
        {
            ReturnBox rb = new ReturnBox();

            return rb;

        }

        bool IsDriveOk()
        {
            return false;
        }

        List<Drive> GetFreeDrives()
        {
            List<Drive> free_drives = new List<Drive>();

            return free_drives;
        }



        #endregion




    }
}
