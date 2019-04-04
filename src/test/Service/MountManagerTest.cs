using Microsoft.VisualStudio.TestTools.UnitTesting;
using golddrive;
using System.Collections.Generic;
using System.IO;
using System;

namespace golddrive.Tests
{
    [TestClass()]
    public class MountManagerTest
    {
        MountService _mountService = new MountService();
        static string _host = System.Environment.GetEnvironmentVariable("%GOLDDRIVE_HOST%");
        Drive _drive = new Drive
            {
                Letter = "X",
                MountPoint = _host,
                Label = "Golddrive"
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


        [TestMethod]
        public void SetGetDrivelLabelTest()
        {
            string current_label = _drive.Label;
            Mount();
            _drive.Label = "NEWLABEL";
            _mountService.SetDriveLabel(_drive);
            string label = _mountService.GetDriveLabel(_drive);
            Assert.AreEqual(label, "NEWLABEL");
            Unmount();
            _drive.Label = current_label;
        }



        [TestMethod, TestCategory("Appveyor")]
        public void LoadSaveSettingsDrivesTest()
        {
            //const string V = "\\settings.xml";
            //string settings_path = _mountService.LocalAppData + V;

            //if (File.Exists(settings_path))
            //{
            //    File.Delete(settings_path);
            //}
            var src = _mountService.LocalAppData + "\\config.json";
            var dst = src + ".bak";
            if (File.Exists(dst))
                File.Delete(dst);
            if (File.Exists(src))
                File.Move(src, dst);
            var settings = _mountService.LoadSettings();
            Assert.AreEqual(settings.Drives.Count, 0);
            var drives = new List<Drive>();
            drives.Add(_drive);
            settings.AddDrives(drives);
            _mountService.SaveSettings(settings);
            settings = _mountService.LoadSettings();
            Assert.AreEqual(settings.Drives.Count, 1);
            var d = settings.Drives["X:"];
            Assert.AreEqual(d.Name, _drive.Name);
            Assert.AreEqual(d.MountPoint, _drive.MountPoint);
            File.Delete(src);
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
            Drive c = new Drive { Letter = "C", MountPoint = "vlcc31" };
            Drive w = new Drive { Letter = "W", MountPoint = "vlcc31" };
            Drive y = new Drive { Letter = "Y", MountPoint = "vlcc31" };
            Assert.AreEqual(_mountService.CheckDriveStatus(c).DriveStatus, DriveStatus.NOT_SUPPORTED);
            Assert.AreEqual(_mountService.CheckDriveStatus(w).DriveStatus, DriveStatus.LETTER_IN_USE);
            Assert.AreEqual(_mountService.CheckDriveStatus(_drive).DriveStatus, DriveStatus.DISCONNECTED);
            Mount();
            Assert.AreEqual(_mountService.CheckDriveStatus(_drive).DriveStatus, DriveStatus.CONNECTED);
            Drive t = new Drive { Letter = "T", MountPoint = _host };
            Assert.AreEqual(_mountService.CheckDriveStatus(t).DriveStatus, DriveStatus.MOUNTPOINT_IN_USE);
            Unmount();
            _mountService.RunLocal("subst W: /d");
        }
    }
}