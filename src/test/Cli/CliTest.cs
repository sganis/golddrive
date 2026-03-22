// src/test/Cli/CliTest.cs
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Diagnostics;
using System.IO;
using System.Net.Sockets;

namespace golddrive_test.Cli
{
    [TestClass]
    public class CliTest
    {
        private static string _exe;
        private static string _host;
        private static string _user;
        private static string _pass;
        private static bool _sshAvailable;

        [ClassInitialize]
        public static void ClassInit(TestContext context)
        {
            _exe = FindGolddriveExe();
            _host = Environment.GetEnvironmentVariable("GOLDDRIVE_HOST");
            _user = Environment.GetEnvironmentVariable("GOLDDRIVE_USER");
            _pass = Environment.GetEnvironmentVariable("GOLDDRIVE_PASS");

            if (!string.IsNullOrEmpty(_host))
            {
                try
                {
                    using (var client = new TcpClient())
                    {
                        var result = client.BeginConnect(_host, 22, null, null);
                        _sshAvailable = result.AsyncWaitHandle.WaitOne(TimeSpan.FromSeconds(3));
                        if (_sshAvailable)
                            client.EndConnect(result);
                    }
                }
                catch { _sshAvailable = false; }
            }
        }

        private static string FindGolddriveExe()
        {
            string dir = Path.GetDirectoryName(
                System.Reflection.Assembly.GetExecutingAssembly().Location);
            while (dir != null)
            {
                string candidate = Path.Combine(dir, "build", "Release", "x64", "golddrive.exe");
                if (File.Exists(candidate))
                    return candidate;
                dir = Path.GetDirectoryName(dir);
            }
            return null;
        }

        private ReturnBox Run(string args, int timeoutSecs = 10)
        {
            var r = new ReturnBox();
            using (var p = new Process())
            {
                p.StartInfo = new ProcessStartInfo
                {
                    FileName = _exe,
                    Arguments = args,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };
                p.Start();
                r.Output = p.StandardOutput.ReadToEnd();
                r.Error = p.StandardError.ReadToEnd();
                if (!p.WaitForExit(timeoutSecs * 1000))
                {
                    try { p.Kill(); } catch { }
                    r.ExitCode = -1;
                    return r;
                }
                r.ExitCode = p.ExitCode;
                r.Success = r.ExitCode == 0;
            }
            return r;
        }

        private void RequireExe()
        {
            if (string.IsNullOrEmpty(_exe) || !File.Exists(_exe))
                Assert.Inconclusive("golddrive.exe not found in build output");
        }

        private void RequireSsh()
        {
            if (!_sshAvailable)
                Assert.Inconclusive("SSH server not available");
        }

        // ------------------------------------------------------------------
        // Basic CLI tests (no SSH required)
        // ------------------------------------------------------------------

        [TestMethod]
        public void Cli_Version_ShowsVersionInfo()
        {
            RequireExe();
            var r = Run("--version");
            Assert.AreEqual(0, r.ExitCode, $"stderr: {r.Error}");
            Assert.IsTrue(r.Error.Contains("Golddrive"),
                $"Expected version banner, got: {r.Error}");
            Assert.IsTrue(r.Error.Contains("Libssh2"),
                "Expected Libssh2 version in output");
            Assert.IsTrue(r.Error.Contains("OpenSSL"),
                "Expected OpenSSL version in output");
            Assert.IsTrue(r.Error.Contains("FUSE"),
                "Expected FUSE version in output");
        }

        [TestMethod]
        public void Cli_Version_ContainsBitness()
        {
            RequireExe();
            var r = Run("--version");
            Assert.IsTrue(r.Error.Contains("64-bit") || r.Error.Contains("32-bit"),
                $"Expected bitness in version output, got: {r.Error}");
        }

        [TestMethod]
        public void Cli_Help_ShowsUsage()
        {
            RequireExe();
            var r = Run("--help");
            Assert.AreEqual(1, r.ExitCode, "help exits with 1");
            Assert.IsTrue(r.Error.Contains("Usage: golddrive"),
                $"Expected usage line, got: {r.Error}");
        }

        [TestMethod]
        public void Cli_Help_ListsAllOptions()
        {
            RequireExe();
            var r = Run("--help");
            string help = r.Error;
            Assert.IsTrue(help.Contains("-h HOST"), "missing -h option");
            Assert.IsTrue(help.Contains("-u USER"), "missing -u option");
            Assert.IsTrue(help.Contains("-k PKEY"), "missing -k option");
            Assert.IsTrue(help.Contains("-p PORT"), "missing -p option");
            Assert.IsTrue(help.Contains("keeplink"), "missing keeplink option");
            Assert.IsTrue(help.Contains("audit"), "missing audit option");
            Assert.IsTrue(help.Contains("cipher"), "missing cipher option");
            Assert.IsTrue(help.Contains("buffer=BYTES"), "missing buffer option");
        }

        [TestMethod]
        public void Cli_NoArgs_FailsWithMessage()
        {
            RequireExe();
            var r = Run("");
            Assert.AreNotEqual(0, r.ExitCode, "no args should fail");
            Assert.IsTrue(r.Error.Contains("bad arguments") || r.Error.Contains("--help"),
                $"Expected error message, got: {r.Error}");
        }

        [TestMethod]
        public void Cli_InvalidArg_Fails()
        {
            RequireExe();
            var r = Run("--invalid-flag");
            Assert.AreNotEqual(0, r.ExitCode, "invalid flag should fail");
        }

        [TestMethod]
        public void Cli_DriveOnly_NoRemote_Fails()
        {
            RequireExe();
            var r = Run("Z:");
            Assert.AreNotEqual(0, r.ExitCode,
                "drive letter without remote should fail");
        }

        [TestMethod]
        public void Cli_BadKey_Fails()
        {
            RequireExe();
            var r = Run(@"Z: \\golddrive\testhost -k C:\nonexistent\key");
            Assert.AreNotEqual(0, r.ExitCode, "bad key path should fail");
        }

        // ------------------------------------------------------------------
        // SSH integration tests (require GOLDDRIVE_HOST)
        // golddrive.exe runs a blocking FUSE loop, so we start it in
        // the background and verify the mount appears.
        // ------------------------------------------------------------------

        [TestMethod]
        public void Cli_MountUnmount_Works()
        {
            RequireExe();
            RequireSsh();

            string drive = "X:";
            string remote = $@"\\golddrive\{_user}@{_host}";
            Process gd = null;

            try
            {
                // start golddrive in background (FUSE loop blocks)
                gd = new Process();
                gd.StartInfo = new ProcessStartInfo
                {
                    FileName = _exe,
                    Arguments = $"{drive} {remote}",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };
                gd.Start();

                // wait for mount to appear (up to 15 seconds)
                bool mounted = false;
                for (int i = 0; i < 30; i++)
                {
                    System.Threading.Thread.Sleep(500);
                    if (gd.HasExited)
                    {
                        string err = gd.StandardError.ReadToEnd();
                        Assert.Fail($"golddrive exited early (code {gd.ExitCode}): {err}");
                    }
                    if (Directory.Exists(@"X:\"))
                    {
                        mounted = true;
                        break;
                    }
                }
                Assert.IsTrue(mounted, "drive X: did not appear within 15 seconds");

                // basic smoke test: list root directory
                var entries = Directory.GetFileSystemEntries(@"X:\");
                Assert.IsNotNull(entries);
            }
            finally
            {
                // unmount and cleanup
                RunProcess("net.exe", $"use /d {drive} /y", 10);
                if (gd != null && !gd.HasExited)
                {
                    gd.WaitForExit(5000);
                    if (!gd.HasExited)
                        try { gd.Kill(); } catch { }
                }
                gd?.Dispose();
            }
        }

        [TestMethod]
        public void Cli_MountCreateDeleteFile_Works()
        {
            RequireExe();
            RequireSsh();

            string drive = "X:";
            string remote = $@"\\golddrive\{_user}@{_host}";
            Process gd = null;

            try
            {
                gd = new Process();
                gd.StartInfo = new ProcessStartInfo
                {
                    FileName = _exe,
                    Arguments = $"{drive} {remote}",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };
                gd.Start();

                // wait for mount
                bool mounted = false;
                for (int i = 0; i < 30; i++)
                {
                    System.Threading.Thread.Sleep(500);
                    if (gd.HasExited)
                    {
                        string err = gd.StandardError.ReadToEnd();
                        Assert.Fail($"golddrive exited early (code {gd.ExitCode}): {err}");
                    }
                    if (Directory.Exists(@"X:\"))
                    {
                        mounted = true;
                        break;
                    }
                }
                Assert.IsTrue(mounted, "drive X: did not appear within 15 seconds");

                // create and delete a file
                string testDir = @"X:\tmp";
                if (!Directory.Exists(testDir))
                    Directory.CreateDirectory(testDir);

                string testFile = Path.Combine(testDir, "cli_test_file.txt");
                File.WriteAllText(testFile, "hello from cli test");
                Assert.IsTrue(File.Exists(testFile), "test file should exist");

                string content = File.ReadAllText(testFile);
                Assert.AreEqual("hello from cli test", content);

                File.Delete(testFile);
                Assert.IsFalse(File.Exists(testFile), "test file should be deleted");
            }
            finally
            {
                RunProcess("net.exe", $"use /d {drive} /y", 10);
                if (gd != null && !gd.HasExited)
                {
                    gd.WaitForExit(5000);
                    if (!gd.HasExited)
                        try { gd.Kill(); } catch { }
                }
                gd?.Dispose();
            }
        }

        private static void RunProcess(string cmd, string args, int timeoutSecs)
        {
            using (var p = new Process())
            {
                p.StartInfo = new ProcessStartInfo
                {
                    FileName = cmd,
                    Arguments = args,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };
                p.Start();
                p.WaitForExit(timeoutSecs * 1000);
            }
        }

        // ------------------------------------------------------------------
        // Helper class for process results
        // ------------------------------------------------------------------

        private class ReturnBox
        {
            public string Output { get; set; }
            public string Error { get; set; }
            public int ExitCode { get; set; }
            public bool Success { get; set; }
        }
    }
}
