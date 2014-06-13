/*
* Copyright (c) 2014, Push Chen
* All rights reserved.
* 
* File Name         : UdpSocket.hpp
* Propose           : Udp Socket Core Function Object.
* 
* Current Version   : 1.0
* Change Log        : First Definition.
* Author            : Push Chen
* Change Date       : 2014-06-06
*/

#pragma once

#ifndef _PLIB_NETWORK_UDPSOCKET_HPP_
#define _PLIB_NETWORK_UDPSOCKET_HPP_

#include <Plib-Network/BasicNetwork.hpp>
#include <Plib-Network/PeerInfo.hpp>

namespace Plib
{
    namespace Network
    {
        // Socket action functions definitions
        // Status Checker
        class UdpSocketStatus
        {
        public:
            HSOCKETSTATUE operator() ( SOCKET_T hSo, HSOCKETOPT option = HSO_CHECK_READ, Uint32 waitTime = 0 ) const
            {
                if ( SOCKET_NOT_VALIDATE(hSo) ) return HSO_INVALIDATE;
                fd_set _fs;
                FD_ZERO( &_fs );
                FD_SET( hSo, &_fs );

                int _ret = 0; struct timeval _tv = {waitTime / 1000, waitTime % 1000 * 1000};

                if ( option & HSO_CHECK_READ ) {
                    do {
                        _ret = ::select( hSo + 1, &_fs, NULL, NULL, &_tv );
                    } while ( _ret < 0 && errno == EINTR );
                    PTRACE("Udp select ret: " << _ret);
                    if ( _ret > 0 ) {
                        // char _word;
                        // // the socket has received a close sig
                        // if ( ::recv( hSo, &_word, 1, MSG_PEEK ) <= 0 ) 
                        //     return HSO_INVALIDATE;
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
        class UdpSocketConnect
        {
        public:
            typedef UdpSocketStatus         SocketStatusChecker;
            // 
            SOCKET_T operator() ( const PeerInfo & peerInfo, struct sockaddr_in &sockAddr ) const {
                const char *_addr = peerInfo.Address.C_Str();
                Uint32 _port = peerInfo.Port;
                memset( &sockAddr, 0, sizeof(sockAddr) );
                sockAddr.sin_family = AF_INET;
                sockAddr.sin_port = htons(_port);
                if ( inet_aton(_addr, &sockAddr.sin_addr) == 0 ) {
                    return -1;
                }

                // Create Socket Handle
                SOCKET_T hSo = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                if ( SOCKET_NOT_VALIDATE(hSo) ) {
                    return INVALIDATE_SOCKET;
                }
                
                if ( peerInfo.Address.Size() == 0 || 
                     peerInfo.Address == "0.0.0.0" || 
                     peerInfo.Port == 0 )
                    return -1;
                
                return hSo;
            }
        };

        // TcpWrite Operator
        class UdpSocketWrite
        {
        public:
            int operator() ( SOCKET_T hSo, 
                             struct sockaddr_in &sockAddr,
                             const NData & data, 
                             Uint32 writeTimeout = 1000 ) const
            {
                if ( data.RefNull() ) return 0;
                if ( SOCKET_NOT_VALIDATE(hSo) ) return -1;
                if ( writeTimeout > 0 ) {
#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
                    setsockopt( hSo, SOL_SOCKET, SO_SNDTIMEO,
                        (const char *)&writeTimeout, sizeof(Uint32) );
#else
                    struct timeval wtv = { writeTimeout / 1000, 
                        static_cast<int>((writeTimeout % 1000) * 1000) };
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
                    _lastSent = ::sendto(hSo, _data + _allSent, 
                        (_length - (unsigned int)_allSent), 0, 
                        (struct sockaddr *)&sockAddr, sizeof(sockAddr));
                    if ( _lastSent < 0 ) {
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
        class UdpSocketRead
        {
        public:
            enum { IdleLoopCount = 5, UdpSocketReadBufferSize = 512 };
        public:
            NData operator() ( SOCKET_T hSo,
                               struct sockaddr_in &sockAddr,
                               Uint32 readTimeout = 1000, 
                               bool waitUntilTimeout = false, 
                               bool idleLoopCount = IdleLoopCount ) const
            {
                if ( SOCKET_NOT_VALIDATE(hSo) ) return NData::Null;

                // Set the receive time out
                struct timeval _tv = { readTimeout / 1000, readTimeout % 1000 * 1000 };
                if ( setsockopt( hSo, SOL_SOCKET, SO_RCVTIMEO, &_tv, sizeof(_tv) ) == -1)
                    return NData::Null;

                char _buffer[UdpSocketReadBufferSize] = { 0 };
                NData _result;
                int _dataLen = 0;
                do {
                    socklen_t _sLen = sizeof(sockAddr);
                    _dataLen = ::recvfrom( hSo, _buffer, UdpSocketReadBufferSize, 0,
                        (struct sockaddr *)&sockAddr, &_sLen);
                    if ( _dataLen > 0 ) {
                        _result.Append( _buffer, _dataLen );
                    }
                } while( _dataLen < UdpSocketReadBufferSize && _dataLen >= 0 );

                return _result;
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
