// src/app/App.xaml.cs
using System.Windows;

namespace golddrive
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            ReturnBox rb = null;
            new MainWindow(rb).ShowDialog();
            this.Shutdown();
        }
    }
}
