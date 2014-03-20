/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: thread.hpp
* Propose  			: Redefinition the thread object. Use Global Mapping to store current thread info.
* 
* Current Version	: 1.1
* Change Log		: Re-Definition.
* Author			: Push Chen
* Change Date		: 2011-01-10
*/


#pragma once

#ifndef _PLIB_THREADING_THREAD_HPP_
#define _PLIB_THREADING_THREAD_HPP_

#include <Plib-Generic/Generic.hpp>
#include <Plib-Threading/Semaphore.hpp>
#include <exception>
#include <map>

namespace Plib
{
	namespace Threading
	{
		// Thread Parameters Definition.
		#define ARGLIST( t )					; _Ty##t m_arg##t
		#define ARGLIST_END						_Ty0 m_arg

		#define ARG_IN_THREAD( t )				, pThread->m_arg##t
		#define ARG_IN_THREAD_END				pThread->m_arg

		#define PARAM_START_THREAD( t )			, _Ty##t __arg##t
		#define PARAM_START_THREAD_END			_Ty0 __arg

		#define PARAM_SET_TRHEAD( t )			; this->m_arg##t = __arg##t
		#define PARAM_SET_THREAD_END			this->m_arg0 = __arg

		#define DEF_ARGLIST( t )				REPEAT_##t( t, ARGLIST, ARGLIST_END )
		#define DEF_ARG_IN_THREAD( t )			REPEAT_##t( t, ARG_IN_THREAD, ARG_IN_THREAD_END )
		#define DEF_PARAM_START_THREAD( t )		REPEAT_##t( t, PARAM_START_THREAD, PARAM_START_THREAD_END )
		#define DEF_PARAM_SET_THREAD( t )		REPEAT_##t( t, PARAM_SET_TRHEAD, PARAM_SET_THREAD_END )

		// For Thread Stack Size
		#ifndef PLIB_THREAD_STACK_SIZE
		
		#if !_DEF_WIN32
		#include <limits.h>
		#endif

		#define PLIB_THREAD_STACK_SIZE	0x00100000			// 1MB
		
		#endif
		
		#define PLIB_THREAD_MIN_STACKSIZE	0x40000			// 256KB

		#if _DEF_WIN32
		typedef long		THANDLE;
		typedef long		TID_T;
		typedef Uint32		CreateTRetVal_T;

		#define _THREAD_CALLBACK	__stdcall
		#else
		typedef int			THANDLE;
		typedef pthread_t	TID_T;
		typedef void *		CreateTRetVal_T;

		#define _THREAD_CALLBACK
		#endif

		// Thread Information Struct
		typedef struct tagTHREAD_OBJECT
		{
			THANDLE			_ThreadHandle;
			TID_T			_ThreadID;
			bool			_ThreadStatue;
			Uint32			_StackSize;
			RWLock			_Locker;
			Mutex			_RunningLock;
			Semaphore		_SyncSem;
			Semaphore		_SignalSem;
		} THREAD_OBJECT, *LPTHREAD_OBJECT;

		INLINE void _INIT_THREAD_OBJECT( THREAD_OBJECT & _TObj )
		{
			_TObj._ThreadHandle = -1;
			_TObj._ThreadID = 0;
			_TObj._ThreadStatue = false;
			_TObj._StackSize = 0;
		}
		
		// Thread Global Info
		template < Uint32 _dummy > class __ThreadInfo 
		{
			static Uint32 & __stackSize( ) {
				static Uint32 _size = PLIB_THREAD_MIN_STACKSIZE;
				return _size;
			}
		public:
			static Uint32 GetStackSize( ) { return __stackSize(); }
			static void SetStackSize( Uint32 _size ) {
				if ( _size <= PLIB_THREAD_MIN_STACKSIZE ) return;
				__stackSize() = _size;
			}
		};
		// Thread Info
		typedef __ThreadInfo< 0 >			ThreadInfo;
		
		// Thread Object Definition.
		template< typename T > class Thread {};

		struct ThreadKernel
		{
			// Global Map & Locker
			static RWLock & ThreadGlobalLock() {
				static RWLock _lock;
				return _lock;
			}
			static std::map< TID_T, LPTHREAD_OBJECT > & ThreadGlobalMap() {
				static std::map< TID_T, LPTHREAD_OBJECT > _map;
				return _map;
			}
			// Try to start the thread call back function.
			// In different operation system, this function active differently.
			// The return value of this function shows if 
			// the thread has been start successfully.
			static INLINE bool BeginThread( void * _Thread, LPTHREAD_OBJECT _ThreadObj, 
				CreateTRetVal_T (_THREAD_CALLBACK * _CallBack)(void *) )
			{
				if ( _Thread == NULL ) return false;
				if ( _ThreadObj == NULL ) return false;
				if ( _CallBack == NULL ) return false;
				WriteLocker tgLock( ThreadGlobalLock() );
				Uint32 _ss = _ThreadObj->_StackSize == 0 ? 
					ThreadInfo::GetStackSize() : _ThreadObj->_StackSize;
		#if _DEF_WIN32
				_ThreadObj->_ThreadHandle = ::_beginthreadex( 
					NULL, _ss, _CallBack, _Thread, 0, (unsigned *)&_ThreadObj->_ThreadID);
				if ( _ThreadObj->_ThreadHandle == 0 || _ThreadObj->_ThreadHandle == -1 )
					return false;
		#else
				pthread_attr_t _tAttr;
				int ret = pthread_attr_init( &_tAttr );
				if ( ret != 0 ) return false;
				ret = pthread_attr_setstacksize( &_tAttr, _ss );
				if ( ret != 0 ) return false;
				_ThreadObj->_ThreadHandle = pthread_create(&_ThreadObj->_ThreadID, 
					&_tAttr, _CallBack, _Thread );
				pthread_attr_destroy(&_tAttr);
				if ( _ThreadObj->_ThreadHandle != 0 ) return false;
		#endif
				ThreadGlobalMap()[_ThreadObj->_ThreadID] = _ThreadObj;
				return true;
			}

			// Close the thread's handle in windows and do nothing
			// in Linux.
			// In Windows, the return handle of _beginthread must be 
			// closed by invoking ::CloseHande explicitly.
			static INLINE void EndThread( LPTHREAD_OBJECT _ThreadObj )
			{
				if ( _ThreadObj->_ThreadID == 0 ) return;
		#if _DEF_WIN32
				::CloseHandle((HANDLE)_ThreadObj->_ThreadHandle);
				_ThreadObj->_ThreadHandle = 0;
		#endif
				WriteLocker _locker( ThreadGlobalLock() );
				std::map< TID_T, LPTHREAD_OBJECT >::iterator _Tit = 
					ThreadKernel::ThreadGlobalMap().find( _ThreadObj->_ThreadID );
				if ( _Tit != ThreadKernel::ThreadGlobalMap().end() )
					ThreadKernel::ThreadGlobalMap().erase( _Tit );
		#if !_DEF_WIN32
				// Detach current thread's resource.
				pthread_detach( _ThreadObj->_ThreadID );
		#endif
				_ThreadObj->_ThreadID = 0;
			}
		};

		struct ThreadSys
		{
			static INLINE long ThreadId( )
			{
		#if _DEF_WIN32
				return GetCurrentThreadId( );
		#elif _DEF_LINUX
				return (Uint64)gettid();
		#else
				return (Uint64)gettid();
		#endif
			}
			static INLINE TID_T SelfID( ) 
			{
		#if _DEF_WIN32
				return GetCurrentThreadId( );
		#else
				return pthread_self( );
		#endif
			}

			static INLINE bool Running( )
			{
				TID_T _Id = SelfID();
				ReadLocker _GLocker( ThreadKernel::ThreadGlobalLock() );
				if ( ThreadKernel::ThreadGlobalMap().find( _Id ) == 
					ThreadKernel::ThreadGlobalMap().end() )
					return false;
				LPTHREAD_OBJECT _ThreadObj = ThreadKernel::ThreadGlobalMap()[_Id];
				ReadLocker _TLocker( _ThreadObj->_Locker );
				return _ThreadObj->_ThreadStatue;
			}

			// Sleep the thread function for millseconds.
			static INLINE void Sleep( Uint32 _Milliseconds )
			{
		#if _DEF_WIN32
				::Sleep( _Milliseconds );
		#else
				struct timespec _ts;
				_ts.tv_sec = _Milliseconds / 1000;
				_ts.tv_nsec = ( _Milliseconds % 1000 ) * 1000;
				int _rtn = 0;
				do {
					_rtn = nanosleep( &_ts, &_ts );
				} while( _rtn != 0 );
		#endif
			}

			// Wait For Some other Thread to give the signal to continue.
			// Default is Wait Infinished.
			static INLINE bool WaitForSignal( Uint32 _Milliseconds = Semaphore::MAXTIMEOUT )
			{
				TID_T _Id = SelfID();
				// BUG HERE!!
				// When any thread invoke this method, is about to block
				// the ThreadGlobalLock, then no other thread can be started or ended.
				//ReadLocker _GLocker( ThreadGlobalLock );
				ThreadKernel::ThreadGlobalLock().ReadLock( );
				if ( ThreadKernel::ThreadGlobalMap().find( _Id ) == 
					ThreadKernel::ThreadGlobalMap().end() ) {
					ThreadKernel::ThreadGlobalLock().UnLock( );
					return false;
				}
				LPTHREAD_OBJECT _ThreadObj = ThreadKernel::ThreadGlobalMap()[_Id];
				ThreadKernel::ThreadGlobalLock().UnLock( );
				return _ThreadObj->_SignalSem.Get( _Milliseconds );
			}
		};

		// Basic Re-define.
	#define _THREADING_THREAD_OBJECT_DEFINITION_( n )										\
		template< typename _TyRet, DEF_PARAM( n ) > class Thread< _TyRet( DEF_TYPE( n ) ) >	\
		{																					\
		public:																				\
			typedef Plib::Generic::Delegate< _TyRet( DEF_TYPE( n ) ) >	ThreadStarter;		\
			typedef Plib::Generic::Delegate< void ( ) >					ThreadJoin;			\
		public:																				\
			ThreadStarter	Jobs;															\
			ThreadJoin		Join;															\
		protected:																			\
			THREAD_OBJECT	_ThreadObj;														\
			DEF_ARGLIST( n );																\
		protected:																			\
			static INLINE CreateTRetVal_T _THREAD_CALLBACK _CallBackFunc( void * _Thread )	\
			{																				\
				Thread< _TyRet( DEF_TYPE(n) ) > * pThread =									\
					( Thread< _TyRet( DEF_TYPE(n) ) > *)_Thread;							\
				pThread->_ThreadObj._ThreadStatue = true;									\
				pThread->_ThreadObj._SignalSem.Init(0);										\
				Locker _RLock( pThread->_ThreadObj._RunningLock );							\
				pThread->_ThreadObj._SyncSem.Release();										\
				if ( pThread->Jobs ) pThread->Jobs( DEF_ARG_IN_THREAD( n ) );				\
				pThread->SetStatue( false );												\
				if ( pThread->Join ) pThread->Join( );										\
				ThreadKernel::EndThread( &pThread->_ThreadObj );							\
				return 0;																	\
			}																				\
		protected:																			\
			INLINE void SetStatue( bool _Statue )											\
			{																				\
				WriteLocker locker( _ThreadObj._Locker );									\
				_ThreadObj._ThreadStatue = _Statue;											\
			}																				\
		public:																				\
			Thread< _TyRet( DEF_TYPE(n) ) > ( ) { _INIT_THREAD_OBJECT( _ThreadObj ); }		\
			Thread< _TyRet( DEF_TYPE(n) ) > ( const ThreadStarter & _Ts )					\
				: Jobs( _Ts ) { _INIT_THREAD_OBJECT(_ThreadObj); }							\
			~Thread< _TyRet( DEF_TYPE(n) ) >( ) { Stop(); }									\
		public:																				\
			INLINE bool Statue( ) const														\
			{																				\
				ReadLocker locker( _ThreadObj._Locker );									\
				return _ThreadObj._ThreadStatue;											\
			}																				\
			INLINE void SetStackSize( Uint32 _ss )											\
			{																				\
				WriteLocker locker( _ThreadObj._Locker );									\
				_ThreadObj._StackSize = _ss;												\
			}																				\
		public:																				\
			INLINE bool Start( DEF_PARAM_START_THREAD( n ) )								\
			{																				\
				WriteLocker _Locker( _ThreadObj._Locker );									\
				if ( _ThreadObj._ThreadStatue == true ) return false;						\
				ThreadKernel::EndThread( &_ThreadObj );										\
				_ThreadObj._SyncSem.Init(0, 1);												\
				DEF_PARAM_SET_THREAD( n );													\
				if ( !ThreadKernel::BeginThread( this, &_ThreadObj,							\
					&Thread< _TyRet( DEF_TYPE( n ) ) >::_CallBackFunc ) ) return false;		\
				_ThreadObj._SyncSem.Get( );													\
				return _ThreadObj._ThreadStatue;											\
			}																				\
			INLINE void Stop( bool _WaitUntilStop = true ) {								\
				SetStatue( false );															\
				GiveSignal( );																\
				if ( _WaitUntilStop ) WaitUntilStop( );										\
			}																				\
			INLINE TID_T GetThreadID( ) {													\
				ReadLocker _locker(_ThreadObj._Locker);										\
				return _ThreadObj._ThreadID;												\
			}																				\
			INLINE void GiveSignal( ) {														\
				_ThreadObj._SignalSem.Release(); }											\
			INLINE void WaitUntilStop( ) {													\
				Locker _RLock( _ThreadObj._RunningLock ); }									\
		};

		template< typename _TyRet > class Thread< _TyRet( ) >
		{
		public:
			typedef Plib::Generic::Delegate< _TyRet( ) >				ThreadStarter;
			typedef Plib::Generic::Delegate< void ( ) >					ThreadJoin;
		public:
			ThreadStarter	Jobs;
			ThreadJoin		Join;
		protected:
			THREAD_OBJECT	_ThreadObj;
		protected:
			static INLINE CreateTRetVal_T _THREAD_CALLBACK _CallBackFunc( void * _Thread )
			{
				Thread< _TyRet( ) > * pThread = ( Thread< _TyRet( ) > *)_Thread;
				pThread->_ThreadObj._ThreadStatue = true;
				pThread->_ThreadObj._SignalSem.Init(0);
				Locker _RLock( pThread->_ThreadObj._RunningLock );
				pThread->_ThreadObj._SyncSem.Release();
				if ( pThread->Jobs ) pThread->Jobs( );
				pThread->SetStatue( false );
				if ( pThread->Join ) pThread->Join( );
				ThreadKernel::EndThread( &pThread->_ThreadObj );
				return 0;
			}
		protected:
			INLINE void SetStatue( bool _Statue )
			{
				WriteLocker locker( _ThreadObj._Locker );
				_ThreadObj._ThreadStatue = _Statue;
			}
		public:
			// C'Str
			Thread< _TyRet( ) > ( ) { _INIT_THREAD_OBJECT( _ThreadObj ); }
			Thread< _TyRet( ) > ( const ThreadStarter & _Ts ) : Jobs( _Ts )
			{
				_INIT_THREAD_OBJECT( _ThreadObj);
			}
			~Thread< _TyRet( ) >( ) { Stop(); }
		public:
			INLINE bool Statue( ) const
			{
				ReadLocker locker( _ThreadObj._Locker );
				return _ThreadObj._ThreadStatue;
			}
			INLINE void SetStackSize( Uint32 _ss )
			{
				WriteLocker locker( _ThreadObj._Locker );
				_ThreadObj._StackSize = _ss;
			}
		public:
			INLINE bool Start( )
			{
				WriteLocker _Locker( _ThreadObj._Locker );
				if ( _ThreadObj._ThreadStatue == true ) return false;
				ThreadKernel::EndThread( &_ThreadObj );
				_ThreadObj._SyncSem.Init(0, 1);
				if ( !ThreadKernel::BeginThread( this, &_ThreadObj, 
					&Thread< _TyRet( ) >::_CallBackFunc ) ) return false;
				_ThreadObj._SyncSem.Get( );
				return _ThreadObj._ThreadStatue;
			}
			INLINE void Stop( bool _WaitUntilStop = true ) {
				SetStatue( false );
				GiveSignal( );
				if ( _WaitUntilStop ) WaitUntilStop( );
			}
			INLINE TID_T GetThreadID( ) {
				ReadLocker _locker(_ThreadObj._Locker);
				return _ThreadObj._ThreadID;
			}
			INLINE void GiveSignal( ) {
				_ThreadObj._SignalSem.Release(); }
			INLINE void WaitUntilStop( ) {
				Locker _RLock( _ThreadObj._RunningLock );
			}
		};
		
		_THREADING_THREAD_OBJECT_DEFINITION_( 0 )
		_THREADING_THREAD_OBJECT_DEFINITION_( 1 )
		_THREADING_THREAD_OBJECT_DEFINITION_( 2 )
		_THREADING_THREAD_OBJECT_DEFINITION_( 3 )
		_THREADING_THREAD_OBJECT_DEFINITION_( 4 )
		_THREADING_THREAD_OBJECT_DEFINITION_( 5 )
		_THREADING_THREAD_OBJECT_DEFINITION_( 6 )
		_THREADING_THREAD_OBJECT_DEFINITION_( 7 )
		_THREADING_THREAD_OBJECT_DEFINITION_( 8 )
		_THREADING_THREAD_OBJECT_DEFINITION_( 9 )
	}
}

#endif // plib.threading.thread.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
