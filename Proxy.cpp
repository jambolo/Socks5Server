/****************************************************************************

                                  Proxy.cpp

						Copyright 2000, John J. Bolton
	----------------------------------------------------------------------

	$Header: //depot/Libraries/Socks5Server/Proxy.cpp#2 $

	$NoKeywords: $

****************************************************************************/


#include "StdAfx.h"

#include "Proxy.h"

#include "Socks.h"
#include "../Socket/Socket.h"
#include "ProxyClient.h"
#include "../Socket/SocketClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



static char s_XferBuffer[ 4096 ];

/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

Proxy::Proxy( ProxyClient * pProxyClient )
	: m_pProxyClient( pProxyClient ),
	m_pListenSocket( 0 ), m_pSocketToClient( 0 ), m_pSocketToServer( 0 ),
	m_Socks5State( STATE_WAITING_FOR_AUTHENTICATION_METHODS )
{
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

Proxy::~Proxy()
{
	CloseSockets();
}




/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

int Proxy::Initialize( Socket * pListenSocket )
{
	m_pSocketToClient = new Socket( this );
	if ( !m_pSocketToClient )
		throw std::bad_alloc();

	if ( !pListenSocket->Accept( *m_pSocketToClient ) )
		return pListenSocket->GetLastError();

	return 0;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Proxy::OnReceive( Socket * pSocket, int nErrorCode )
{
	ASSERT( pSocket == m_pSocketToClient || pSocket == m_pSocketToServer );

	switch ( m_Socks5State )
	{
	case STATE_WAITING_FOR_AUTHENTICATION_METHODS:
	{
		ASSERT( pSocket == m_pSocketToClient );

		// Receive methods list

		int nbytes = m_pSocketToClient->Receive( s_XferBuffer, sizeof s_XferBuffer );

		// If the version is wrong the ignore the message

		if ( s_XferBuffer[0] != Socks::S5AuthenticationMethodRequest::VERSION )
			return;

		// Send a reply AUTH_NONE (the only one supported)

		s_XferBuffer[0] = Socks::S5AuthenticationMethodReply::VERSION;			// Version
		s_XferBuffer[1] = Socks::AUTH_NONE;										// Method

		nbytes = m_pSocketToClient->Send( s_XferBuffer, 2 );

		// New state

		m_Socks5State = STATE_WAITING_FOR_REQUEST;

		break;
	}

	case STATE_WAITING_FOR_REQUEST:
	{
		ASSERT( pSocket == m_pSocketToClient );

		// Receive request

		int nbytes = m_pSocketToClient->Receive( s_XferBuffer, sizeof s_XferBuffer );

		if ( s_XferBuffer[0] != Socks::S5Request::VERSION )
			return;

		switch ( s_XferBuffer[1] )
		{
		case Socks::CMD_CONNECT:

			// Allow only IPV4 ip addresses

			if ( s_XferBuffer[3] != Socks::IP_V4 )
			{
				SendReply( m_pSocketToClient, Socks::REPLY_UNSUPPORTED_ADDRESS_TYPE );
				CloseSockets();
				return;
			}

			// Handle the connect request

			HandleConnectRequest( *reinterpret_cast< in_addr * >( &s_XferBuffer[4] ), ntohs( *reinterpret_cast< u_short * >( &s_XferBuffer[8] ) ) );
			break;

		case Socks::CMD_BIND:

			// Allow only IPV4 ip addresses

			if ( s_XferBuffer[3] != Socks::IP_V4 )
			{
				SendReply( m_pSocketToClient, Socks::REPLY_UNSUPPORTED_ADDRESS_TYPE );
				CloseSockets();
				return;
			}

			// Handle the bind request

			HandleBindRequest( *reinterpret_cast< in_addr * >( &s_XferBuffer[4] ), ntohs( *reinterpret_cast< u_short * >( &s_XferBuffer[8] ) ) );
			break;

		default:
			ASSERT( ( "Unhandled SOCKS 5 request.", false ) );
			SendReply( m_pSocketToClient, Socks::REPLY_NOT_SUPPORTED );
			CloseSockets();
			return;
		}

		// New state

		m_Socks5State = STATE_PROXY_COMPLETE;

		break;
	}

	case STATE_PROXY_COMPLETE:
	{
		// Receive the data

		int nbytes = pSocket->Receive( s_XferBuffer, sizeof s_XferBuffer );

		// Notify the proxy's client (not the client socket)

		if ( m_pProxyClient )
			m_pProxyClient->OnConvey( this, *pSocket, s_XferBuffer, nbytes );

		// If errors, then shutdown

		if ( nbytes == 0 || nbytes == SOCKET_ERROR )
		{
			CloseSockets();
			return;
		}

		// Send the data out the other end

		Socket * const other_socket = ( pSocket == m_pSocketToClient ) ? m_pSocketToServer : m_pSocketToClient;
		nbytes = other_socket->Send( s_XferBuffer, nbytes );

		// If errors, then shutdown
		
		if ( nbytes == 0 || nbytes == SOCKET_ERROR )
		{
			CloseSockets();
			return;
		}
		break;
	}

	default:
		ASSERT( ( "Unhandled proxy state", false ) );
		break;
	}
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Proxy::OnAccept( Socket * pSocket, int nErrorCode )
{
	// This function is called when the client wants the server to connect to
	// it (via a BIND request) and the server has connected.

	m_pSocketToServer = new Socket( this );
	if ( !m_pSocketToServer )
		throw std::bad_alloc();

	sockaddr_in addr;
	int length = sizeof( addr );

	if ( !m_pListenSocket->Accept( *m_pSocketToServer, reinterpret_cast< sockaddr * >( &addr ), &length ) )
	{
		switch ( m_pSocketToServer->GetLastError() )
		{
		case WSAENETDOWN:
			SendReply( m_pSocketToClient, Socks::REPLY_SERVER_FAILURE );
			break;

		case WSANOTINITIALISED:
		case WSAEFAULT:
		case WSAEINPROGRESS:
		case WSAEINVAL:
		case WSAEMFILE:
		case WSAENOBUFS:
		case WSAENOTSOCK:
		case WSAEOPNOTSUPP:
		case WSAEWOULDBLOCK:
		default:
			ASSERT( m_pSocketToServer->GetLastError() != 0 );
			SendReply( m_pSocketToClient, Socks::REPLY_SERVER_FAILURE );
			break;
		}

		CloseSockets();
		return;
	}

	// Send second reply

	SendReply( m_pSocketToClient, Socks::REPLY_OK, addr.sin_addr, ntohs( addr.sin_port ) );

	// The listen socket is not needed any more

	delete m_pListenSocket; m_pListenSocket = 0;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Proxy::OnClose( Socket * pSocket, int nErrorCode )
{
	CloseSockets();
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Proxy::HandleConnectRequest( in_addr const & ip, int port )
{
	// Notify the proxy's client (not the client socket)

	if ( m_pProxyClient )
		m_pProxyClient->OnConnectRequest( this, *m_pSocketToClient, ip, port );

	// Create the socket to the server

	m_pSocketToServer = new Socket( this );
	if ( !m_pSocketToServer )
		throw std::bad_alloc();

	// Initialize the socket

	if ( !m_pSocketToServer->Create() )
	{
		SendReply( m_pSocketToClient, Socks::REPLY_SERVER_FAILURE );
		CloseSockets();
		return;
	}

	// Get the server ip address string and connect

	CString ip_address = inet_ntoa( ip );

	if ( !m_pSocketToServer->Connect( LPCTSTR( ip_address ), port ) )
	{
		switch ( m_pSocketToServer->GetLastError() )
		{
		case WSAENETDOWN:
			SendReply( m_pSocketToClient, Socks::REPLY_SERVER_FAILURE );
			break;

		case WSAECONNREFUSED:
			SendReply( m_pSocketToClient, Socks::REPLY_CONNECTION_REFUSED );
			break;

		case WSAEADDRINUSE:
		case WSAEINVAL:
			SendReply( m_pSocketToClient, Socks::REPLY_INVALID_ADDRESS );
			break;

		case WSAENETUNREACH:
			SendReply( m_pSocketToClient, Socks::REPLY_NETWORK_UNREACHABLE );
			break;

		case WSAETIMEDOUT:
			SendReply( m_pSocketToClient, Socks::REPLY_TTL_EXPIRED );
			break;

		case WSANOTINITIALISED:
		case WSAEINPROGRESS:
		case WSAEDESTADDRREQ:
		case WSAEFAULT:
		case WSAENOBUFS:
		case WSAENOTSOCK:
		case WSAEISCONN:
		case WSAEMFILE:
		case WSAEWOULDBLOCK:
		case WSAEADDRNOTAVAIL:
		case WSAEAFNOSUPPORT:
		default:
			ASSERT( m_pSocketToServer->GetLastError() != 0 );
			SendReply( m_pSocketToClient, Socks::REPLY_SERVER_FAILURE );
			break;
		}

		CloseSockets();
		return;
	}

	// Need to find out the ip address on our end of the connected socket

	sockaddr_in addr;
	int length = sizeof( addr );

	m_pSocketToServer->GetSockName( reinterpret_cast< sockaddr *>( &addr ), &length );

	// Send first reply

	SendReply( m_pSocketToClient, Socks::REPLY_OK, addr.sin_addr, ntohs( addr.sin_port ) );
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Proxy::HandleBindRequest( in_addr const & ip, int port )
{
	// Save the ip and port for when the connect happens

	m_ConnectingIp = ip;
	m_RequestedBindPort = port;

	// Notify the proxy's client (not the client socket)

	if ( m_pProxyClient )
		m_pProxyClient->OnBindRequest( this, *m_pSocketToClient, ip, port );

	// Create the listen socket

	m_pListenSocket = new Socket( this );
	if ( !m_pListenSocket )
		throw std::bad_alloc();

	// Initialize the listen socket

	if ( !m_pListenSocket->Create( port ) )
	{
		switch ( m_pSocketToServer->GetLastError() )
		{
		case WSAENETDOWN:
			SendReply( m_pSocketToClient, Socks::REPLY_SERVER_FAILURE );
			break;

		case WSAEADDRINUSE:
		case WSAEPROTONOSUPPORT:
		case WSAEPROTOTYPE:
			SendReply( m_pSocketToClient, Socks::REPLY_INVALID_ADDRESS );
			break;

		case WSAEINVAL:
		case WSANOTINITIALISED:
		case WSAEINPROGRESS:
		case WSAEFAULT:
		case WSAENOBUFS:
		case WSAENOTSOCK:
		case WSAEMFILE:
		case WSAEAFNOSUPPORT:
		case WSAESOCKTNOSUPPORT:
		default:
			ASSERT( m_pSocketToServer->GetLastError() != 0 );
			SendReply( m_pSocketToClient, Socks::REPLY_SERVER_FAILURE );
			break;
		}

		CloseSockets();
		return;
	}

	// Now listen on it. OnAccept will be called when the server connects.

	if ( !m_pListenSocket->Listen() )
	{
		switch ( m_pSocketToServer->GetLastError() )
		{
		case WSAENETDOWN:
			SendReply( m_pSocketToClient, Socks::REPLY_SERVER_FAILURE );
			break;

		case WSAEADDRINUSE:
			SendReply( m_pSocketToClient, Socks::REPLY_INVALID_ADDRESS );
			break;

		case WSANOTINITIALISED:
		case WSAEINPROGRESS:
		case WSAEINVAL:
		case WSAENOBUFS:
		case WSAENOTSOCK:
		case WSAEISCONN:
		case WSAEMFILE:
		case WSAEOPNOTSUPP:
		default:
			ASSERT( m_pSocketToServer->GetLastError() != 0 );
			SendReply( m_pSocketToClient, Socks::REPLY_SERVER_FAILURE );
			break;
		}

		CloseSockets();
		return;
	}

	// Need to find out the ip address of the listen socket

	sockaddr_in addr;
	int length = sizeof( addr );

	m_pListenSocket->GetSockName( reinterpret_cast< sockaddr *>( &addr ), &length );

	// Send first reply

	SendReply( m_pSocketToClient, Socks::REPLY_OK, addr.sin_addr, ntohs( addr.sin_port ) );
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Proxy::CloseSockets()
{
	// Notify the proxy's client (not the client socket)

	if ( m_pProxyClient )
		m_pProxyClient->OnClose( this );
	
	delete m_pListenSocket;		m_pListenSocket = 0;
	delete m_pSocketToClient;	m_pSocketToClient = 0;
	delete m_pSocketToServer;	m_pSocketToServer = 0;

	m_pProxyClient = 0;		// No more client notifications
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Proxy::SendReply( Socket * pSocket, Socks::ReplyType reply, in_addr const & ip, int port )
{
	s_XferBuffer[0] = Socks::S5Reply::VERSION;
	s_XferBuffer[1] = reply;
	s_XferBuffer[2] = 0;
	s_XferBuffer[3] = Socks::IP_V4;
	*reinterpret_cast< in_addr * >( &s_XferBuffer[4] ) = ip;
	*reinterpret_cast< u_short * >( &s_XferBuffer[8] ) = htons( port );


	pSocket->Send( s_XferBuffer, 10 );
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Proxy::SendReply( Socket * pSocket, Socks::ReplyType reply )
{
	in_addr ip;
	ip.S_un.S_addr = 0;
	
	SendReply( pSocket, reply, ip, 0 );
}