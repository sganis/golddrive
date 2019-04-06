using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace golddrive
{
    public enum Page
    {
        Main,
        Password,
        Host,
        Settings,
        About,
    }
    public enum DriveStatus
    {
        FREE,
        IN_USE,
        MOUNTPOINT_IN_USE,
        BROKEN,
        DISCONNECTED,
        CONNECTED,
        NOT_SUPPORTED,
        UNKNOWN,
    }
    public enum MountStatus
    {
        OK,
        BAD_DRIVE,
        BAD_HOST,
        BAD_KEY,
        BAD_PASSWORD,
        BAD_SSH,
        BAD_WINFSP,
        UNKNOWN,
    }
}
