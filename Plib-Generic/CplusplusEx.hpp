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
			enum { value = }
			operator bool () { }
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
