
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

void getTcpConnection()
{
    TcpSocket _ssvr(gSvrSockTcp);
    _ssvr.SetReUsable(true);
    struct pollfd _svrfd;
    while ( ThreadSys::Running() ) {

        if ( _ssvr.isReadable(100) == false ) {
            //PDEBUG( "no connection..." );
            continue;
        }
        PINFO("new incoming connection via tcp.");
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
        _sclt.Close();
        // Redirect...
        // Write back...
    }
}

void getUdpConnection()
{

}

int main( int argc, char * argv[] ) {
    // Wait for exit
    SetSignalHandle();

    Uint32 _svrPort = 1053;
    // Start to build the socket for both tcp and udp
    struct sockaddr_in _sockAddrTcp, _sockAddrUdp;

    // Create Socket
    gSvrSockTcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    cout << gSvrSockTcp << endl;
    gSvrSockUdp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    cout << gSvrSockUdp << endl;

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
    }
    if ( -1 == listen(gSvrSockTcp, 100) ) {
        cout << "failed to listen tcp" << endl;
    }

    // Start listen thread
    Thread<void ()> _tcpWorker = Thread<void()>(getTcpConnection);
    _tcpWorker.Start();

    WaitForExitSignal();
    _tcpWorker.Stop();
    return 0;
}