/*
* Copyright (c) 2014, Push Chen
* All rights reserved.
* 
* File Name			: CplusplusEx.hpp
* Propose  			: Provide some template structures can be found in 
*					  c++11 which are not supported by the old complier
* 
* Current Version	: 1.0
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2014-02-09
*/

#pragma once

#ifndef _PLIB_GENERIC_CPLUSPLUSEX_HPP_
#define _PLIB_GENERIC_CPLUSPLUSEX_HPP_

namespace Plib
{
	namespace Generic
	{

		template < class T, T v >
		struct __constant {
			static const T value = v;
			typedef T value_type;
			const operator value_type() const { return value; }
			const value_type operator()() const { return value; }
		};

		typedef __constant< bool, true > _True;
		typedef __constant< bool, false > _False;

		// Enable If 
#if __cplusplus > 199711L
		template< bool B, class T = void >
		struct Enable_If : public std::enable_if<B, T> {};
#else
		template< bool B, class T = void >
		struct Enable_If { typedef T type; };

		template< class T >
		struct Enable_If<false, T> {};
#endif

		// Is_Convertible
#if __cplusplus > 199711L
		template< class From, class To >
		struct Is_Convertible : public std::is_convertible<From, To> {};
#else
		template< class From, class To >
		struct Is_Convertible
		{
			typedef unsigned char __false_type;
			typedef unsigned int  __true_type;

			static From __from;

			template < typename T > struct _checker {
				static __false_type _do_check( ... );
				static __true_type _do_check(T);
			};
			static const bool value = __constant< bool, ( 
				sizeof(_checker<To>::_do_check(__from)) ==
				sizeof(__true_type)
				) 
			>::value;
			
			operator bool() const { return value; }
		};
#endif
	}
}

#endif // plib.generic.cplusplusex.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
