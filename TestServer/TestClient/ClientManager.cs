using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TestClient
{
    public class ClientInfo
    {
        
        public TcpClient socket;
        public UInt64 nsend;
        public UInt64 nrecv;
        public ListViewItem item;
        public byte[] data;
        public ClientInfo()
        {
            nsend = nrecv = 0;
            data = new byte[65535];
        }
    }
    public  class ClientManager
    {
        public delegate void AddClient(ClientInfo info);
        public delegate void StopClient(ClientInfo info);
        private List<ClientInfo> m_allClient;
        public AddClient AddClientFun;
        public StopClient StopClientFun;
        public ClientManager()
        {
            m_allClient = new List<ClientInfo>();
        }

        public void StartClient(string IP, string Port, int size)
        {
            if (size < 1)
                return;
            int nPort;
            if (!Int32.TryParse(Port, out nPort))
                return;
            if (nPort < 0 || nPort > 65535)
                return;
            IPAddress ipAddress;
            
            try
            {
                ipAddress = IPAddress.Parse(IP);
                
                for (int i = 0; i < size; i++)
                {
                    ClientInfo info = new ClientInfo();
                    info.socket = new TcpClient();
                    m_allClient.Add(info);
                    info.socket.BeginConnect(ipAddress, nPort, ConnectCallBack, info);
                }
            }
            catch (Exception)
            {
                return;
            }
        }

        private void ConnectCallBack(IAsyncResult ar)
        {
            ClientInfo info = ar.AsyncState as ClientInfo;
            if (info == null)
                return;
            try
            {
                info.socket.EndConnect(ar);
                AddClientFun?.Invoke(info);
                int length = info.socket.Client.Send(Encoding.UTF8.GetBytes("client test message"));
                info.nsend += (UInt64)length;
                info.socket.Client.BeginReceive(info.data, 0, 65535, 0, ReadCallBack, info);
            }
            catch (Exception ex)
            {
                return;
            }
            
        }

        private void ReadCallBack(IAsyncResult ar)
        {
            ClientInfo info = ar.AsyncState as ClientInfo;
            if (info == null)
                return;
            try
            {
                int nlength = info.socket.Client.EndReceive(ar);
                if (nlength < 1)
                {
                    StopClientFun?.Invoke(info);
                }
                info.nrecv += (UInt64)nlength;
                nlength = info.socket.Client.Send(Encoding.UTF8.GetBytes("client test message"));
                info.nsend += (UInt64)nlength;

                info.socket.Client.BeginReceive(info.data, 0, 65535, 0, ReadCallBack, info);
            }
            catch (Exception ex)
            {
                StopClientFun?.Invoke(info);
                return;
            }
            
        }
    }
}
