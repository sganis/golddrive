// src/test/Unit/SettingsTest.cs
using golddrive;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Newtonsoft.Json;
using System.Collections.Generic;
using System.IO;

namespace golddrive_test.Unit
{
    [TestClass]
    public class SettingsTest
    {
        [TestMethod]
        public void Settings_DefaultConstructor_HasEmptyDrives()
        {
            var settings = new Settings();
            Assert.IsNotNull(settings.Drives);
            Assert.AreEqual(0, settings.Drives.Count);
        }

        [TestMethod]
        public void Settings_AddDrive_NewDrive()
        {
            var settings = new Settings();
            var drive = new Drive
            {
                Letter = "Z",
                Host = "myhost",
                MountPoint = "myhost"
            };
            settings.AddDrive(drive);
            Assert.AreEqual(1, settings.Drives.Count);
            Assert.IsTrue(settings.Drives.ContainsKey("Z:"));
        }

        [TestMethod]
        public void Settings_AddDrive_UpdatesExisting()
        {
            var settings = new Settings();
            var drive1 = new Drive { Letter = "Z", Host = "host1", MountPoint = "host1" };
            settings.AddDrive(drive1);

            var drive2 = new Drive { Letter = "Z", Host = "host2", MountPoint = "host2", Label = "updated" };
            settings.AddDrive(drive2);

            Assert.AreEqual(1, settings.Drives.Count);
            Assert.AreEqual("updated", settings.Drives["Z:"].Label);
        }

        [TestMethod]
        public void Settings_AddDrives_ReplacesAll()
        {
            var settings = new Settings();
            settings.AddDrive(new Drive { Letter = "Z", MountPoint = "host1" });
            settings.AddDrive(new Drive { Letter = "Y", MountPoint = "host2" });
            Assert.AreEqual(2, settings.Drives.Count);

            var newDrives = new List<Drive>
            {
                new Drive { Letter = "X", MountPoint = "host3" }
            };
            settings.AddDrives(newDrives);
            Assert.AreEqual(1, settings.Drives.Count);
            Assert.IsTrue(settings.Drives.ContainsKey("X:"));
        }

        [TestMethod]
        public void Settings_Load_NonExistentFile_DoesNotThrow()
        {
            var settings = new Settings
            {
                Filename = @"C:\nonexistent\path\config.json"
            };
            settings.Load();
            Assert.AreEqual(0, settings.Drives.Count);
        }

        [TestMethod]
        public void Settings_Load_ValidJson_ParsesDrives()
        {
            string tempFile = Path.GetTempFileName();
            try
            {
                string json = @"{
                    ""Drives"": {
                        ""Z:"": {
                            ""MountPoint"": ""user@myhost"",
                            ""Label"": ""TestDrive"",
                            ""Args"": ""-odebug""
                        }
                    }
                }";
                File.WriteAllText(tempFile, json);

                var settings = new Settings { Filename = tempFile };
                settings.Load();

                Assert.AreEqual(1, settings.Drives.Count);
                Assert.IsTrue(settings.Drives.ContainsKey("Z:"));
                Assert.AreEqual("TestDrive", settings.Drives["Z:"].Label);
                Assert.AreEqual("-odebug", settings.Drives["Z:"].Args);
            }
            finally
            {
                File.Delete(tempFile);
            }
        }

        [TestMethod]
        public void Settings_Load_SelectedDrive_IsResolved()
        {
            string tempFile = Path.GetTempFileName();
            try
            {
                string json = @"{
                    ""Selected"": ""Z:"",
                    ""Drives"": {
                        ""Z:"": {
                            ""MountPoint"": ""myhost""
                        }
                    }
                }";
                File.WriteAllText(tempFile, json);

                var settings = new Settings { Filename = tempFile };
                settings.Load();

                Assert.IsNotNull(settings.SelectedDrive);
                Assert.AreEqual("Z", settings.SelectedDrive.Letter);
            }
            finally
            {
                File.Delete(tempFile);
            }
        }

        [TestMethod]
        public void Settings_Load_EmptyDrives_Succeeds()
        {
            string tempFile = Path.GetTempFileName();
            try
            {
                string json = @"{ ""Drives"": {} }";
                File.WriteAllText(tempFile, json);

                var settings = new Settings { Filename = tempFile };
                settings.Load();

                Assert.AreEqual(0, settings.Drives.Count);
            }
            finally
            {
                File.Delete(tempFile);
            }
        }
    }
}
