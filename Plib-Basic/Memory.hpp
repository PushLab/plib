/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: Memory.hpp
* Propose  			: Malloc/Free/Realloc
* 
* Current Version	: 1.0
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2011-07-13
*/

#pragma once

#ifndef _PLIB_BASIC_MEMORY_HPP_
#define _PLIB_BASIC_MEMORY_HPP_

#include <Plib-Basic/Plib.hpp>

namespace Plib
{
	namespace Basic
	{

		// Object Operator.
	#ifdef PLIB_OBJECT_DEBUG
		// Dummy Spin Lock Function
		inline void objd_print_( const char * prefix, const char * func, void * point )
		{
			static Plib::SpinLocker locker;
			locker.Lock( );
			std::cerr << prefix << "[" << point << "]" << func << std::endl;
			locker.UnLock( );
		}
		#define CONSTRUCTURE															\
			Plib::Basic::objd_print_( "C'STR: ", _PLIB_FUNC_NAME_FULL_, (void *)this )
		#define DESTRUCTURE																\
			Plib::Basic::objd_print_( "D'STR: ", _PLIB_FUNC_NAME_FULL_, (void *)this )

	#else

		#define CONSTRUCTURE
		#define DESTRUCTURE

	#endif

		// Memory Operator.
#ifdef PLIB_MEMORY_DEBUG
		// Static Locker.
		inline Plib::SpinLocker & memd_get_lock_( )
		{
			static Plib::SpinLocker locker;
			return locker;
		}

		// Print
		inline void memd_print_( void * point, const char * func, 
				unsigned int line, const char * _statement )
		{			memd_get_lock_().Lock();
			std::cerr << "[" << func << "][" << line << "](" << point << ") + ";
			std::cerr << _statement << std::endl;
			memd_get_lock_().UnLock();
		}

	#define PMALLOC( _Type, _Obj, _Size )												\
		_Obj = (_Type *)malloc( (_Size ) );												\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__, 				\
				STATEMENT_TO_STRING( _Obj = (_Type *)malloc(_Size)) )
	#define PCMALLOC( _Type, _Obj, _Size ) 												\
		_Type * _Obj = (_Type *)malloc( (_Size) );										\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__, 				\
				STATEMENT_TO_STRING( _Type * _Obj = (_Type *)malloc(_Size)) )
	#define PCREALLOC( _Type, _Source, _Obj, _Size ) 									\
		_Type * _Obj = (_Type *)realloc( _Source, (_Size) );							\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__,				\
				STATEMENT_TO_STRING(_Type * _Obj = (_Type *)realloc(_Source, _Size)))
	#define PFREE( _Obj )																\
		free( _Obj );																	\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__,				\
				STATEMENT_TO_STRING( free(_Obj) ) )

	#define PNEW( _Type, _Obj )															\
		_Obj = new _Type;																\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__,				\
				STATEMENT_TO_STRING( _Obj = new _Type ) )
	#define PNEWPARAM( _Type, _Obj, _Param )											\
		_Obj = new _Type( _Param );														\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__,				\
				STATEMENT_TO_STRING( _Obj = new _Type( _Param ) ) )
	#define PCNEW( _Type, _Obj ) 														\
		_Type * _Obj = new _Type;														\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__,				\
				STATEMENT_TO_STRING( _Type * _Obj = new _Type ) )
	#define PCNEWPARAM( _Type, _Obj, _Param )											\
		_Type * _Obj = new _Type( _Param );												\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__,				\
				STATEMENT_TO_STRING( _Type * _Obj = new _Type( _Param ) ) )
	#define PDELETE( _Obj ) 															\
		delete _Obj;																	\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__,				\
				STATEMENT_TO_STRING( delete _Obj ) )
	#define PNEWARRAY( _Type, _Obj, _Size ) 											\
		_Obj = new _Type[_Size];														\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__,				\
				STATEMENT_TO_STRING( _Obj = new _Type[_Size] )
	#define PDELETEARRAY( _Obj )														\
		delete [] _Obj;																	\
		Plib::Basic::memd_print_( _Obj, _PLIB_FUNC_NAME_FULL_, __LINE__,				\
				STATEMENT_TO_STRING( delete [] _Obj )

#else

#define PMALLOC( _Type, _Obj, _Size )	\
		_Obj = (_Type *)malloc( (_Size) )
#define PCMALLOC( _Type, _Obj, _Size )	\
		_Type * _Obj = (_Type *)malloc( (_Size ) )
#define PCREALLOC( _Type, _Source, _Obj, _Size ) \
		_Type * _Obj = (_Type *)realloc( _Source, (_Size) )
#define PFREE( _Obj )	\
		free( _Obj )

#define PNEW( _Type, _Obj )	\
		_Obj = new _Type
#define PNEWPARAM( _Type, _Obj, _Param ) \
		_Obj = new _Type( _Param )
#define PCNEW( _Type, _Obj ) \
		_Type * _Obj = new _Type
#define PCNEWPARAM( _Type, _Obj, _Param ) \
		_Type * _Obj = new _Type( _Param )
#define PDELETE( _Obj )	\
		delete _Obj
#define PNEWARRAY( _Type, _Obj, _Size ) \
		_Obj = new _Type[_Size]
#define PDELETEARRA(_Obj) \
		delete [] _Obj

#endif
	}
}

#endif // plib.basic.memory.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
