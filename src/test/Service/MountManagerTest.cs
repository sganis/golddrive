
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Threading.Tasks;

namespace golddrive.Tests
{
    [TestClass()]
    public class MountManagerTest
    {
        private MountService _mountService = new MountService();
        private static readonly string _host = Environment.GetEnvironmentVariable("GOLDDRIVE_HOST");
        private static readonly string _user = Environment.GetEnvironmentVariable("GOLDDRIVE_USER");
        private static readonly string _pass = Environment.GetEnvironmentVariable("GOLDDRIVE_PASS");
        private static bool _sshAvailable;
        private Drive _drive = new Drive
        {
            Letter = "X",
            MountPoint = $"{_user}@{_host}",
            User = _user,
            Label = "Golddrive",
            IsGoldDrive = true,
            Status = DriveStatus.DISCONNECTED,
        };

        [ClassInitialize]
        public static void ClassInit(TestContext context)
        {
            if (string.IsNullOrEmpty(_host))
            {
                _sshAvailable = false;
                return;
            }
            try
            {
                using (var client = new System.Net.Sockets.TcpClient())
                {
                    var result = client.BeginConnect(_host, 22, null, null);
                    _sshAvailable = result.AsyncWaitHandle.WaitOne(TimeSpan.FromSeconds(3));
                    if (_sshAvailable)
                        client.EndConnect(result);
                }
            }
            catch
            {
                _sshAvailable = false;
            }
        }

        private void RequireSsh()
        {
            if (!_sshAvailable)
                Assert.Inconclusive("SSH server not available");
        }

        [TestInitialize]
        public void Init()
        {
            _mountService.RunLocal("subst /d W:");
            Assert.IsFalse(string.IsNullOrEmpty(_host), "set GOLDDRIVE_HOST env var");
        }

        [TestCleanup]
        public void Teardown()
        {
            _mountService.RunLocal($"net use /d {_drive.Name} /y");
        }

        public void Mount()
        {
            RequireSsh();
            _drive.AppKey = _drive.DefaultAppKey;
            var r = _mountService.Connect(_drive, null);
            if (r.DriveStatus != DriveStatus.CONNECTED)
                Console.WriteLine($"ERROR: {r.Error}");
            Assert.AreEqual(DriveStatus.CONNECTED, r.DriveStatus);
        }

        public void Unmount()
        {
            var r = _mountService.Unmount(_drive);
            Assert.AreEqual(DriveStatus.DISCONNECTED, r.DriveStatus);
        }

        public void RandomFile(string path, int megabytes)
        {
            FileStream fs = new FileStream(path, FileMode.CreateNew);
            fs.Seek(1024L * 1024 * megabytes, SeekOrigin.Begin);
            fs.WriteByte(0);
            fs.Close();
        }

        public string Md5(string path)
        {
            using (var md5 = MD5.Create())
            {
                using (var stream = File.OpenRead(path))
                {
                    var hash = md5.ComputeHash(stream);
                    return BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
                }
            }
        }

        [TestMethod, TestCategory("Appveyor")]
        public void DrivelLabelTest()
        {
            string current_label = _drive.Label;
            Mount();
            _drive.Label = "NEWLABEL";
            _mountService.SetExplorerDriveLabel(_drive);
            string label = _mountService.GetExplorerDriveLabel(_drive);
            Assert.AreEqual("NEWLABEL", label);
            Unmount();
            _drive.Label = current_label;
        }

        [TestMethod, TestCategory("Appveyor")]
        public void SettingsDrivesTest()
        {
            if (!Directory.Exists(_mountService.LocalAppData))
                Directory.CreateDirectory(_mountService.LocalAppData);
            var src = _mountService.LocalAppData + "\\config.json";
            var dst = src + ".bak";
            if (File.Exists(dst))
                File.Delete(dst);
            if (File.Exists(src))
                File.Move(src, dst);
            var settings = _mountService.LoadSettings();
            Assert.AreEqual(0, settings.Drives.Count);
            var drives = new List<Drive> { _drive };
            settings.AddDrives(drives);
            _mountService.SaveSettings(settings);
            settings = _mountService.LoadSettings();
            Assert.AreEqual(1, settings.Drives.Count);
            var d = settings.Drives["X:"];
            Assert.AreEqual(_drive.Name, d.Name);
            Assert.AreEqual(_drive.MountPoint, d.MountPoint);
            File.Delete(src);
            if (File.Exists(dst))
                File.Move(dst, src);
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void MountUnmountTest()
        {
            Mount();
            Unmount();
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void FreeUsedDrivesTest()
        {
            Mount();
            var free_drives = _mountService.GetFreeDrives();
            var used_drives = _mountService.GetUsedDrives();
            Assert.IsNull(free_drives.Find(x => x.Name == "X:"));
            Assert.IsNotNull(used_drives.Find(x => x.Name == "X:"));
            Unmount();
            free_drives = _mountService.GetFreeDrives();
            used_drives = _mountService.GetUsedDrives();
            Assert.IsNotNull(free_drives.Find(x => x.Name == "X:"));
            Assert.IsNull(used_drives.Find(x => x.Name == "X:"));
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void CheckDriveStatusTest()
        {
            RequireSsh();
            _mountService.RunLocal($"subst W: {_mountService.LocalAppData}");
            Drive c = new Drive { Letter = "C", MountPoint = "sshserver" };
            Drive w = new Drive { Letter = "W", MountPoint = "sshserver" };
            Assert.AreEqual(DriveStatus.NOT_SUPPORTED, _mountService.CheckDriveStatus(c).DriveStatus);
            Assert.AreEqual(DriveStatus.IN_USE, _mountService.CheckDriveStatus(w).DriveStatus);
            var status = _mountService.CheckDriveStatus(_drive).DriveStatus;
            Assert.IsTrue(status == DriveStatus.DISCONNECTED);
            Mount();
            Assert.AreEqual(DriveStatus.CONNECTED, _mountService.CheckDriveStatus(_drive).DriveStatus);
            Drive t = new Drive { Letter = "T", MountPoint = $"{_user}@{_host}" };
            Assert.AreEqual(DriveStatus.MOUNTPOINT_IN_USE, _mountService.CheckDriveStatus(t).DriveStatus);
            Unmount();
            _mountService.RunLocal("subst W: /d");
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void MakeDirTest()
        {
            Mount();
            var path = "X:\\tmp\\tempdir";
            Directory.CreateDirectory(path);
            Assert.IsTrue(Directory.Exists(path));
            Directory.Delete(path);
            Assert.IsFalse(Directory.Exists(path));
            Unmount();
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void MakeDirManyTest()
        {
            Mount();
            Parallel.ForEach(Enumerable.Range(1, 20), f =>
            {
                CreateDir(f);
            });
            Parallel.ForEach(Enumerable.Range(1, 20), f =>
            {
                DeleteDir(f);
            });
            Unmount();
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void CreateDeleteFileTest()
        {
            Mount();
            CreateFile(1);
            DeleteFile(1);
            Unmount();
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void CreateManyFileTest()
        {
            Mount();
            Parallel.ForEach(Enumerable.Range(1, 20), f =>
            {
                CreateFile(f);
            });
            Parallel.ForEach(Enumerable.Range(1, 20), f =>
            {
                DeleteFile(f);
            });
            Unmount();
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void CopyBigFileTest()
        {
            Mount();
            var tempfile1 = Path.GetTempPath() + "file_" + Guid.NewGuid().ToString() + ".bin";
            var tempfile2 = Path.GetTempPath() + "file_" + Guid.NewGuid().ToString() + ".bin";
            RandomFile(tempfile1, 10);
            var hash1 = Md5(tempfile1);
            var path = "X:\\tmp\\file_random.bin";
            if (File.Exists(path))
                File.Delete(path);
            Assert.IsFalse(File.Exists(path));
            File.Copy(tempfile1, path);
            Assert.IsTrue(File.Exists(path));
            if (File.Exists(tempfile2))
                File.Delete(tempfile2);
            File.Copy(path, tempfile2);
            var hash2 = Md5(tempfile2);
            Assert.AreEqual(hash1, hash2);
            File.Delete(path);
            File.Delete(tempfile1);
            File.Delete(tempfile2);
            Assert.IsFalse(File.Exists(path));
            Assert.IsFalse(File.Exists(tempfile1));
            Assert.IsFalse(File.Exists(tempfile2));
            Unmount();
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void TestSshTest()
        {
            RequireSsh();
            var r = _mountService.TestSsh(_drive);
            Assert.IsTrue(r.Success);
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void SetupSshTest()
        {
            RequireSsh();
            var seckey = _drive.DefaultAppKey;
            var pubkey = _drive.DefaultAppPubKey;
            var now = DateTime.Now.Ticks.ToString();
            var backup_sec = $"{seckey}.{now}.bak";
            var backup_pub = $"{pubkey}.{now}.bak";

            bool has_sec = File.Exists(seckey);
            bool has_pub = File.Exists(pubkey);
            if (has_sec)
                File.Move(seckey, backup_sec);
            if (has_pub)
                File.Move(pubkey, backup_pub);
            try
            {
                var r = _mountService.TestSsh(_drive);
                Assert.IsFalse(r.Success);

                // generate keys using system ssh-keygen
                var keygen = _mountService.RunLocal("ssh-keygen.exe",
                    $@"-t rsa -b 4096 -m PEM -N """" -f ""{seckey}""");
                Assert.AreEqual(0, keygen.ExitCode, "ssh-keygen failed: " + keygen.Error);

                r = _mountService.SetupSsh(_drive, _pass);
                Assert.AreEqual(MountStatus.OK, r.MountStatus);
            }
            finally
            {
                if (File.Exists(seckey))
                    File.Delete(seckey);
                if (File.Exists(pubkey))
                    File.Delete(pubkey);
                if (has_sec)
                    File.Move(backup_sec, seckey);
                if (has_pub)
                    File.Move(backup_pub, pubkey);
            }
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void TestHostTest()
        {
            RequireSsh();
            Drive nohost = new Drive { Host = "nohost", Port = "22" };
            Drive noport = new Drive { Host = _host, Port = "222" };
            var r = _mountService.TestHost(nohost);
            Assert.AreEqual(MountStatus.BAD_HOST, r.MountStatus);
            r = _mountService.TestHost(noport);
            Assert.AreEqual(MountStatus.BAD_HOST, r.MountStatus);
            r = _mountService.TestHost(_drive);
            Assert.AreEqual(MountStatus.OK, r.MountStatus);
        }

        void CreateFile(int id)
        {
            var path = $"X:\\tmp\\file_{id}.txt";
            if (File.Exists(path))
                DeleteFile(id);
            var myFile = File.Create(path);
            myFile.Close();
            Assert.IsTrue(File.Exists(path));
        }

        void DeleteFile(int id)
        {
            var path = $"X:\\tmp\\file_{id}.txt";
            if (File.Exists(path))
                File.Delete(path);
            Assert.IsTrue(!File.Exists(path));
        }

        void CreateDir(int id)
        {
            var path = $"X:\\tmp\\folder_{id}";
            if (Directory.Exists(path))
                DeleteDir(id);
            Directory.CreateDirectory(path);
            Assert.IsTrue(Directory.Exists(path));
        }

        void DeleteDir(int id)
        {
            var path = $"X:\\tmp\\folder_{id}";
            if (Directory.Exists(path))
                Directory.Delete(path);
            Assert.IsTrue(!Directory.Exists(path));
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void RenameFileTest()
        {
            Mount();
            CreateFile(1);
            DeleteFile(2);
            var f1 = $"X:\\tmp\\file_1.txt";
            var f2 = $"X:\\tmp\\file_2.txt";
            File.Move(f1, f2);
            Assert.IsTrue(!File.Exists(f1));
            Assert.IsTrue(File.Exists(f2));
            DeleteFile(2);
            Assert.IsTrue(!File.Exists(f2));
            Unmount();
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void RenameDirTest()
        {
            Mount();
            CreateDir(1);
            var f1 = $"X:\\tmp\\folder_1";
            var f2 = $"X:\\tmp\\folder_2";
            Directory.Move(f1, f2);
            Assert.IsTrue(!Directory.Exists(f1));
            Assert.IsTrue(Directory.Exists(f2));
            DeleteDir(2);
            Assert.IsTrue(!Directory.Exists(f2));
            Unmount();
        }
    }
}
