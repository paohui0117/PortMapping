#pragma once
#include <uv.h>
#include <map>
using namespace std;
//for  nState
#define MAPPING_STOP	0x00000000
#define MAPPING_START	0x00000001
#define MAPPING_FAIL	0x00000002

#define TCP_CONNECT_FAIL	0x00000010
#define TCP_CONNECT_SUCC	0x00000020
#define TCP_CLIENT_BREAK	0x00000040
#define TCP_SERVER_BREAK	0x00000080

#define WORK_START			0x00000001
#define WORK_STOP			0x00000002
#define WORK_DELETE			0x00000004
/////////////////////////////////////////////////////////////////////////
//client————>agent————>server
//client<————agent<————server
struct MappingInfo
{
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
	};
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
		};
		uv_udp_t		server_udp;//udp与服务端通信的socket，本socket收到的信息通过listen_tcp发送给客户端
								   //listen_tcp收到的信息通过server_udp发送给服务端
	};
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
class CLibuvAdapter
{
public:
	CLibuvAdapter();
	virtual ~CLibuvAdapter();
public:
	MappingInfo* AddMapping(LPCWSTR strAgentIP, LPCWSTR strAgentPort, LPCWSTR strServerIP, LPCWSTR strServerPort, bool bTcp, int& err);
	
	bool StartMapping(MappingInfo* pMapping);//开始一个映射
	bool GetLocalIP(vector<wstring>& vecIP);
private:
	bool InitLoop();
	void RegisterAnsycWork(uv__work* pwork, void(*done)(struct uv__work *w, int status));
	
public:
	uv_loop_t*		m_pLoop;
private:
	
	uv_thread_t		m_Loop_thread;
	uv_check_t		m_check_keeprun;
	//data:
	map<USHORT, MappingInfo>	m_mapMapping;
	map<Connectkey, ConnectInfo> m_mapConnect;
};

