using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace golddrive.Tests
{
    [TestClass()]
    public class MountManagerTest
    {
        MountManager mountManager;

       [TestInitialize]
        public void Init()
        {
            mountManager = new MountManager();
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
    }
}