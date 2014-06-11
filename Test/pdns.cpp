
#include <Plib-Generic/Generic.hpp>
#include <Plib-Utility/Utility.hpp>
#include <Plib-Text/Text.hpp>
#include <Plib-Threading/Threading.hpp>
#include <Plib-Network/Network.hpp>
#include <poll.h>

using namespace Plib;
using namespace Plib::Generic;
using namespace Plib::Text;
using namespace Plib::Threading;
using namespace Plib::Network;
using namespace Plib::Utility;
using namespace std;

#pragma pack(push, 1)
struct dnsPackage {
    Uint16          transactionId;
    struct {
        Uint8       qr:1;
        Uint8       opcode:4;
        Uint8       aa:1;
        Uint8       tc:1;
        Uint8       rd:1;
        Uint8       ra:1;
        Uint8       z:3;
        Uint8       rcode:4;
    }               flags;
    Uint16          qdCount;
    Uint16          anCount;
    Uint16          nsCount;
    Uint16          arCount;
};
#pragma pack(pop)

SOCKET_T            gSvrSockTcp;
SOCKET_T            gSvrSockUdp;

bool isDomainInBlacklist( Array<String> domainComs )
{

	return false;
}

void getTcpConnection()
{
    TcpSocket _ssvr(gSvrSockTcp);
    _ssvr.SetReUsable(true);
    struct pollfd *_svrfd = (struct pollfd *)calloc(1, sizeof(struct pollfd));
    size_t _nfds = 1;   // number of fd
    _svrfd->events = POLLIN | POLLPRI;
    _svrfd->fd = gSvrSockTcp;
    int _ret = 0;
    while ( ThreadSys::Running() ) {
        _ret = poll( _svrfd, _nfds, 100 );
        if ( _ret == -1 ) {
            PERROR( "Server connection has been dropped." );
            break;
        }

        if ( _svrfd->revents == 0 ) {
            // No incoming socket
            continue;
        }
        if ( _svrfd->revents & POLLIN ) {
            PINFO("New incoming socket.");
            struct sockaddr_in _sockInfoClient;
            int _len = 0;
            SOCKET_T _clt = accept(gSvrSockTcp, (struct sockaddr *)&_sockInfoClient, (socklen_t *)&_len);
            PINFO("client socket: " << _clt);
            if ( _clt == -1 ) continue;
            TcpSocket _sclt(_clt);
            _sclt.SetReUsable(true);
            PINFO("clinet info: " << _sclt.RemotePeerInfo() );
            // Try to read the package...
            NData _data = _sclt.Read();
            PrintAsHex(_data);

            struct dnsPackage _dnsPkg;
            const char * _buffer = _data.c_str();
            memcpy(&_dnsPkg, _buffer, sizeof(struct dnsPackage));
            String _domain;
            const char *_pDomain = _buffer + sizeof(struct dnsPackage);
            for ( ;; ) {
                Int8 _l = 0;
                memcpy(&_l, _pDomain, sizeof(Int8));
                //_l = ntohs(_l);
                if ( _l == 0 ) {
                    break;
                }
                _pDomain += sizeof(Int8);
                if ( _domain.Size() > 0 ) {
                    _domain.Append('.');
                }
                _domain.Append(_pDomain, _l);
                _pDomain += _l;
            }
            PINFO("Is about lookup domain: " << _domain);

            _sclt.Close();
            // Redirect...
            // Write back...
        }
    }
    PINFO("Will stop tcp connection worker");
    _ssvr.Close();
}

void getUdpConnection()
{
    UdpSocket _ssvr(gSvrSockUdp);
    _ssvr.SetReadTimeOut( 100 );
    char _buffer[1024];
    int _bLen = 1024;
    while ( ThreadSys::Running() ) {
        struct sockaddr_in _sockAddr;
        socklen_t _sLen = sizeof(_sockAddr);
        int _dataLen = ::recvfrom( gSvrSockUdp, _buffer, _bLen, 0,
            (struct sockaddr *)&_sockAddr, &_sLen);
        //PINFO("Data Len: " << _dataLen);
        if ( _dataLen <= 0 ) continue;
        _buffer[_dataLen] = '\0';
        String _address = String::Parse("%u.%u.%u.%u",
            (unsigned int)(_sockAddr.sin_addr.s_addr >> (0 * 8)) & 0x00FF,
            (unsigned int)(_sockAddr.sin_addr.s_addr >> (1 * 8)) & 0x00FF,
            (unsigned int)(_sockAddr.sin_addr.s_addr >> (2 * 8)) & 0x00FF,
            (unsigned int)(_sockAddr.sin_addr.s_addr >> (3 * 8)) & 0x00FF );
        PINFO("Incoming udp data..." << _address << ":" << ntohs(_sockAddr.sin_port));
        PrintAsHex( _buffer, _dataLen );

        struct dnsPackage _dnsPkg;
        memcpy(&_dnsPkg, _buffer, sizeof(struct dnsPackage));
        String _domain;
        char *_pDomain = _buffer + sizeof(struct dnsPackage);
        for ( ;; ) {
            Int8 _l = 0;
            memcpy(&_l, _pDomain, sizeof(Int8));
            //_l = ntohs(_l);
            if ( _l == 0 ) {
                break;
            }
            _pDomain += sizeof(Int8);
            if ( _domain.Size() > 0 ) {
                _domain.Append('.');
            }
            _domain.Append(_pDomain, _l);
            _pDomain += _l;
        }
        PINFO("Is about lookup domain: " << _domain);

        ::sendto(gSvrSockUdp, _buffer, _dataLen, 0, (struct sockaddr *)&_sockAddr, _sLen);
    }
    PINFO("Will stop udp connection worker");
    _ssvr.Close();
}

int main( int argc, char * argv[] ) {

	// Try to parse the command line argument
	String _blackListFilePath = "~/.pdns.baacklist";
	String _localParentDns = "202.96.209.5";
	String _blackParentDns = "8.8.8.8";

	if ( argc >= 2 ) {

	}

    // Wait for exit
    SetSignalHandle();

    Uint32 _svrPort = 53;
    // Start to build the socket for both tcp and udp
    struct sockaddr_in _sockAddrTcp, _sockAddrUdp;

    // Create Socket
    gSvrSockTcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //cout << gSvrSockTcp << endl;
    gSvrSockUdp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //cout << gSvrSockUdp << endl;

    memset((char *)&_sockAddrTcp, 0, sizeof(_sockAddrTcp));
    memset((char *)&_sockAddrUdp, 0, sizeof(_sockAddrUdp));

    _sockAddrUdp.sin_family = AF_INET;
    _sockAddrUdp.sin_port = htons(_svrPort);
    _sockAddrUdp.sin_addr.s_addr = htonl(INADDR_ANY);
    if ( bind(gSvrSockUdp, (struct sockaddr *)&_sockAddrUdp, sizeof(_sockAddrUdp)) == -1 ) {
        cout << "failed to bind udp" << endl;
    }

    _sockAddrTcp.sin_family = AF_INET;
    _sockAddrTcp.sin_port = htons(_svrPort);
    _sockAddrTcp.sin_addr.s_addr = htonl(INADDR_ANY);
    if ( bind(gSvrSockTcp, (struct sockaddr *)&_sockAddrTcp, sizeof(_sockAddrTcp)) == -1 ) {
        cout << "failed to bind tcp" << endl;
        return 1;
    }
    if ( -1 == listen(gSvrSockTcp, 100) ) {
        cout << "failed to listen tcp" << endl;
        return 2;
    }

    // Start listen thread
    Thread<void ()> _tcpWorker = Thread<void()>(getTcpConnection);
    _tcpWorker.Start();

    Thread<void ()> _udpWorker = Thread<void()>(getUdpConnection);
    _udpWorker.Start();

    WaitForExitSignal();
    _tcpWorker.Stop();
    _udpWorker.Stop();
    return 0;
}
