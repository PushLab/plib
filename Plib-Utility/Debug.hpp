/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: debug.hpp
* Propose  			: Debug Macro Definition.
* 
* Current Version	: 1.1
* Change Log		: Re-Definied.
* Author			: Push Chen
* Change Date		: 2011-01-08
*/

#pragma once

#ifndef _PLIB_UTILITY_DEBUG_HPP_
#define _PLIB_UTILITY_DEBUG_HPP_

#include <Plib-Text/Text.hpp>
#include <Plib-Threading/Thread.hpp>

#ifndef _PLIB_DEBUG_SIMPLE_

#define PLIB_COMMON_PRINT_HEAD(_TObj)		\
	_TObj << "[" << Plib::Text::GetCurrentTimeBasic() <<	\
	"][" << Plib::Threading::ThreadSys::ThreadId() << "][" << __FILE__ << "][" << \
	PLIB_FUNC_NAME << "][" << __LINE__ << "]"
	
#else

#define PLIB_COMMON_PRINT_HEAD(_TObj)		\
	_TObj << "[" << Plib::Text::GetCurrentTimeSimple() <<	\
	"][" << Plib::Threading::ThreadSys::ThreadId() << "][" << PLIB_FUNC_NAME << \
	"][" << __LINE__ << "]"

#endif

    INLINE bool __PLIB_PRINT_BOOL( std::ostream & _os, bool _Exp ) {
        _os << (_Exp ? "True" : "False") << std::endl;
        return _Exp;
    }
	INLINE bool __PLIB_PRINT_ELSE_BOOL( 
		std::ostream & _os, const char * _Exp, bool _bExp ) {
		PLIB_COMMON_PRINT_HEAD(_os) << " {else: " << _Exp <<
			"}: " << (_bExp ? "True" : "False") << std::endl;
		return _bExp;
	}
    
#define PLIB_COMMON_TRACE_OBJ_( _TObj, _Words ) 	\
	PLIB_COMMON_PRINT_HEAD(_TObj) << _Words << std::endl
#define PLIB_COMMON_DUMP_OBJ_( _TObj, _Dump )		\
	PLIB_COMMON_PRINT_HEAD(_TObj) << "{" #_Dump "}:" <<	\
	Plib::Text::Convert::ToString(_Dump) << std::endl;
#define PLIB_COMMON_CHECK_OBJ_( _TObj, _Exp )		\
	PLIB_COMMON_PRINT_HEAD(_TObj) << "{" #_Exp "}: ";	\
	if ( __PLIB_PRINT_BOOL( _TObj, (_Exp) ) )
#define PLIB_COMMON_ELSECHK_OBJ_( _TObj, _Exp )		\
	else if ( __PLIB_PRINT_ELSE_BOOL( _TObj, #_Exp, (_Exp) ) )

#define PLIB_COMMON_STRACE( _Words )	\
	PLIB_COMMON_TRACE_OBJ_( std::cout, _Words )
#define PLIB_COMMON_ETRACE( _Words )	\
	PLIB_COMMON_TRACE_OBJ_( std::cerr, _Words )
#define PLIB_COMMON_SDUMP( _Dump )		\
	PLIB_COMMON_DUMP_OBJ_( std::cout, _Dump )
#define PLIB_COMMON_EDUMP( _Dump )		\
	PLIB_COMMON_DUMP_OBJ_( std::cerr, _Dump )
#define PLIB_COMMON_SCHECK( _Exp )		\
	PLIB_COMMON_CHECK_OBJ_( std::cout, _Exp )
#define PLIB_COMMON_ECHECK( _Exp )		\
	PLIB_COMMON_CHECK_OBJ_( std::cerr, _Exp )
#define PLIB_COMMON_SELSECHK( _Exp )	\
	PLIB_COMMON_ELSECHK_OBJ_( std::cout, _Exp )
#define PLIB_COMMON_EELSECHK( _Exp )	\
	PLIB_COMMON_ELSECHK_OBJ_( std::cerr, _Exp )
		
// Debug Compnonent Definition.
#ifdef _PLIB_STD_DEBUG_
#define PTRACE( _Words )	PLIB_COMMON_STRACE( _Words )
#define PDEBUG( _Words )	PLIB_COMMON_STRACE( _Words )
#define PNOTIFY( _Words )	PLIB_COMMON_STRACE( _Words )
#define PINFO( _Words )		PLIB_COMMON_STRACE( _Words )
#define PWARN( _Words )		PLIB_COMMON_STRACE( _Words )
#define PERROR( _Words )	PLIB_COMMON_STRACE( _Words )
#define PFATAL( _Words )	PLIB_COMMON_STRACE( _Words )
#define PDUMP( _Dump )		PLIB_COMMON_SDUMP( _Dump )
#define PIF( _Exp )			PLIB_COMMON_SCHECK( _Exp )
#define PELSEIF( _Exp )		PLIB_COMMON_SELSECHK( _Exp )
#define PELSE				else
		
#elif defined _PLIB_ERR_DEBUG_
#define PTRACE( _Words )	PLIB_COMMON_ETRACE( _Words )
#define PDEBUG( _Words )	PLIB_COMMON_ETRACE( _Words )
#define PNOTIFY( _Words )	PLIB_COMMON_ETRACE( _Words )
#define PINFO( _Words )		PLIB_COMMON_ETRACE( _Words )
#define PWARN( _Words )		PLIB_COMMON_ETRACE( _Words )
#define PERROR( _Words )	PLIB_COMMON_ETRACE( _Words )
#define PFATAL( _Words )	PLIB_COMMON_ETRACE( _Words )
#define PDUMP( _Dump )		PLIB_COMMON_EDUMP( _Dump )
#define PIF( _Exp )			PLIB_COMMON_ECHECK( _Exp )
#define PELSEIF( _Exp )		PLIB_COMMON_EELSECHK( _Exp )
#define PELSE				else

#elif defined _PLIB_LOG_DEBUG_
#define PTRACE( _Words )	Plib::Text::Logger::GLog.LTrace << _Words << Plib::Text::Logger::Endl
#define PDEBUG( _Words )	Plib::Text::Logger::GLog.LDebug << _Words << Plib::Text::Logger::Endl
#define PNOTIFY( _Words )	Plib::Text::Logger::GLog.LNotify << _Words << Plib::Text::Logger::Endl
#define PINFO( _Words )		Plib::Text::Logger::GLog.LInfo << _Words << Plib::Text::Logger::Endl
#define PWARN( _Words )		Plib::Text::Logger::GLog.LWarn << _Words << Plib::Text::Logger::Endl
#define PERROR( _Words )	Plib::Text::Logger::GLog.LError << _Words << Plib::Text::Logger::Endl
#define PFATAL( _Words )	Plib::Text::Logger::GLog.LFatal << _Words << Plib::Text::Logger::Endl
#define PDUMP( _Dump )		Plib::Text::Logger::GLog.LTrace << "{" #_Dump "}" << \
								Plib::Text::Convert::ToString(_Dump) << Plib::Text::Logger::Endl
#define PIF( _Exp )			if ( _Exp )
#define PELSEIF( _Exp )		else if ( _Exp )
#define PELSE				else

#else
#define PTRACE( _Words )
#define PDEBUG( _Words )
#define PNOTIFY( _Words )
#define PINFO( _Words )
#define PWARN( _Words )
#define PERROR( _Words )
#define PFATAL( _Words )
#define PDUMP( _Dump )
#define PIF( _Exp )			if ( _Exp )
#define PELSEIF( _Exp )		else if ( _Exp )
#define PELSE				else

#endif

#endif // plib.utility.debug.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */

