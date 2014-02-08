/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: common.hpp
* Propose  			: Common Macro Definition For Debuging Usage.
* 
* Current Version	: 1.2
* Change Log		: V1.1 Re-Definied.
					: V1.2 Add Thread ID to the string format.
* Author			: Push Chen
* Change Date		: 2011-01-08
*/


#pragma once

#ifndef _PLIB_UTILITY_COMMON_HPP_
#define _PLIB_UTILITY_COMMON_HPP_

#include <Plib-Text/Convert.hpp>

namespace Plib
{
	namespace Text
	{
		// Get the full format time string
		// Return Reference Point String Object.
		// format is based on PLIB_TIME_FORMAT_BASIC
		INLINE Plib::Text::String GetCurrentTimeBasic() {
			Plib::Text::String timeString;
		#if _DEF_WIN32
			::SYSTEMTIME sysTime;
			::GetLocalTime( &sysTime );
			timeString.Format( PLIB_TIME_FORMAT_BASIC, 
				sysTime.wYear, sysTime.wMonth, sysTime.wDay,
				sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
				sysTime.wMilliseconds
			);
		#else
			struct timeb _timeBasic;
			struct tm *  _timeStruct;
			ftime( &_timeBasic );
			_timeStruct = localtime( &_timeBasic.time );
			timeString.Format( PLIB_TIME_FORMAT_BASIC, 
				(Uint16)(_timeStruct->tm_year + 1900), (Uint8)(_timeStruct->tm_mon + 1), 
				(Uint8)(_timeStruct->tm_mday), (Uint8)(_timeStruct->tm_hour), 
				(Uint8)(_timeStruct->tm_min), (Uint32)(_timeStruct->tm_sec), 
				(Uint16)(_timeBasic.millitm)
			);
		#endif
			return timeString;
		}
		
		// Get the simple format time string
		// Return Reference Point String Object.
		// format is based on PLIB_TIME_FORMAT_SIMPLE
		INLINE Plib::Text::String GetCurrentTimeSimple() {
			Plib::Text::String timeString;
		#if _DEF_WIN32
			::SYSTEMTIME sysTime;
			::GetLocalTime( &sysTime );
			timeString.Format( PLIB_TIME_FORMAT_SIMPLE, 
				sysTime.wYear, sysTime.wMonth, sysTime.wDay,
				sysTime.wHour, sysTime.wMinute
			);
		#else
			time_t _timeBasic;
			struct tm *  _timeStruct;
			_timeStruct = localtime( &(_timeBasic = time(NULL)) );
			timeString.Format( PLIB_TIME_FORMAT_SIMPLE, 
				(Int32)_timeStruct->tm_year + 1900, (Int32)_timeStruct->tm_mon + 1, 
				(Int32)_timeStruct->tm_mday, (Int32)_timeStruct->tm_hour, 
				(Int32)_timeStruct->tm_min
			);
		#endif
			return timeString;
		}
		
		INLINE Plib::Text::String GetCurrentTimePostfix( ) {
		#if _DEF_WIN32
			::SYSTEMTIME sysTime;
			::GetLocalTime( &sysTime );
			return String::Parse( PLIB_TIME_FORMAT_POSTFIX, 
				sysTime.wYear, sysTime.wMonth, sysTime.wDay,
				sysTime.wHour, sysTime.wMinute, sysTime.wSecond );
		#else
			time_t _timeBasic;
			struct tm *  _timeStruct;
			_timeStruct = localtime( &(_timeBasic = time(NULL)) );
			return String::Parse( PLIB_TIME_FORMAT_POSTFIX, 
				(Uint16)(_timeStruct->tm_year + 1900), (Uint8)(_timeStruct->tm_mon + 1), 
				(Uint8)(_timeStruct->tm_mday), (Uint8)(_timeStruct->tm_hour), 
				(Uint8)(_timeStruct->tm_min), (Uint32)(_timeStruct->tm_sec) 
				);
		#endif
		}
		
		#ifdef _PLIB_DEBUG_SIMPLE_
		#define GetCurrentTime		GetCurrentTimeSimple
		#else
		#define GetCurrentTime		GetCurrentTimeBasic
		#endif
		
		// Get the Specified Error Code's Error Message String.
		INLINE Plib::Text::String GetErrorMessage( int _Code ) {
			Plib::Text::String errorString;
			
		#if _DEF_WIN32
			char _Buffer[0x080] = { 0 };
			::FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS, NULL, 
				_Code, 
				MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
				_Buffer, 0x080, 
				NULL );
		#else
			errorString = ::strerror( _Code );
		#endif
			return errorString;
		}
		// Get the last error message object.
		#define LastErrorMessage	GetErrorMessage( PLIB_LASTERROR )
		
		// Print the given data as hex format.
		INLINE void PrintAsHex( const char * _Data, Uint32 _Length )
		{
			const static Uint32 _cPerLine = 16;		// 16 Characters Per-Line.
			const static Uint32 _addrSize = sizeof(intptr_t) * 2 + 2;
			const static Uint32 _bufferSize = _cPerLine * 4 + 3 + _addrSize + 2;
			Uint32 _Lines = ( _Length / _cPerLine ) + 
				(Uint32)((_Length % _cPerLine) > 0);
			Uint32 _LastLineSize = ( _Lines == 1 ) ? _Length : _Length % _cPerLine;
			if ( _LastLineSize == 0 ) _LastLineSize = _cPerLine;
			char _BufferLine[ _bufferSize ];

			for ( Uint32 _l = 0; _l < _Lines; ++_l ) {
				Uint32 _LineSize = ( _l == _Lines - 1 ) ? _LastLineSize : _cPerLine;
				::memset( _BufferLine, 0x20, _bufferSize );
				if ( sizeof(intptr_t) == 4 )
					sprintf( _BufferLine, "%08x: ", (Uint32)(intptr_t)(_Data + (_l * _cPerLine)) );
				else
					sprintf( _BufferLine, "%016lx: ", (unsigned long)(intptr_t)(_Data + (_l * _cPerLine)) );
				for ( Uint32 _c = 0; _c < _LineSize; ++_c ) {
					sprintf( _BufferLine + _c * 3 + _addrSize, "%02x ", 
						(Uint8)_Data[_l * _cPerLine + _c]
					);
					_BufferLine[ (_c + 1) * 3 + _addrSize ] = ' ';	// Reset the '\0'
					_BufferLine[ _cPerLine * 3 + 1 + _c + _addrSize + 1 ] = 
						( (isprint((Uint8)(_Data[_l * _cPerLine + _c])) ?
							_Data[_l * _cPerLine + _c] : '.')
						);
				}
				_BufferLine[ _cPerLine * 3 + _addrSize ] = '\t';
				_BufferLine[ _cPerLine * 3 + _addrSize + 1] = '|';
				_BufferLine[ _bufferSize - 3 ] = '|';
				_BufferLine[ _bufferSize - 2 ] = '\0';
				std::cout << _BufferLine << std::endl;
				// printf( "%s\n", _BufferLine );
			}
		}
		
		// Print the Plib::Text::String as hex data
		INLINE void PrintAsHex( const Plib::Text::String & _string ) {
			PrintAsHex( _string.c_str(), _string.size() );
		}
	}
}


#endif // plib.utility.common.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */

