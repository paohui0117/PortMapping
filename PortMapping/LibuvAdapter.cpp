#include "stdafx.h"
#include "LibuvAdapter.h"
#include "../libuv-dll/src/queue.h"

class IOCallBack
{
public:
	//所有的回调函数

	//空回调,用来保持循环存活
	static void null_cb(uv_check_t* handle)
	{}
	
	//线程回调函数
	static void LoopThread(void *arg)
	{
		CLibuvAdapter* pThis = (CLibuvAdapter*)arg;
		if (!pThis)
			return;
		uv_run(pThis->m_pLoop, UV_RUN_DEFAULT);//运行循环
	}

	//回调函数，异步开启TCP映射
	static void AnsycStartTCPMapping(uv__work* w, int status)
	{
		if (!w)
			return;
		MappingInfo* mapping_info = (MappingInfo*)(*(int*)((PCHAR)w + sizeof(uv__work)));
		if (!mapping_info)
		{
			free(w);
			return;
		}
		//初始化tcp
		if (uv_tcp_init_ex(mapping_info->pLoop, &mapping_info->u.listen_tcp, AF_INET) != 0)
		{
			mapping_info->nState = MAPPING_FAIL | INIT_FAIL;
			free(w);
			return;
		}
		//绑定
		if (uv_tcp_bind(&mapping_info->u.listen_tcp, (sockaddr*)&mapping_info->Addr_agent, AF_INET) != 0)
		{
			mapping_info->nState = MAPPING_FAIL | BIND_FAIL;
			free(w);
			return;
		}
		//监听
		if (uv_listen((uv_stream_t*)&mapping_info->u.listen_tcp, 10, tcp_listen_connection_cb) != 0)
		{
			mapping_info->nState = MAPPING_FAIL | LISTEN_FAIL;
			free(w);
			return;
		}
	}
	
	//回调函数，异步开启UDP映射
	static void AnsycStartUDPMapping(uv__work* w, int status)
	{
		if (!w)
			return;
		MappingInfo* mapping_info = (MappingInfo*)(*(int*)((PCHAR)w + sizeof(uv__work)));
		if (!mapping_info)
		{
			free(w);
			return;
		}
		//初始化udp
		if (uv_udp_init_ex(mapping_info->pLoop, &mapping_info->u.listen_udp, AF_INET) != 0)
		{
			mapping_info->nState = MAPPING_FAIL | INIT_FAIL;
			free(w);
			return;
		}
		//绑定
		if (uv_udp_bind(&mapping_info->u.listen_udp, (sockaddr*)&mapping_info->Addr_agent, AF_INET) != 0)
		{
			mapping_info->nState = MAPPING_FAIL | BIND_FAIL;
			free(w);
			return;
		}
		//开始接收
		uv_udp_recv_start(&mapping_info->u.listen_udp, alloc_cb, udp_listen_recv_cb);
		mapping_info->nState = MAPPING_START;
	}
	
	//回调函数，异步关闭映射
	static void AnsycStopMapping(uv__work* w, int status)
	{
		if (!w)
			return;
		MappingInfo* mapping_info = (MappingInfo*)(*(int*)((PCHAR)w + sizeof(uv__work)));
		if (!mapping_info)
		{
			free(w);
			return;
		}
		//停止监听
		uv_close((uv_handle_t*)&mapping_info->u.listen_tcp, tcp_listen_close_cb);
	}

	//异步关闭一条连接
	static void AnsycRemoveConnect(uv__work* w, int status)
	{
		if (!w)
			return;
		ConnectInfo* pConInfo = (ConnectInfo*)(*(int*)((PCHAR)w + sizeof(uv__work)));
		if (!pConInfo)
			return;
		//关闭本tcp handle
		pConInfo->u.tcp.server_tcp.data = pConInfo;
		uv_close((uv_handle_t*)&pConInfo->u.tcp.server_tcp, tcp_connect_close_cb);
		//断开与客户端的链接
		pConInfo->u.tcp.client_tcp.data = pConInfo;
		uv_close((uv_handle_t*)&pConInfo->u.tcp.client_tcp, tcp_connect_close_cb);
	}

	//libuv相关  TCP回调函数
	//内存申请回调函数
	static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		buf->base = (char*)malloc(suggested_size);
		buf->len = suggested_size;
	}

	//监听接受到链接的回调
	static void tcp_listen_connection_cb(uv_stream_t* server, int status)
	{
		if (!server)//error
			return;
		if (status < 0)//监听不会在继续下去了
		{
			//改变状态
			MappingInfo *pMapping = CONTAINING_RECORD(server, MappingInfo, u.listen_tcp);
			pMapping->nState = MAPPING_FAIL | LISTEN_FAIL;
			return;
		}
		//新建一个链接
		ConnectInfo* pConInfo = new ConnectInfo;
		memset(pConInfo, 0, sizeof ConnectInfo);
		pConInfo->bInMap = false;
		//通过成员指针的地址获取结构体本身的地址
		pConInfo->pMapping = CONTAINING_RECORD(server, MappingInfo, u.listen_tcp);
		uv_tcp_init_ex(pConInfo->pMapping->pLoop, &pConInfo->u.tcp.client_tcp, AF_INET);
		if (uv_accept(server, (uv_stream_t*)&pConInfo->u.tcp.client_tcp) != 0)
		{
			delete pConInfo;
			return;
		}
		//开始链接服务端
		uv_connect_t* pConnectReq = new uv_connect_t;
		pConnectReq->data = pConInfo;
		uv_tcp_init_ex(pConInfo->pMapping->pLoop, &pConInfo->u.tcp.server_tcp, AF_INET);
		if (uv_tcp_connect(pConnectReq, &pConInfo->u.tcp.server_tcp, (sockaddr*)&pConInfo->pMapping->Addr_server, tcp_connect_cb) != 0)
		{
			delete pConInfo;
			return;
		}
		//根据socket获取客户端的地址地址
		int nLength = sizeof(sockaddr);
		getsockname(pConInfo->u.tcp.client_tcp.socket, (sockaddr*)&pConInfo->Addr_Client, &nLength);
	}

	//主动链接回调函数
	static void tcp_connect_cb(uv_connect_t* req, int status)
	{
		if (!req || !req->data)
			return;
		ConnectInfo* pConInfo = (ConnectInfo*)req->data;
		if (status != 0)
		{
			//出错了
			//关闭本tcp handle
			pConInfo->u.tcp.server_tcp.data = pConInfo;
			uv_close((uv_handle_t*)&pConInfo->u.tcp.server_tcp, tcp_connect_close_cb);
			//断开与客户端的链接
			pConInfo->u.tcp.client_tcp.data = pConInfo;
			uv_close((uv_handle_t*)&pConInfo->u.tcp.client_tcp, tcp_connect_close_cb);
		}
		//成功了，将记录添加到对应的容器
		CLibuvAdapter* pThis = (CLibuvAdapter*)pConInfo->pMapping->pLoop->data;
		pThis->AddTCPConnect(pConInfo);
		//开始读取数据
		pConInfo->u.tcp.server_tcp.data = pConInfo;
		uv_read_start((uv_stream_t*)&pConInfo->u.tcp.server_tcp, alloc_cb, tcp_read_cb);
		pConInfo->u.tcp.client_tcp.data = pConInfo;
		uv_read_start((uv_stream_t*)&pConInfo->u.tcp.client_tcp, alloc_cb, tcp_read_cb);
	}

	//关闭监听的回调函数
	static void tcp_listen_close_cb(uv_handle_t* handle)
	{
		MappingInfo* pMappingInfo = CONTAINING_RECORD(handle, MappingInfo, u.listen_tcp);
		//关闭所有该端口上的连接
		CLibuvAdapter* pThis = (CLibuvAdapter*)pMappingInfo->pLoop->data;
		pThis->RemoveAllConnect(pMappingInfo);
		//改变状态
		pMappingInfo->nState = MAPPING_STOP;
	}

	//连接的关闭回调，
	static void tcp_connect_close_cb(uv_handle_t* handle)
	{
		ConnectInfo* pConInfo = (ConnectInfo*)handle->data;
		handle->data = nullptr;//处理一个就设置为空
		if (!pConInfo)
			return;
		//对于tcp，要处理两个连接
		if (pConInfo->u.tcp.client_tcp.data || pConInfo->u.tcp.server_tcp.data)
			return;
		if (pConInfo->bInMap)
		{
			//已经添加到记录中了，要删除记录
			CLibuvAdapter* pThis = (CLibuvAdapter*)pConInfo->pMapping->pLoop->data;
			pThis->RemoveConnect(pConInfo, false);
		}
		delete pConInfo;
	}

	//tcp读取数据的回调
	static void tcp_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
	{
		ConnectInfo* pConInfo = (ConnectInfo*)(stream->data);
		if (nread < 0)
		{
			//断开了，这条连接需要删除
			uv_close((uv_handle_t*)&pConInfo->u.tcp.client_tcp, tcp_connect_close_cb);
			uv_close((uv_handle_t*)&pConInfo->u.tcp.server_tcp, tcp_connect_close_cb);
			return;
		}
		//正常读取
		if (nread > 0)
		{
			if (stream == (uv_stream_t*)&pConInfo->u.tcp.client_tcp)
			{
				//client发过来的数据,先统计
				pConInfo->nCurFromClientB += nread;
				pConInfo->pMapping->nTotalFromClientB += nread;
				pConInfo->nCurFromClientM += pConInfo->nCurFromClientB >> 10;
				pConInfo->pMapping->nTotalFromClientM += pConInfo->nCurFromClientB >> 10;
				pConInfo->nCurFromClientB &= 0x3ff;
				pConInfo->pMapping->nTotalFromClientB &= 0x3ff;
				//通过serversocket发送出去
				uv_write_t* pWrite = new uv_write_t;
				uv_write(pWrite, (uv_stream_t*)&pConInfo->u.tcp.server_tcp, buf, 1, tcp_write_cb);
			}
			else if (stream == (uv_stream_t*)&pConInfo->u.tcp.server_tcp)
			{
				//server发过来的数据
				pConInfo->nCurFromServerB += nread;
				pConInfo->pMapping->nTotalFromServerB += nread;
				pConInfo->nCurFromServerM += pConInfo->nCurFromClientB >> 10;
				pConInfo->pMapping->nTotalFromServerM += pConInfo->nCurFromClientB >> 10;
				pConInfo->nCurFromServerB &= 0x3ff;
				pConInfo->pMapping->nTotalFromServerB &= 0x3ff;
				//通过serversocket发送出去
				uv_write_t* pWrite = new uv_write_t;
				uv_write(pWrite, (uv_stream_t*)&pConInfo->u.tcp.client_tcp, buf, 1, tcp_write_cb);
			}
		}
		//释放
		free(buf->base);
	}

	//写回调
	static void tcp_write_cb(uv_write_t* req, int status)
	{
		if (req)
			delete req;
		if (status != 0)
		{

		}
	}

	//udp
	//dup listen的读回调
	static void udp_listen_recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf,
		const struct sockaddr* addr, unsigned flags)
	{
		MappingInfo* mapping_info = CONTAINING_RECORD(handle, MappingInfo, u.listen_udp);
		if (nread < 0)//没有再继续接收了
		{
			mapping_info->nState = MAPPING_FAIL | LISTEN_FAIL;
			return;
		}
		//根据addr判断是否已记录了该链接
		ConnectInfo* pConnect = ((CLibuvAdapter*)mapping_info->pLoop->data)->GetUDPConnect(mapping_info, (sockaddr_in*)addr);
		if (!pConnect)
			return;
		//将受到的数据发送给server
		uv_udp_send_t* pSend = new uv_udp_send_t;
		uv_udp_send(pSend, &pConnect->u.server_udp, buf, 1, (sockaddr*)&mapping_info->Addr_server, udp_send_cb);
		//更新数据
		pConnect->nCurFromClientB += nread;
		pConnect->pMapping->nTotalFromClientB += nread;
		pConnect->nCurFromClientM += pConnect->nCurFromClientB >> 10;
		pConnect->pMapping->nTotalFromClientM += pConnect->nCurFromClientB >> 10;
		pConnect->nCurFromClientB &= 0x3ff;
		pConnect->pMapping->nTotalFromClientB &= 0x3ff;
	}
	//dup server的读回调
	static void udp_server_recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf,
		const struct sockaddr* addr, unsigned flags)
	{
		ConnectInfo* pConnect = CONTAINING_RECORD(handle, ConnectInfo, u.server_udp);
		if (nread < 0)//没有再继续接收了
		{
			uv_close((uv_handle_t*)&pConnect->u.server_udp, udp_close_cb);
			return;
		}
		//将受到的数据发送给client
		uv_udp_send_t* pSend = new uv_udp_send_t;
		uv_udp_send(pSend, &pConnect->u.server_udp, buf, 1, (sockaddr*)&pConnect->Addr_Client, udp_send_cb);
		//更新数据
		pConnect->nCurFromServerB += nread;
		pConnect->pMapping->nTotalFromServerB += nread;
		pConnect->nCurFromServerM += pConnect->nCurFromServerB >> 10;
		pConnect->pMapping->nTotalFromServerM += pConnect->nCurFromServerB >> 10;
		pConnect->nCurFromServerB &= 0x3ff;
		pConnect->pMapping->nTotalFromServerB &= 0x3ff;
	}

	static void udp_close_cb(uv_handle_t* handle)
	{
		ConnectInfo* mapping_info = CONTAINING_RECORD(handle, ConnectInfo, u.server_udp);
		if (mapping_info->bInMap)
		{
			//已经添加到记录中了，要删除记录
			CLibuvAdapter* pThis = (CLibuvAdapter*)mapping_info->pMapping->pLoop->data;
			pThis->RemoveConnect(mapping_info, false);
		}
		delete mapping_info;
	}

	static void udp_send_cb(uv_udp_send_t* req, int status)
	{
		if (req)
			delete req;
	}
};
//common fun
wstring a2w(const char* str)//内存需要自己释放
{
	if (!str)
		return L"";
	int wlength = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
	if (wlength < 1)
		return L"";
	wchar_t* pw = new wchar_t[wlength + 1];
	memset(pw, 0, sizeof(wchar_t) * (wlength + 1));
	MultiByteToWideChar(CP_ACP, 0, str, -1, pw, wlength);
	wstring wstr(pw);
	delete[] pw;
	return wstr;
}

string w2a(LPCWSTR* str)
{
	if (!str)
		return "";
	int alength = WideCharToMultiByte(CP_ACP, 0, str, -1, nullptr, 0, nullptr, nullptr);
	if (alength < 1)
		return "";
	char* pa = new char[alength + 1];
	memset(pa, 0, sizeof(char) * (alength + 1));
	WideCharToMultiByte(CP_ACP, 0, str, -1, pa, alength, nullptr, nullptr);
	string astr(pa);
	delete[] pa;
	return astr;
}


CLibuvAdapter::CLibuvAdapter() : m_pLoop(nullptr), m_Loop_thread(nullptr)
{
	WSADATA wsa_data;
	int errorno = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (errorno != 0) {

	}
}

CLibuvAdapter::~CLibuvAdapter()
{
	WSACleanup();
}

const MappingInfo* CLibuvAdapter::AddMapping(LPCWSTR strAgentIP, LPCWSTR strAgentPort, LPCWSTR strServerIP, LPCWSTR strServerPort, bool bTcp, int& err)
{
	err = 0;
	USHORT nAgentPort = _wtoi(strAgentPort);
	if (m_mapMapping.find(nAgentPort) != m_mapMapping.end())
	{
		err = 1;//映射已存在
		return nullptr;
	}
	MappingInfo& curInfp = m_mapMapping[nAgentPort];//添加映射
	memset(&curInfp, 0, sizeof MappingInfo);//初始化映射
	curInfp.pLoop = m_pLoop;
	curInfp.bTCP = bTcp;
	string strIP = w2a(strAgentIP);
	uv_ip4_addr(strIP.c_str(), nAgentPort, &curInfp.Addr_agent);

	strIP = w2a(strServerIP);
	USHORT nServerPort = _wtoi(strServerPort);
	uv_ip4_addr(strIP.c_str(), nServerPort, &curInfp.Addr_server);
	if (curInfp.Addr_agent.sin_addr.S_un.S_addr == curInfp.Addr_server.sin_addr.S_un.S_addr && 
		curInfp.Addr_agent.sin_port == curInfp.Addr_server.sin_port)
	{
		err = 2;//映射对象与本地对象相同
		m_mapMapping.erase(nAgentPort);
		return nullptr;
	}
	return &curInfp;
}

bool CLibuvAdapter::StartMapping(MappingInfo* pMapping)
{
	if (!m_pLoop && !InitLoop())
		return false;
	if (!pMapping || pMapping->nState == MAPPING_START)
		return false;
	//工作线程已经开启，接下来的操作都是异步的
	if (pMapping->bTCP)
		AsyncOperate(pMapping, IOCallBack::AnsycStartTCPMapping);
	else
		AsyncOperate(pMapping, IOCallBack::AnsycStartUDPMapping);
	return true;
}

bool CLibuvAdapter::StopMapping(MappingInfo* pMapping)
{
	if (!m_pLoop && !InitLoop())
		return false;
	if (!pMapping || pMapping->nState != MAPPING_START)
		return false;
	//工作线程已经开启，接下来的操作都是异步的
	AsyncOperate(pMapping, IOCallBack::AnsycStopMapping);
	return true;
}

//初始化loop
bool CLibuvAdapter::InitLoop()
{
	m_pLoop = uv_default_loop();
	m_pLoop->data = this;//将loop与本类联系起来
	if (!m_pLoop)
		return false;
	//单纯的保持事件循环存活，一个空的回调
	uv_check_init(m_pLoop, &m_check_keeprun);
	uv_check_start(&m_check_keeprun, IOCallBack::null_cb);
	int ret = uv_thread_create(&m_Loop_thread, IOCallBack::LoopThread, this);//开启工作线程
	return ret == 0;
}
//注册一个任务到loop，本任务将会在loop所在的线程中执行
void CLibuvAdapter::RegisterAnsycWork(uv__work * pwork, AsyncWork done)
{
	if (!pwork || !m_pLoop)
		return;
	pwork->done = done;
	pwork->loop = m_pLoop;
	uv_mutex_lock(&m_pLoop->wq_mutex);
	pwork->work = nullptr;
	QUEUE_INSERT_TAIL(&m_pLoop->wq, &pwork->wq);
	uv_async_send(&m_pLoop->wq_async);
	uv_mutex_unlock(&m_pLoop->wq_mutex);
}
//移除一条连接，有两种情
//1.用户主动移除链接
//2.由于client或者server端断开，loop线程删除一条连接
bool CLibuvAdapter::RemoveConnect(ConnectInfo* connect_info, bool bAsync)
{
	if (!connect_info)
		return false;
	if (bAsync)//异步删除链接，也就是loop线程之外的删除操作，对应情况一
	{
		if (!m_pLoop && !InitLoop())
			return false;
		//工作线程已经开启，接下来的操作都是异步的
		AsyncOperate(connect_info, IOCallBack::AnsycRemoveConnect);
		return true;
	}
	else//内部自己删除的，也就是loop线程自己删除的，直接删除，对应情况二
	{
		USHORT nPort = htons(connect_info->pMapping->Addr_agent.sin_port);
		map<USHORT, map<Connectkey, ConnectInfo*>>::iterator itPort = m_mapConnect.find(nPort);
		if (itPort == m_mapConnect.end())//没有记录
		{
			delete connect_info;
			connect_info = nullptr;
			return true;
		}
		//继续查找
		Connectkey curKey = { connect_info->Addr_Client.sin_addr.S_un.S_addr, connect_info->Addr_Client.sin_port };
		map<Connectkey, ConnectInfo*>::iterator itAddr = itPort->second.find(curKey);
		if (itAddr == itPort->second.end())
		{
			delete connect_info;
			connect_info = nullptr;
			return true;
		}
		//有记录，删除记录
		//先通知外接
		for (set<INotifyLoop*>::iterator it = m_setNotify.begin(); it != m_setNotify.end(); it++)
		{
			(*it)->NotifyConnectMessage(DELETE_CONNECT, connect_info);
		}
		itPort->second.erase(itAddr);
		delete connect_info;
		return true;
	}
}

void CLibuvAdapter::AsyncOperate(void* p, AsyncWork workfun)
{
	//偷懒的写法
	uv__work* pworkEx = (uv__work*)malloc(sizeof(uv__work) + sizeof(int));
	*(int*)((PCHAR)pworkEx + sizeof(uv__work)) = (int)p;

	RegisterAnsycWork(pworkEx, workfun);
}



bool CLibuvAdapter::GetLocalIP(vector<wstring>& vecIP)
{
	vecIP.clear();
	char hostname[NI_MAXHOST] = { 0 };
	if (gethostname(hostname, NI_MAXHOST - 1) != 0)
		return false;
	struct addrinfo * result;
	int error = getaddrinfo(hostname, NULL, NULL, &result);
	if (0 != error)
	{
		return false;
	}
	addrinfo* cur = nullptr;
	SOCKADDR_IN* curAddr = nullptr;
	char ip[32] = { 0 };
	for (cur = result; cur != NULL; cur = cur->ai_next)
	{
		curAddr = (SOCKADDR_IN*)(cur->ai_addr);
		if (curAddr->sin_family != AF_INET)
			continue;
		if (uv_ip4_name(curAddr, ip, 32) != 0)
			continue;
		vecIP.push_back(a2w(ip));
	}
	freeaddrinfo(result);
	return true;
}

bool CLibuvAdapter::AddNotify(INotifyLoop* p)
{
	if (!p)
		return false;
	pair<set<INotifyLoop*>::iterator, bool> ret = m_setNotify.insert(p);
	return ret.second;
}

bool CLibuvAdapter::RemoveNotify(INotifyLoop* p)
{
	m_setNotify.erase(p);
	return true;
}

void CLibuvAdapter::RemoveAllConnect(MappingInfo* pMappingInfo)
{
	USHORT nPort = htons(pMappingInfo->Addr_agent.sin_port);
	map<USHORT, map<Connectkey, ConnectInfo*>>::iterator itPort = m_mapConnect.find(nPort);
	if (itPort == m_mapConnect.end())//没有记录
	{
		return;
	}
	map<Connectkey, ConnectInfo*>::iterator itAddr = itPort->second.begin();
	if (pMappingInfo->bTCP)
	{
		for (; itAddr != itPort->second.end(); itAddr++)
		{
			ConnectInfo* pCurInfo = itAddr->second;
			pCurInfo->u.tcp.client_tcp.data = pCurInfo;
			pCurInfo->bInMap = false;		//不需要再一个一个的删除了，直接清空容器就可以了
			uv_close((uv_handle_t*)&pCurInfo->u.tcp.client_tcp, IOCallBack::tcp_connect_close_cb);
			pCurInfo->u.tcp.server_tcp.data = pCurInfo;
			uv_close((uv_handle_t*)&pCurInfo->u.tcp.server_tcp, IOCallBack::tcp_connect_close_cb);
		}
	}
	else
	{
		for (; itAddr != itPort->second.end(); itAddr++)
		{
			ConnectInfo* pCurInfo = itAddr->second;
			pCurInfo->u.tcp.client_tcp.data = pCurInfo;
			pCurInfo->bInMap = false;		//不需要再一个一个的删除了，直接清空容器就可以了
			uv_udp_recv_stop(&pCurInfo->u.server_udp);
			
		}
	}
	//清空记录
	itPort->second.clear();
}

ConnectInfo* CLibuvAdapter::GetUDPConnect(MappingInfo* mapping_info, const sockaddr_in* addr)
{
	if (!mapping_info)
		return nullptr;
	ConnectInfo* pinfo = nullptr;
	USHORT nport = htons(mapping_info->Addr_agent.sin_port);
	Connectkey curKey = { addr->sin_addr.S_un.S_addr, addr->sin_port };
	map<USHORT, map<Connectkey, ConnectInfo*>>::iterator itorPort = m_mapConnect.find(nport);
	if (itorPort == m_mapConnect.end())//没有记录
	{
		//创建记录
		auto ret = m_mapConnect.insert(pair<USHORT, map<Connectkey, ConnectInfo*>>(nport, map<Connectkey, ConnectInfo*>()));
		itorPort = ret.first;
		pinfo = new ConnectInfo;
	}
	else
	{
		
		map<Connectkey, ConnectInfo*>::iterator itAddr = itorPort->second.find(curKey);
		if (itAddr == itorPort->second.end())
			pinfo = new ConnectInfo;
		else
			return itAddr->second;
	}
	memset(pinfo, 0, sizeof ConnectInfo);
	pinfo->pMapping = mapping_info;
	//udp是无链接的，直接放到容器中，不用判断两边是否连接成功
	pinfo->bInMap = true;
	pinfo->Addr_Client.sin_port = addr->sin_port;
	pinfo->Addr_Client.sin_addr.S_un.S_addr = addr->sin_addr.S_un.S_addr;
	//新建与服务端交流的socket
	uv_udp_init_ex(mapping_info->pLoop, &pinfo->u.server_udp, AF_INET);
	//开始接收
	uv_udp_recv_start(&pinfo->u.server_udp, IOCallBack::alloc_cb, IOCallBack::udp_server_recv_cb);
	itorPort->second.insert(pair<Connectkey, ConnectInfo*>(curKey, pinfo));
	return pinfo;
}

void CLibuvAdapter::AddTCPConnect(ConnectInfo* connect_info)
{
	USHORT nPort = htons(connect_info->pMapping->Addr_agent.sin_port);
	map<USHORT, map<Connectkey, ConnectInfo*>>::iterator itPort = m_mapConnect.find(nPort);
	Connectkey curKey = { connect_info->Addr_Client.sin_addr.S_un.S_addr, connect_info->Addr_Client.sin_port };
	if (itPort == m_mapConnect.end())//没有记录,直接添加
	{
		auto ret = m_mapConnect.insert(pair<USHORT, map<Connectkey, ConnectInfo*>>(nPort, map<Connectkey, ConnectInfo*>()));
		ret.first->second[curKey] = connect_info;
		return;
	}
	//有记录
	auto ret = itPort->second[curKey] = connect_info;
}