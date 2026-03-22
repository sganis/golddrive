// src/test/Unit/SanitizeShellArgTest.cs
using System;
using golddrive;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace golddrive_test.Unit
{
    [TestClass]
    public class SanitizeShellArgTest
    {
        [TestMethod]
        public void SanitizeShellArg_ValidInput_ReturnsUnchanged()
        {
            Assert.AreEqual("Z:", MountService.SanitizeShellArg("Z:"));
            Assert.AreEqual(@"\\golddrive\user@host", MountService.SanitizeShellArg(@"\\golddrive\user@host"));
            Assert.AreEqual("simple-name", MountService.SanitizeShellArg("simple-name"));
        }

        [TestMethod]
        public void SanitizeShellArg_NullOrEmpty_ReturnsAsIs()
        {
            Assert.IsNull(MountService.SanitizeShellArg(null));
            Assert.AreEqual("", MountService.SanitizeShellArg(""));
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_Ampersand_Throws()
        {
            MountService.SanitizeShellArg("foo&bar");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_Pipe_Throws()
        {
            MountService.SanitizeShellArg("foo|bar");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_Semicolon_Throws()
        {
            MountService.SanitizeShellArg("foo;bar");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_Backtick_Throws()
        {
            MountService.SanitizeShellArg("foo`bar");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_DollarSign_Throws()
        {
            MountService.SanitizeShellArg("foo$bar");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_Parentheses_Throws()
        {
            MountService.SanitizeShellArg("foo(bar)");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_CurlyBraces_Throws()
        {
            MountService.SanitizeShellArg("foo{bar}");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_AngleBrackets_Throws()
        {
            MountService.SanitizeShellArg("foo<bar>");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_DoubleQuote_Throws()
        {
            MountService.SanitizeShellArg("foo\"bar");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_SingleQuote_Throws()
        {
            MountService.SanitizeShellArg("foo'bar");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_Newline_Throws()
        {
            MountService.SanitizeShellArg("foo\nbar");
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentException))]
        public void SanitizeShellArg_CarriageReturn_Throws()
        {
            MountService.SanitizeShellArg("foo\rbar");
        }
    }
}
