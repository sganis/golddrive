
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Collections.Generic;
using System.IO;
using System;
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
        private Drive _drive = new Drive
        {
            Letter = "X",
            MountPoint = $"{_user}@{_host}",
            User = _user,
            Label = "Golddrive",
            IsGoldDrive = true,
            Status = DriveStatus.DISCONNECTED,
        };

        [TestInitialize]
        public void Init()
        {
            _mountService.RunLocal("subst /d W:");
            Assert.IsFalse(string.IsNullOrEmpty(_host), "set GOLDDRIVE_HOST env var");
        }

        [TestCleanup]
        public void Teardown()
        {
            _mountService.Unmount(_drive);
        }
        public void Mount()
        {
            var r = _mountService.Connect(_drive);
            if (r.DriveStatus != DriveStatus.CONNECTED)
                Console.WriteLine($"ERROR: {r.Error}");
            Assert.AreEqual(r.DriveStatus, DriveStatus.CONNECTED);

        }
        public void Unmount()
        {
            var r = _mountService.Unmount(_drive);
            Assert.AreEqual(r.DriveStatus, DriveStatus.DISCONNECTED);

        }
        public void RandomFile(string path, int gigabytes)
        {
            FileStream fs = new FileStream(path, FileMode.CreateNew);
            fs.Seek(1024L * 1024 * 1024 * gigabytes, SeekOrigin.Begin);
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
        public void SetGetDrivelLabelTest()
        {
            string current_label = _drive.Label;
            Mount();
            _drive.Label = "NEWLABEL";
            _mountService.SetExplorerDriveLabel(_drive);
            string label = _mountService.GetExplorerDriveLabel(_drive);
            Assert.AreEqual(label, "NEWLABEL");
            Unmount();
            _drive.Label = current_label;
        }



        [TestMethod, TestCategory("Appveyor")]
        public void LoadSaveSettingsDrivesTest()
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
            Assert.AreEqual(settings.Drives.Count, 0);
            var drives = new List<Drive> { _drive };
            settings.AddDrives(drives);
            _mountService.SaveSettings(settings);
            settings = _mountService.LoadSettings();
            Assert.AreEqual(settings.Drives.Count, 1);
            var d = settings.Drives["X:"];
            Assert.AreEqual(d.Name, _drive.Name);
            Assert.AreEqual(d.MountPoint, _drive.MountPoint);
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
            Assert.AreEqual(free_drives.Find(x => x.Name == "X:"), null);
            Assert.AreNotEqual(used_drives.Find(x => x.Name == "X:"), null);
            Unmount();
            free_drives = _mountService.GetFreeDrives();
            used_drives = _mountService.GetUsedDrives();
            Assert.AreNotEqual(free_drives.Find(x => x.Name == "X:"), null);
            Assert.AreEqual(used_drives.Find(x => x.Name == "X:"), null);
        }
        [TestMethod(), TestCategory("Appveyor")]
        public void CheckDriveStatusTest()
        {
            //Unmount();
            _mountService.RunLocal($"subst W: {_mountService.LocalAppData}");
            Drive c = new Drive { Letter = "C", MountPoint = "sshserver" };
            Drive w = new Drive { Letter = "W", MountPoint = "sshserver" };
            Drive y = new Drive { Letter = "Y", MountPoint = "sshserver" };
            Assert.AreEqual(_mountService.CheckDriveStatus(c).DriveStatus, DriveStatus.NOT_SUPPORTED);
            Assert.AreEqual(_mountService.CheckDriveStatus(w).DriveStatus, DriveStatus.IN_USE);
            var status = _mountService.CheckDriveStatus(_drive).DriveStatus;
            Assert.IsTrue(status==DriveStatus.DISCONNECTED);
            Mount();
            Assert.AreEqual(_mountService.CheckDriveStatus(_drive).DriveStatus, DriveStatus.CONNECTED);
            Drive t = new Drive { Letter = "T", MountPoint = $"{_user}@{_host}" };
            Assert.AreEqual(_mountService.CheckDriveStatus(t).DriveStatus, DriveStatus.MOUNTPOINT_IN_USE);
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
            Parallel.ForEach(Enumerable.Range(1, 100), f =>
            {
                CreateDir(f);
            });
            Parallel.ForEach(Enumerable.Range(1, 100), f =>
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

            Parallel.ForEach(Enumerable.Range(1, 100), f =>
            {
                CreateFile(f);
            });
            Parallel.ForEach(Enumerable.Range(1, 100), f =>
            {
                DeleteFile(f);
            });
            Unmount();
        }
        [TestMethod(), TestCategory("Appveyor")]
        public void Copy1GBFileTest()
        {
            Mount();
            var tempfile1 = Path.GetTempPath() + "file_" + Guid.NewGuid().ToString() + ".bin";
            var tempfile2 = Path.GetTempPath() + "file_" + Guid.NewGuid().ToString() + ".bin";
            RandomFile(tempfile1, 1);
            var hash1 = Md5(tempfile1);
            var path = "X:\\tmp\\file_random.bin";
            if (File.Exists(path))
                File.Delete(path);
            Assert.IsFalse(File.Exists(path));
            File.Copy(tempfile1, path);
            Assert.IsTrue(File.Exists(path));
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
            var r = _mountService.TestSsh(_drive);
            Assert.IsTrue(r.Success);
        }
        [TestMethod(), TestCategory("Appveyor")]
        public void SetupSshTest()
        {
            var now = DateTime.Now.Ticks.ToString();
            var backup_sec = $"{_drive.AppKey}.{now}.bak";
            var backup_pub = $"{_drive.AppPubKey}.{now}.bak";
            File.Move(_drive.AppKey, backup_sec);
            bool has_pub = File.Exists(_drive.AppPubKey);
            if (has_pub)
                File.Move(_drive.AppPubKey, backup_pub);
            var r = _mountService.TestSsh(_drive);
            Assert.IsFalse(r.Success);
            r = _mountService.SetupSsh(_drive, _pass);
            Assert.IsTrue(r.MountStatus == MountStatus.OK);
            File.Delete(_drive.AppKey);
            File.Delete(_drive.AppPubKey);
            File.Move(backup_sec, _drive.AppKey);
            if (has_pub)
                File.Move(backup_pub, _drive.AppPubKey);
        }

        [TestMethod(), TestCategory("Appveyor")]
        public void TestHostTest()
        {
            Drive nohost = new Drive { Host = "nohost", Port = 22 };
            Drive noport = new Drive { Host = "localhost", Port = 222 };
            var r = _mountService.TestHost(nohost);
            Assert.IsTrue(r.MountStatus == MountStatus.BAD_HOST);
            r = _mountService.TestHost(noport);
            Assert.IsTrue(r.MountStatus == MountStatus.BAD_HOST);
            r = _mountService.TestHost(_drive);
            Assert.IsTrue(r.MountStatus == MountStatus.OK);
        }


        //bool IsFileLocked(string path)
        //{
        //    try
        //    {
        //        using (File.Open(path, FileMode.Open))
        //        {
        //            return false;
        //        }
        //    }
        //    catch
        //    {
        //        Console.WriteLine("File locked: " + path);
        //        return true;
        //    }
        //}
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
            if(File.Exists(path))
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
            if(Directory.Exists(path))
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