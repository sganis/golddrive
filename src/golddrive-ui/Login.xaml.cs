using System.IO;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace golddrive_ui
{
    /// <summary>
    /// Interaction logic for Login.xaml
    /// </summary>
    public partial class Login : UserControl
    {
        public Login()
        {
            InitializeComponent();
            //txtStatus.Text = "";
            //progressBar.Visibility = Visibility.Hidden;
        }

        private async void btnLogin_Click(object sender, RoutedEventArgs e)
        {
            btnLogin.IsEnabled = false;
            //txtStatus.Text = "Connecting...";
            //progressBar.Visibility = Visibility.Visible;

            string user = "sant";
            string server = "";// txtRemote.Text;
            string password = "";// txtPassword.Password;
            int port = 22;
            string pkey = System.Environment.ExpandEnvironmentVariables(
                $@"%USERPROFILE%\.ssh\id_rsa-{user}-golddrive");
            if (!File.Exists(pkey))
                pkey = "";
            var r = await Task.Run(() => {
                ReturnBox rb = new ReturnBox();
                var ok = App.Controller.Connect(server, port, user, password, pkey );
                if(!ok)
                {
                    rb.Error = App.Controller.Error;                    
                }
                rb.Success = string.IsNullOrEmpty(rb.Error);
                return rb;
            });

            if(!r.Success)
            {
                //txtStatus.Text = r.Error;
            }
            else
            {
               // txtStatus.Text = "Done";                
                await Task.Delay(1000);
                App.Controller.MainWindow.IsLoginOpen = false;
            }
            //progressBar.Visibility = Visibility.Hidden;
            btnLogin.IsEnabled = true;

        }
    }
}
