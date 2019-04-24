using NLog;
using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace golddrive
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            if ( e.Args.Length > 0 )
            {
                string apppath = System.Reflection.Assembly.GetEntryAssembly().Location;
                string appdir = System.IO.Path.GetDirectoryName(apppath);
                Console.WriteLine("Running console...");
                string cmd = $"{appdir}\\golddrive.exe";
                string args = string.Join(" ", e.Args);
                Console.WriteLine($"{cmd} {args}");
                Process.Start(cmd, args);
            }
            else
            {
                new MainWindow().ShowDialog();
            }
            this.Shutdown();
        }
    }
}
