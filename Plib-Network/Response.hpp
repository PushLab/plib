/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: Response.hpp
* Propose  			: Response Basic Interface
* 
* Current Version	: 1.0
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2013-02-17
*/

#pragma once

#ifndef _PLIB_NETWORK_RESPONSE_HPP_
#define _PLIB_NETWORK_RESPONSE_HPP_

#include <Plib-Network/BasicNetwork.hpp>
#include <Plib-Network/PeerInfo.hpp>

namespace Plib
{
	namespace Network
	{

		class IResponse
		{
		public:
            virtual ~IResponse() {}
			virtual void generateResponseObjectFromPackage( const NData & receiveData ) = 0;
		};
	}
}

#endif // plib.network.response.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
