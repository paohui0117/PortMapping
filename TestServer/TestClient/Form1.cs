using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TestClient
{
    public partial class Form1 : Form
    {
        private delegate void MyAddClient(ClientInfo info);

        private MyAddClient myAdd;
        private delegate void MyStopClient(ClientInfo info);

        private MyStopClient myStop;
        private ClientManager clientManager;
        private Timer UpdataTimer;
        public Form1()
        {
            InitializeComponent();
            clientManager = new ClientManager();
            clientManager.AddClientFun += AsyncAddClient;
            clientManager.StopClientFun += AsyncStopClient;

            myAdd += _MyAddClient;
            myStop += _MyStopClient;

            UpdataTimer = new Timer();
            UpdataTimer.Interval = 600;
            UpdataTimer.Tick += UpDataItem;
            UpdataTimer.Start();
        }

        private void UpDataItem(object sender, EventArgs e)
        {
            listView1.BeginUpdate();
            ListViewItem item;
            ClientInfo info;
            for (int i = 0; i < listView1.Items.Count; i++)
            {
                item = listView1.Items[i];
                if (item.Tag == null)
                    continue;
                info = item.Tag as ClientInfo;
                item.SubItems[4].Text = info.nsend.ToString();
                item.SubItems[5].Text = info.nrecv.ToString();
            }
            listView1.EndUpdate();
        }

        private void _MyStopClient(ClientInfo info)
        {
            if (info.item == null)
                return;
            info.item.Tag = null;
            info.item.SubItems[3].Text = "停止";
            info.item = null;
        }

        private void _MyAddClient(ClientInfo info)
        {
            ListViewItem item = new ListViewItem();
            item.Tag = info;
            info.item = item;
            IPEndPoint ipaddr = info.socket.Client.LocalEndPoint as IPEndPoint;
            item.Text = ipaddr.Port.ToString();
            ipaddr = info.socket.Client.RemoteEndPoint as IPEndPoint;
            item.SubItems.Add(ipaddr.Address.ToString());
            item.SubItems.Add(ipaddr.Port.ToString());
            item.SubItems.Add("开始");
            item.SubItems.Add("0");
            item.SubItems.Add("0");
            listView1.Items.Add(item);
        }

        private void AsyncStopClient(ClientInfo info)
        {
            BeginInvoke(myStop, info);
        }

        private void AsyncAddClient(ClientInfo info)
        {
            BeginInvoke(myAdd, info);
        }

        private void button1_Click(object sender, EventArgs e)
        {
            clientManager.StartClient(textBox1.Text, textBox2.Text, int.Parse(textBox3.Text));
        }
    }
}
