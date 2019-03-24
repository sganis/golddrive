using golddrive;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace golddrive.Tests
{
    [TestClass()]
    public class MountManagerTest
    {
        MountService mountManager;

        [TestInitialize]
        public void Init()
        {
            mountManager = new MountService();
        }

        [TestMethod()]
        public void GetGoldDrivesTest()
        {
            // arrange
            //MountManager mountManager = new MountManager();
            // act
            var drives = mountManager.GetGoldDrives();
            // assert
            Assert.AreEqual(drives.Count, 3);
        }

        [TestMethod()]
        public void SetDriveNameTest()
        {
            //Drive d = new Drive();
            //d.Remote = @"\\golddrive\192.168.100.201\path";
            //d.Name = "A Name";
            //mountManager.SetDriveName(d);
        }

        [TestMethod()]
        public void GetDriveLabelTest()
        {
            Assert.Fail();
        }
    }
}