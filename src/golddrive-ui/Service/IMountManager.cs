using System.Collections.Generic;
using System.Threading.Tasks;

namespace golddrive
{
    public interface IMountManager
    {
        bool Connected { get; }
        string Error { get; set; }        
        bool Connect(string host, int port, string user, string password, string pkey);
        ReturnBox DownloadFile(string src, string dst);
        List<Drive> GetGoldDrives();
        ReturnBox RunLocal(string cmd);
        ReturnBox RunLocal(string cmd, string args);
        ReturnBox RunRemote(string cmd, int timeout_secs = 3600);
        ReturnBox UploadFile(string src, string dir, string filename);
        Task<ReturnBox> ls();
    }
}