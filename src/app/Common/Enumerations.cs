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
        NOT_SUPPORTED,
        MOUNTPOINT_IN_USE,
        LETTER_IN_USE,
        BROKEN,
        DISCONNECTED,
        CONNECTED,
        UNKNOWN,
    }
    public enum MountStatus
    {
        OK,
        BAD_DRIVE,
        BAD_HOST,
        BAD_LOGIN,
        BAD_SSH,
        BAD_WINFSP,
        UNKNOWN,
    }
}
