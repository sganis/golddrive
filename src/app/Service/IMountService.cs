using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace golddrive
{
    public interface IDriver
    {
        bool Connected { get; }
        string Error { get; set; }
        bool HasDrives { get; }
        ObservableCollection<Drive> Drives { get; set; }
        ObservableCollection<Drive> FreeDrives { get; set; }
        Drive SelectedDrive { get; set; }
        void LoadDrives();
        bool Connect(string host, int port, string user, string password, string pkey);
        ReturnBox DownloadFile(string src, string dst);
        //List<Drive> LoadDrives();
        List<Drive> GetUsedDrives();
        List<Drive> GetFreeDrives();
        ReturnBox RunLocal(string cmd);
        ReturnBox RunLocal(string cmd, string args);
        ReturnBox RunRemote(string cmd, int timeout_secs = 3600);
        ReturnBox UploadFile(string src, string dir, string filename);
        List<Drive> GetGoldDrives();
        string GetUid(string user);
        void SaveSettingsDrives(List<Drive> drives);
        ReturnBox Mount(Drive drive);
        ReturnBox Unmount(Drive drive);
        DriveStatus CheckDriveStatus(Drive drive);
        bool CheckIfDriveWorks(Drive drive);
        ReturnBox Connect(Drive drive);
    }
}