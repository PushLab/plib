/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: plib.hpp
* Propose  			: Common Definition for all components of PLib
* 
* Current Version	: 1.1
* Change Log		: Re-group the definition from old stdc.hpp
* Author			: Push Chen
* Change Date		: 2010-11-25
*/

#pragma once

#ifndef _PLIB_PLIB_HPP_
#define _PLIB_PLIB_HPP_

// COMMON WINDOWS Macro Check Definiton.
#if ( defined WIN32 | defined _WIN32 | defined WIN64 | defined _WIN64 )
	#define _PLIB_PLATFORM_WIN		1
#elif TARGET_OS_WIN32
	#define _PLIB_PLATFORM_WIN		1
#elif defined __CYGWIN__
	#define _PLIB_PLATFORM_WIN		1
#else
	#define _PLIB_PLATFORM_WIN		0
#endif
#ifdef __APPLE__
	#define _PLIB_PLATFORM_MAC		1
#else
	#define _PLIB_PLATFORM_MAC		0
#endif
#if _PLIB_PLATFORM_WIN == 0 && _PLIB_PLATFORM_MAC == 0
	#define _PLIB_PLATFORM_LINUX	1
#else
	#define _PLIB_PLATFORM_LINUX	0
#endif
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	#define _PLIB_PLATFORM_IOS		1
#else
	#define _PLIB_PLATFORM_IOS		0
#endif

#define _DEF_WIN32	(_PLIB_PLATFORM_WIN == 1)
#define _DEF_LINUX	(_PLIB_PLATFORM_LINUX == 1)
#define _DEF_MAC	(_PLIB_PLATFORM_MAC == 1)
#define _DEF_IOS	(_PLIB_PLATFORM_IOS == 1)

#if _DEF_WIN32
// Disable the certain warn in Visual Studio for old functions.
#pragma warning (disable : 4996)
#pragma warning (disable : 4251)

#endif

// Usable C Header files including.
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <stddef.h>
#include <math.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
// Usable CPP Header files
#include <locale>
#include <iostream>
#include <string>

// Socket Including file in Windows must in a specified order.
#if _DEF_WIN32
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <stddef.h>
#include <sys/time.h>
#endif

// Linux Thread, pit_t
#if _DEF_LINUX
#include <sys/syscall.h>
#include <unistd.h>
#define gettid()	syscall(__NR_gettid)
#endif

// For Mac OS X
#ifdef __APPLE__
#include <libkern/OSAtomic.h>
#include <unistd.h>
#include <sys/syscall.h>
#define gettid()    syscall(SYS_gettid)
#endif

#ifndef __UNICODE
#define UNICODE_(x)		x
#else
#define UNICODE_(x)		_T(x)
#endif

// If your project has only one thread
// undef the following macro
#define PLIB_BASIC_THREAD_SAFE

// undef the following macro to use simple function name in debug.
#define _PLIB_FULL_FUNCNAME

// If you do not want to use memory pool
// just comment the following macro
//#define PLIB_USE_MEMPOOL

namespace Plib
{
    #define _DUMMY_CLASS class	// Unused Class definiton.
    #define _DUMMY_VALUE		// Unused Parameter.
	
	#define INLINE		inline

    // Common Type definition
    typedef signed char				Int8;
    typedef signed short int		Int16;
    typedef signed int				Int32;
    typedef signed long long int	Int64;
    typedef unsigned char			Uint8;
    typedef unsigned short int		Uint16;
    typedef unsigned int			Uint32;
    typedef unsigned long long int	Uint64;

    typedef void * Vptr;

    typedef enum { False = 0, True = 1 } Boolean;

    // Type Change Functions
    template < typename _TyInt >
    INLINE _TyInt OrderSwitch( _TyInt _Value ) {
        char * _PValue = (char *)&_Value;
        for ( 
        	Uint8 _SwitchCount = 0; 
        	_SwitchCount < (sizeof( _TyInt ) / 2); 
        	++_SwitchCount 
        	) 
        {
            _PValue[_SwitchCount] ^= _PValue[sizeof(_TyInt) - _SwitchCount - 1];
            _PValue[sizeof(_TyInt) - _SwitchCount - 1] ^= _PValue[_SwitchCount];
            _PValue[_SwitchCount] ^= _PValue[sizeof(_TyInt) - _SwitchCount - 1];
        }
        return _Value;
    }

    // Common Macro Definiton
    #define SELF_INCREASE( x )		++(x)
    #define SELF_DECREASE( x )		--(x)
    #define VALUE_INCREASED( x )	(x)++
    #define VALUE_DECREASED( x )	(x)--

// Function and error.
#if _DEF_WIN32

  #define _PLIB_FUNC_NAME_FULL_ __FUNCSIG__
  #define _PLIB_FUNC_NAME_SIMPLE_   __FUNCTION__

  #define PLIB_LASTERROR    GetLastError()

#else

  #define _PLIB_FUNC_NAME_FULL_ __PRETTY_FUNCTION__
  #define _PLIB_FUNC_NAME_SIMPLE_ __FUNCTION__

  #define PLIB_LASTERROR    errno   

#endif

#ifdef _PLIB_FULL_FUNCNAME
  #define PLIB_FUNC_NAME    _PLIB_FUNC_NAME_FULL_
#else
  #define PLIB_FUNC_NAME    _PLIB_FUNC_NAME_SIMPLE_
#endif

    // Base Locker Object.
    template < typename _TyLocker >
    class LockerT
    {
        _TyLocker & _lock;
#ifdef PLIB_LOCK_DEBUG
        const char *_func_name_;
#endif
    public:
        LockerT( _TyLocker & _l ) : _lock( _l )
#ifdef PLIB_LOCK_DEBUG
        , _func_name_(NULL)
#endif
        { 
            _lock.Lock(); 
        }
#ifdef PLIB_LOCK_DEBUG
        LockerT( _TyLocker & _l, const char *_func ) : _lock( _l ), _func_name_(_func) {
            if ( _func_name_ != NULL ) {
                std::cout << _func_name_ << " try to lock." << std::endl;
            }
            _lock.Lock();
            if ( _func_name_ != NULL ) {
                std::cout << _func_name_ << " locked." << std::endl;
            }
        }
#endif
        ~LockerT( ) { 
            _lock.UnLock(); 
#ifdef PLIB_LOCK_DEBUG
            if ( _func_name_ != NULL ) {
                std::cout << _func_name_ << " unlocked." << std::endl;
            }
#endif
        }
    };

    // Spin Lock In Both Windows and linux.
    class SpinLocker
    {
    protected:
#if _DEF_WIN32
        ::CRITICAL_SECTION _handle;
#elif _DEF_MAC
		OSSpinLock _handle;
#else
        pthread_spinlock_t _handle;
#endif
		// For Lock debug in multi thread.
#ifdef PLIB_LOCK_DEBUG
		Uint64			   _threadId;
#endif
    public:
        // Create a mutex
        SpinLocker( ) {
#if _DEF_WIN32
            #if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0403)
            // for better performance.
            ::InitializeCriticalSectionAndSpinCount( &_handle, 4000 );
            #else
            ::InitializeCriticalSection( &_handle );
            #endif
#elif _DEF_MAC
			_handle = OS_SPINLOCK_INIT;
#else
            pthread_spin_init(&_handle, PTHREAD_PROCESS_SHARED);
#endif
#ifdef PLIB_LOCK_DEBUG
			_threadId = 0;
#endif
        }
        ~SpinLocker( ){
#if _DEF_WIN32
            ::DeleteCriticalSection( &_handle );
#elif _DEF_MAC
			// nothing to do
#else
            pthread_spin_destroy(&_handle);
#endif
        }

        // Lock the mutex.
        INLINE void Lock()
        {
#if _DEF_WIN32
            ::EnterCriticalSection( &_handle );
#elif _DEF_MAC
			Uint32 _Count;
		RE_TRY_LOCK:
			_Count = 0;
			while( _Count <= 2000 && !OSSpinLockTry(&_handle) )
				++_Count;
			if ( _Count > 2000 ) {
				::usleep(1);
				goto RE_TRY_LOCK;
			}
#else 
            // Ignore the return value is OK.
			Uint32 _Count;
		RE_TRY_LOCK:
			_Count = 0;
			while( _Count <= 2000 && 0 != pthread_spin_trylock( &_handle ) )
				++_Count;
			if ( _Count > 2000 ) {
				::usleep(1);	// Force to change the thread context.
				goto RE_TRY_LOCK;
			}
#endif
#ifdef PLIB_LOCK_DEBUG
	#if _DEF_WIN32
			_threadId = (Uint64)GetCurrentThreadId( );
	#elif _DEF_LINUX
			_threadId = (Uint64)gettid();
	#else
			_threadId = (Uint64)syscall(SYS_gettid);
	#endif
#endif
        }

        // Unlock the mutex
        INLINE void UnLock()
        {
#ifdef PLIB_LOCK_DEBUG
			_threadId = 0;
#endif
#if _DEF_WIN32
            ::LeaveCriticalSection( &_handle );
#elif _DEF_MAC
			OSSpinLockUnlock(&_handle);
#else
            pthread_spin_unlock(&_handle);
#endif
        }
    };

#define STATEMENT_TO_STRING( x )                #x
#define CHAR_CONNECT_BASIC( x, y )              x##y
#define CHAR_CONNECT( x, y )                    CHAR_CONNECT_BASIC( x, y )
#define PUT_CHAR_TOGETHER( x, y )               x y

// LOCK MACRO, Wrap The LockerT.
#ifdef PLIB_BASIC_THREAD_SAFE
#ifdef PLIB_LOCK_DEBUG
    #define SPINLOCK( _lock )       Plib::LockerT< Plib::SpinLocker > CHAR_CONNECT(__lock, __LINE__)( _lock, PLIB_FUNC_NAME)
#else
    #define SPINLOCK( _lock )		Plib::LockerT< Plib::SpinLocker > CHAR_CONNECT(__lock, __LINE__)( _lock )
#endif
#define PLIB_THREAD_SAFE_DEFINE		mutable Plib::SpinLocker __THREAD_SAFE_SPINLOCK
#define PLIB_THREAD_SAFE			SPINLOCK( __THREAD_SAFE_SPINLOCK )
#define PLIB_SUBCLASS_THREAD_SAFE	SPINLOCK( TFather::__THREAD_SAFE_SPINLOCK )
#define PLIB_OBJ_THREAD_SAFE(obj)\
	Plib::LockerT< Plib::SpinLocker > __lock##__LINE__OBJ__( (obj).__THREAD_SAFE_SPINLOCK )
#else
#define SPINLOCK( _lock )
#define	PLIB_THREAD_SAFE_DEFINE
#define PLIB_THREAD_SAFE
#define PLIB_SUBCLASS_THREAD_SAFE
#define PLIB_OBJ_THREAD_SAFE(obj)
#endif

// Repeat Macro
#define DECREASE_0								0
#define DECREASE_1								0
#define DECREASE_2								1
#define DECREASE_3								2
#define DECREASE_4								3
#define DECREASE_5								4
#define DECREASE_6								5
#define DECREASE_7								6
#define DECREASE_8								7
#define DECREASE_9								8
#define DECREASE( n )							DECREASE_##n

#define SUBDEC_1								1
#define SUBDEC_2								1
#define SUBDEC_3								2
#define SUBDEC_4								3
#define SUBDEC_5								4
#define SUBDEC_6								5
#define SUBDEC_7								6
#define SUBDEC_8								7
#define SUBDEC_9								8
#define SUBDEC( n )								SUBDEC_##n

#define REPEAT_0( n, f, e ) CHAR_CONNECT( e, n )
#define REPEAT_1( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( REPEAT_, \
	DECREASE( n ) )( DECREASE( n ), f, e ), f( n ) ) 
#define REPEAT_2( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( REPEAT_, \
	DECREASE( n ) )( DECREASE( n ), f, e ), f( n ) )
#define REPEAT_3( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( REPEAT_, \
	DECREASE( n ) )( DECREASE( n ), f, e ), f( n ) ) 
#define REPEAT_4( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( REPEAT_, \
	DECREASE( n ) )( DECREASE( n ), f, e ), f( n ) ) 
#define REPEAT_5( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( REPEAT_, \
	DECREASE( n ) )( DECREASE( n ), f, e ), f( n ) ) 
#define REPEAT_6( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( REPEAT_, \
	DECREASE( n ) )( DECREASE( n ), f, e ), f( n ) ) 
#define REPEAT_7( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( REPEAT_, \
	DECREASE( n ) )( DECREASE( n ), f, e ), f( n ) ) 
#define REPEAT_8( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( REPEAT_, \
	DECREASE( n ) )( DECREASE( n ), f, e ), f( n ) ) 
#define REPEAT_9( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( REPEAT_, \
	DECREASE( n ) )( DECREASE( n ), f, e ), f( n ) ) 

#define SUBREPEAT_1( n, f, e ) CHAR_CONNECT( e, n )
#define SUBREPEAT_2( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( \
	SUBREPEAT_, SUBDEC( n ) )( SUBDEC( n ), f, e ), f( n ) )
#define SUBREPEAT_3( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( \
	SUBREPEAT_, SUBDEC( n ) )( SUBDEC( n ), f, e ), f( n ) )
#define SUBREPEAT_4( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( \
	SUBREPEAT_, SUBDEC( n ) )( SUBDEC( n ), f, e ), f( n ) )
#define SUBREPEAT_5( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( \
	SUBREPEAT_, SUBDEC( n ) )( SUBDEC( n ), f, e ), f( n ) )
#define SUBREPEAT_6( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( \
	SUBREPEAT_, SUBDEC( n ) )( SUBDEC( n ), f, e ), f( n ) )
#define SUBREPEAT_7( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( \
	SUBREPEAT_, SUBDEC( n ) )( SUBDEC( n ), f, e ), f( n ) )
#define SUBREPEAT_8( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( \
	SUBREPEAT_, SUBDEC( n ) )( SUBDEC( n ), f, e ), f( n ) )
#define SUBREPEAT_9( n, f, e ) PUT_CHAR_TOGETHER( CHAR_CONNECT( \
	SUBREPEAT_, SUBDEC( n ) )( SUBDEC( n ), f, e ), f( n ) )
	
#define BUILD_TAB( _lv ) \
	for ( Uint32 __i_in_build_tab_loop = 0; \
		__i_in_build_tab_loop < (_lv); \
		++__i_in_build_tab_loop ) os << "    "

// Log Basic Format: [YYYY-mm-dd HH:MM:SS,ms][ThreadId][FILE][Function][Line]
#define PLIB_TIME_FORMAT_BASIC	UNICODE_("%04d-%02d-%02d %02d:%02d:%02d,%03d")
#define PLIB_LOG_FORMAT_BASIC	UNICODE_("[%s][%u][%s][%s][%d]")
// Log Simple Format: [Time][ThreadId][FUNCTION][LINE][ThreadId]
#define PLIB_TIME_FORMAT_SIMPLE	UNICODE_("%04d-%02d-%02d %02d:%02d")
#define PLIB_LOG_FORMAT_SIMPLE	UNICODE_("[%s][%u][%s][%d]")
// Log Postfix Fomat: [YYYY-mm-dd-HH-MM-SS]
#define PLIB_TIME_FORMAT_POSTFIX UNICODE_("%04d-%02d-%02d-%02d-%02d-%02d")


}

// Include Memory Debug
#if _DEF_IOS
#include "Memory.hpp"
#else
#include <Plib-Basic/Memory.hpp>
#endif
#endif // plib.plib.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
