/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: BasicNetwork.hpp
* Propose  			: The basic network functions
* 
* Current Version	: 1.0
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2013-02-16
*/

#pragma once

#ifndef _PLIB_NETWORK_BASICNETWORK_HPP_
#define _PLIB_NETWORK_BASICNETWORK_HPP_

#include <Plib-Generic/Generic.hpp>
#include <Plib-Threading/Threading.hpp>
#include <Plib-Text/Text.hpp>

#if _DEF_WIN32
	#include <WS2tcpip.h>
	#pragma comment( lib, "Ws2_32.lib" )
	#define PLIB_NETWORK_NOSIGNAL			0
	#define PLIB_NETWORK_IOCTL_CALL			ioctlsocket
	#define PLIB_NETWORK_CLOSESOCK			::closesocket
#else 
	#include <sys/socket.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <netinet/tcp.h>
	#define PLIB_NETWORK_NOSIGNAL			MSG_NOSIGNAL
	#define PLIB_NETWORK_IOCTL_CALL			ioctl
	#define PLIB_NETWORK_CLOSESOCK			close
#endif

#if _DEF_MAC
	#undef 	PLIB_NETWORK_NOSIGNAL
	#define PLIB_NETWORK_NOSIGNAL			0
#endif

/*
 * Inline Function, common methods
 */
namespace Plib {
    namespace Network {
		typedef long SOCKET_T;
		#ifndef INVALIDATE_SOCKET
		#define INVALIDATE_SOCKET			((long)((long)0 - (long)1))
		#endif

		#define SOCKET_NOT_VALIDATE( so )	((so) == INVALIDATE_SOCKET)

		typedef Plib::Text::String 			NData;

		// In No-Windows
		#ifndef FAR
		#define FAR
		#endif

		/* Translate Domain to IP Address */
		INLINE char * Domain2Ip(const char * cp_Domain, char * pb_IPOutput, Uint32 vb_Len)
		{
			struct hostent FAR * xp_HostEnt;
			struct in_addr v_inAddr;
			char * xp_addr;

			memset(pb_IPOutput, 0, vb_Len);

			xp_HostEnt = gethostbyname(cp_Domain);
			if (xp_HostEnt == NULL) return pb_IPOutput;

			xp_addr = xp_HostEnt->h_addr_list[0];
			if (xp_addr == NULL) return pb_IPOutput;

			memmove(&v_inAddr, xp_addr, 4);
			strcpy(pb_IPOutput, inet_ntoa(v_inAddr));

			return pb_IPOutput;
		}

		/* Translate Domain to InAddr */
		inline unsigned int Domain2InAddr(const char * cp_Domain)
		{
			/* Get the IP Address of the domain by invoking usp_DomainToIP */
			char xa_IPAddress[16];

			if (cp_Domain == NULL) return INADDR_ANY;
			if (Domain2Ip(cp_Domain, xa_IPAddress, 16)[0] == '\0')
				return (unsigned int)(-1L);
			return inet_addr(xa_IPAddress);
		}

		#ifndef __SOCKET_SERVER_INIT_IN_WINDOWS__
		#define __SOCKET_SERVER_INIT_IN_WINDOWS__

		#if _DEF_WIN32

		// In Windows Only.
		// This class is used to initialize the WinSock Server.
		// A global instance of this object will be create and
		// provide nothing. only the c'str of this object
		// will invoke WSAStartup and the d'str will invoke 
		// WSACleanup.
		// In Linux or other platform, this object will not be
		// defined.
		template< int __TMP_VALUE__ = 0 >
		class __socket_init_svr_in_windows
		{
			__socket_init_svr_in_windows< __TMP_VALUE__ >()
			{
				WSADATA v_wsaData;
				WORD v_wVersionRequested;

				v_wVersionRequested = MAKEWORD(1, 1);
				WSAStartup(v_wVersionRequested, &v_wsaData);
			}

		public:
			~__socket_init_svr_in_windows< __TMP_VALUE__ >()
			{
				WSACleanup();
			}
			static __socket_init_svr_in_windows< __TMP_VALUE__ > __g_socksvrInWindows;
		};

		template< > __socket_init_svr_in_windows< 0 > 
		__socket_init_svr_in_windows< 0 >::__g_socksvrInWindows;

		#endif

		#endif

        // Get local Hostname
        INLINE Plib::Text::String LocalHostname( ) {
            char __Hostname[256] = { 0 };
            if ( gethostname( __Hostname, sizeof(__Hostname) ) == -1 ) {
                return Plib::Text::String::Null;
            }
            return Plib::Text::String::Parse( "%s", __Hostname );
        }
        
        // Get local Ip Address.
        INLINE Plib::Generic::Array< Uint32 > LocalIp( ) {
            Plib::Text::String __Hostname = LocalHostname( );
            if ( __Hostname == Plib::Text::String::Null )
                return Plib::Generic::Array< Uint32 >::Null;
            struct hostent * __hosten = gethostbyname( __Hostname.c_str( ) );
            if ( __hosten == 0 ) 
                return Plib::Generic::Array< Uint32 >::Null;
            
            Plib::Generic::Array< Uint32 > __ipArray;
            // Loop to get all local ip address.
            for ( Uint32 i = 0; __hosten->h_addr_list[i] != 0; ++i ) {
                struct in_addr __addr;
                memcpy( &__addr, __hosten->h_addr_list[i], sizeof( struct in_addr ) );
                __ipArray.PushBack( __addr.s_addr );
            }
            return __ipArray;
        }
        
        // Get local Ip Address, String format
        INLINE Plib::Generic::Array< Plib::Text::String > LocalIpAddress( ) {
            // Get the ip first.
            Plib::Generic::Array< Uint32 > __ips = LocalIp( );
            if ( __ips.RefNull() || __ips.Size() == 0 )
                return Plib::Generic::Array< Plib::Text::String >::Null;
            
            // Convert the ip to the string.
            Plib::Generic::Array< Plib::Text::String > __ipAddr;
            for ( Uint32 i = 0; i < __ips.Size(); ++i ) {
                __ipAddr.PushBack( Plib::Text::Convert::ToIP( __ips[i] ) );
            }
            return __ipAddr;
        }

        // Global Error Handler
        // template < typename TSocket >
        // void GlobalConnectionErrorHandler( TSocket )
    }
}


#endif // plib.network.basicnetwork.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */

