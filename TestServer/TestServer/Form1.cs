using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TestServer
{
    public partial class Form1 : Form
    {
        private delegate void _AddConnect(ConnectInfo a);

        private _AddConnect myAddConnect;
        private delegate void _StopConnect(ConnectInfo a);
        private _StopConnect myStopConnect;
        private ServerManager SerManager;
        private Timer updataTimer;
        public Form1()
        {
            InitializeComponent();
            SerManager = new ServerManager();
            SerManager.AddConnectEvent += AddConnect;
            SerManager.StopConnectEvent += StopConnect;
            myAddConnect = new _AddConnect(_MyAddConnect);
            myStopConnect = new _StopConnect(_MyStopConnect);

            string hostName = Dns.GetHostName();//本机名  
            System.Net.IPAddress[] addressList = Dns.GetHostAddresses(hostName);//会返回所有地址，包括IPv4和IPv6   
            foreach (IPAddress ip in addressList)
            {
                if (ip.AddressFamily == AddressFamily.InterNetwork)
                    combIP.Items.Add(ip.ToString());
            }
            updataTimer = new Timer();
            updataTimer.Interval = 600;
            updataTimer.Tick += EventTimer;
            updataTimer.Start();
        }

        void EventTimer(object sender, EventArgs e)
        {
            listView1.BeginUpdate();
            for (int i = 0; i < listView1.Items.Count; i++)
            {
                if (listView1.Items[i].Tag == null)
                    continue;
                UpdataServerInfo(listView1.Items[i].Tag as ServerInfo);
            }
            listView1.EndUpdate();

            listView2.BeginUpdate();
            for (int i = 0; i < listView2.Items.Count; i++)
            {
                if (listView2.Items[i].Tag == null)
                    continue;
                UpdataConnectInfo(listView2.Items[i].Tag as ConnectInfo);
            }
            listView2.EndUpdate();
        }

        void AddConnect(ConnectInfo info)
        {
            BeginInvoke(myAddConnect, info);
        }

        void _MyAddConnect(ConnectInfo info)
        {
            ListViewItem item = new ListViewItem();
            listView2.Items.Add(item);
            info.item = item;
            item.Tag = info;
            IPEndPoint ipaddr = info.LocalClient.Client.LocalEndPoint as IPEndPoint;
            item.Text = ipaddr.Port.ToString();
            ipaddr = info.LocalClient.Client.RemoteEndPoint as IPEndPoint;
       
            item.SubItems.Add(ipaddr.Address.ToString());
            item.SubItems.Add(ipaddr.Port.ToString());
            item.SubItems.Add("运行");
            
            item.SubItems.Add(info.nrecv.ToString());
            item.SubItems.Add(info.nsent.ToString());
        }
        void StopConnect(ConnectInfo info)
        {
            BeginInvoke(myStopConnect, info);
        }
        void _MyStopConnect(ConnectInfo info)
        {
            info.item.Tag = null;
            info.item.SubItems[2].Text = "断开";
            info.item = null;
        }
        private void button1_Click(object sender, EventArgs e)
        {
            ServerInfo sinfo = SerManager.AddServer(combIP.Text, textBox1.Text);
            if (sinfo == null)
                return;
            ListViewItem item = new ListViewItem();
            listView1.Items.Add(item);
            sinfo.item = item;
            item.Text = sinfo.addr.Address.ToString();
            item.Tag = sinfo;
            //item.SubItems.Add(sinfo.addr.Address.ToString());
            item.SubItems.Add(sinfo.addr.Port.ToString());
            item.SubItems.Add("开始");
            item.SubItems.Add("");
            item.SubItems.Add("");
            item.SubItems.Add("");
            SerManager.StartServer(sinfo.addr.Port);
            UpdataServerInfo(sinfo);
        }

        private void UpdataServerInfo(ServerInfo sinfo)
        {
            if (sinfo.item == null)
                return;
            sinfo.item.SubItems[3].Text = sinfo.nsize.ToString();
            sinfo.item.SubItems[4].Text = sinfo.nrecv.ToString();
            sinfo.item.SubItems[5].Text = sinfo.nsent.ToString();
        }
        private void UpdataConnectInfo(ConnectInfo sinfo)
        {
            if (sinfo.item == null)
                return;
            sinfo.item.SubItems[4].Text = sinfo.nrecv.ToString();
            sinfo.item.SubItems[5].Text = sinfo.nsent.ToString();
        }
    }
}
