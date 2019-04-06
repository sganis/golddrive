using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace golddrive
{
    public class ReturnBox
    {
        public ReturnBox()
        {
            MountStatus = MountStatus.UNKNOWN;
            DriveStatus = DriveStatus.UNKNOWN;
            Success = false;
            ExitCode = -999;
        }
        public string Error { get; set; }
        public string Output { get; set; }
        public int ExitCode { get; set; }
        public bool Success { get; set; }
        public object Object { get; set; }
        public MountStatus MountStatus { get; set; }
        public DriveStatus DriveStatus { get; set; }
    }
}
