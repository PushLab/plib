/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: Threading.hpp
* Propose  			: Include All Threading Head Files together.
* 
* Current Version	: 1.0
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2011-01-10
*/

#pragma once

#ifndef _PLIB_THREADING_THREADING_HPP_
#define _PLIB_THREADING_THREADING_HPP_

#include <Plib-Threading/Locker.hpp>
#include <Plib-Threading/Semaphore.hpp>
#include <Plib-Threading/Thread.hpp>
#include <Plib-Threading/Stopwatch.hpp>
#include <Plib-Threading/Timer.hpp>

// For Server Running in Background:
#include <signal.h>
namespace Plib
{
	namespace Threading
	{
#if _DEF_WIN32
		INLINE void SetSignalHandle( ) {
		}
		INLINE void WaitForExitSignal( )
		{
			char _c = getc( );
		}
#else
		// Global Signal
		struct __GlobalSignal {
			static Semaphore & __WaitFor( ) {
				static Semaphore _sem(0, 1);
				return _sem;
			}
		};
		
		INLINE void __HandleSignal( int _Sig ) {
			if (SIGTERM == _Sig || SIGINT == _Sig || SIGQUIT == _Sig) {
				__GlobalSignal::__WaitFor().Release();			
			}
		}
		INLINE void SetSignalHandle( ) {
			// Hook the signal
#ifdef __APPLE__
			signal(SIGINT, __HandleSignal);
#else
			sigset_t sgset, osgset;
			sigfillset(&sgset);
			sigdelset(&sgset, SIGTERM);
			sigdelset(&sgset, SIGINT);
			sigdelset(&sgset, SIGQUIT);
			sigdelset(&sgset, 11);
			sigprocmask(SIG_SETMASK, &sgset, &osgset);
			signal(SIGTERM, __HandleSignal);
			signal(SIGINT, __HandleSignal);
			signal(SIGQUIT, __HandleSignal);	
#endif		
		}
		INLINE void WaitForExitSignal( )
		{
			// Wait for exit signal
			__GlobalSignal::__WaitFor().Get( );		
		}
#endif
	}
}

#endif // plib.threading.threading.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
