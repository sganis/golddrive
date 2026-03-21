// src/test/Unit/DriveTest.cs
using golddrive;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace golddrive_test.Unit
{
    [TestClass]
    public class DriveTest
    {
        [TestMethod]
        public void Drive_DefaultConstructor_SetsDefaults()
        {
            var drive = new Drive();
            Assert.IsNull(drive.Host);
            Assert.IsNull(drive.Letter);
            Assert.AreEqual(DriveStatus.FREE, drive.Status);
        }

        [TestMethod]
        public void Drive_CopyConstructor_ClonesProperties()
        {
            var original = new Drive
            {
                Host = "myhost",
                Port = "2222",
                User = "testuser",
                Letter = "Z",
                Label = "MyDrive",
                Args = "-oarg1"
            };
            var copy = new Drive(original);
            Assert.AreEqual("myhost", copy.Host);
            Assert.AreEqual("2222", copy.Port);
            Assert.AreEqual("testuser", copy.User);
            Assert.AreEqual("Z", copy.Letter);
            Assert.AreEqual("MyDrive", copy.Label);
            Assert.AreEqual("-oarg1", copy.Args);
        }

        [TestMethod]
        public void Drive_Clone_CopiesAllFields()
        {
            var source = new Drive
            {
                Host = "server1",
                Port = "22",
                User = "admin",
                Letter = "G",
                Label = "Test",
                Args = "-odebug"
            };
            var target = new Drive();
            target.Clone(source);
            Assert.AreEqual("server1", target.Host);
            Assert.AreEqual("22", target.Port);
            Assert.AreEqual("admin", target.User);
            Assert.AreEqual("G", target.Letter);
        }

        [TestMethod]
        public void Drive_Clone_NullSource_DoesNotThrow()
        {
            var drive = new Drive { Host = "original" };
            drive.Clone(null);
            Assert.AreEqual("original", drive.Host);
        }

        [TestMethod]
        public void Drive_Name_ReturnsLetterWithColon()
        {
            var drive = new Drive { Letter = "Z" };
            Assert.AreEqual("Z:", drive.Name);
        }

        [TestMethod]
        public void Drive_MountPoint_SetsHostAndUser()
        {
            var drive = new Drive();
            drive.MountPoint = "user@myhost!2222";
            Assert.AreEqual("myhost", drive.Host);
            Assert.AreEqual("user", drive.User);
            Assert.AreEqual("2222", drive.Port);
        }

        [TestMethod]
        public void Drive_MountPoint_HostOnly()
        {
            var drive = new Drive();
            drive.MountPoint = "192.168.1.1";
            Assert.AreEqual("192.168.1.1", drive.Host);
        }

        [TestMethod]
        public void Drive_CurrentPort_DefaultIs22()
        {
            var drive = new Drive();
            Assert.AreEqual(22, drive.CurrentPort);
        }

        [TestMethod]
        public void Drive_CurrentPort_ParsesPortString()
        {
            var drive = new Drive { Port = "2222" };
            Assert.AreEqual(2222, drive.CurrentPort);
        }

        [TestMethod]
        public void Drive_CurrentUser_FallsBackToEnvironment()
        {
            var drive = new Drive();
            Assert.IsFalse(string.IsNullOrEmpty(drive.CurrentUser));
        }

        [TestMethod]
        public void Drive_CurrentUser_ReturnsSetUser()
        {
            var drive = new Drive { User = "testuser" };
            Assert.AreEqual("testuser", drive.CurrentUser);
        }

        [TestMethod]
        public void Drive_Args_NullWhenEmpty()
        {
            var drive = new Drive { Args = "" };
            Assert.IsNull(drive.Args);
        }

        [TestMethod]
        public void Drive_Args_ReturnsValueWhenSet()
        {
            var drive = new Drive { Args = "-odebug" };
            Assert.AreEqual("-odebug", drive.Args);
        }
    }
}
