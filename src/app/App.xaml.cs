using System.Windows;

namespace golddrive
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            string apppath = System.Reflection.Assembly.GetEntryAssembly().Location;
            string appdir = System.IO.Path.GetDirectoryName(apppath);
            string args = "";
            bool ok = false;
            Drive drive = null;
            ReturnBox rb = null;
            MountService ms = new MountService();

            if (e.Args.Length > 0)
            {
                args = string.Join(" ", e.Args);
                Logger.Log($"Starting app: {apppath} {args}");
                // process args
                // golddrive.exe z: \\golddrive\host\path -ouid=-1,gid=-1
                drive = ms.GetDriveFromArgs(args);

                // check ssh auth
                rb = ms.TestSsh(drive);
                if (rb.MountStatus == MountStatus.OK)
                {
                    ok = true;
                }
                //else
                //{
                //    rb = ms.TestSsh(drive, drive.UserKey);
                //    if (rb.MountStatus == MountStatus.OK)
                //    {
                //        rb = ms.SetupSshWithUserKey(drive, drive.UserKey);
                //        ok = rb.MountStatus == MountStatus.OK;
                //    }
                //}
            }

            if (ok)
            {
                //string cmd = $"{appdir}\\golddrive.exe";
                //Logger.Log($"Starting cli: {cmd} {args}");
                //Process.Start(cmd, args);
                //Process.Start("net.exe", "use " + args);
                rb = ms.Connect(drive);
                if (rb.MountStatus != MountStatus.OK)
                {
                    drive.Status = rb.DriveStatus;
                    ok = false;
                }
            }

            if (!ok)
            {
                new MainWindow(rb, drive).ShowDialog();
            }

            this.Shutdown();
        }
    }
}
