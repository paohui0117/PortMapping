#include "stdafx.h"
#include "LibuvAdapter.h"
#include "../libuv-dll/src/queue.h"

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
string w2a(const wchar_t* str)
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
CLibuvAdapter::CLibuvAdapter() : m_pLoop(nullptr)
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

MappingInfo* CLibuvAdapter::AddMapping(LPCWSTR strAgentIP, LPCWSTR strAgentPort, LPCWSTR strServerIP, LPCWSTR strServerPort, bool bTcp, int& err)
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
	curInfp.bTCP = bTcp;
	string strIP = w2a(strAgentIP);
	uv_ip4_addr(strIP.c_str(), nAgentPort, &curInfp.Addr_agent);

	strIP = w2a(strServerIP);
	USHORT nServerPort = _wtoi(strServerPort);
	uv_ip4_addr(strIP.c_str(), nAgentPort, &curInfp.Addr_server);

	return &curInfp;
}

bool CLibuvAdapter::StartMapping(MappingInfo* pMapping)
{
	if (!m_pLoop && !InitLoop())
		return false;
	//工作线程已经开启，接下来的操作都是异步的
	return true;
}

//空回调  
void null_cb(uv_check_t* handle)
{}
 void LoopThread(void *arg)
{
	CLibuvAdapter* pThis = (CLibuvAdapter*)arg;
	if (!pThis)
		return;
	uv_run(pThis->m_pLoop, UV_RUN_DEFAULT);//运行循环
}
//初始化loop
bool CLibuvAdapter::InitLoop()
{
	m_pLoop = uv_default_loop();
	if (!m_pLoop)
		return false;
	//单纯的保持事件循环存活，一个空的回调
	uv_check_init(m_pLoop, &m_check_keeprun);
	uv_check_start(&m_check_keeprun, null_cb);
	int ret = uv_thread_create(&m_Loop_thread, LoopThread, this);//开启工作线程
	return ret == 0;
}
//注册一个任务到loop，本任务将会在loop所在的线程中执行
void CLibuvAdapter::RegisterAnsycWork(uv__work* pwork, void(* done)(uv__work* w, int status))
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

bool CLibuvAdapter::GetLocalIP(vector<wstring>& vecIP)
{
	vecIP.clear();
	char hostname[NI_MAXHOST] = { 0 };
	if (!gethostname(hostname, NI_MAXHOST - 1) < 1)
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