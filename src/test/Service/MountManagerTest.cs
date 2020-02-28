
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
        private Drive _drive = new Drive
        {
            Letter = "X",
            MountPoint = _host,
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
        [TestMethod]
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


        [TestMethod()]
        public void MountUnmountTest()
        {
            Mount();
            Unmount();

        }
        [TestMethod()]
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
        [TestMethod()]
        public void CheckDriveStatusTest()
        {
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
            Drive t = new Drive { Letter = "T", MountPoint = _host };
            Assert.AreEqual(_mountService.CheckDriveStatus(t).DriveStatus, DriveStatus.MOUNTPOINT_IN_USE);
            Unmount();
            _mountService.RunLocal("subst W: /d");
        }

        [TestMethod()]
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
        [TestMethod()]
        public void MakeDirManyTest()
        {
            Mount();
            Parallel.ForEach(Enumerable.Range(1, 1000), f =>
            {
                CreateDir(f);
            });
            Parallel.ForEach(Enumerable.Range(1, 1000), f =>
            {
                DeleteDir(f);
            });
            Unmount();
        }

        [TestMethod()]
        public void CreateDeleteFileTest()
        {
            Mount();
            CreateFile(1);
            DeleteFile(1);
            Unmount();
        }
        [TestMethod()]
        public void CreateManyFileTest()
        {
            Mount();

            Parallel.ForEach(Enumerable.Range(1, 1000), f =>
            {
                CreateFile(f);
            });
            Parallel.ForEach(Enumerable.Range(1, 1000), f =>
            {
                DeleteFile(f);
            });
            Unmount();
        }
        [TestMethod()]
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
            Directory.Delete(path);
            Assert.IsTrue(!Directory.Exists(path));
        }
        [TestMethod()]
        public void RenameFileTest()
        {
            Mount();
            CreateFile(1);
            var f1 = $"X:\\tmp\\file_1.txt";
            var f2 = $"X:\\tmp\\file_2.txt";
            File.Move(f1, f2);
            Assert.IsTrue(!File.Exists(f1));
            Assert.IsTrue(File.Exists(f2));
            DeleteFile(2);
            Assert.IsTrue(!File.Exists(f2));
            Unmount();
        }
        [TestMethod()]
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