/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: Request.hpp
* Propose  			: Basic Request Interface
* 
* Current Version	: 1.0
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2013-02-17
*/

#pragma once

#ifndef _PLIB_NETWORK_REQUEST_HPP_
#define _PLIB_NETWORK_REQUEST_HPP_

#include <Plib-Network/BasicNetwork.hpp>
#include <Plib-Network/PeerInfo.hpp>
#include <Plib-Network/Response.hpp>
#include <Plib-Generic/Generic.hpp>

namespace Plib
{
	namespace Network
	{
		using Plib::Text::String;
		using Plib::Generic::Enable_If;
		using Plib::Generic::Is_Convertible;

		// Request Interface, which is an ABC
		class IRequest
		{
		protected:
			PeerInfo					m_peerInfo;
			NData						m_packageBuffer;
			String 						m_identify;

			// Connection Info.
			ISocket						*m_socket;
			IResponse					*m_response;
		public:
			IRequest( )
				: Identify(m_identify)
			{
				// Nothing
			}
			IRequest( const String & host, Uint32 port, Uint32 timeout = 1000 ) 
				: Identify(m_identify)
			{
				m_peerInfo.Address.DeepCopy(host);
				m_peerInfo.Port = port;
				m_peerInfo.ConnectTimeOut = timeout; 
			}
			IRequest( const PeerInfo & peer ) : Identify(m_identify) 
			{ m_peerInfo = peer; }

			virtual ~IRequest( ) {
			};
		public:
			// Before the connection try to send the request, 
			// will invoke this method to connect to the peer.
			virtual bool InitializeConnector() = 0;

			// Before the connection try to receive response data,
			// will invoke this method to initialize the response
			// object.
			virtual bool InitializeResponser() = 0;
		public:

			// Get the request's peer infomation
			const PeerInfo & RequestPeerInfo( ) const { return m_peerInfo; }
			// Get the connection socket
			ISocket * Connector( )const { return m_socket; }
			// Get the responser object
			IResponse * Responser( )const { return m_response; }
		public:
			// Public Properties
			const String &				Identify;
			void SetIdentify( const String & identify ) {
				this->m_identify.DeepCopy( identify );
			}
		public:

			// After setting properties of the specified type request.
			// System will invoke generateFullPackage to get write data.
			virtual const NData & generateFullPackage( ) = 0;
		};

		// Referencable Request Object.
		// The [Connection] will use this shell object to store the request
		class Request
		{
		public:
			typedef Plib::Generic::Delegate< void (void *) >		TDeallactor;
		private:
			static void _delegateForFree( void * pVoid ) {
				delete (IRequest *)pVoid;
			}
		private:
			struct _TInnerHandler {
				IRequest			*m_reqHandler;	// The stored request handler
				Uint32				m_count;
				TDeallactor 		m_deallactor;
				PLIB_THREAD_SAFE_DEFINE;

				_TInnerHandler() : m_reqHandler(NULL), m_count(0) {
					CONSTRUCTURE;
				}
				~_TInnerHandler() {
					DESTRUCTURE;
					if ( m_reqHandler != NULL ) {
						m_deallactor( m_reqHandler );
					}
				}

				void increase() {
					PLIB_THREAD_SAFE;
					SELF_INCREASE(m_count); 
				}
				void decrease() {
					PLIB_THREAD_SAFE;
					if ( SELF_DECREASE(m_count) == 0 ) {
						PDELETE( this );
					}
				}
			};
		private:
			_TInnerHandler *_refHandler;

			// Private, cannot be used.
			Request() : _refHandler(NULL){}
		public:
			// Default Constructure
			Request( IRequest *pReq ) {
				CONSTRUCTURE;
				PNEW(_TInnerHandler, _refHandler);
				_refHandler->m_reqHandler = pReq;
				_refHandler->m_count = 1;
				// Default, use [free] to release the handler.
				_refHandler->m_deallactor = TDeallactor(&Request::_delegateForFree);
			}
			Request( IRequest *pReq, TDeallactor deallactor ) {
				CONSTRUCTURE;
				PNEW(_TInnerHandler, _refHandler);
				_refHandler->m_reqHandler = pReq;
				_refHandler->m_count = 1;
				_refHandler->m_deallactor = deallactor;
			}
			// D'str
			~Request() {
				DESTRUCTURE;
				if ( _refHandler == NULL ) return;
				_refHandler->decrease();
			}
			// Copy C'str
			Request( const Request & rhs ) : _refHandler(NULL) {
				CONSTRUCTURE;
				if ( rhs._refHandler == NULL ) return;
				_refHandler = rhs._refHandler;
				_refHandler->increase();
			}
			// Operator =
			Request & operator = ( const Request & rhs ) {
				if ( this == &rhs || _refHandler == rhs._refHandler ) return *this;
				if ( _refHandler != NULL ) _refHandler->decrease( );
				_refHandler = rhs._refHandler;
				if ( _refHandler != NULL ) _refHandler->increase( );
				return *this;
			}

			// Compare. 
			// To use Reference, the Internal Object must support 
			// operator == and operator != at least.
			// the compare will check the handle inside at first
			// time, if they are not the same, then check the 
			// internal object.
			INLINE bool operator == ( const Request & rhs ) const
			{
				if ( _refHandler == rhs._refHandler ) return true;
				if ( _refHandler == NULL || rhs._refHandler == NULL ) return false;
				return _refHandler->m_reqHandler == rhs._refHandler->m_reqHandler;
			}
			
			INLINE bool operator == ( const IRequest *pReq ) const
			{
				return ( _refHandler == NULL ) ? false : _refHandler->m_reqHandler == pReq;
			}
			
			INLINE bool operator != ( const Request & rhs ) const
			{
				return !(*this == rhs);
			}
			
			INLINE bool operator != ( const IRequest *pReq ) const
			{
				return !(*this == pReq);
			}
			
			// Point Simulation.
			INLINE IRequest * operator -> ( )
			{
				return _refHandler->m_reqHandler;
			}
			INLINE const IRequest * operator -> ( ) const
			{
				return _refHandler->m_reqHandler;
			}

			// Cast Operator.
			template<typename T> operator T *()
			{
				return static_cast<
					typename Enable_If< 
						Is_Convertible<T*, IRequest*>::value,
						T
					>::type*
				>(_refHandler->m_reqHandler);
			}
			template<typename T> operator const T *() const
			{
				return static_cast<
					typename Enable_If< 
						Is_Convertible<T*, IRequest*>::value,
						T
					>::type*
				>(_refHandler->m_reqHandler);
			}

			// Reference Check Statue.
			INLINE bool RefNull( ) const {
				return _refHandler == NULL;
			}			
		};
	}
}


#endif // plib.network.request.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
