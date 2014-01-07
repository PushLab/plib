/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: TcpSocket.hpp
* Propose  			: Tcp Socket Core Function Object.
* 
* Current Version	: 1.0
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2013-02-17
*/

#pragma once

#ifndef _PLIB_NETWORK_TCPSOCKET_HPP_
#define _PLIB_NETWORK_TCPSOCKET_HPP_

#include <Plib-Network/BasicNetwork.hpp>
#include <Plib-Network/PeerInfo.hpp>

namespace Plib
{
	namespace Network
	{
		// Socket action functions definitions

		typedef enum {
			HSO_INVALIDATE 	= -1,
			HSO_IDLE 		= 0,
			HSO_OK			= 1
		} HSOCKETSTATUE;

		typedef enum {
			HSO_CHECK_WRITE		= 1,
			HSO_CHECK_READ		= 2,
			HSO_CHECK_CONNECT	= 4
		} HSOCKETOPT;

		// Status Checker
		class TcpSocketStatus
		{
		public:
			HSOCKETSTATUE operator() ( SOCKET_T hSo, HSOCKETOPT option = HSO_CHECK_READ ) const
			{
				if ( SOCKET_NOT_VALIDATE(hSo) ) return HSO_INVALIDATE;
				fd_set _fs;
				FD_ZERO( &_fs );
				FD_SET( hSo, &_fs );

				int _ret = 0; struct timeval _tv = {0, 0};

				if ( option & HSO_CHECK_READ ) {
					do {
						_ret = ::select( hSo + 1, &_fs, NULL, NULL, &_tv );
					} while ( _ret < 0 && errno == EINTR );
					if ( _ret > 0 ) {
						char _word;
						// the socket has received a close sig
						if ( ::recv( hSo, &_word, 1, MSG_PEEK ) <= 0 ) 
							return HSO_INVALIDATE;
						return HSO_OK;
					}
					if ( _ret < 0 ) return HSO_INVALIDATE;
				}

				if ( option & HSO_CHECK_WRITE ){
					do {
						_ret = ::select( hSo + 1, NULL, &_fs, NULL, &_tv );
					} while ( _ret < 0 && errno == EINTR );
					if ( _ret > 0 ) return HSO_OK;
					if ( _ret < 0 ) return HSO_INVALIDATE;
				}
				return HSO_IDLE;
			}
		};

		// Connect as a tcp socket
		class TcpSocketConnect
		{
		public:
			// 
			SOCKET_T operator() ( const PeerInfo & peerInfo ) const {
				if ( peerInfo.Address.Size() == 0 || peerInfo.Address == "0.0.0.0" || peerInfo.Port == 0 )
					return INVALIDATE_SOCKET;
				
				const char *_addr = peerInfo.Address.C_Str();
				Uint32 _port = peerInfo.Port;
				Uint32 _timeOut = peerInfo.ConnectTimeOut;

				// Try to nslookup the host
				unsigned int _inAddr = Domain2InAddr( _addr );
				if ( _inAddr == (unsigned int)(-1) ) {
					return INVALIDATE_SOCKET;
				}

				// Create Socket Handle
				SOCKET_T hSo = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if ( SOCKET_NOT_VALIDATE(hSo) ) {
					return INVALIDATE_SOCKET;
				}
				
				// Set With TCP_NODELAY
				int flag = 1;
				if( setsockopt( hSo, IPPROTO_TCP, 
					TCP_NODELAY, (const char *)&flag, sizeof(int) ) == -1 )
				{
					PLIB_NETWORK_CLOSESOCK( hSo );
					return INVALIDATE_SOCKET;
				}

				struct sockaddr_in _sockAddr;
				memset( &_sockAddr, 0, sizeof(_sockAddr) );
				_sockAddr.sin_addr.s_addr = _inAddr;
				_sockAddr.sin_family = AF_INET;
				_sockAddr.sin_port = htons(_port);

				// Async Socket Connecting
				if ( _timeOut > 0 )
				{
					unsigned long _u = 1;
					PLIB_NETWORK_IOCTL_CALL(hSo, FIONBIO, &_u);
				}

				// Connect
				if ( ::connect( hSo, (struct sockaddr *)&_sockAddr, 
						sizeof(_sockAddr) ) == -1 )
				{
					if ( _timeOut == 0 ) {
						PLIB_NETWORK_CLOSESOCK( hSo );
						return INVALIDATE_SOCKET;
					}
					struct timeval _tm = { _timeOut / 1000, 
						static_cast<__darwin_suseconds_t>((_timeOut % 1000) * 1000) };
					fd_set _fs;
					int _error = 0, len = sizeof(_error);
					FD_ZERO( &_fs );
					FD_SET( hSo, &_fs );

					// Wait until timeout
					do {
						_error = ::select( hSo + 1, NULL, &_fs, NULL, &_tm );
					} while( _error < 0 && errno == EINTR );

					// _error > 0 means writable, then check if has any error.
					if ( _error > 0 ) {
						getsockopt( hSo, SOL_SOCKET, SO_ERROR, 
							(char *)&_error, (socklen_t *)&len);
						if ( _error != 0 ) {
							// Failed to connect
							PLIB_NETWORK_CLOSESOCK( hSo );
							return INVALIDATE_SOCKET;
						}
					} else {
						// Failed to connect
						PLIB_NETWORK_CLOSESOCK( hSo );
						return INVALIDATE_SOCKET;
					}
				}
				// Reset Socket Statue
				if ( _timeOut > 0 )
				{
					unsigned long _u = 0;
					PLIB_NETWORK_IOCTL_CALL(hSo, FIONBIO, &_u);
				}

				return hSo;
			}
		};

		// TcpWrite Operator
		class TcpSocketWrite
		{
		public:
			int operator() ( SOCKET_T hSo, const NData & data, Uint32 writeTimeout = 1000 ) const
			{
				if ( data.RefNull() ) return 0;
				if ( SOCKET_NOT_VALIDATE(hSo) ) return -1;
				if ( writeTimeout > 0 ) {
#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
					setsockopt( hSo, SOL_SOCKET, SO_SNDTIMEO,
						(const char *)&writeTimeout, sizeof(Uint32) );
#else
					struct timeval wtv = { writeTimeout / 1000, 
						static_cast<__darwin_suseconds_t>((writeTimeout % 1000) * 1000) };
					setsockopt(hSo, SOL_SOCKET, SO_SNDTIMEO, 
						(const char *)&wtv, sizeof(struct timeval));
#endif
				}
				int _allSent = 0;
				int _lastSent = 0;

				Uint32 _length = data.Size();
				const char *_data = data.C_Str();

				// use a stopwatch
				Plib::Threading::StopWatch wStopWatch;
				wStopWatch.SetStart();
				while ( (unsigned int)_allSent < _length )
				{
					_lastSent = ::send( hSo, _data + _allSent, 
						(_length - (unsigned int)_allSent), 0 | PLIB_NETWORK_NOSIGNAL );
					if( _lastSent < 0 ) {
						// Failed to send
						return _lastSent;
					}
					_allSent += _lastSent;
					if ( writeTimeout > 0 ) {
						wStopWatch.Tick();
						if ( wStopWatch.GetMileSecUsed() >= writeTimeout && 
							((unsigned int)_allSent < _length) ) 
							break;
					}
				}
				return _allSent;
			}
		};

		// TcpRead Operator
		class TcpSocketRead
		{
		public:
			enum { IdleLoopCount = 5, TcpSocketReadBufferSize = 512 };
		public:
			NData operator() ( SOCKET_T hSo, Uint32 readTimeout = 1000, 
				bool waitUntilTimeout = false, bool idleLoopCount = IdleLoopCount ) const
			{
				if ( SOCKET_NOT_VALIDATE(hSo) ) return NData::Null;

				// Data buffer
				NData _readBuffer;

				struct timeval _tv = { (long)readTimeout / 1000, 
					static_cast<__darwin_suseconds_t>(((long)readTimeout % 1000) * 1000) };
				fd_set recvFs;
				FD_ZERO( &recvFs );
				FD_SET( hSo, &recvFs );

				TcpSocketStatus _statusChecker;
				HSOCKETSTATUE _status;

				// Buffer
				char _buffer[TcpSocketReadBufferSize] = { 0 };
				int _idleLoopCount = idleLoopCount;

				do {
					// Wait for the incoming
					int _retCode = 0;
					do {
						_retCode = ::select( hSo + 1, &recvFs, NULL, NULL, &_tv );
					} while ( _retCode < 0 && errno == EINTR );

					if ( _retCode < 0 )	// Error
						return NData::Null;
					if ( _retCode == 0 )	// TimeOut
						return _readBuffer;

					// Get data from the socket cache
					_retCode = ::recv( hSo, _buffer, TcpSocketReadBufferSize, 0 );
					// Error happen when read data, means the socket has become invalidate
					if ( _retCode < 0 ) return NData::Null;
					if ( _retCode == 0 ) break;	// Get EOF
					_readBuffer.Append( _buffer, _retCode );

					if ( waitUntilTimeout ) continue;
					do {
						// Check if the socket has more data to read
						_status = _statusChecker(hSo, HSO_CHECK_READ);
						// Socket become invalidate
						if (_status == HSO_INVALIDATE) 
						{
							if ( _readBuffer.Size() > 0 ) return _readBuffer;
							return NData::Null;
						}
						// Socket become idle, and already read some data, means the peer finish sending one
						// package. We return the buffer and make the up-level to process the package.
						//PDUMP(_idleLoopCount);
						if (_status == HSO_IDLE) {
							//PDUMP( _idleLoopCount );
							if ( _idleLoopCount > 0 ) _idleLoopCount -= 1;
							else return _readBuffer;
						} else break;
					} while ( _idleLoopCount > 0 );
				} while ( true );

				// Useless
				return _readBuffer;
			}
		};
	}
}

#endif // plib.network.tcpsocket.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
