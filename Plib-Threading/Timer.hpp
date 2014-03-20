/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: timer.hpp
* Propose  			: Redefinition the timer object. Caculate the time more detail.
* 
* Current Version	: 1.1
* Change Log		: Re-Definition.
* Author			: Push Chen
* Change Date		: 2011-01-10
*/

#pragma once

#ifndef _PLIB_THREADING_TIMER_HPP_
#define _PLIB_THREADING_TIMER_HPP_

#include <Plib-Threading/Thread.hpp>
#include <Plib-Threading/Stopwatch.hpp>

namespace Plib
{
	namespace Threading
	{
		// An easy timer impelnment with thread.
		// the OnTick delegate accept a function which return void and with
		// no parameters.
		class Timer
		{
		public:
			// An void() delegate will be invoked when a tick arrived.
			typedef Thread< void ( ) >					TickThread;
			typedef Plib::Generic::Delegate< void ( ) >	TickHandler;
		protected:
			bool										_Enabled;
			Uint64										_Interval;
			Mutex										_TickMutex;
			TickThread									_TickThread;
			TickHandler									_OnTick;

			// The thread function to do a loop and calc the time.
			void _TimerInternalThread( )
			{
				StopWatch _StopWatch( false );

				while( ThreadSys::Running() )
				{
					_StopWatch.SetStart( );
					//Locker _Lock( _TickMutex );
					_TickMutex.Lock();
					if ( _OnTick ) _OnTick( );
					_StopWatch.Tick( );
					Uint64 _LeftTime = _Interval - (_StopWatch.GetMileSecUsed() % _Interval);
					_TickMutex.UnLock();

					// If we get any signal, just break and return
					if ( ThreadSys::WaitForSignal( (Uint32)_LeftTime ) ) {
						break;
					}
				}
			}

		public:
			// C'Str
			Timer( Uint64 _Milliseconds = 1000, bool _Statue = false )
				: _Enabled( _Statue ), _Interval( _Milliseconds )
			{
				_TickThread.Jobs += std::make_pair(this, &Timer::_TimerInternalThread);
				if ( _Enabled ) _TickThread.Start( );
			}

			void SetInterval( Uint64 _Milliseconds )
			{
				if ( _Milliseconds == 0 ) return;
				Locker _Lock( _TickMutex);
				_Interval = _Milliseconds;
			}

			void SetEnable( bool _Statue )
			{
				Locker _Lock( _TickMutex);
				if ( this->_Enabled == _Statue ) return;
				_Enabled = _Statue;
				if ( _Enabled ) _TickThread.Start( );
				else _TickThread.Stop( );
			}

			// Append the OnTick Delegate.
			Timer & operator += ( void (*_fp)() )
			{
				Locker _Lock( _TickMutex);
				_OnTick += _fp;
				return (*this);
			}

			template < typename _TyIns >
			Timer & operator += ( std::pair<_TyIns *, void(_TyIns::*)() > _p )
			{
				Locker _Lock( _TickMutex);
				_OnTick += _p;
				return (*this);
			}
		};
	}
}

#endif // plib.threading.timer.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
