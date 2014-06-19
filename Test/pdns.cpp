
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

// Black List Node
struct TBlackListNode;
typedef pair< String, TBlackListNode * >    TBlackItem;
typedef map< String, TBlackListNode * >     TComKey;      
struct TBlackListNode
{
    Array<TBlackItem>               prefixList;
    Array<TBlackItem>               suffixList;
    Array<TBlackItem>               containList;
    TComKey                         keys;
    TBlackListNode                  *everything;
    operator bool() const 
    {
        return prefixList.Size() != 0 || suffixList.Size() != 0 || 
            containList.Size() != 0 || keys.size() != 0 || everything != NULL;
    }
    TBlackListNode() : everything(NULL) {}
};

// Global Black List Root Node.
TBlackListNode       *gBlRoot = NULL;

void addPatternToBlackList( String pattern ) 
{
    if ( gBlRoot == NULL ) {
        gBlRoot = new TBlackListNode;
    }
    TBlackListNode *_blNode = gBlRoot;
    Array<String> _components = pattern.Split(".");
    for ( int i = _components.Size() - 1; i >= 0; --i ) {
        String _com = _components[i];
        //PINFO("Parsing component: " << _com);
        bool _beginWithPattern = _com[0] == '*';
        bool _endWithPattern = _com[_com.Size() - 1] == '*';
        if ( _com == "*" ) {
            //PINFO( _com << " stands for everything..." );
            _blNode->everything = new TBlackListNode;
            _blNode = _blNode->everything;
            continue;
        }
        if ( _beginWithPattern == false && _endWithPattern == false ) {
            //PINFO(_com << " is a static key");
            if ( _blNode->keys.find(_com) == _blNode->keys.end() ) {
                _blNode->keys[_com] = new TBlackListNode;
            } 
            _blNode = _blNode->keys[_com];
        } else if ( _beginWithPattern == true && _endWithPattern == true ) {
            //PINFO(_com << " has both patten.");
            bool _hasSameKey = false;
            String _ckey = _com.SubString(1, _com.Size() - 2);
            //PINFO("static part is: " << _ckey);
            for ( int c = 0; c < _blNode->containList.Size(); ++c ) {
                TBlackItem _item = _blNode->containList[c];
                if ( _item.first == _ckey ) {
                    _hasSameKey = true;
                    _blNode = _item.second;
                    break;
                }
            }
            if ( !_hasSameKey ) {
                TBlackListNode *_nextLevelNode = new TBlackListNode;
                TBlackItem _newItem = make_pair(_ckey, _nextLevelNode);
                _blNode->containList.PushBack(_newItem);
                _blNode = _nextLevelNode;
            }
        } else if ( _beginWithPattern == true && _endWithPattern == false ) {
            //PINFO(_com << " is a suffix.");
            bool _hasSameKey = false;
            String _ckey = _com.SubString(1);
            //PINFO("static part is: " << _ckey);
            for ( int c = 0; c < _blNode->suffixList.Size(); ++c ) {
                TBlackItem _item = _blNode->suffixList[c];
                if ( _item.first == _ckey ) {
                    _hasSameKey = true;
                    _blNode = _item.second;
                    break;
                }
            }
            if ( !_hasSameKey ) {
                TBlackListNode *_nextLevelNode = new TBlackListNode;
                TBlackItem _newItem = make_pair(_ckey, _nextLevelNode);
                _blNode->suffixList.PushBack(_newItem);
                _blNode = _nextLevelNode;
            }
        } else if ( _beginWithPattern == false && _endWithPattern == true ) {
            //PINFO(_com << " is a prefix.");
            bool _hasSameKey = false;
            String _ckey = _com.SubString(0, _com.Size() - 1);
            //PINFO("static part is: " << _ckey);
            for ( int c = 0; c < _blNode->prefixList.Size(); ++c ) {
                TBlackItem _item = _blNode->prefixList[c];
                if ( _item.first == _ckey ) {
                    _hasSameKey = true;
                    _blNode = _item.second;
                    break;
                }
            }
            if ( !_hasSameKey ) {
                TBlackListNode *_nextLevelNode = new TBlackListNode;
                TBlackItem _newItem = make_pair(_ckey, _nextLevelNode);
                _blNode->prefixList.PushBack(_newItem);
                _blNode = _nextLevelNode;
            }
        }
    }
}

bool _isDomainInBlacklist( String domain, TBlackListNode *blNode )
{
    if ( blNode == NULL && domain.Size() == 0 ) return true;
    if ( blNode == NULL ) return false;
    if ( blNode->everything ) return true;
    if ( blNode->containList.Size() ) {
        for ( int _ctnt = 0; _ctnt < blNode->containList.Size(); ++_ctnt ) {
            if ( domain.Find(blNode->containList[_ctnt].first) != String::NoPos ) return true;
        }
    }
    if ( blNode->suffixList.Size() ) {
        for ( int _sl = 0; _sl < blNode->suffixList.Size(); ++_sl ) {
            if ( domain.EndWith(blNode->suffixList[_sl].first) ) return true;
        }
    }
    if ( blNode->prefixList.Size() ) {
        for ( int _pl = 0; _pl < blNode->prefixList.Size(); ++_pl ) {
            if ( domain.StartWith(blNode->prefixList[_pl].first) ) return true;
        }
    }
    if ( blNode->keys.size() ) {
        String _com, _leftDomain;
        String::Size_T _lastDot = domain.FindLast('.');
        if ( _lastDot == String::NoPos ) {
            _com = domain;
        } else {
            _com = domain.SubString(_lastDot + 1);
            _leftDomain = domain.SubString(0, _lastDot);
        }
        //PINFO(_leftDomain);
        TComKey::iterator _it = blNode->keys.find(_com);
        if ( _it == blNode->keys.end() ) return false;
        return _isDomainInBlacklist( _leftDomain, _it->second );
    }
    return false;
}

bool isDomainInBlacklist( String domain )
{
    return _isDomainInBlacklist( domain, gBlRoot );
}

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
Array<String>       gLocalDnsServer;
Array<String>       gBlackDnsServer;
Uint32              gBlackDnsPort;

// Udp Worker
struct TUdpDnsRequest {
    String                  queryDomain;
    NData                   queryData;
    struct sockaddr_in      clientAddr;
};
Queue< TUdpDnsRequest >     gUdpReqQueue;
Semaphore                   gUdpSem;
Mutex                       gUdpMutex;

void udpRedirectWorker()
{
    while ( ThreadSys::Running() ) {
        if ( gUdpSem.Get(100) == false ) continue;
        gUdpMutex.Lock();
        TUdpDnsRequest _dnsReq = gUdpReqQueue.Head();
        gUdpReqQueue.Pop();
        gUdpMutex.UnLock();

        bool _needProxy = isDomainInBlacklist( _dnsReq.queryDomain );
        // PINFO("query domain<" << _dnsReq.queryDomain << "> " << 
        //     (_needProxy ? "need proxy" : "direct") );
        NData _result = NData::Null;

        if ( _needProxy == false ) {
            for ( int i = 0; i < gLocalDnsServer.Size(); ++i ) {
                UdpSocket _udpSock;
                PeerInfo _dnsServerPeer;
                _dnsServerPeer.Address = gLocalDnsServer[i];
                _dnsServerPeer.Port = 53;
                _dnsServerPeer.ConnectTimeOut = 500;
                if ( _udpSock.Connect( _dnsServerPeer ) == false ) continue;
                if ( _udpSock.Write( _dnsReq.queryData ) == false ) continue;
                _result = _udpSock.Read();
                if ( _result == NData::Null ) continue;
                socklen_t _sLen = sizeof(_dnsReq.clientAddr);
                //PrintAsHex(_result);
                ::sendto( gSvrSockUdp, _result.c_str(), _result.size(), 
                    0, (struct sockaddr *)&_dnsReq.clientAddr, _sLen);
                break;
            }
        } else {
            // Redirect to tcp request
            for ( int i = 0; i < gBlackDnsServer.Size(); ++i ) {
                TcpSocket _tcpSock;
                PeerInfo _dnsServerPeer;
                _dnsServerPeer.Address = gBlackDnsServer[i];
                _dnsServerPeer.Port = gBlackDnsPort;
                _dnsServerPeer.ConnectTimeOut = 1000;
				PINFO("Redirect to server: " << _dnsServerPeer);
                if ( _tcpSock.Connect( _dnsServerPeer ) == false ) continue;
                if ( _tcpSock.Write( _dnsReq.queryData ) == false ) continue;
                _result = _tcpSock.Read();
                if ( _result == NData::Null ) continue;
                socklen_t _sLen = sizeof(_dnsReq.clientAddr);
                ::sendto( gSvrSockUdp, _result.c_str(), _result.size(), 
                    0, (struct sockaddr *)&_dnsReq.clientAddr, _sLen);
                break;
            }
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
            TcpSocket _sclt(_clt);
            _sclt.SetReUsable(true);
            // PINFO("clinet info: " << _sclt.RemotePeerInfo() );
            // Try to read the package...
            NData _data = _sclt.Read();
            // PrintAsHex(_data);
            String _domain = getDomainFromRequestData( _data );
            //bool _needProxy = isDomainInBlacklist( _domain );

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

    // Start udp redirect workers
    gUdpSem.Init(0);
    Array< Thread< void( ) > * > _workList;
    for ( int i = 0; i < 20; ++i ) {
        Thread<void()> *_pRedirectWorker = new Thread<void()>(udpRedirectWorker);
        _workList.PushBack( _pRedirectWorker );
        _pRedirectWorker->Start();
    }
    while ( ThreadSys::Running() ) {
        struct sockaddr_in _sockAddr;
        socklen_t _sLen = sizeof(_sockAddr);
        int _dataLen = ::recvfrom( gSvrSockUdp, _buffer, _bLen, 0,
            (struct sockaddr *)&_sockAddr, &_sLen);
        //PINFO("Data Len: " << _dataLen);
        if ( _dataLen <= 0 ) continue;
        _buffer[_dataLen] = '\0';
        // String _address = String::Parse("%u.%u.%u.%u",
        //     (unsigned int)(_sockAddr.sin_addr.s_addr >> (0 * 8)) & 0x00FF,
        //     (unsigned int)(_sockAddr.sin_addr.s_addr >> (1 * 8)) & 0x00FF,
        //     (unsigned int)(_sockAddr.sin_addr.s_addr >> (2 * 8)) & 0x00FF,
        //     (unsigned int)(_sockAddr.sin_addr.s_addr >> (3 * 8)) & 0x00FF );
        // PINFO("Incoming udp data..." << _address << ":" << ntohs(_sockAddr.sin_port));
        PrintAsHex( _buffer, _dataLen );
        NData _queryData = NData( _buffer, _dataLen );
        String _domain = getDomainFromRequestData( _queryData );

        // Todo
        TUdpDnsRequest _dnsReq = { _domain, _queryData, _sockAddr };
        gUdpMutex.Lock();
        gUdpReqQueue.Push( _dnsReq );
        gUdpSem.Release();
        gUdpMutex.UnLock();
    }
    PINFO("Will stop udp connection worker");
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
    printf( "pdns\tversion 1.0\n" );
}

void pdnsHelpInfo()
{
    printf( "pdns -h|--help\n" );
    printf( "pdns -v|--version\n" );
    printf( "pdns [-b|--blacklist <path>] [-ld|--localdns <dns>] [-bd|--blackdns <dns>] [-bp|--blackport <port>]\n");
}

int main( int argc, char * argv[] ) {

	// Try to parse the command line argument
	String _blackListFilePath = "~/.pdns.blacklist";
	String _localParentDns = "202.96.209.133";
	String _blackParentDns = "66.175.221.214";
	gBlackDnsPort = 1025;

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
            if ( _command == "-b" || _command == "--blacklist" ) {
                if ( _arg + 1 < argc ) {
                    _blackListFilePath = argv[++_arg];
                } else {
                    cerr << "Invalidate argument: " << _command << ", missing parameter." << endl;
                    return 1;
                }
                continue;
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
            if ( _command == "-bd" || _command == "--blackdns" ) {
                if ( _arg + 1 < argc ) {
                    _blackParentDns = argv[++_arg];
                } else {
                    cerr << "Invalidate argument: " << _command << ", missing parameter." << endl;
                    return 1;
                }
                continue;
            }
            if ( _command == "-bp" || _command == "--blackport" ) {
                if ( _arg + 1 < argc ) {
                    gBlackDnsPort = String(argv[++_arg]).UintValue();
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
    gBlackDnsServer.PushBack(_blackParentDns);

    ReadStream _blStream(_blackListFilePath);
    if ( !_blStream ) {
        cerr << "Failed to open black list file at path: " << _blackListFilePath << endl;
        cerr << "All dns query will be route to the local dns server." << endl;
    } else {
        String _configLine = _blStream.ReadLine();
        for ( ; _configLine != String::Null; _configLine = _blStream.ReadLine() ) {
            //cout << _configLine << endl;
            // Try to build the search tree.
            addPatternToBlackList( _configLine );
        }
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
    //PINFO("Tcp worker stopped");
    _udpWorker.Stop();
    //PINFO("Udp worker stopped");
    exit(0);
    return 0;
}
