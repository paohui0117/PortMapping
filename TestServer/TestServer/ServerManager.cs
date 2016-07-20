using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TestServer
{
    public class ServerInfo
    {
        public TcpListener listener;
        public IPEndPoint addr;
        public int nstatue;
        public UInt64 nrecv;
        public UInt64 nsent;
        public ListViewItem item;
        public int nsize;
        public ServerInfo()
        {
            nsize = nstatue = 0;
            nrecv = nsent = 0;
        }
    }

    public class ConnectInfo
    {
        public TcpClient LocalClient;
        public UInt64 nrecv;
        public UInt64 nsent;
        public ServerInfo server;
        public byte[] data ;
        public ListViewItem item;
        public ConnectInfo()
        {
            nrecv = nsent = 0;
            data = new byte[65535];
        }
    }
    public class ServerManager
    {
        public delegate void AddConnect(ConnectInfo info);
        public delegate void StopConnect(ConnectInfo info);

        public event AddConnect AddConnectEvent;
        public event StopConnect StopConnectEvent;
        private Dictionary<int, ServerInfo> m_AllServer;
        private Dictionary<int, List<ConnectInfo>> m_AllConnect;

        public ServerManager()
        {
            m_AllServer = new Dictionary<int, ServerInfo>();
            m_AllConnect = new Dictionary<int, List<ConnectInfo>>();
        }

        public ServerInfo AddServer(string IP, string Port)
        {
            int nPort;
            if (!Int32.TryParse(Port, out nPort))
                return null;
            if (m_AllServer.ContainsKey(nPort))
                return null;
            if (nPort < 0 || nPort > 65535)
                return null;
            IPAddress ipAddress;
            ServerInfo info = new ServerInfo();
            try
            {
                ipAddress = IPAddress.Parse(IP);
                
                info.addr = new IPEndPoint(ipAddress, nPort);
                info.listener = new TcpListener(info.addr);
                m_AllServer.Add(nPort, info);
            }
            catch (Exception)
            {
                return null;
            }
            
            return info;
        }

        void AcceptCallback(IAsyncResult ar)
        {
            try
            {
                ServerInfo curInfo = ar.AsyncState as ServerInfo;
                if (curInfo == null)
                    return;
                ++curInfo.nsize;
                ConnectInfo connectinfo = new ConnectInfo();
                connectinfo.LocalClient = curInfo.listener.EndAcceptTcpClient(ar);
                connectinfo.server = curInfo;
                int nPort = (connectinfo.LocalClient.Client.LocalEndPoint as IPEndPoint).Port;
                if (m_AllConnect.ContainsKey(nPort))
                {
                    m_AllConnect[nPort].Add(connectinfo);
                }
                else
                {
                    m_AllConnect.Add(nPort, new List<ConnectInfo>());
                    m_AllConnect[nPort].Add(connectinfo);
                }
                AddConnectEvent?.Invoke(connectinfo);
                connectinfo.LocalClient.Client.BeginReceive(connectinfo.data, 0, 65535, SocketFlags.None, RecevieCallBack, connectinfo);
                curInfo.listener.BeginAcceptTcpClient(AcceptCallback, curInfo);
            }
            catch (Exception ex)
            {
                return;
            }
        }

        void RecevieCallBack(IAsyncResult ar)
        {
            ConnectInfo connectinfo = ar.AsyncState as ConnectInfo;
            try
            {
                if (connectinfo == null)
                    return;
                int nlength = connectinfo.LocalClient.Client.EndReceive(ar);
                if (nlength <= 0)
                {
                    int nPort = (connectinfo.LocalClient.Client.LocalEndPoint as IPEndPoint).Port;
                    StopConnectEvent?.Invoke(connectinfo);
                    if (m_AllConnect.ContainsKey(nPort))
                    {
                        m_AllConnect[nPort].Remove(connectinfo);
                    }
                    return;
                }
                connectinfo.nrecv += (ulong)nlength;
                connectinfo.server.nrecv += (ulong)nlength;
                nlength = connectinfo.LocalClient.Client.Send(Encoding.UTF8.GetBytes("test message"));
                connectinfo.nsent += (ulong)nlength;
                connectinfo.server.nsent += (ulong)nlength;
                Thread.Sleep(600);
                connectinfo.LocalClient.Client.BeginReceive(connectinfo.data, 0, 65535, SocketFlags.None, RecevieCallBack, connectinfo);
            }
            catch (Exception ex)
            {
                StopConnectEvent?.Invoke(connectinfo);
            }
            
        }
        public bool StartServer(int nPort)
        {
            if (!m_AllServer.ContainsKey(nPort))
                return false;
            ServerInfo curInfo = m_AllServer[nPort];
            if (curInfo.nstatue == 1)
                return false;
            try
            {
                curInfo.listener.Start();
                curInfo.listener.BeginAcceptTcpClient(AcceptCallback, curInfo);
                curInfo.nstatue = 1;
            }
            catch (Exception ex)
            {
                
                return false;
            }
           
            return true;
        }

        public bool StopServer(int nPort)
        {
            if (!m_AllServer.ContainsKey(nPort))
                return false;
            ServerInfo curInfo = m_AllServer[nPort];
            if (curInfo.nstatue != 1)
                return false;
            curInfo.listener.Stop();
            curInfo.listener.Server.Close();
            if (m_AllConnect.ContainsKey(nPort))
            {
                //停止所有链接
                foreach (var connectinfo in m_AllConnect[nPort])
                {
                    connectinfo.LocalClient.Client.Close();
                    StopConnectEvent?.Invoke(connectinfo);
                }
                m_AllConnect[nPort].Clear();
            }
            curInfo.nstatue = 0;
            return true;
        }
    }
}
