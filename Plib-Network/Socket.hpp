/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: Socket.hpp
* Propose  			: Socket Framework in Network Group. Rewrite the old Socketbasic class
* 
* Current Version	: 1.0
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2013-02-17
*/

#pragma once

#ifndef _PLIB_NETWORK_SOCKET_HPP_
#define _PLIB_NETWORK_SOCKET_HPP_

#include <Plib-Generic/Generic.hpp>
#include <Plib-Network/TcpSocket.hpp>
#include <Plib-Network/UdpSocket.hpp>

namespace Plib
{
	namespace Network
	{
		// The socket interface
		class ISocket
		{
		public:
			typedef Plib::Generic::Delegate< HSOCKETSTATUE(SOCKET_T,HSOCKETOPT,Uint32) >	_TChecker;
			// The socket handler
			SOCKET_T						m_hSocket;
			_TChecker 						m_statChk;

			ISocket():m_hSocket(INVALIDATE_SOCKET){}
			ISocket(SOCKET_T hSo): m_hSocket(hSo) { }

			virtual ~ISocket() {}

			// Get Local & Remote Port Info
			virtual const PeerInfo & RemotePeerInfo( ) const = 0;
			virtual Uint32 LocalPort( ) const = 0;

			// Basic method: Connect
			// Connect to peer according to the PeerInfo
			virtual bool Connect( const PeerInfo & peerInfo ) = 0;

			// Write data
			virtual int Write( const NData & data, Uint32 timeout = 1000 ) = 0;
			// Read data
			virtual NData Read( 
				Uint32 timeout = 1000, 
				bool waitUntilTimeout = false, 
				int idleLoopCount = 5 ) = 0;

			// Close the connection of the socket
			virtual void Close( )
			{
				if ( SOCKET_NOT_VALIDATE(m_hSocket) ) return;
				PLIB_NETWORK_CLOSESOCK(m_hSocket);
				m_hSocket = INVALIDATE_SOCKET;
			}

			// Socket status
			bool isReadable( Uint32 waitTime = 0 ) const
			{
				if ( m_hSocket == INVALIDATE_SOCKET ) return false;
				HSOCKETSTATUE _result = m_statChk(m_hSocket, HSO_CHECK_READ, waitTime);
				return _result == HSO_OK;
			}

			bool isWriteable( Uint32 waitTime = 0 ) const 
			{
				if ( m_hSocket == INVALIDATE_SOCKET ) return false;
				HSOCKETSTATUE _result = m_statChk(m_hSocket, HSO_CHECK_WRITE, waitTime);
				return _result == HSO_OK;
			}

			bool isConnected( Uint32 waitTime = 0 ) const
			{
				if ( m_hSocket == INVALIDATE_SOCKET ) return false;
				HSOCKETSTATUE _result = m_statChk(m_hSocket, HSO_CHECK_CONNECT, waitTime);
				return _result != HSO_INVALIDATE;
			}
		};

		// Socket Framework
		// In All Default setting, we make the Socket to be a TCP Socket
		// If you want to change the socket's working flow. change the functio
		// objects.
		template < 
			typename _TyConnect = TcpSocketConnect, 		// Function object to connect to peer
			typename _TyWrite = TcpSocketWrite, 			// Function object to write data
			typename _TyRead = TcpSocketRead 				// Function object to read data
		>
		class Socket : public ISocket
		{
		protected:
			Plib::Threading::StopWatch		idleTimer;

			PeerInfo 						m_remoteInfo;
			Uint32							m_localPort;
			struct sockaddr_in				m_sockAddr;

		protected:
			void _getSocketInfo( )
			{
				m_remoteInfo.GetRemotePeerInfo( m_hSocket );

				// Get local port
				struct sockaddr_in _addr;
				socklen_t _addrLen = sizeof(_addr);
				::memset( &_addr, 0, sizeof(_addr) );
				if ( 0 == getsockname( m_hSocket, (struct sockaddr *)&_addr, &_addrLen) )
				{
					m_localPort = ntohs(_addr.sin_port);
				}
			}

			void _initStatusChecker() {
				typename _TyConnect::SocketStatusChecker _chker;
				m_statChk = _TChecker(_chker);
			}
		public:
			// Properties
			virtual const PeerInfo & RemotePeerInfo( ) const { return m_remoteInfo; }
			virtual Uint32 LocalPort( ) const { return m_localPort; }

		public:
			// Structure
			Socket< _TyConnect, _TyWrite, _TyRead >( ) { this->_initStatusChecker(); }
			Socket< _TyConnect, _TyWrite, _TyRead >( SOCKET_T hSo ) : ISocket( hSo )
			{
				this->_initStatusChecker();
				if ( SOCKET_NOT_VALIDATE(hSo) ) return;
				this->_getSocketInfo( );
				idleTimer.SetStart();
			}
			virtual ~Socket< _TyConnect, _TyWrite, _TyRead > ()
			{
				this->Close();
			}

			// Kernal Functions
			virtual bool Connect( const PeerInfo & peerInfo )
			{
				if ( !SOCKET_NOT_VALIDATE(m_hSocket) ) this->Close( );
				m_hSocket = _TyConnect()(peerInfo, m_sockAddr);
				if ( !SOCKET_NOT_VALIDATE(m_hSocket) ) {
					idleTimer.SetStart();
					this->_getSocketInfo();
					this->m_remoteInfo.ConnectTimeOut = peerInfo.ConnectTimeOut;
				}
				return !SOCKET_NOT_VALIDATE(m_hSocket);
			}

			virtual int Write( const NData & data, Uint32 timeout = 1000 )
			{
				int _ret = _TyWrite()(m_hSocket, m_sockAddr, data, timeout);
				idleTimer.SetStart();
				if ( _ret == -1 ) this->Close();
				return _ret;
			}

			virtual NData Read( Uint32 timeout = 1000, bool waitUntilTimeout = false, int idleLoopCount = _TyRead::IdleLoopCount )
			{
				NData _buffer = _TyRead()(m_hSocket, m_sockAddr, timeout, waitUntilTimeout, idleLoopCount);
				idleTimer.SetStart();
				if ( _buffer.RefNull() ) this->Close();
				return _buffer;
			}

			// Properties
			double IdleTime( )
			{
				if ( SOCKET_NOT_VALIDATE(m_hSocket) ) return 0.f;
				idleTimer.Tick();
				return idleTimer.GetMileSecUsed();
			}

			bool SetReUsable( bool beReused = true )
			{
				if ( m_hSocket == INVALIDATE_SOCKET ) return false;
				int _reused = beReused ? 1 : 0;
				return setsockopt( m_hSocket, SOL_SOCKET, SO_REUSEADDR,
					(const char *)&_reused, sizeof(int) ) != -1;
			}

			bool SetNoDelay( bool beNoDelay = true )
			{
				if ( m_hSocket == INVALIDATE_SOCKET ) return false;
				int flag = beNoDelay ? 1 : 0;
				return setsockopt( m_hSocket, IPPROTO_TCP, 
					TCP_NODELAY, (const char *)&flag, sizeof(int) ) != -1;
 			}

 			bool SetReadTimeOut( unsigned int _milleseconds ) 
 			{
 				if ( m_hSocket == INVALIDATE_SOCKET ) return false;
 				struct timeval _tv = { _milleseconds / 1000, _milleseconds % 1000 * 1000 };
 				return setsockopt( m_hSocket, SOL_SOCKET, SO_RCVTIMEO, &_tv, sizeof(_tv) ) != -1;
 			}

			bool SetWriteBufferSize( unsigned int _size )
			{
				if ( m_hSocket == INVALIDATE_SOCKET ) return false;
				return setsockopt( m_hSocket, SOL_SOCKET, SO_SNDBUF,
					(const char *)&_size, sizeof(unsigned int) ) != -1;
			}
			bool SetReadBufferSize( unsigned int _size )
			{
				if ( m_hSocket == INVALIDATE_SOCKET ) return false;
				return setsockopt( m_hSocket, SOL_SOCKET, SO_RCVBUF,
					(const char *)&_size, sizeof(unsigned int) ) != -1;
			}
			// Set the Linger time when close socket.
			// Set _time to 0 to turn off this feature.
			bool SetLingerTime( unsigned int _time )
			{
				if ( m_hSocket == INVALIDATE_SOCKET ) return false;
				linger _lnger = { (_time > 0 ? 1 : 0), static_cast<int>(_time) };
				return setsockopt( m_hSocket, SOL_SOCKET, SO_LINGER,
					(const char*)&_lnger, sizeof(linger) ) != -1;
			}
		};

		typedef Socket< >		TcpSocket;
		typedef Socket< UdpSocketConnect, UdpSocketWrite, UdpSocketRead >	UdpSocket;
	
		template <typename _TyConnect, typename _TyWrite, typename _TyRead>
		std::ostream & operator << ( std::ostream & os, const Socket< _TyConnect, _TyWrite, _TyRead > & sock )
		{
			os << "[Socket FD: " << sock.m_hSocket << "(" << sock.LocalPort() << ")] --> " << sock.RemotePeerInfo();
			return os;
		}
	}
}

#endif // plib.network.socket.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
