﻿using System.Windows;

namespace golddrive
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            DataContext = new MainWindowViewModel();            
        }
    }
}