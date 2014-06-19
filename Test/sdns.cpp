
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
Array<String>       gLocalDnsServer;

// Udp Worker
struct TTcpDnsRequest {
    String                  queryDomain;
    NData                   queryData;
    TcpSocket               *clientSocket;
};
Queue< TTcpDnsRequest >     gTcpReqQueue;
Semaphore                   gTcpSem;
Mutex                       gTcpMutex;

void tcpRedirectWorker()
{
    while ( ThreadSys::Running() ) {
        if ( gTcpSem.Get(100) == false ) continue;
        gTcpMutex.Lock();
        TTcpDnsRequest _dnsReq = gTcpReqQueue.Head();
        gTcpReqQueue.Pop();
        gTcpMutex.UnLock();

        for ( int i = 0; i < gLocalDnsServer.Size(); ++i ) {
            UdpSocket _udpSock;
            PeerInfo _dnsServerPeer;
            _dnsServerPeer.Address = gLocalDnsServer[i];
            _dnsServerPeer.Port = 53;
            _dnsServerPeer.ConnectTimeOut = 500;
            PIF ( _udpSock.Connect( _dnsServerPeer ) == false ) continue;
            PIF ( _udpSock.Write( _dnsReq.queryData ) == false ) continue;
            NData _result = _udpSock.Read();
            PIF ( _result == NData::Null ) continue;
			PrintAsHex(_result);
            _dnsReq.clientSocket->Write( _result );
            delete _dnsReq.clientSocket;
            break;
        }
    }
}

String getDomainFromRequestData( NData queryData ) 
{
    struct dnsPackage _dnsPkg;
    const char * _buffer = queryData.c_str();
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
    return _domain;
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

    // Start tcp redirect workers
    gTcpSem.Init(0);
    Array< Thread< void( ) > * > _workList;
    for ( int i = 0; i < 20; ++i ) {
        Thread<void()> *_pRedirectWorker = new Thread<void()>(tcpRedirectWorker);
        _workList.PushBack( _pRedirectWorker );
        _pRedirectWorker->Start();
    }

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
            // PINFO("New incoming socket.");
            struct sockaddr_in _sockInfoClient;
            int _len = 0;
            SOCKET_T _clt = accept(gSvrSockTcp, (struct sockaddr *)&_sockInfoClient, (socklen_t *)&_len);
            // PINFO("client socket: " << _clt);
            if ( _clt == -1 ) continue;
            TcpSocket *_sclt = new TcpSocket(_clt);
            _sclt->SetReUsable(true);
            // PINFO("clinet info: " << _sclt.RemotePeerInfo() );
            // Try to read the package...
            NData _queryData = _sclt->Read();
            PrintAsHex(_queryData);
            String _domain = getDomainFromRequestData( _queryData );
            //bool _needProxy = isDomainInBlacklist( _domain );

            TTcpDnsRequest _dnsReq = { _domain, _queryData, _sclt };
            gTcpMutex.Lock();
            gTcpReqQueue.Push( _dnsReq );
            gTcpSem.Release();
            gTcpMutex.UnLock();
        }
    }
    PINFO("Will stop tcp connection worker");
    _ssvr.Close();
    for ( int i = 0; i < _workList.Size(); ++i ) {
        //PINFO("Will stop redirect worker: " << i);
        _workList[i]->Stop();
        //PINFO("Did stop redirect worker: " << i);
        delete _workList[i];
    }
}

void pdnsVersionInfo()
{
    printf( "sdns\tversion 1.0\n" );
}

void pdnsHelpInfo()
{
    printf( "pdns -h|--help\n" );
    printf( "pdns -v|--version\n" );
    printf( "pdns [-ld|--localdns <dns>] [-p|--port port]\n");
}

int main( int argc, char * argv[] ) {

    // Try to parse the command line argument
    String _localParentDns = "8.8.8.8";
    String _tcpServerPort = "1025";

    if ( argc >= 2 ) {
        int _arg = 1;
        for ( ; _arg < argc; ++_arg ) {
            String _command = argv[_arg];
            if ( _command == "-h" || _command == "--help" ) {
                // Help
                pdnsHelpInfo();
                return 0;
            }
            if ( _command == "-v" || _command == "--version" ) {
                // Version
                pdnsVersionInfo();
                return 0;
            }
            if ( _command == "-ld" || _command == "--localdns" ) {
                if ( _arg + 1 < argc ) {
                    _localParentDns = argv[++_arg];
                } else {
                    cerr << "Invalidate argument: " << _command << ", missing parameter." << endl;
                    return 1;
                }
                continue;
            }
            if ( _command == "-p" || _command == "--port" ) {
                if ( _arg + 1 < argc ) {
                    _tcpServerPort = argv[++_arg];
                } else {
                    cerr << "Invalidate argument: " << _command << ", missing parameter." << endl;
                    return 1;
                }
                continue;
            }
            cerr << "Invalidate argument: " << _command << "." << endl;
            return 1;
        }
    }

    // Add to global cache.
    gLocalDnsServer.PushBack(_localParentDns);

    // Wait for exit
    SetSignalHandle();

    Uint32 _svrPort = _tcpServerPort.UintValue();
    // Start to build the socket for both tcp and udp
    struct sockaddr_in _sockAddrTcp;

    // Create Socket
    gSvrSockTcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //cout << gSvrSockTcp << endl;

    memset((char *)&_sockAddrTcp, 0, sizeof(_sockAddrTcp));

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

    WaitForExitSignal();
    _tcpWorker.Stop();
    //PINFO("Tcp worker stopped");
    exit(0);
    return 0;
}
