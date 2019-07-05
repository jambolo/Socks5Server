#if !defined( SOCKS5SERVERCLIENT_H_INCLUDED )
#define SOCKS5SERVERCLIENT_H_INCLUDED

#pragma once

/****************************************************************************

                             Socks5ServerClient.h

						Copyright 2000, John J. Bolton
	----------------------------------------------------------------------

	$Header: //depot/Libraries/Socks5Server/Socks5ServerClient.h#2 $

	$NoKeywords: $

****************************************************************************/

struct in_addr;
class Socket;

/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

class Socks5ServerClient  
{
public:

	virtual ~Socks5ServerClient();

	virtual void OnAccept( Socket const & socket );
	virtual void OnClose( Socket const & socket );
	virtual void OnConnectRequest( Socket const & socket, in_addr const & ip, int port );
	virtual void OnBindRequest( Socket const & socket, in_addr const & ip, int port );
	virtual void OnConvey( Socket const & socket, char const * data, int length );

protected:

	Socks5ServerClient();
};


#endif // !defined( SOCKS5SERVERCLIENT_H_INCLUDED )
