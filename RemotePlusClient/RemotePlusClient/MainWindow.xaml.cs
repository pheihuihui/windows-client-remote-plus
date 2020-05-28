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
using System.Windows.Interop;
using DShowLib;

namespace RemotePlusClient
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private unsafe void MenuItem_Click(object sender, RoutedEventArgs e)
        {
            Window window = GetWindow(this);
            var wih = new WindowInteropHelper(window);
            IntPtr hWnd = wih.Handle;
            //var vl = Class1.GetNum();
            //MessageBox.Show(hWnd.ToString());
            var ptr = hWnd.ToPointer();
            RefRemoteScreen.PlayAt(ptr);
        }
    }
}
