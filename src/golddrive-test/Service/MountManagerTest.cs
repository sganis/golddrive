using Microsoft.VisualStudio.TestTools.UnitTesting;
using golddrive;
using System.Collections.Generic;
using System.IO;

namespace golddrive.Tests
{
    [TestClass()]
    public class MountManagerTest
    {
        MountService _mountService;
        Drive _drive;

        [TestInitialize]
        public void Init()
        {
            _mountService = new MountService();
            _drive = new Drive
            {
                Letter = "X",
                MountPoint = "vlcc31",
                Label = "GOLDDRIVE"
            };
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


        [TestMethod()]
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



        [TestMethod()]
        public void LoadSaveSettingsDrivesTest()
        {
            string settings_path = _mountService.AppPath + "\\settings.xml";
            if (File.Exists(settings_path))
                File.Delete(settings_path);
            var drives = new List<Drive>();
            drives.Add(_drive);
            var saved_drives = _mountService.LoadSettingsDrives();
            Assert.AreEqual(saved_drives.Count, 0);
            _mountService.SaveSettingsDrives(drives);
            saved_drives = _mountService.LoadSettingsDrives();
            Assert.AreEqual(saved_drives.Count, 1);
            var d = saved_drives[0];
            Assert.AreEqual(d.Name, _drive.Name);
            Assert.AreEqual(d.MountPoint, _drive.MountPoint);
            File.Delete(settings_path);
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
            _mountService.RunLocal("subst W: C:\\Temp");
            Drive c = new Drive { Letter = "C", MountPoint = "vlcc31" };
            Drive w = new Drive { Letter = "W", MountPoint = "vlcc31" };
            Drive y = new Drive { Letter = "Y", MountPoint = "vlcc31" };
            Assert.AreEqual(_mountService.CheckDriveStatus(c).DriveStatus, DriveStatus.NOT_SUPPORTED);
            Assert.AreEqual(_mountService.CheckDriveStatus(w).DriveStatus, DriveStatus.IN_USE);
            Assert.AreEqual(_mountService.CheckDriveStatus(_drive).DriveStatus, DriveStatus.DISCONNECTED);
            Mount();
            Assert.AreEqual(_mountService.CheckDriveStatus(_drive).DriveStatus, DriveStatus.CONNECTED);

            Assert.AreEqual(_mountService.CheckDriveStatus(_drive).DriveStatus, DriveStatus.CONNECTED);

            _mountService.RunLocal("subst W: /d");
        }
    }
}