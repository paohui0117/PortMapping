#pragma once
#include <uv.h>
#include <map>
#include <set>

using namespace std;
//for  nState
#define MAPPING_STOP	0x00000001
#define MAPPING_START	0x00000002
#define MAPPING_FAIL	0x00000004
#define MAPPING_DELETING	0x00000006
//
#define INIT_FAIL		0x00000010
#define BIND_FAIL		0x00000020
#define LISTEN_FAIL		0x00000040

#define TCP_CONNECT_FAIL	0x00000080
#define TCP_CONNECT_SUCC	0x00000100


#define TCP_CLIENT_BREAK	0x00010000
#define TCP_SERVER_BREAK	0x00020000


/////////////////////////////////////////////////////////////////////////
//client————>agent————>server
//client<————agent<————server
struct MappingInfo
{
	uv_loop_t*		pLoop;
	int				nState;		
	sockaddr_in		Addr_agent;
	sockaddr_in		Addr_server;
	bool			bTCP;
	int				nConnect;

	UINT32			nTotalFromClientB;
	UINT32			nTotalFromClientM;

	UINT32			nTotalFromServerB;
	UINT32			nTotalFromServerM;
	union socket_listen
	{
		uv_tcp_t	listen_tcp;
		uv_udp_t	listen_udp;
	} u;
	void*			pUserData;
};
struct ConnectInfo
{
	MappingInfo*	pMapping;		//本链接所属的对应映射
	sockaddr_in		Addr_Client;	//客户端地址

	UINT32			nCurFromClientB;
	UINT32			nCurFromClientM;

	UINT32			nCurFromServerB;
	UINT32			nCurFromServerM;
	union socket_connect
	{
		struct socket_tcp
		{
			uv_tcp_t	client_tcp;//与客户端连接的socket，本socket收到的信息通过server_tcp发送出去
			uv_tcp_t	server_tcp;//与服务端链接的socket，本socket收到的信息通过client_tcp发送出去
		} tcp;
		uv_udp_t		server_udp;//udp与服务端通信的socket，本socket收到的信息通过listen_tcp发送给客户端
								   //listen_tcp收到的信息通过server_udp发送给服务端
	} u;
	bool			bInMap;			//是否已经加入到记录中
	void*			pUserData;
};
struct Connectkey
{
	UINT	nClientIP;
	USHORT	nClientPort;
	bool operator<(const Connectkey& other)
	{
		if (nClientIP < other.nClientIP)
			return true;
		if (nClientIP == other.nClientIP && nClientPort < other.nClientPort)
			return true;
		return false;
	}
};
inline bool operator<(const Connectkey& A, const Connectkey& B)
{
	if (A.nClientIP < B.nClientIP)
		return true;
	if (A.nClientIP == B.nClientIP && A.nClientPort < B.nClientPort)
		return true;
	return false;
}
wstring a2w(const char* str);
string w2a(LPCWSTR str);

#define MSG_ADD_CONNECT		0x0000001
#define MSG_DELETE_CONNECT	0x0000002

#define MSG_CLEAR_CONNECT	0x0000004

#define MSG_LISTEN_FAIL		0x0000010
#define MSG_REMOVE_MAPPING	0x0000020

class INotifyLoop
{
public:
	virtual void NotifyConnectMessage(UINT nType, ConnectInfo* pInfo) = 0; //connect信息发生变化时的通知
	virtual void NotifyMappingMessage(UINT nType, MappingInfo* pInfo) = 0; //映射信息发生变化
	virtual void NotifyGetAllConnectByMapping(ConnectInfo** pInfo, size_t size) = 0;//获取某一映射的所有链接信息
};
class IOCallBack;
class CLibuvAdapter
{
	friend IOCallBack;
public:
	CLibuvAdapter();
	virtual ~CLibuvAdapter();
public:
	MappingInfo* AddMapping(LPCWSTR strAgentIP, LPCWSTR strAgentPort, LPCWSTR strServerIP, LPCWSTR strServerPort, bool bTcp, int& err);
	//开始一个映射
	bool StartMapping(MappingInfo* pMapping);
	//停止一个映射
	bool StopMapping(MappingInfo* pMapping);
	//移除一个映射
	bool RemoveMapping(MappingInfo* pMapping);
	//断开一对连接
	bool RemoveConnect(ConnectInfo* connect_info, bool bAsync = true);//bAsync:是否需要异步
	//
	bool GetAllConnect(MappingInfo* pMapping);//获取一个映射的所有连接信息
	//获取所有本地ip  ipv4
	bool GetLocalIP(vector<wstring>& vecIP);
	//
	bool AddNotify(INotifyLoop* p);
	//
	bool RemoveNotify(INotifyLoop* p);

	bool GetRemoveAllIfFail();

	void SetRemoveAllIfFail(bool b);
	
private:
	typedef void(*AsyncWork)(struct uv__work*, int);
	bool InitLoop();	//初始化loop

	void RegisterAnsycWork(uv__work* pwork, AsyncWork);//向loop注册异步任务
	
	void AsyncOperate(void* p, AsyncWork workfun);//异步处理用户的操作
												  
	void AddConnect(ConnectInfo* connect_info);//添加一条记录
	
	void RemoveAllConnect(MappingInfo* pMappingInfo);//移除某一映射端口相关的全部链接

	ConnectInfo* GetUDPConnect(MappingInfo* mapping_info, const sockaddr_in* addr);//获取对应的udp链接记录，没有就添加

	void _RemoveMapping(MappingInfo* mapping_info);

	void _GetAllConnect(MappingInfo* mapping_info);
public:
	uv_loop_t*		m_pLoop;
private:
	bool				m_bRemoveAll;		//当本地监听发生错误断开时或者用户停止时，是否删除该映射的所有链接,默认为true
	set<INotifyLoop*>	m_setNotify;

	uv_thread_t		m_Loop_thread;
	uv_check_t		m_check_keeprun;
	//data:
	map<USHORT, MappingInfo>	m_mapMapping;
	map<USHORT, map<Connectkey, ConnectInfo*>> m_mapConnect;
};

