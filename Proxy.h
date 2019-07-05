#if !defined( PROXY_H_INCLUDED )
#define PROXY_H_INCLUDED

#pragma once

/****************************************************************************

                                   Proxy.h

						Copyright 2000, John J. Bolton
	----------------------------------------------------------------------

	$Header: //depot/Libraries/Socks5Server/Proxy.h#2 $

	$NoKeywords: $

****************************************************************************/

#include "../Socket/SocketClient.h"
#include "Socks.h"

class ProxyClient;
class Socket;
struct in_addr;


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

class Proxy : public SocketClient  
{
public:
	Proxy( ProxyClient * pNotifyClient = 0 );
	virtual ~Proxy();

	int Initialize( Socket * pListenSocket );
	Socket * GetSocketToClient() { return m_pSocketToClient; }
	Socket * GetSocketToServer() { return m_pSocketToServer; }

	// *** SocketClient overrides
	virtual void OnAccept( Socket * pSocket, int nErrorCode );
	virtual void OnReceive( Socket * pSocket, int nErrorCode );
	virtual void OnClose( Socket * pSocket, int nErrorCode );
	// *** End SocketClient overrides

private:

	enum Socks5State
	{
		STATE_WAITING_FOR_AUTHENTICATION_METHODS,
		STATE_WAITING_FOR_REQUEST,
		STATE_PROXY_COMPLETE
	};


	void HandleConnectRequest( in_addr const & ip, int port );
	void HandleBindRequest( in_addr const & ip, int port );
	void SendReply( Socket * pSocket, Socks::ReplyType reply );
	void SendReply( Socket * pSocket, Socks::ReplyType reply, in_addr const & ip, int port );
	void CloseSockets();


	ProxyClient *	m_pProxyClient;
	Socket *		m_pListenSocket;
	Socket *		m_pSocketToClient;
	Socket *		m_pSocketToServer;
	Socks5State		m_Socks5State;
	int				m_RequestedBindPort;
	in_addr			m_ConnectingIp;
};

#endif // !defined( PROXY_H_INCLUDED )
