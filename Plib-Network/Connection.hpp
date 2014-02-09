/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: Connection.hpp
* Propose  			: Connection Maintainer Framework. 
* 
* Current Version	: 1.0
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2013-02-17
*/

#pragma once

#ifndef _PLIB_NETWORK_CONNECTION_HPP_
#define _PLIB_NETWORK_CONNECTION_HPP_

#include <Plib-Network/BasicNetwork.hpp>
#include <Plib-Network/PeerInfo.hpp>
#include <Plib-Network/Socket.hpp>
#include <Plib-Network/Request.hpp>
#include <Plib-Network/Response.hpp>

#include <Plib-Threading/Thread.hpp>

namespace Plib
{
	namespace Network
	{

		#define __REF			Plib::Generic::Reference
		#define __DELG			Plib::Generic::Delegate
		using Plib::Text::String;
		using Plib::Threading::Mutex;
		using Plib::Threading::Semaphore;
		// using std::enable_if;
		// using std::is_convertible;

		// Typedef
		typedef Plib::Generic::Delegate< void ( const Request & ) > tReqSuccess;
		typedef Plib::Generic::Delegate< void ( const Request &, String ) > tReqFailed;

		typedef Plib::Threading::Thread< void () >	tReqWorker;
		typedef Plib::Generic::Reference< tReqWorker > refWorker;

		class RequestSuccessCallback
		{
		public:
			// Just output the log with level info.
			void operator() ( const Request & req ) const
			{
				PINFO( "[Request: " << ((const IRequest *)req)->Identify << "] Successed.");
			}

			operator tReqSuccess ()
			{
				tReqSuccess _delegate( *this );
				return _delegate;
			}
		};

		class RequestFailedCallback
		{
		public:
			// Just output the log with level error.
			void operator() ( const Request & req, String errorMsg) const
			{
				PERROR( "[Request: " << ((const IRequest *)req)->Identify << 
					"] Failed, Error Info: <" << errorMsg << ">." );
			}

			operator tReqFailed ()
			{
				tReqFailed _delegate( *this );
				return _delegate;
			}
		};

		//class TcpConnectCreator
		class Connection
		{
		private:
			typedef struct tagConnInfo {
				Request 			request;
				tReqSuccess			onSuccess;
				tReqFailed			onFailed;

				tagConnInfo( const Request &req ): request(req) {}
			} ConnInfo;
		protected:
			Plib::Generic::Queue< ConnInfo >			m_pendingQueue;
			Plib::Generic::Queue< ConnInfo >			m_idleQueue;
			Plib::Generic::Queue< ConnInfo >			m_fetchingQueue;
			Semaphore									m_pendingSem;
			Semaphore									m_idleSem;
			Semaphore									m_fetchingSem;
			Mutex										m_pendingMutex;
			Mutex 										m_idleMutex;
			Mutex 										m_fetchingMutex;

			Plib::Generic::Array< refWorker >			m_fetchingWorkerList;
			Plib::Generic::Array< refWorker >			m_postingWorkingList;

		protected:
			Uint32										m_maxPostingWorkingCount;
			Uint32										m_maxFetchingWorkingCount;

		protected:
			refWorker __fetcherCreater()
			{
				refWorker _refWorker;
				_refWorker->Jobs += std::make_pair( this, 
					&Connection::__thread_ReceiveResponse);
				// Start the thread automatically.
				_refWorker->Start();
				return _refWorker;
			}
			refWorker __posterCreater()
			{
				refWorker _refWorker;
				_refWorker->Jobs += std::make_pair( this,
					&Connection::__thread_SendRequest);
				// Start the thread automatically.
				_refWorker->Start();
				return _refWorker;
			}

			void __thread_ReceiveResponse( )
			{
				while ( Plib::Threading::ThreadSys::Running() ) {
					// Fetch a sent request from queue
					if ( !m_fetchingSem.Get(100) ) continue;
					m_fetchingMutex.Lock();
					ConnInfo ci = m_fetchingQueue.Head();
					m_fetchingQueue.Pop();
					m_fetchingMutex.UnLock();
					// Wait for the response data
					Request _req = ci.request;
					IRequest *_pReq = _req;
					ISocket *_pSock = _pReq->Connector();
					if ( _pSock->isReadable() == false ) {
						// No incoming data right now
						// Push the fetching ci to the end of the queue
						m_fetchingMutex.Lock();
						m_fetchingQueue.Push(ci);
						m_fetchingSem.Release();
						m_fetchingMutex.UnLock();
						continue;
					}
					// Initialize a responder
					_pReq->InitializeResponser();
					// Generate the response object
					const NData &_body = _pSock->Read();
					PDUMP(_body);
					IResponse *_pResp = _pReq->Responser();
					_pResp->generateResponseObjectFromPackage(_body);
					// Tell the delegate to call back
					ci.onSuccess(_req);
					// If the request is set keepAlive
					// move the request to the idle list
					if ( _req->RequestPeerInfo().KeepAlive ) {

					}
				}
			}
			void __thread_SendRequest( )
			{
				while ( Plib::Threading::ThreadSys::Running() ) {
					// Fetch a pending request
					if ( !m_pendingSem.Get(100) ) continue;
					m_pendingMutex.Lock();
					ConnInfo ci = m_pendingQueue.Head();
					m_pendingQueue.Pop();
					m_pendingMutex.UnLock();
					// check if the request's socket has been connected
					Request _req = ci.request;
					IRequest *_pReq = _req;
					if ( _pReq->InitializeConnector() == false ) {
						ci.onFailed( _req, "Cannot connect to peer." );
						// Just drop this request object
						continue;
					}
					// send the package
					const NData &_package = _pReq->generateFullPackage();
					ISocket *_pSock = _pReq->Connector();
					int _ret = _pSock->Write( _package, 0 );
					if ( _ret < 0 ) {
						// Failed to write
						ci.onFailed( _req, "Failed to send package to peer.");
						continue;
					}
					if ( _ret == 0 ) {
						PNOTIFY("The package is zero, wait for peer to send request first.");
					}
					// move the request to the fetch list
					m_fetchingMutex.Lock();
					m_fetchingQueue.Push(ci);
					m_fetchingSem.Release();
					m_fetchingMutex.UnLock();
				}
			}

		protected:

			// Singleton Constructure
			Connection( ):  m_maxPostingWorkingCount(1), m_maxFetchingWorkingCount(1)
			{
				m_pendingSem.Init(0);
				m_fetchingSem.Init(0);

				// Create the initialized posting worker
				refWorker _posterWorker = this->__posterCreater();
				m_postingWorkingList.PushBack(_posterWorker);

				// Create the first fetching worker
				refWorker _fetcherWorker = this->__fetcherCreater();
				m_fetchingWorkerList.PushBack(_fetcherWorker);
			}

			~Connection( )
			{
				// Close all worker
				for ( int i = 0; i < m_postingWorkingList.Size(); ++i ) {
					m_postingWorkingList[i]->Stop(true);
				}
				for ( int i = 0; i < m_fetchingWorkerList.Size(); ++i ) {
					m_fetchingWorkerList[i]->Stop(true);
				}
			}

			// Singleton Object
			static Connection & self() {
				static Connection _self;
				return _self;
			}
		public:
			// Async Sending Request
			static INLINE void sendAsyncRequest( 
				Request req, 
				tReqSuccess success = RequestSuccessCallback(),
				tReqFailed failed = RequestFailedCallback()
				)
			{
				self().m_pendingMutex.Lock();
				ConnInfo ci(req);
				ci.onSuccess = success;
				ci.onFailed = failed;
				self().m_pendingQueue.Push(ci);
				self().m_pendingSem.Release();
				self().m_pendingMutex.UnLock();
			}
		};
	}
}

#endif // plib.network.connection.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
