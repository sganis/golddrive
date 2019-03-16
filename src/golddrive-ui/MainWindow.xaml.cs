using Renci.SshNet;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace golddrive_ui2
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        SshClient m_ssh;
        bool m_connected;

        public MainWindow()
        {
            InitializeComponent();            
        }
        private void Connect()
        {
            try
            {
                m_ssh = new SshClient("192.168.100.201", txtUser.Text, txtPassword.Password);
                m_ssh.Connect();
                m_connected = true;
            }
            catch (Renci.SshNet.Common.SshAuthenticationException ex)
            {
                Console.WriteLine(ex.Message);
            }
        }

        private void button_Click(object sender, RoutedEventArgs e)
        {
            Connect();
            if (!m_connected)
                return;
            SshCommand cmd = m_ssh.RunCommand("ls -l");    
            Console.WriteLine(cmd.Result);
        }
    }
}
