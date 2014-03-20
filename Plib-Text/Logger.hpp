/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: Logger.hpp
* Propose  			: Rewrite the old iologger, use new String and ArrayList.
* 
* Current Version	: 1.1
* Change Log		: Re-Write..
* Author			: Push Chen
* Change Date		: 2011-07-08
*/

#pragma once
#ifndef _PLIB_TEXT_LOGGER_HPP_
#define _PLIB_TEXT_LOGGER_HPP_

#include <Plib-Text/Common.hpp>
#include <Plib-Text/File.hpp>
#include <Plib-Threading/Threading.hpp>

#ifdef PLIB_LOCK_DEBUG
#define LOGGER_LOCK		Threading::Locker _lock( __LogLocker, PLIB_FUNC_NAME );
#else
#define LOGGER_LOCK		Threading::Locker _lock( __LogLocker );
#endif

namespace Plib
{
	namespace Text
	{
		// The Log Level
		typedef enum 
		{
			LLV_TRACE		= 0,
			LLV_DEBUG		= 1,
			LLV_NOTIFY		= 2,
			LLV_INFO		= 3,
			LLV_WARN		= 4,
			LLV_ERROR		= 5,
			LLV_FATAL		= 6
		} LOGLEVEL;
		
		// Convert the log level to the string.
		INLINE String LogLevelWord( LOGLEVEL _llv )
		{
			static String _Words[ ] = { 
				String("TRACE"), 
				String("DEBUG"),
				String("NOTIFY"),
				String("INFO"),
				String("WARN"),
				String("ERROR"),
				String("FATAL") 
			};
			return _Words[(Uint32)_llv % 7];
		}
		
		// Convert the log level word to loglevel.
		INLINE LOGLEVEL WordLogLevel( const String & _word )
		{
			static LOGLEVEL _logLevels[] = {
				LLV_TRACE, LLV_TRACE, LLV_TRACE, LLV_DEBUG, LLV_ERROR,
				LLV_FATAL, LLV_TRACE, LLV_TRACE, LLV_INFO, LLV_TRACE,
				LLV_TRACE, LLV_TRACE, LLV_TRACE, LLV_NOTIFY, LLV_TRACE,
				LLV_TRACE, LLV_TRACE, LLV_TRACE, LLV_TRACE, LLV_TRACE,
				LLV_TRACE, LLV_TRACE, LLV_WARN, LLV_TRACE, LLV_TRACE,
				LLV_TRACE
			};
			return _logLevels[ (Uint32)((_word[0] | 0x20) - 0x61) % 26 ];
		}
		
		// End of Line for Logger
		struct __Log_End_of_Line {
			// Nothing to do for this struct.
			void * _dummy;
		};
				
		/*
		 * How to use the log:
		 * Logger.DebugFormat( "format-string", ... );
		 * Logger.LDebug << "Data" << "Value" << Logger::Endl;
		 * Logger.DebugHex( Prefix, Data, Length );
		 */
		template < Uint32 _dummy = 0 >
		class Logger_
		{
		public:
			static __Log_End_of_Line				Endl;
			static Logger_< _dummy >				GLog;
		protected:
			class __LoggerWriter
			{
				friend class Logger_< _dummy >;

			protected:
				// The Father Logger Object.
				Logger_< _dummy > * 		__Logger;
				LOGLEVEL					__Level;
				bool 						__Locked;
				Plib::Threading::Mutex		__Locker;
				String						__Buffer;

				PLIB_THREAD_SAFE_DEFINE;
			protected:
				__LoggerWriter( ) : __Locked( false ) { }
				__LoggerWriter( Logger_<_dummy> * _logger, LOGLEVEL _llv ) 
					: __Logger( _logger ), __Level( _llv ), __Locked( false ) { }
				void __Init( Logger_<_dummy> * _logger, LOGLEVEL _llv ) {
					__Logger = _logger;
					__Level = _llv;
				}
				// For thread safe log.
				void __CheckAndLock( ) {
					{
						PLIB_THREAD_SAFE;
						if ( __Locked ) return;
					}
					__Locker.Lock( );
					__Locked = true;
					__Buffer += "[";
					__Buffer += GetCurrentTime();
					__Buffer += "][";
					__Buffer += LogLevelWord( __Level );
					__Buffer += "][";
					__Buffer += Convert::ToString( Threading::ThreadSys::SelfID() );
					__Buffer += "]";
				}

				void __CheckAndUnLock( ) {
					{
						PLIB_THREAD_SAFE;
						if ( !__Locked ) return;
					}
					__Locked = false;
					__Locker.UnLock( );
				}
			public:
				// Common Stream Writer.
				template < typename _TyObject >
				__LoggerWriter & operator << ( const _TyObject & _data ) {
					if ( !__Logger->__LevelApprove( __Level ) ) return *this;
					__CheckAndLock( );
					__Buffer += Convert::ToString( _data );
					return *this;
				}

				__LoggerWriter & operator << ( const __Log_End_of_Line & _eof ) {
					if ( !__Logger->__LevelApprove( __Level ) ) return *this;
					__Buffer += "\r\n";
					// Append the log and clear buffer.
					__Logger->__AppendLogLine( __Buffer );
					__Buffer.Clear( );

					// Un lock.
					__CheckAndUnLock( );
					return *this;
				}
			};

		public:
			__LoggerWriter							Trace_;
			__LoggerWriter							Debug_;
			__LoggerWriter							Notify_;
			__LoggerWriter							Info_;
			__LoggerWriter							Warn_;
			__LoggerWriter							Error_;
			__LoggerWriter							Fatal_;
			
		protected:
			// Internal Parameters.
			
			LOGLEVEL								__Level;
			Uint64									__MaxBytes;
			Uint64									__SplitInterval;
			String									__LogFilePath;
			
			Uint64									__CurrentSize;
			Uint64									__LastSplitTime;
			
			Threading::Mutex						__LogLocker;
			String									__Buffer;
			String									__FlushString;
			
			Threading::Timer						__FlushTimer;
		protected:
			// Append the buffer head.
			void __WriteLineHead( LOGLEVEL _llv, const char * __file, const char * __func, Uint32 __line ) 
			{
				__Buffer += "[";
				__Buffer += GetCurrentTimeBasic( );
				__Buffer += "][";
				__Buffer += LogLevelWord( _llv );
				__Buffer += "][" ;
				__Buffer += Convert::ToString( Threading::ThreadSys::SelfID() );
				__Buffer += "][";
				__Buffer += __file;
				__Buffer += "][";
				__Buffer += __func;
				__Buffer += "][";
				__Buffer += Convert::ToString( __line ) += "]";
			}
			
			void __WriteLineHead( LOGLEVEL _llv, const char * __func, Uint32 __line )
			{
				__Buffer += "[";
				__Buffer += GetCurrentTimeSimple( );
				__Buffer += "][";
				__Buffer += LogLevelWord( _llv );
				__Buffer += "][";
				__Buffer += Convert::ToString( Threading::ThreadSys::SelfID() );
				__Buffer += "][";
				__Buffer += __func;
				__Buffer += "][";
				__Buffer += Convert::ToString( __line );
				__Buffer += "]";				
			}
			
			void __HexPrinter( const char * _Data, Uint32 _Length )
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
					
					__Buffer.Append( _BufferLine, _bufferSize - 1 );
					__Buffer += "\r\n";
				}
			}
			
			// The working timer delegate to flush the log data to the file.
			void __FlushLogData( ) 
			{
				// Switch the buffer.
				do {
					LOGGER_LOCK
					if ( __Buffer.Size() == 0 ) {
						return;
					}
					String __Temp = __Buffer;
					__Buffer = __FlushString;
					__FlushString = __Temp;
				} while ( false );
				
				// Write to the file
				// Check the time and the file size.
				time_t __now = time(NULL);
				if ( ((Int64)(__now - __LastSplitTime)) > (Int64)__SplitInterval || __CurrentSize >= __MaxBytes ) {
					String __newFileName;
					__newFileName.DeepCopy( __LogFilePath );
					__newFileName.Append( GetCurrentTimePostfix( ) );
					File::Move( __LogFilePath, __newFileName );
					
					__LastSplitTime = __now;
					__CurrentSize = 0;
				}
				
				File::Append( __LogFilePath, __FlushString );
				__CurrentSize += __FlushString.Size();
				__FlushString.Clear();
			}
		
			// Check the log level
			bool __LevelApprove( LOGLEVEL _llv ) const {
				return _llv >= __Level;
			}
			
			void __AppendLogLine( const String & _data ) {
				LOGGER_LOCK
				__Buffer += _data;
			}
			
		public:
			// Default Logger C'Str, LogLevel is INFO, Max File Size 
			// is 100MB, Each 24 hours split the file.
			Logger_<_dummy>( ) : 
				__Level( LLV_TRACE ), 
				__MaxBytes( 1024 * 1024 * 100 ), 
				__SplitInterval( 60 * 60 * 24 ),
				__CurrentSize( 0 ),
				__LastSplitTime( (Uint64)time(NULL) )
			{
				Trace_.__Init( this, LLV_TRACE );
				Debug_.__Init( this, LLV_DEBUG );
				Notify_.__Init( this, LLV_NOTIFY );
				Info_.__Init( this, LLV_INFO );
				Warn_.__Init( this, LLV_WARN );
				Error_.__Init( this, LLV_ERROR );
				Fatal_.__Init( this, LLV_FATAL );
				
				__FlushTimer += std::make_pair( this, &Logger_<_dummy>::__FlushLogData );
				//__FlushTimer.SetEnable( true );
			}
			
			~Logger_<_dummy>( ) { __FlushTimer.SetEnable( false ); __FlushLogData( ); }
			
			INLINE void SetFlushTimer( bool _enable ) {
				__FlushTimer.SetEnable( _enable );
			}
			// Common configure.
			INLINE void SetFlushInterval( Uint32 _milliseconds ) {
				__FlushTimer.SetInterval( _milliseconds );
			}
			
			INLINE void SetLogLevel( LOGLEVEL _llv ) {
				__Level = _llv;
			}
			
			INLINE void SetMaxFileSize( Uint64 _maxBytes ) {
				__MaxBytes = _maxBytes;
			}
			
			INLINE void SetSplitInterval( Uint64 _seconds ) {
				__SplitInterval = _seconds;
			}
			
			INLINE void SetLogFilePath( const String & _filepath ) {
				__LogFilePath.DeepCopy( _filepath );
			}
			
			INLINE void SetLastSplitAsTodayBegin( ) {
				Uint64 __time = (Uint64)time(NULL);
				__LastSplitTime = (__time / (60 * 60 * 24)) * (60 * 60 * 24);
			}
			
			INLINE void SetLastSplitTime( Uint64 _time ) {
				__LastSplitTime = _time;
			}
			
			// Simple Format Log Type.
			void FormatWriteSimple_( 
				LOGLEVEL _llv, const char * __func, Uint32 __line, 
			 	const char * __format, ... )
			{
				if ( !__LevelApprove( _llv ) ) return;
				
				String _Line;
				
				va_list pArgList;
				va_start(pArgList, __format);
				Uint32 _length = _Basic_Char::CalcVStringLen( __format, pArgList );
				va_end( pArgList );
				
				va_start(pArgList, __format);
				_Line->Format( __format, _length, pArgList );
				va_end( pArgList );
				
				LOGGER_LOCK
				__WriteLineHead( _llv, __func, __line );
				__Buffer += _Line;
				__Buffer += "\r\n";
			}
			
			// Basic Format log Type.
			void FormatWriteBasic_( 
				LOGLEVEL _llv, const char * __file, 
				const char * __func, Uint32 __line, 
			 	const char * __format, ... )
			{
				if ( !__LevelApprove( _llv ) ) return;
				
				String _Line;
				
				va_list pArgList;
				va_start(pArgList, __format);
				Uint32 _length = _Basic_Char::CalcVStringLen( __format, pArgList );
				va_end( pArgList );
				
				va_start(pArgList, __format);
				_Line->Format( __format, _length, pArgList );
				va_end( pArgList );
				
				LOGGER_LOCK
				__WriteLineHead( _llv, __file, __func, __line );
				__Buffer += _Line;
				__Buffer += "\r\n";
			}
			
			// Hex Log 
			void HexLogSimple_( 
				LOGLEVEL _llv, const char * __func, Uint32 __line, 
				const char * _Prefix, const char * _Data, Uint32 _Length )
			{
				if ( !__LevelApprove( _llv ) ) return;
				
				LOGGER_LOCK
				__WriteLineHead( _llv, __func, __line );
				__Buffer += _Prefix;
				__Buffer += "\r\n";
				__HexPrinter( _Data, _Length );
			}
			
			void HexLogBasic_( 
				LOGLEVEL _llv, const char * __file, const char * __func, Uint32 __line, 
				const char * _Prefix, const char * _Data, Uint32 _Length )
			{
				if ( !__LevelApprove( _llv ) ) return;
				
				LOGGER_LOCK
				__WriteLineHead( _llv, __file, __func, __line );
				__Buffer += _Prefix;
				__Buffer += "\r\n";
				__HexPrinter( _Data, _Length );
			}			
		};
		
		// Static End of Line.
		template < Uint32 _dummy > __Log_End_of_Line
			Logger_<_dummy>::Endl;
		template < Uint32 _dummy > Logger_<_dummy>
			Logger_<_dummy>::GLog;
			
		typedef Logger_<0>		Logger;
		
		#ifdef _PLIB_DEBUG_SIMPLE_
			#define LTrace			Trace_ << "[" << PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LDebug			Debug_ << "[" << PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LNotify			Notify_ << "[" << PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LInfo			Info_ << "[" << PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LWarn			Warn_ << "[" << PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LError			Error_ << "[" << PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LFatal			Fatal_ << "[" << PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			
			
			#define TraceFormat(format, ...)	\
				FormatWriteSimple_( LLV_TRACE, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define DebugFormat(format, ...)	\
				FormatWriteSimple_( LLV_DEBUG, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define NotifyFormat(format, ...)	\
				FormatWriteSimple_( LLV_NOTIFY, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define InfoFormat(format, ...)	\
				FormatWriteSimple_( LLV_INFO, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define WarnFormat(format, ...)	\
				FormatWriteSimple_( LLV_WARN, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define ErrorFormat(format, ...)	\
				FormatWriteSimple_( LLV_ERROR, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define FatalFormat(format, ...)	\
				FormatWriteSimple_( LLV_FATAL, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
				
				
			#define TraceHex( prefix, _data, _length )	\
				HexLogSimple_( LLV_TRACE, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define DebugHex( prefix, _data, _length )	\
				HexLogSimple_( LLV_DEBUG, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define NotifyHex( prefix, _data, _length )	\
				HexLogSimple_( LLV_NOTIFY, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define InfoHex( prefix, _data, _length )	\
				HexLogSimple_( LLV_INFO, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define WarnHex( prefix, _data, _length )	\
				HexLogSimple_( LLV_WARN, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define ErrorHex( prefix, _data, _length )	\
				HexLogSimple_( LLV_ERROR, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define FatalHex( prefix, _data, _length )	\
				HexLogSimple_( LLV_FATAL, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
		#else
			#define LTrace			Trace_ << "[" << __FILE__ << "][" << \
										PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LDebug			Debug_ << "[" << __FILE__ << "][" << \
										PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LNotify			Notify_ << "[" << __FILE__ << "][" << \
										PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LInfo			Info_ << "[" << __FILE__ << "][" << \
										PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LWarn			Warn_ << "[" << __FILE__ << "][" << \
										PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LError			Error_ << "[" << __FILE__ << "][" << \
										PLIB_FUNC_NAME << "][" << __LINE__ << "]"
			#define LFatal			Fatal_ << "[" << __FILE__ << "][" << \
										PLIB_FUNC_NAME << "][" << __LINE__ << "]"
										
										
			#define TraceFormat(format, ...)	\
				FormatWriteBasic_( LLV_TRACE, __FILE__, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define DebugFormat(format, ...)	\
				FormatWriteBasic_( LLV_DEBUG, __FILE__, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define NotifyFormat(format, ...)	\
				FormatWriteBasic_( LLV_NOTIFY, __FILE__, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define InfoFormat(format, ...)	\
				FormatWriteBasic_( LLV_INFO, __FILE__, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define WarnFormat(format, ...)	\
				FormatWriteBasic_( LLV_WARN, __FILE__, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define ErrorFormat(format, ...)	\
				FormatWriteBasic_( LLV_ERROR, __FILE__, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
			#define FatalFormat(format, ...)	\
				FormatWriteBasic_( LLV_FATAL, __FILE__, PLIB_FUNC_NAME, __LINE__, format, __VA_ARGS__ )
		
				
			#define TraceHex( prefix, _data, _length )	\
				HexLogBasic_( LLV_TRACE, __FILE__, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define DebugHex( prefix, _data, _length )	\
				HexLogBasic_( LLV_DEBUG, __FILE__, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define NotifyHex( prefix, _data, _length )	\
				HexLogBasic_( LLV_NOTIFY, __FILE__, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define InfoHex( prefix, _data, _length )	\
				HexLogBasic_( LLV_INFO, __FILE__, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define WarnHex( prefix, _data, _length )	\
				HexLogBasic_( LLV_WARN, __FILE__, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define ErrorHex( prefix, _data, _length )	\
				HexLogBasic_( LLV_ERROR, __FILE__, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
			#define FatalHex( prefix, _data, _length )	\
				HexLogBasic_( LLV_FATAL, __FILE__, PLIB_FUNC_NAME, __LINE__, prefix, _data, _length )
				
		#endif
	}
}

#endif // plib.text.logger.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
