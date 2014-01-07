// Request Test 
// 

#include <Plib-Generic/Generic.hpp>
#include <Plib-Utility/Utility.hpp>
#include <Plib-Text/Text.hpp>
#include <Plib-Threading/Threading.hpp>
#include <Plib-Network/Network.hpp>
//#include <Plib-Network/ConnectInfo.hpp>
//#include <Plib-Network/Syncsock.hpp>

using namespace Plib;
using namespace Plib::Generic;
using namespace Plib::Text;
using namespace Plib::Threading;
using namespace Plib::Network;
using namespace Plib::Utility;
using namespace std;

class NcResponse : public IResponse
{
public:
	virtual void generateResponseObjectFromPackage( const NData & receiveData )
	{

	}
};
class NcRequest : public IRequest
{
public:
	// C'str, set the time out
	NcRequest( ) 
	{
		m_peerInfo.ConnectTimeOut = 30000;
		m_socket = NULL;
		m_response = NULL;
	}
	NcRequest( const String & address, Uint32 port ) : IRequest( address, port )
	{
		m_peerInfo.ConnectTimeOut = 30000;
	}
	virtual ~NcRequest() {
		// Should release the memory alloced in initialize.
		this->DestroyResponser();
		this->DestroyConnector();
	}

	void SetConnectInfo( const String & address, Uint32 port )
	{
		m_peerInfo.Address.DeepCopy( address );
		m_peerInfo.Port = port;
	}
	void AppendBody( const String & body )
	{
		m_packageBuffer.Append( body );
	}
	const NData & generateFullPackage( )
	{
		return m_packageBuffer;
	}

	// Initialize methods
	virtual bool InitializeConnector() {
		PIF ( this->m_socket == NULL ) {
			this->m_socket = new TcpSocket();
		}
		printf("%ld\n", this->m_socket->m_hSocket);
		printf("%ld\n", INVALIDATE_SOCKET);
		PIF ( this->m_socket->isConnected() == false ) {
			PDUMP(this->m_peerInfo);
			this->m_socket->Connect( this->m_peerInfo );
		}
		PIF ( this->m_socket->isConnected() == false ) {
			this->DestroyConnector();
			return false;
		}
		return true;
	}
	void DestroyConnector() {
		if ( this->m_socket != NULL ) {
			this->m_socket->Close();
			delete this->m_socket;
			this->m_socket = NULL;
		}
	}
	virtual bool InitializeResponser() {
		if ( this->m_response == NULL ) {
			this->m_response = new NcResponse();
		}
		return this->m_response != NULL;
	}
	void DestroyResponser() {
		if ( this->m_response != NULL ) {
			delete this->m_response;
			this->m_response = NULL;
		}
	}
};

int main ( int argc, char * argv[] )
{
	Plib::Threading::SetSignalHandle();

	String _string = "GET /index.html HTTP/1.1\r\nHOST : tech.pushchen.com\r\n\r\n";
	//cout << _string << endl;
	PINFO( Plib::Network::LocalHostname() );
	PINFO( Identify::New() );

	cout << boolalpha;
	cout << is_convertible< IRequest *, IRequest * >::value << endl;
	cout << is_convertible< NcRequest *, IRequest * >::value << endl;
	// What I want to write:
	/*
	SomeRequest _req(ServerAddress, ServerPort);
	_req.ParameterOne = ValueOne;
	_req.ParameterTwo = ValueTwo;
	_req.Set(Key1, Value1);
	_req.Append(PackageBody);
	_req.Identify = TIMESTAMP();
	Plib::Network::Connection::Success onSuccess( somefunction );

	Plib::Network::Connection::Async( _req, onSuccess, global_onFailed );
	*/
	if ( 1 ) {
		Request _req( new NcRequest );
		((NcRequest *)_req)->SetConnectInfo( "pushchen.com", 80 );
		((NcRequest *)_req)->AppendBody( _string );
		Connection::sendAsyncRequest( _req );
	}

	// PeerInfo remotePeer;
	// remotePeer.Address = "127.0.0.1";
	// remotePeer.Port = 9090;
	// //PINFO( localPeer );
	// PDUMP( remotePeer );
/*
	{
		TcpSocket so;
		PIF ( so.Connect(_ncReq.RequestPeerInfo()) ) {
			PDUMP( so );
			so.Write( _ncReq.generateFullPackage() );
			NData resp = so.Read( 5000, false, 1600 );
			PIF ( !resp.RefNull() ) {
				PDUMP( resp.Size() );
				// PrintAsHex( resp );
				PDUMP( resp );
			} else {
				so.Close();
			}
		}
	}
*/
	Plib::Threading::WaitForExitSignal();
	return 0;
}
