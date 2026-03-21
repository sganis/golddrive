// src/test/Unit/ReturnBoxTest.cs
using golddrive;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace golddrive_test.Unit
{
    [TestClass]
    public class ReturnBoxTest
    {
        [TestMethod]
        public void ReturnBox_DefaultConstructor_SetsDefaults()
        {
            var rb = new ReturnBox();
            Assert.AreEqual(MountStatus.UNKNOWN, rb.MountStatus);
            Assert.AreEqual(DriveStatus.UNKNOWN, rb.DriveStatus);
            Assert.IsFalse(rb.Success);
            Assert.AreEqual(-999, rb.ExitCode);
            Assert.IsNull(rb.Error);
            Assert.IsNull(rb.Output);
            Assert.IsNull(rb.Drive);
        }

        [TestMethod]
        public void ReturnBox_SetProperties_RetainsValues()
        {
            var rb = new ReturnBox
            {
                MountStatus = MountStatus.OK,
                DriveStatus = DriveStatus.CONNECTED,
                Success = true,
                ExitCode = 0,
                Error = "",
                Output = "test output"
            };
            Assert.AreEqual(MountStatus.OK, rb.MountStatus);
            Assert.AreEqual(DriveStatus.CONNECTED, rb.DriveStatus);
            Assert.IsTrue(rb.Success);
            Assert.AreEqual(0, rb.ExitCode);
            Assert.AreEqual("test output", rb.Output);
        }

        [TestMethod]
        public void ReturnBox_Drive_CanBeAssigned()
        {
            var drive = new Drive { Letter = "Z", Host = "myhost" };
            var rb = new ReturnBox { Drive = drive };
            Assert.IsNotNull(rb.Drive);
            Assert.AreEqual("Z", rb.Drive.Letter);
        }
    }
}
