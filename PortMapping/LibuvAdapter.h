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


/////////////////////////////////////////////////////////////////////////
//client！！！！>agent！！！！>server
//client<！！！！agent<！！！！server
struct MappingInfo
{
	uv__work		async_work;
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
	uv__work		async_work;
	MappingInfo*	pMapping;
	sockaddr_in		Addr_Client;

	UINT32			nCurFromClientB;
	UINT32			nCurFromClientM;

	UINT32			nCurFromServerB;
	UINT32			nCurFromServerM;
	union socket_connect
	{
		struct socket_tcp
		{
			uv_tcp_t	client_tcp;
			uv_tcp_t	server_tcp;
		};
		uv_udp_t		server_udp;
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
private:
	uv_loop_t*		m_pLoop;
	//data:
	map<USHORT, MappingInfo>	m_mapMapping;
	map<Connectkey, Connectkey> m_mapConnect;
};

