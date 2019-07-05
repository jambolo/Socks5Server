#if !defined( SOCKS5SERVER_H_INCLUDED )
#define SOCKS5SERVER_H_INCLUDED

#pragma once

/****************************************************************************

                                Socks5Server.h

						Copyright 2000, John J. Bolton
	----------------------------------------------------------------------

	$Header: //depot/Libraries/Socks5Server/Socks5Server.h#2 $

	$NoKeywords: $

****************************************************************************/

#include <list>
#include <queue>

#include "ProxyClient.h"
#include "../Socket/SocketClient.h"

class Proxy;
class Socket;
struct in_addr;
class Socks5ServerClient;

/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

class Socks5Server : public ProxyClient, public SocketClient
{
public:
	Socks5Server( Socks5ServerClient * pClient = 0 );
	virtual ~Socks5Server();

	int Initialize( int listenPort );

	// Call this when idle to let the server delete closed proxies
	void OnIdle();

	// *** ProxyClient overrides
	virtual void OnConnectRequest( Proxy * pProxy, Socket const & socket, in_addr const & ip, int port );
	virtual void OnBindRequest( Proxy * pProxy, Socket const & socket, in_addr const & ip, int port );
	virtual void OnConvey( Proxy * pProxy, Socket const & socket, char const * pData, int length );
	virtual void OnClose( Proxy * pProxy );
	// *** End ProxyClient overrides

	// *** SocketClient overrides 
	virtual void OnAccept( Socket * pSocket, int nErrorCode );
//	virtual void OnReceive( Socket * pSocket, int nErrorCode );
//	virtual void OnClose( Socket * pSocket, int nErrorCode );
	// *** End SocketClient overrides 

private:

	typedef std::list< Proxy * >	ProxyList;
	typedef std::queue< Proxy * >	ProxyQueue;

	int						m_ListenPort;
	Socket *				m_pListenSocket;
	Socks5ServerClient *	m_pSocks5ServerClient;
	ProxyList				m_ProxyList;
	ProxyQueue				m_ProxyCleanupQueue;
};


#endif // !defined( SOCKS5SERVER_H_INCLUDED )
