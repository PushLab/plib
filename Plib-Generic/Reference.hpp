/*
* Copyright (c) 2011, Push Chen
* All rights reserved.
* 
* File Name			: reference.hpp
* Propose  			: Reference Count Object Definition.
* 
* Current Version	: 1.2
* Change Log		: Re-Definied.
					: Move to Generic Namespace
* Author			: Push Chen
* Change Date		: 2011-04-26
*/

#pragma once

#ifndef _PLIB_GENERIC_REFERENCE_HPP_
#define _PLIB_GENERIC_REFERENCE_HPP_

#include <Plib-Basic/Allocator.hpp>

namespace Plib
{
	namespace Generic
	{		
		// Reference Object Frame.
		// Replace the ref class in the old version.
		// Use Spin lock to make the thread safe.
		template< 
			typename _TyInternal, 
			typename _TyInterAlloc = Plib::Basic::Allocator< _TyInternal >
		>
		class Reference {
			static _TyInterAlloc	RPItemAlloc;
			// Internal Reference Handle
			// template < typename _TyHandle >
			struct ReferenceHandleT {
				Uint32			_Count;
				_TyInternal * 	_PHandle;
				PLIB_THREAD_SAFE_DEFINE;

				ReferenceHandleT( )
					: _Count( 0 ), _PHandle( NULL ) 
				{
					CONSTRUCTURE;
					_PHandle = RPItemAlloc.Create( );
				}
				ReferenceHandleT( const _TyInternal & _RInternal )
					: _Count( 0 ), _PHandle( NULL )
				{
					CONSTRUCTURE;
					_PHandle = RPItemAlloc.Create( _RInternal );
				}
				ReferenceHandleT( _TyInternal * pInternal )
					: _Count( 1 ), _PHandle(pInternal)
				{
					CONSTRUCTURE;
				}
				~ReferenceHandleT( ) {
					DESTRUCTURE;
					if ( _PHandle != NULL )
						RPItemAlloc.Destroy( _PHandle );
				}
				
				INLINE void Increase( ) 
				{ 
					PLIB_THREAD_SAFE;
					SELF_INCREASE(_Count); 
				}
				INLINE void Decrease( ) 
				{
					PLIB_THREAD_SAFE;
					if ( SELF_DECREASE(_Count) == 0 ) {
						PDELETE( this );
					}
				}
				
				INLINE void Print( std::ostream & os, Uint32 _level = 0 ) {
					PLIB_THREAD_SAFE;
					BUILD_TAB( _level );
					os << "@{//ReferenceHandleT\n"; 
					BUILD_TAB( _level + 1 );
					os << "this: " << this << "\n";
					BUILD_TAB( _level + 1 );
					os << "count: " << _Count << "\n";
					BUILD_TAB( _level + 1 );
					os << "_PHandle: " << _PHandle << "\n";
					BUILD_TAB( _level );
					os << "}";
				}
			};
			
		protected:
			ReferenceHandleT * _Handle;
			
		public: 
			// Default C'Str. Create the Reference 
			// Handle and the Internal Object.
			// Set _Init to false to create an empty reference object.
			Reference< _TyInternal, _TyInterAlloc >( bool _Init = true )
				: _Handle( NULL ) 
			{
				CONSTRUCTURE;
				if ( !_Init ) return;
				PNEW( ReferenceHandleT, _Handle );
				_Handle->Increase( );
			}
			
			// Reference Internal Copy C'str,
			// Create the internal object by its own Copy C'str
			Reference< _TyInternal, _TyInterAlloc >( 
				const _TyInternal & _RInternal )
			{
				CONSTRUCTURE;
				PNEWPARAM( ReferenceHandleT, _Handle, _RInternal );
				_Handle->Increase( );
			}
			
			// Copy C'Str
			Reference< _TyInternal, _TyInterAlloc >( 
				const Reference< _TyInternal, _TyInterAlloc > & RP )
				: _Handle( NULL )
			{
				CONSTRUCTURE;
				if ( RP._Handle == NULL ) return;
				RP._Handle->Increase( );
				_Handle = RP._Handle;
			}

			// Create Reference Object from a point
			Reference<_TyInternal, _TyInterAlloc>( _TyInternal * p )
				: _Handle( NULL )
			{
				CONSTRUCTURE;
				PNEWPARAM( ReferenceHandleT, _Handle, p );
			}
			
			// D'Str
			// Decrease the reference count.
			~Reference< _TyInternal, _TyInterAlloc >( )
			{
				DESTRUCTURE;
				if ( _Handle == NULL ) return;
				_Handle->Decrease( );
			}

			// Operator =, Copy Object.
			Reference< _TyInternal, _TyInterAlloc > & operator = (
				const Reference< _TyInternal, _TyInterAlloc > & RP )
			{
				if ( this == &RP || _Handle == RP._Handle ) return *this;
				if ( _Handle != NULL ) _Handle->Decrease( );
				_Handle = RP._Handle;
				if ( _Handle != NULL ) _Handle->Increase( );
				return *this;
			}
			
			Reference< _TyInternal, _TyInterAlloc > & DeepCopy(
				const Reference< _TyInternal, _TyInterAlloc > & RP )
			{
				if ( this == &RP || _Handle == RP._Handle ) return *this;
				if ( RP._Handle == NULL ) return *this;
				if ( _Handle != NULL ) _Handle->Decrease( );
				PLIB_OBJ_THREAD_SAFE( (*RP._Handle) );
				PNEWPARAM( ReferenceHandleT, _Handle, *RP._Handle->_PHandle );
				_Handle->Increase( );
				return *this;
			}
			
			// Compare. 
			// To use Reference, the Internal Object must support 
			// operator == and operator != at least.
			// the compare will check the handle inside at first
			// time, if they are not the same, then check the 
			// internal object.
			INLINE bool operator == ( const Reference< _TyInternal, _TyInterAlloc > & RP ) const
			{
				if ( _Handle == RP._Handle ) return true;
				if ( _Handle == NULL || RP._Handle == NULL ) return false;
				return (*_Handle->_PHandle == *(RP._Handle->_PHandle));
			}
			
			INLINE bool operator == ( const _TyInternal & _Int ) const
			{
				return ( _Handle == NULL ) ? false : (*_Handle->_PHandle) == _Int;
			}
			
			INLINE bool operator != ( const Reference< _TyInternal, _TyInterAlloc > & RP ) const
			{
				return !(*this == RP);
			}
			
			INLINE bool operator != ( const _TyInternal & _Int ) const
			{
				return !(*this == _Int);
			}
			
			// Point Simulation.
			INLINE _TyInternal & operator * ( )
			{
				return *( _Handle->_PHandle );
			}
			
			INLINE const _TyInternal & operator * ( ) const 
			{
				return *( _Handle->_PHandle );
			}
			
			INLINE _TyInternal * operator -> ( )
			{
				return _Handle->_PHandle;
			}
			INLINE const _TyInternal * operator -> ( ) const
			{
				return _Handle->_PHandle;
			}

			INLINE const operator _TyInternal * () const
			{
				return _Handle->_PHandle;
			}

			INLINE operator _TyInternal * ()
			{
				return _Handle->_PHandle;
			}
			
			// Reference Check Statue.
			INLINE bool RefNull( ) const {
				return _Handle == NULL;
			}
			
			INLINE std::ostream & Print ( std::ostream & os, Uint32 _level = 0 ) {
				BUILD_TAB( _level );
				os << "@{//Reference\n";
				BUILD_TAB( _level + 1 );
				os << "this: " << this << "\n";
				BUILD_TAB( _level + 1 );
				os << "_Handle:";
				if ( _Handle == NULL ) {
					os << "(nil)\n";
					BUILD_TAB( _level );
					return os;
				}
				os << "\n";
				_Handle->Print( os, _level + 1 );
				os << "\n";
				BUILD_TAB( _level );
				os << "}";
				return os;
			}

			const static Reference< _TyInternal, _TyInterAlloc > NullRefObj;
			// The internal type.
			typedef _TyInternal type;
			typedef _TyInterAlloc allocator;
		};

		// Const Null Reference Object.
		template< typename _TyInternal, typename _TyInterAlloc >
		const Reference< _TyInternal, _TyInterAlloc > 
			Reference< _TyInternal, _TyInterAlloc >::NullRefObj( false );
		template< typename _TyInternal, typename _TyInterAlloc >
			_TyInterAlloc Reference< _TyInternal, _TyInterAlloc >::RPItemAlloc;
	}
}

#endif // plib.generic.reference.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */

