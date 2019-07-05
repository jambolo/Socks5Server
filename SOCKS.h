#if !defined( SOCKS_H_INCLUDED )
#define SOCKS_H_INCLUDED

#pragma once

/****************************************************************************

								  Socks.h
						Copyright 2000, John J. Bolton

	----------------------------------------------------------------------

	$Header: //depot/Libraries/Socks5Server/SOCKS.h#2 $

	$NoKeywords: $

 ****************************************************************************/

#include <string>
#include <winsock.h>

namespace Socks
{
	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	enum CommandType
	{
		CMD_CONNECT	=			1,
		CMD_BIND =				2,
		CMD_UDP_ASSOCIATE =		3,	// Invalid in SOCKS 4
		CMD_IANA_RESERVED =		4,	// Through 127 (invalid in SOCKS 4)
		CMD_PRIVATE_RESERVED =	128	// Through 255 (invalid in SOCKS 4)
	};

	enum IpAddressType
	{
		IP_V4 =		1,
		IP_DOMAIN =	3,
		IP_V6 =		4
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	struct IpV6
	{
		u_char ip[16];
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	class Address
	{
	public:

		virtual ~Address() = 0;
	};

	class IpV4Address : public Address
	{
	public:
		virtual ~IpV4Address();

		in_addr	m_Address;
	};


	class DomainAddress : public Address
	{
	public:
		virtual ~DomainAddress();

		std::string	m_Address;
	};


	class IpV6Address : public Address
	{
	public:
		virtual ~IpV6Address();

		IpV6	m_Address;
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	class ConnectRequest
	{
	public:
		enum
		{
			VERSION =	4,
			COMMAND =	CMD_CONNECT
		};
		
		int		m_Port;
		in_addr	m_Ip;
		char *	m_pData;
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	class ConnectReply
	{
	public:

		enum { VERSION = 4 };

		int		m_Result;
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	class BindRequest
	{
	public:
		enum
		{
			VERSION =	4,
			COMMAND =	CMD_BIND
		};
		
		int		m_Port;
		in_addr	m_Ip;
		char *	m_pData;
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	class BindReply
	{
	public:

		enum { VERSION = 4 };

		int		m_Result;
		in_addr	m_Ip;
		char *	m_pData;
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	enum S5AuthenticationType
	{
		AUTH_NONE =					0,
		AUTH_GSSAPI =				1,
		AUTH_USERNAME_PASSWORD =	2,
		AUTH_CHAP =					3,
		AUTH_IANA_RESERVED =		4,		// Through 127
		AUTH_PRIVATE_RESERVED =		128,	// Through 254
		AUTH_UNACCEPTABLE	=		255
	};

	enum ReplyType
	{
		REPLY_OK =							0,
		REPLY_SERVER_FAILURE =				1,
		REPLY_DISALLOWED =					2,
		REPLY_NETWORK_UNREACHABLE =			3,
		REPLY_HOST_UNREACHABLE =			4,
		REPLY_CONNECTION_REFUSED =			5,
		REPLY_TTL_EXPIRED =					6,
		REPLY_NOT_SUPPORTED =				7,
		REPLY_UNSUPPORTED_ADDRESS_TYPE =	8,
		REPLY_INVALID_ADDRESS =				9
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	class S5AuthenticationMethodRequest
	{
	public:
		enum { VERSION = 5 };

		int						m_MethodCount;
		S5AuthenticationType	m_Methods[255];
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	class S5AuthenticationMethodReply
	{
	public:
		enum { VERSION = 5 };

		int		m_Method;
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	class S5Request
	{
	public:

		enum { VERSION = 5 };

		CommandType	m_Command;
		unsigned		m_Flag;
		int				m_AddressType;
		Address *		m_pAddress;
		int				m_Port;
	};


	/********************************************************************************************************************/
	/*																													*/
	/*																													*/
	/********************************************************************************************************************/

	class S5Reply
	{
	public:

		enum { VERSION = 5 };

		ReplyType		m_Reply;
		unsigned		m_Flag;
		int				m_AddressType;
		Address *		m_pAddress;
		int				m_Port;
	};

} // namespace Socks


#endif // !defined( SOCKSSERVER_H_INCLUDED )
