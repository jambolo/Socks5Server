/****************************************************************************

                               Socks5Server.cpp

						Copyright 2000, John J. Bolton
	----------------------------------------------------------------------

	$Header: //depot/Libraries/Socks5Server/Socks5Server.cpp#2 $

	$NoKeywords: $

****************************************************************************/

#include "StdAfx.h"

#include "Socks5Server.h"

#include <list>
#include <queue>

#include "Proxy.h"
#include "../Socket/Socket.h"
#include "Socks5serverClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

Socks5Server::Socks5Server( Socks5ServerClient * pClient )
	: m_ListenPort( 0 ),
	m_pListenSocket( 0 ),
	m_pSocks5ServerClient( pClient )
{
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

Socks5Server::~Socks5Server()
{
	delete m_pListenSocket;

	// Delete all proxies

	for ( ProxyList::iterator i = m_ProxyList.begin(); i != m_ProxyList.end(); i++ )
	{
		delete *i;
	}
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

int Socks5Server::Initialize( int listenPort )
{
	m_ListenPort = listenPort;

	// Create the listen socket

	m_pListenSocket = new Socket( this );
	if ( !m_pListenSocket )
		throw std::bad_alloc();

	// Initialize the listen socket

	if ( !m_pListenSocket->Create( listenPort ) )
	{
		return m_pListenSocket->GetLastError();
	}

	// Now listen on it. OnAccept will be called when the server connects.

	if ( !m_pListenSocket->Listen() )
	{
		return m_pListenSocket->GetLastError();
	}

	return 0;
}



/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Socks5Server::OnIdle()
{
	// For each proxy in the clean up queue...
	// Proxys can't be cleaned up immediately because they are still active
	// at that point. They have to be put into a queue so they can be cleaned
	// up when they are no longer active (like now).

	while ( !m_ProxyCleanupQueue.empty() )
	{
		Proxy * const pProxy = m_ProxyCleanupQueue.front();
		
		// Delete the proxy

		delete pProxy;

		// Remove it from the proxy list

		m_ProxyList.remove( pProxy );

		// Remove it from the cleanup queue

		m_ProxyCleanupQueue.pop();
	}
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Socks5Server::OnConnectRequest( Proxy * pProxy, Socket const & socket, in_addr const & ip, int port )
{
	if ( m_pSocks5ServerClient )
		m_pSocks5ServerClient->OnConnectRequest( socket, ip, port );
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Socks5Server::OnBindRequest( Proxy * pProxy, Socket const & socket, in_addr const & ip, int port )
{
	if ( m_pSocks5ServerClient )
		m_pSocks5ServerClient->OnBindRequest( socket, ip, port );
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Socks5Server::OnConvey( Proxy * pProxy, Socket const & socket, char const * pData, int length )
{
	if ( m_pSocks5ServerClient )
		m_pSocks5ServerClient->OnConvey( socket, pData, length );
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Socks5Server::OnClose( Proxy * pProxy )
{
	if ( m_pSocks5ServerClient )
		m_pSocks5ServerClient->OnClose( *pProxy->GetSocketToServer() );

	// The proxy should be cleaned up now but it can't since it is still
	// active. Put it in a queue to be cleaned up later.

	m_ProxyCleanupQueue.push( pProxy );
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void Socks5Server::OnAccept( Socket * pSocket, int nErrorCode )
{
	// This function is called by the listen socket when someone tries to
	// connect to it.

	ASSERT( pSocket == m_pListenSocket );

	// Create a new proxy (deleted by destructor)

	Proxy * const pProxy = new Proxy( this );
	if ( !pProxy )
		throw std::bad_alloc();

	// Initialize the proxy. If that fails then delete the socket and ignore
	// everything

	if ( pProxy->Initialize( m_pListenSocket ) != 0 )
	{
		delete pProxy;
		return;
	}
	
	// Save the proxy in the proxy list

	m_ProxyList.push_back( pProxy );

	// Notify our client about the accept

	if ( m_pSocks5ServerClient )
		m_pSocks5ServerClient->OnAccept( *pProxy->GetSocketToClient() );
}
