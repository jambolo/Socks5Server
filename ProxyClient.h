#if !defined( PROXYCLIENT_H_INCLUDED )
#define PROXYCLIENT_H_INCLUDED

#pragma once

/****************************************************************************

                                ProxyClient.h

						Copyright 2000, John J. Bolton
	----------------------------------------------------------------------

	$Header: //depot/Libraries/Socks5Server/ProxyClient.h#2 $

	$NoKeywords: $

****************************************************************************/

struct in_addr;
class Socket;
class Proxy;


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

class ProxyClient  
{
public:
	virtual ~ProxyClient();

	virtual void OnConnectRequest( Proxy * pProxy, Socket const & socket, in_addr const & ip, int port );
	virtual void OnBindRequest( Proxy * pProxy, Socket const & socket, in_addr const & ip, int port );
	virtual void OnConvey( Proxy * pProxy, Socket const & socket, char const * data, int length );
	virtual void OnClose( Proxy * pProxy );

protected:
	ProxyClient();		// Must be derived from
};

#endif // !defined( PROXYCLIENT_H_INCLUDED )
