/*
* Copyright (c) 2010, Push Chen
* All rights reserved.
* 
* File Name			: string.hpp
* Propose  			: Reference String Definition.
* 
* Current Version	: 1.1
* Change Log		: for 1.1, I found several bugs in the constructures.
* Change Log		: First Definition.
* Author			: Push Chen
* Change Date		: 2011-01-09
*/

#pragma once

#ifndef _PLIB_COMMON_STRING_HPP_
#define _PLIB_COMMON_STRING_HPP_

#include <Plib-Generic/Pool.hpp>
#include <Plib-Generic/ArrayList.hpp>

namespace Plib
{
	namespace Text
	{
		// Pre-definition
		template< typename _Basic_C >
		class _RString;
		
		// String Basic Object.
		// Can also act as string buffer.
		// The template parameter _Basic_C is used to 
		// specified different char type in case you need to use
		// Unicode.
		template< typename _Basic_C >
		class _StringBasic
		{
			friend class _RString< _Basic_C >;
			// _Basic_C should support:
			//	Typename CharType, the char type.
			//	CharType EOL, the end of line, '\0' or L'\0';
			//	Function StringLength.
			typedef typename _Basic_C::CharType				CharType;
			typedef Uint32									Size_T;
			typedef Plib::Generic::Pair< 
				typename _Basic_C::CharType *, Uint32 >		DataPairT;

			#define _FORMAT_LENGTH( _Basic, _Captial ) 	\
				( ( ( _Basic / _Captial ) + (Uint32)( _Basic % _Captial > 0) ) * _Captial )
				
		public:
			enum { NoPos = (Size_T)-1, Captial = 0x40U };
		
		protected:
			Size_T					_BufferSize;
			Size_T					_Length;
			CharType *				_Buffer;
			DataPairT *				_BufferPair;
			
			// To solve the static object's iniitlize order, 
			// I use this static method to create a static
			// variable. 
			static Plib::Generic::Pool< DataPairT > & gBufferPool() {
				// the static pool is to store the global CharType buffer.
				static Plib::Generic::Pool< DataPairT > _pool;
				return _pool;
			}
			PLIB_THREAD_SAFE_DEFINE;
		protected:
			// Check if has enough buffer to append words.
			// if not, realloc the buffer.
			INLINE void _CheckAndRealloc( Size_T _AppendSize )
			{
				// The Append Size cannot be zero,
				// the main logic should check and always
				// make user the _AppendSize is large than zero.
				assert( _AppendSize != 0 );
				
				// Before we make any change to the buffer,
				// the length of the data should always
				// equal less than the buffer size.
				assert( _Length <= _BufferSize );
				
				// If the empty buffer is large than the append size,
				// which means we still has enough empty space
				// to insert/append the data, then there is no need
				// to realloc the buffer.
				if ( (_BufferSize - _Length ) > _AppendSize ) return;
				
				// Calculate the alloc size and modify the buffer size.
				Size_T _FormatSize = _FORMAT_LENGTH( _AppendSize, Captial );
				_BufferSize += _FormatSize;
				
				// We need sizeof(CharType) to store End-Of-Line character.
				Size_T _AllocSize = (_BufferSize + 1) * sizeof(CharType);
				
				// Check if the buffer is still null
				if ( _Buffer == NULL ) {
					PMALLOC( CharType, _Buffer, _AllocSize);
				} else {
					PCREALLOC( CharType, _Buffer, _Tmp, _AllocSize );
					_Buffer = _Tmp;
				}
				// Make sure we still have enough memory.
				assert( _Buffer != NULL );
			}
			
			// Get a buffer pair from the global buffer pool
			INLINE void _INIT_BUFFER( ) {
				// This method can only be invoked in constructure.
				assert( _BufferPair == NULL );
				_BufferPair = gBufferPool( ).Get( );
				// We need to confirm the buffer pair returned from
				// the global pool is not null.
				assert( _BufferPair != NULL );
				_Buffer = _BufferPair->First;
				_BufferSize = _BufferPair->Second;
			}
			
			// Return my buffer pair to the global buffer pool
			INLINE void _REL_BUFFER( ) {
				// Reset the data.
				_BufferPair->First = _Buffer;
				_BufferPair->Second = _BufferSize;
				gBufferPool( ).Return( _BufferPair );
				_BufferPair = NULL;
			}
		public:
			// Default C'Str
			_StringBasic< _Basic_C > ( ) 
				: _BufferSize( 0 ), _Length( 0 ), _BufferPair( NULL )
			{
				CONSTRUCTURE;
				_INIT_BUFFER( );
				_CheckAndRealloc( 1 );
				_Buffer[0] = _Basic_C::EOL;
			}
			
			// Copy C'Str.
			_StringBasic< _Basic_C > ( const _StringBasic< _Basic_C > & _String )
				: _Length( 0 ), _BufferPair( NULL )
			{
				CONSTRUCTURE;
				_INIT_BUFFER( );
				// For thread safe.
				PLIB_OBJ_THREAD_SAFE( _String );
				
				Size_T _RealLength = ( _String._Length == 0 ) ? 
					Captial : _String._Length;
				_CheckAndRealloc( _RealLength );
				
				if ( _String._Length != 0 ) {
					::memcpy( _Buffer, _String._Buffer, _String._Length );
					_Length = _String._Length;
				}
				_Buffer[_Length] = _Basic_C::EOL;
			}
			
			// D'Str.
			// Return the Buffer Pair.
			~_StringBasic< _Basic_C >( )
			{
				DESTRUCTURE;
				_REL_BUFFER( );
			}
						
			// Basic Operator Function.
			// Append new words to the end of the string.
			INLINE void Append( const CharType _c ) {
				PLIB_THREAD_SAFE;
				_CheckAndRealloc( 1 );
				_Buffer[_Length] = _c;
				SELF_INCREASE( _Length );
				_Buffer[_Length] = _Basic_C::EOL;
			}
			
			// Append several character to the end of the string.
			INLINE void Append( Size_T _Count, const CharType _c ) {
				// if the count is zero, this method will be
				// no meaning.
				assert( _Count != 0 );
				PLIB_THREAD_SAFE;
				_CheckAndRealloc( _Count );
				for ( Uint32 i = 0; i < _Count; ++i ) {
					_Buffer[_Length++] = _c;
				}
				_Buffer[_Length] = _Basic_C::EOL;
			}
			
			// Append specified data to the end of the string.
			INLINE void Append( const CharType * _Data, Size_T _DLength ) {
				// The invoker should always check the incoming data
				// is not null.
				assert( _Data != NULL );
				// and the data's length is not zero.
				assert( _DLength != 0 );
				
				PLIB_THREAD_SAFE;
				_CheckAndRealloc( _DLength );
				::memcpy( _Buffer + _Length, _Data, _DLength );
				_Length += _DLength;
				_Buffer[_Length] = _Basic_C::EOL;
			}
			
			// Append the terminated string to the current string's end
			INLINE void Append( const CharType * _Data ) {
				// The invoker should always check the incoming data
				// is not null
				assert( _Data != NULL );
				Size_T _AppendLength = _Basic_C::StringLength( _Data );
				Append( _Data, _AppendLength );
			}

			// Append another string object to current string's end.
			INLINE void Append( const _StringBasic< _Basic_C > & SB ) {
				PLIB_OBJ_THREAD_SAFE( SB );
				// No need to append if the SB is empty.
				if ( SB._Length == 0 ) return;
				Append( SB._Buffer, SB._Length );
			}

			// Insert some words to specified position of the string.
			INLINE void Insert( const CharType * _Data, Size_T _DLength, Size_T _Pos) {
				// The invoker should always check the incoming parameters
				// are validate to current method.
				assert( _Data != NULL );
				assert( _DLength != 0 );
				
				// if the position is NoPos or equal to the _length
				// invoke append instead.
				if ( _Pos == NoPos || _Pos == _Length ) {
					Append( _Data, _DLength );
					return;
				}
				
				// the _pos must be less than the _length
				assert( _Pos < _Length );
				
				PLIB_THREAD_SAFE;
				this->_CheckAndRealloc( _DLength );
				// Move and copy.
				::memmove( _Buffer + _Pos + _DLength, _Buffer + _Pos, 
					sizeof(CharType) * (_Length - _Pos + 1) );
				::memcpy( _Buffer + _Pos, _Data, sizeof(CharType) * _DLength );
				_Length += _DLength;
				_Buffer[_Length] = _Basic_C::EOL;
			}
			
			// Insert a non-terminal string.
			INLINE void Insert( const CharType * _Data, Size_T _Pos ) {
				// Check the incoming parameter
				assert( _Data != NULL );
				Size_T _DLength = _Basic_C::StringLength( _Data );
				return Insert( _Data, _DLength, _Pos );
			}
			
			// Insert another string to current string.
			INLINE void Insert( const _StringBasic< _Basic_C > & SB, Size_T _Pos ) {
				PLIB_OBJ_THREAD_SAFE( SB );
				// Check if SB is empty.
				if ( SB._Length == 0 ) return;
				Insert( SB._Buffer, SB._Length, _Pos );
			}
			
			// Remove from _First, remove _Size characters.
			INLINE void Remove( Size_T _First, Size_T _Size ) {
				// Check the incoming parameters
				// at least, current string must not be empty
				// otherwise the remove operator is meaning-less..
				assert( _First < _Length );
				// Nothing need to do if one want to remove zero byte.
				if ( _Size == 0 ) return;
				// One can not remove zero byte, or more then the string contains.
				assert( _Size != 0 && _Size <= ( _Length - _First ) );
				//Size_T _RemoveSize = _Last - _First;
				PLIB_THREAD_SAFE;
				if ( _Size != (_Length - _First) ) {
					// if one want to remove from the _First to the end
					// of the string, we do not need to memmove anymore.
					::memmove( _Buffer + _First, _Buffer + _First + _Size, 
						sizeof(CharType) * ( _Length - _First - _Size ) );
				}
				_Length -= _Size;
				_Buffer[_Length] = _Basic_C::EOL;
			}
			
			// Remove the "_First" character.
			INLINE void Remove( Size_T _First ) {
				Remove( _First, 1 );
			}

			// Cast
			INLINE operator const CharType * ( ) const {
				return _Buffer;
			}

			// for std::string.
			INLINE const CharType * C_Str( ) const {
				return _Buffer;
			}

			// Size and clear.
			INLINE Size_T Size( ) const {
				return _Length;
			}

			// The clear method is just to set the 
			// length to zero.
			INLINE void Clear( ) {
				PLIB_THREAD_SAFE;
				_Length = 0;
				_Buffer[_Length] = _Basic_C::EOL;
			}
			
			// Check if the string is an empty one.
			INLINE bool Empty( ) const {
				return _Length == 0;
			}
						
			// Operators
			INLINE CharType & operator [] ( Uint32 _Idx ) {
				// The Index must be validate, means
				// must less than the string length.
				assert( _Idx < _Length );
				return _Buffer[_Idx];
			}

			INLINE const CharType & operator [] ( Uint32 _Idx ) const {
				// The Index must be validate, means
				// must less than the string length.
				assert( _Idx < _Length );
				return _Buffer[_Idx];
			}
			
			// Append Operator +=
			INLINE _StringBasic< _Basic_C > & operator += ( const CharType _c ) {
				Append( _c );
				return *this;
			}
			
			INLINE _StringBasic< _Basic_C > & operator += ( const CharType * _Data ) {
				Append( _Data );
				return *this;
			}

			INLINE _StringBasic< _Basic_C > & operator += ( 
				const _StringBasic< _Basic_C > & SB ) {
				Append( SB );
				return *this;
			}

			// Operator +, create new string object.
			INLINE _StringBasic< _Basic_C > operator + ( const CharType * _Data ) const {
				_StringBasic< _Basic_C > _String(*this);
				_String.Append( _Data );
				return _String;
			}

			INLINE _StringBasic< _Basic_C > operator + ( 
				const _StringBasic< _Basic_C > & SB ) const {
				_StringBasic< _Basic_C > _String(*this);
				_String.Append( SB );
				return _String;
			}

			// Copy Operators.
			// Change current string's data to some other.
			INLINE _StringBasic< _Basic_C > & operator = ( 
				const _StringBasic< _Basic_C > & SB ) {
				if ( this == &SB ) return *this;
				// We need to lock both object.
				PLIB_THREAD_SAFE;
				PLIB_OBJ_THREAD_SAFE( SB );
				// Nothing need to do if SB is empty.
				// just set current string to emtpy.
				// otherwise, copy the data.
				if ( SB._Length != 0 ) {
					if ( _BufferSize < SB._BufferSize )
						this->_CheckAndRealloc( SB._BufferSize - _BufferSize );
					::memcpy( _Buffer, SB._Buffer, SB._Length );
				}
				_Length = SB._Length;
				_Buffer[_Length] = _Basic_C::EOL;
				return *this;
			}

			// Copy the _Data to current string. the data cannot be NULL.
			INLINE _StringBasic< _Basic_C > & operator = ( const CharType * _Data ) {
				// Data checking
				assert( _Data != NULL );
				this->Clear();
				this->Append( _Data );
				return *this;
			}
			
			// Compare Operators.
			INLINE bool operator == ( const _StringBasic< _Basic_C > & SB ) const {
				PLIB_THREAD_SAFE;
				PLIB_OBJ_THREAD_SAFE( SB );
				// The same object.
				if ( this == &SB ) return true;
				// the length is not equal, means these two string is not equal.
				if ( _Length != SB._Length ) return false;
				// according the last statement, these two
				// strings' length are equal. And current string's length
				// is zero, means both strings are empty. 
				// They must be equal.
				if ( _Length == 0 ) return true;
				// use memcmp to compare the memory.
				return (memcmp( _Buffer, SB._Buffer, sizeof(CharType) * _Length ) == (int)0);
			}

			INLINE bool operator == ( const CharType * _Data ) const {
				PLIB_THREAD_SAFE;
				// NULL != NULL, because NULL means nothing.
				// I don't know what nothing means, so even
				// current string is empty, if _Data is NULL,
				// they are still not equal.
				if ( _Data == NULL ) return false;
				// if the _Data is not NULL but current string
				// is empty, they are not equal.
				if ( _Length == 0 ) return false;
				
				// Get the _Data's length and compare.
				Size_T _DLength = _Basic_C::StringLength( _Data );
				if ( _DLength != _Length ) return false;
				return (memcmp( _Buffer, _Data, sizeof(CharType) * _Length ) == (int)0);
			}
			
			// Not equal, the oppsite of equal.
			INLINE bool operator != ( const _StringBasic< _Basic_C > & SB ) const {
				return !( *this == SB );
			}

			INLINE bool operator != ( const CharType * _Data ) const {
				return !( *this == _Data );
			}
			
			// Smaller than, usually used in std::map.
			INLINE bool operator < ( const _StringBasic< _Basic_C > & SB ) const {
				// the two strings are the same one.
				if ( this == &SB ) return false;

				PLIB_THREAD_SAFE;
				PLIB_OBJ_THREAD_SAFE( SB );
				// if current string is empty and the other string is not empty
				// it's true current string is smaller than the other one.
				if ( _Length == 0 && SB._Length > 0 ) return true;
				// if these two strings are both empty, that means they should be
				// the same, so false.
				if ( _Length == 0 && SB._Length == 0 ) return false;
				// if current string's length is smaller than the other string,
				// just need to confirm if in current string's length, 
				// the data is smaller.
				if ( _Length < SB._Length ) {
					return (memcmp(_Buffer, SB._Buffer, sizeof(CharType) * _Length) <= (int)0);
				}
				// otherwise, confirm in other string's length
				return (memcmp(_Buffer, SB._Buffer, sizeof(CharType) * SB._Length) < (int)0);
			}

			// Compare with non-terminal string.
			INLINE bool operator < ( const CharType * _Data ) const {
				// You cannot compare with a null string. always false.
				if ( _Data == NULL ) return false;
				// if the _Data is not NULL and current string is empty,
				// must be true.
				if ( _Length == 0 ) return true;
				// Get the _Data's length and compare.
				Size_T _DLength = _Basic_C::StringLength( _Data );

				PLIB_THREAD_SAFE;
				if ( _Length < _DLength ) {
					return (memcmp(_Buffer, _Data, sizeof(CharType) * _Length) <=(int)0);
				}
				return (memcmp(_Buffer, _Data, sizeof(CharType) * _DLength) < (int)0);
			}
			
			// Advance Methods.
			
			// Format
			Size_T Format( const CharType * _Format, Size_T _FLength, va_list _Args )
			{
				this->Clear();
				PLIB_THREAD_SAFE;
				this->_CheckAndRealloc( _FLength );
				_Length = _Basic_C::StringPrintf( _Buffer, _BufferSize, _Format, _Args );
				return _Length;
			}

			// Change Case
			INLINE _StringBasic< _Basic_C > & ToUpper( ) {
				PLIB_THREAD_SAFE;

				std::locale _Loc;
				for ( Uint32 i = 0; i < _Length; ++i )
					_Buffer[i] = std::toupper( _Buffer[i], _Loc );
				return *this;
			}
			INLINE _StringBasic< _Basic_C > & ToLower( ) {
				PLIB_THREAD_SAFE;
				
				std::locale _Loc;
				for ( Uint32 i = 0; i < _Length; ++i )
					_Buffer[i] = std::tolower( _Buffer[i], _Loc );
				return *this;
			}
			// Remove the white space at the beginning and end of current string.
			INLINE _StringBasic< _Basic_C > & Trim( ) {
				std::locale _Loc;
				// Trim Head.
				Size_T _EmptyCount = 0;
				while ( _EmptyCount < _Length && 
					std::isspace(_Buffer[_EmptyCount], _Loc ) ) ++_EmptyCount;
				this->Remove(0, _EmptyCount);
				// Trim Tail.
				_EmptyCount = 0;
				while ( _EmptyCount < _Length &&
					std::isspace(_Buffer[_Length - 1 - _EmptyCount], _Loc) )
					++_EmptyCount;
				if ( _EmptyCount == 0 ) return *this;
				_Length -= _EmptyCount;
				_Buffer[_Length] = _Basic_C::EOL;
				return *this;
			}

			// Check if the string is started with specified substring.
			INLINE bool StartWith( const CharType * _Data ) const {
				// Parameter checking
				assert( _Data != NULL );
				// If current string is empty, it's false.
				if ( _Length == 0 ) return false;
				Size_T _DLength = _Basic_C::StringLength( _Data );
				if ( _Length < _DLength ) return false;
				PLIB_THREAD_SAFE;
				return _Basic_C::StrnCmp( _Buffer, _Data, _DLength );
			}

			INLINE bool StartWith( const _StringBasic< _Basic_C > & SB ) const {
				if ( SB._Length == 0 ) return false;
				if ( _Length < SB._Length ) return false;
				PLIB_THREAD_SAFE;
				PLIB_OBJ_THREAD_SAFE( SB );
				return _Basic_C::StrnCmp( _Buffer, SB._Buffer, SB._Length );
			}

			// Check if the string is ended with specified substring.
			INLINE bool EndWith( const CharType * _Data ) const {
				// Parameter checking
				assert( _Data != NULL );
				// If current string is empty, it's false.
				if ( _Length == 0 ) return false;
				Size_T _DLength = _Basic_C::StringLength( _Data );
				if ( _Length < _DLength ) return false;
				PLIB_THREAD_SAFE;
				return _Basic_C::StrnCmp( _Buffer + _Length - _DLength, 
					_Data, _DLength );
			}

			INLINE bool EndWith( const _StringBasic< _Basic_C > & SB ) const {
				if ( SB._Length == 0 ) return false;
				if ( _Length < SB._Length ) return false;
				PLIB_THREAD_SAFE;
				PLIB_OBJ_THREAD_SAFE( SB );
				return _Basic_C::StrnCmp( _Buffer + _Length - SB._Length, 
					SB._Buffer, SB._Length );
			}

			// Alogrithm.
			INLINE Size_T Find( CharType _C, Size_T _OffSet = 0 ) const {
				if ( _Length == 0 ) return NoPos;
				if ( _OffSet >= _Length ) return NoPos;
				PLIB_THREAD_SAFE;
				// Loop to find
				for ( Size_T _idx = _OffSet; _idx < _Length; ++_idx )
					if ( _Buffer[_idx] == _C ) return _idx;
				return NoPos;
			}

			// Find the substring.
			INLINE Size_T Find( const CharType * _Data, 
				Size_T _DLength, Uint32 _OffSet ) const {
				// Parameter checking
				if ( _Data == NULL || _DLength == 0 ) return NoPos;
				if ( _OffSet >= _Length || _DLength > (_Length - _OffSet) ) return NoPos;
				// The default algorithm is not kmp.
				// I haven't take any name yet.
				PLIB_THREAD_SAFE;
				Size_T _Pos = _Basic_C::Find(
				 	_Buffer + _OffSet, _Length - _OffSet, _Data, _DLength );
				if ( _Pos == NoPos ) return NoPos;
				return _OffSet + _Pos;
			}
			
			INLINE Size_T Find( const CharType * _Data, Uint32 _OffSet = 0 ) const {
				if ( _Data == NULL ) return NoPos;
				Size_T _DLength = _Basic_C::StringLength( _Data );
				return Find( _Data, _DLength, _OffSet );
			}
			
			Size_T Find( const _StringBasic< _Basic_C > & SB, Uint32 _OffSet = 0) const  {
				return Find( SB._Buffer, SB._Length, _OffSet );
			}
			
			// Find the last character.
			Size_T FindLast( CharType _C, Size_T _OffSet = NoPos ) const
			{
				if ( _Length == 0 ) return NoPos;
				if ( _OffSet == NoPos || _OffSet >= _Length ) _OffSet = _Length - 1;
				PLIB_THREAD_SAFE;
				for ( Size_T _idx = _OffSet; _idx > 0; --_idx )
					if ( _Buffer[_idx] == _C ) return _idx;
				if ( _Buffer[0] == _C ) return 0;
				return NoPos;
			}
			
			// Converting.
			// Return the int value.
			INLINE Int32 IntValue() const {
				return _Basic_C::AtoI( _Buffer );
			}
			INLINE Uint32 UintValue() const {
				return (Uint32)_Basic_C::AtoI( _Buffer );
			}
			INLINE Int64 Int64Value() const {
				return _Basic_C::AtoL( _Buffer );
			}
			INLINE Uint64 Uint64Value() const {
				return (Uint64)_Basic_C::AtoL( _Buffer );
			}
			// Return Float Value
			INLINE double DoubleValue() const {
				return (double)_Basic_C::AtoF( _Buffer );
			}
			INLINE float FloatValue() const {
				return (float)_Basic_C::AtoF( _Buffer );
			}
			// Bool Value
			INLINE bool BoolValue() const {
				return _Basic_C::BoolValue( _Buffer, _Length );
			}
		};
		
		// This is the reference version of _StringBasic.
		template< typename _Basic_C >
		class _RString : public Plib::Generic::Reference< _StringBasic< _Basic_C > >
		{
		public:
			typedef typename _Basic_C::CharType								CharType;
			typedef Uint32													Size_T;
			typedef Plib::Generic::Reference< _StringBasic< _Basic_C > >	TFather;
			enum { NoPos = _StringBasic<_Basic_C>::NoPos };
		protected:
			// For Null String C'Str
			_RString< _Basic_C >( bool _beNull ) : TFather( false ) { CONSTRUCTURE; }
		public:
			// Default C'Str
			_RString< _Basic_C >( ) : TFather( true ) { CONSTRUCTURE; }
			// Initialize with single character.
			_RString< _Basic_C >( CharType _c ) : TFather( true ) {
				CONSTRUCTURE;
				TFather::_Handle->_PHandle->Append( _c );
			}
			// Initialize the string with _l _c.
			_RString< _Basic_C >( Size_T _l, CharType _c ) : TFather( true ) {
				CONSTRUCTURE;
				TFather::_Handle->_PHandle->Append( _l, _c );
			}
			// Initialize the string with a non-terminal string
			_RString< _Basic_C >( const CharType * _data ) : TFather( true ) {
				CONSTRUCTURE;
				TFather::_Handle->_PHandle->Append( _data );
			}
			// Initialzie the string with a data buffer.
			_RString< _Basic_C >( const CharType * _data, Size_T _len ) : TFather( true ) {
				CONSTRUCTURE;
				TFather::_Handle->_PHandle->Append( _data, _len );
			}
			// Copy C'str.
			_RString< _Basic_C >( const _StringBasic< _Basic_C > & _string )
				: TFather( _string ) { CONSTRUCTURE; }
			// Default Copy C'Str.
			_RString< _Basic_C >( const _RString< _Basic_C > & rhs ) : TFather( rhs )
				{ CONSTRUCTURE; }
			~_RString< _Basic_C >( ){ DESTRUCTURE; }
			
			// Normal Methods
			// Append new words to the end of the string.
			INLINE void Append( const CharType _c ) {
				return TFather::_Handle->_PHandle->Append( _c );
			}
			// Append several character to the end of the string.
			INLINE void Append( Size_T _Count, const CharType _c ) {
				return TFather::_Handle->_PHandle->Append( _Count, _c );
			}
			// Append specified data to the end of the string.
			INLINE void Append( const CharType * _data, Size_T _len ) {
				return TFather::_Handle->_PHandle->Append( _data, _len );
			}
			// Append the terminated string to the current string's end
			INLINE void Append( const CharType * _data ) {
				return TFather::_Handle->_PHandle->Append( _data );
			}
			// Append another rstring object to current string's end.
			INLINE void Append( const _RString< _Basic_C > & _rstring ) {
				return TFather::_Handle->_PHandle->Append( 
					*(_rstring._Handle->_PHandle) );
			}
			// Append another string object to current string's end.
			INLINE void Append( const _StringBasic< _Basic_C > & _string ) {
				return TFather::_Handle->_PHandle->Append( _string );
			}
			
			// Insert a non-terminal string.
			INLINE void Insert( const CharType * _data, Size_T _pos ) {
				return TFather::_Handle->_PHandle->Insert( _data, _pos );
			}
			// Insert another string to current string.
			INLINE void Insert( const CharType * _data, 
				Size_T _length, Size_T _pos ) { 
				return TFather::_Handle->_PHandle->Insert( _data, _length, _pos );
			}
			INLINE void Insert( const _StringBasic< _Basic_C > & _string ) {
				return TFather::_Handle->_PHandle->Insert( _string );
			}
			INLINE void Insert( const _RString< _Basic_C > & _rstring ) {
				return TFather::_Handle->_PHandle->Insert( 
					*(_rstring._Handle->_PHandle) );
			}
			
			// Remove from _First, remove _Size characters.
			INLINE void Remove( Size_T _first, Size_T _size ) {
				TFather::_Handle->_PHandle->Remove( _first, _size );
			}
			// Remove the "_First" character.			
			INLINE void Remove( Size_T _first ) {
				TFather::_Handle->_PHandle->Remove( _first );
			}
			
			// Cast
			INLINE operator const CharType * ( ) const { 
				return (const CharType *)(*(*this));
			}
			// Convert to CharType * 
			INLINE const CharType * C_Str( ) const {
				return TFather::_Handle->_PHandle->C_Str();
			}
			// for old std::string.
			INLINE const CharType * c_str( ) const {
				return TFather::_Handle->_PHandle->C_Str();
			}
			// The current string's length.
			INLINE Size_T Size( ) const {
				return TFather::_Handle->_PHandle->Size();
			}
			// for old std::string
			INLINE Size_T size( ) const {
				return TFather::_Handle->_PHandle->Size();
			}
			// Check if current string is empty.
			INLINE bool Empty( ) const {
				return TFather::_Handle->_PHandle->Empty( );
			}
			// for old std::string
			INLINE bool empty( ) const {
				return TFather::_Handle->_PHandle->Empty( );
			}
			// Clear and reset the string buffer.
			INLINE void Clear( ) {
				TFather::_Handle->_PHandle->Clear();
			}
			// for old std::string.
			INLINE void clear( ) { 
				TFather::_Handle->_PHandle->Clear();
			}
			
			// Create a substring.
			INLINE _RString< _Basic_C > SubString( Size_T _OffSet, 
				Size_T _len = _StringBasic< _Basic_C >::NoPos ) const 
			{
				// We can create an empty substring.
				// if the invoker really want to do this
				assert( _OffSet <= TFather::_Handle->_PHandle->_Length );
				PLIB_OBJ_THREAD_SAFE( *(*this) );
				Size_T _CopyLength = ( _len == NoPos ) ?
					(TFather::_Handle->_PHandle->_Length - _OffSet) : _len;
				// Check if the _CopyLength is validate.
				assert( (_CopyLength + _OffSet) <= TFather::_Handle->_PHandle->_Length );
				// Create the new string.
				_RString< _Basic_C > _tRString( 
					TFather::_Handle->_PHandle->_Buffer + _OffSet, _CopyLength );
				return _tRString;
			}
			
			// Opeartors.
			INLINE CharType & operator [] ( Int32 _idx ) {
				return (*this)->operator[] ( (Uint32)_idx );
				//return *(TFather::_Handle->_PHandle)[(Uint32)_idx];
			}
			INLINE const CharType & operator [] ( Int32 _idx ) const {
				return *(TFather::_Handle->_PHandle)[(Uint32)_idx];
			}
            // Append data to the end of current string.
			INLINE _RString< _Basic_C > & operator += ( const CharType _c ) {
				TFather::_Handle->_PHandle->operator += ( _c );
				return *this;
			}
			INLINE _RString< _Basic_C > & operator += ( const CharType * _data ) {
				TFather::_Handle->_PHandle->operator += ( _data );
				return *this;
			}
			INLINE _RString< _Basic_C > & operator += ( 
				const _RString< _Basic_C > & _rstring ) {
				TFather::_Handle->_PHandle->operator += ( 
					*_rstring._Handle->_PHandle );
				return *this;
			}
			INLINE _RString< _Basic_C > & operator += ( 
				const _StringBasic< _Basic_C > & _string ) {
				TFather::_Handle->_PHandle->operator += ( _string) ;
				return *this;
			}
            // Create new string object,
            // Copy from current string.
			INLINE _RString< _Basic_C > operator + ( 
				const CharType * _data ) const {
				_RString< _Basic_C > _tmp( *(TFather::_Handle->_PHandle) );
				_tmp += _data;
				return _tmp;
			}
			INLINE _RString< _Basic_C > operator + ( 
				const _RString< _Basic_C > & _rstring ) const {
				_RString< _Basic_C > _tmp( *(TFather::_Handle->_PHandle) );
				_tmp += _rstring;
				return _tmp;
			}
			INLINE _RString< _Basic_C > operator + ( 
				const _StringBasic< _Basic_C > & _string ) const {
				_RString< _Basic_C > _tmp( *(TFather::_Handle->_PHandle) );
				_tmp += _string;
				return _tmp;
			}
            
            // Reset current string's data.
			INLINE _RString< _Basic_C > & operator = ( const CharType * _data ) {
				TFather::_Handle->_PHandle->operator = ( _data );
				return *this;
			}
			INLINE _RString< _Basic_C > & operator = ( 
				const _StringBasic< _Basic_C > & _string ) {
				TFather::_Handle->_PHandle->operator = ( _string );
				return *this;
			}
            // Compare operator.
			INLINE bool operator == ( const CharType * _data ) const {
				return TFather::_Handle->_PHandle->operator == ( _data );
			}
			INLINE bool operator == ( const _RString< _Basic_C > & _rstring ) const {
				return TFather::operator == ( _rstring );
			}
			INLINE bool operator != ( const CharType * _data ) const {
				return !(TFather::_Handle->_PHandle->operator == ( _data ));
			}
			INLINE bool operator != ( const _RString< _Basic_C > & _rstring ) const {
				return TFather::operator != ( _rstring );
			}
			INLINE bool operator < ( const _RString< _Basic_C > & _rstring ) const {
				return TFather::_Handle->_PHandle->operator < ( *_rstring._Handle->_PHandle );
			}
			INLINE bool operator < ( const _StringBasic< _Basic_C > & _string ) const { 
				return TFather::_Handle->_PHandle->operator < ( _string );
			}
			INLINE bool operator < ( const CharType * _data) const {
				return TFather::_Handle->_PHandle->operator < ( _data );
			}
			
			// Format.			
			INLINE Size_T Format( const CharType * _format, ... ) { 
				va_list pArgList;
				va_start(pArgList, _format);
				Size_T _length = _Basic_C::CalcVStringLen( _format, pArgList );
				va_end( pArgList );
				va_start(pArgList, _format);
				Size_T _L = TFather::_Handle->_PHandle->Format( _format, _length, pArgList );
				va_end( pArgList );
				return _L;
			}
			static _RString< _Basic_C > Parse( const CharType * _format, ... ) {
				_RString< _Basic_C > _String;
				va_list pArgList;
				va_start( pArgList, _format );
				Size_T _FLength = _Basic_C::CalcVStringLen( _format, pArgList );
				va_end( pArgList );
				va_start( pArgList, _format );
				_String._Handle->_PHandle->Format( _format, _FLength, pArgList );
				va_end(pArgList);
				return _String;
			}
			
            // Cast
			INLINE _RString< _Basic_C > & ToUpper( ) {
				TFather::_Handle->_PHandle->ToUpper( );
				return *this;
			}
			INLINE _RString< _Basic_C > & ToLower( ) { 
				TFather::_Handle->_PHandle->ToLower( );
				return *this;
			}
            // Remove the whitespace from both head and tail.
			INLINE _RString< _Basic_C > & Trim( ) { 
				TFather::_Handle->_PHandle->Trim( );
				return *this;
			}
			
            // Split the string into an array according to every character in
            // the parameter string.
			INLINE Plib::Generic::Array< _RString< _Basic_C > >
			 	Split ( _RString< _Basic_C > _Carry ) const {
				Plib::Generic::Array< _RString< _Basic_C > > _resultArray;
				if ( this->Size() == 0 ) return _resultArray;
				Uint32 _pos = 0;
				do {
					Uint32 _lastPos = _StringBasic< _Basic_C >::NoPos;
					for ( Uint32 i = 0; i < _Carry.size(); ++i ) {
						Uint32 _nextCarry = this->Find( _Carry[i], _pos );
						_lastPos = ( _nextCarry < _lastPos ) ? _nextCarry : _lastPos;
					}
					if ( _lastPos == _StringBasic< _Basic_C >::NoPos ) _lastPos = this->size();
					_RString< _Basic_C > _value = this->SubString( _pos, _lastPos - _pos );
					if ( _value.Trim().Size() > 0 )
						_resultArray.PushBack( _value );
					_pos = _lastPos + 1;
				} while ( _pos < this->size() );
				return _resultArray;
			}
			
            // String compare methods.
			INLINE bool StartWith( const CharType * _data ) const { 
				return TFather::_Handle->_PHandle->StartWith( _data );
			}
			INLINE bool StartWith( const _StringBasic< _Basic_C > & _string ) const {
				return TFather::_Handle->_PHandle->StartWith( _string );
			}
			INLINE bool StartWith( const _RString< _Basic_C > & _rstring ) const  {
				return TFather::_Handle->_PHandle->StartWith( *_rstring );
			}
			
			INLINE bool EndWith( const CharType * _data ) const {
				return TFather::_Handle->_PHandle->EndWith( _data );
			}
			INLINE bool EndWith( const _StringBasic< _Basic_C > & _string ) const {
				return TFather::_Handle->_PHandle->EndWith( _string );
			}
			INLINE bool EndWith( const _RString< _Basic_C > & _rstring ) const {
				return TFather::_Handle->_PHandle->EndWith( *_rstring );
			}
			
            // String Searching
			INLINE Size_T Find( CharType _c, Size_T _offset = 0 ) const {
				return TFather::_Handle->_PHandle->Find( _c, _offset );
			}
			INLINE Size_T Find( const CharType * _data, Size_T 
                               _length, Size_T _offSet ) const {
				return TFather::_Handle->_PHandle->Find( 
                                _data, _length, _offSet );
			}
			INLINE Size_T Find( const CharType * _data, 
                               Size_T _offSet = 0 ) const {
				return TFather::_Handle->_PHandle->Find( _data, _offSet );
			}
			INLINE Size_T Find( const _StringBasic< _Basic_C > & _string, 
                               Size_T _offSet = 0 ) const {
				return TFather::_Handle->_PHandle->Find( _string, _offSet );
			}
			INLINE Size_T Find( const _RString< _Basic_C > & _rstring, 
                               Size_T _offSet = 0 ) const {
				return TFather::_Handle->_PHandle->Find( 
                                *_rstring._Handle->_PHandle, _offSet );
			}
			INLINE Size_T FindLast( CharType _c, Size_T _offset = 
                                    _StringBasic< _Basic_C >::NoPos ) const {
				return TFather::_Handle->_PHandle->FindLast( _c, _offset );
			}		
			
			// Return the int value.
			INLINE Int32 IntValue() const {
				return TFather::_Handle->_PHandle->IntValue();
			}
			INLINE Uint32 UintValue() const {
				return TFather::_Handle->_PHandle->UintValue();
			}
			INLINE Int64 Int64Value() const {
				return TFather::_Handle->_PHandle->Int64Value();
			}
			INLINE Uint64 Uint64Value() const {
				return TFather::_Handle->_PHandle->Uint64Value();
			}
			// Return Float Value
			INLINE double DoubleValue() const {
				return TFather::_Handle->_PHandle->DoubleValue();
			}
			INLINE float FloatValue() const {
				return TFather::_Handle->_PHandle->FloatValue();
			}
			// Bool Value
			INLINE bool BoolValue() const {
				return TFather::_Handle->_PHandle->BoolValue();
			}
			
			const static _RString< _Basic_C > Null;
			
			static _RString< _Basic_C > CreateNullString( )
			{
				return _RString< _Basic_C >( false );
			}
		};
		
		template< typename _Basic_C >
		std::ostream & operator << ( std::ostream & os, const _RString< _Basic_C > & SB )
		{
			os << SB.C_Str();
			return os;
		}

		template< typename _Basic_C >
		_RString< _Basic_C > operator + ( const typename _Basic_C::CharType * _Data, 
				const _RString< _Basic_C > & SB )
		{
			_RString< _Basic_C > _tmp( _Data );
			_tmp += SB;
			return _tmp;	
		}
		
		template< typename _Basic_C >
		const _RString< _Basic_C > _RString< _Basic_C >::Null( false );
					
		// ASCII String Function Struct.
		struct _Basic_Char
		{
			typedef char CharType;
			const static char EOL = '\0';
			static INLINE Uint32 StringLength( const CharType * _Data ) {
				return (Uint32)::strlen( _Data );
			}
			static INLINE Uint32 CalcVStringLen( const CharType * _Format, va_list _Args )
			{
				Uint32 _Len = 0;
#if _DEF_WIN32
				_Len = (Uint32)_vscprintf( _Format, _Args );
#else
				_Len = (Uint32)vsnprintf(NULL, 0, _Format, _Args);
#endif
				return _Len;
			}

			static INLINE Uint32 StringPrintf( CharType * _Buffer, Uint32 _Size,
				const CharType * _Format, va_list _Args )
			{
				Uint32 _Len = 0;
#if _DEF_WIN32
				_Len = vsprintf_s( _Buffer, _Size, _Format, _Args );
#else
				_Len = vsnprintf( _Buffer, _Size, _Format, _Args );
				//_Len = vsprintf( _Buffer, _Format, _Args );
#endif
				return _Len;
			}

			static bool StrnCmp( const CharType * _Src1, const CharType * _Src2, Uint32 _Size )
			{
				return strncmp(_Src1, _Src2, _Size) == (int)0;
			}
			
			// Return the int value.
			static Int32 AtoI( const CharType * _Buffer ) {
				return atoi(_Buffer);
			}
			static Int64 AtoL( const CharType * _Buffer ) {
	#if _DEF_WIN32
				return _atoi64(_Buffer);
	#else
				return atoll(_Buffer);
	#endif
			}
			// Return Float Value
			static double AtoF( const CharType * _Buffer ) {
				double _d = 0.0;
				sscanf( _Buffer, "%lf", &_d );
				return _d;
			}
			// Bool Value
			static bool BoolValue( const CharType * _Buffer, Uint32 _Length ) {
				if ( _Length == 5 ) {
					CharType _tempFalse[6] = { 0 };
					std::locale loc;
					for ( Uint32 i = 0; i < 5; ++i ) {
						_tempFalse[i] = std::tolower(_Buffer[i], loc);
					}
					return strncmp( _tempFalse, "false", 5 ) != (int)0;
				}
				return true;
			}
			
			static Uint32 Find( const CharType * _Buffer, Uint32 _Length, 
								const CharType * _Data, Uint32 _DLength )
			{
				if ( _Length < _DLength ) return (Uint32)-1;
				if ( _Length == _DLength ) 
					return ( strncmp( _Buffer, _Data, _Length ) == (int)0 ) ? 0 : (Uint32)-1;
					
				// Build BitMap;
				Uint16 _InData[256] = { (Uint16)-1 };
				for ( Uint16 i = 0; i < (Uint16)_DLength; ++i ) {
					_InData[(Uint8)_Data[i]] = i;
				}
				
				for ( Uint32 i = 0; ; /*Nothing to do, all in for*/) {
					if ( _Buffer[i] == _Data[0] ) {
						Uint32 k = 0;
						for ( ; k < _DLength; ++k ) {
							if ( _Buffer[i + k] != _Data[k] ) break;
						}
						if ( k == _DLength ) return (Uint32)i;
					}
					i += _DLength;
					while ( (Uint16)-1 == _InData[ (Uint8)_Buffer[i] ] && i < _Length ) ++i;
					if ( i >= _Length ) return (Uint32)-1;
					i -= _InData[ (Uint8)_Buffer[i] ];
				}
				return (Uint32)-1;
			}		
		};
		
		// Unicode String Function Struct.
		struct _Basic_WChar
		{
			typedef wchar_t CharType;
			const static wchar_t EOL = L'\0';
			static INLINE Uint32 StringLength( const CharType * _Data ) {
				return (Uint32)::wcslen( _Data );
			}
			static INLINE Uint32 CalcVStringLen( const CharType * _Format, va_list _Args )
			{
				Uint32 _Len = 0;
#if _DEF_WIN32
				_Len = (Uint32)_vscwprintf( _Format, _Args );
#else
				_Len = (Uint32)vswprintf(NULL, 0, _Format, _Args);
#endif
				return _Len;
			}

			static INLINE Uint32 StringPrintf( CharType * _Buffer, Uint32 _Size,
				const CharType * _Format, va_list _Args )
			{
				Uint32 _Len = 0;
#if _DEF_WIN32
				_Len = vswprintf_s( _Buffer, _Size, _Format, _Args );
#else
				_Len = vswprintf( _Buffer, _Size, _Format, _Args );
#endif
				return _Len;
			}
			static bool StrnCmp( const CharType * _Src1, const CharType * _Src2, Uint32 _Size )
			{
				return wcsncmp(_Src1, _Src2, _Size) == (int)0;
			}
			// Return the int value.
			static Int32 AtoI( const CharType * _Buffer ) {
#if _DEF_WIN32
				return _wtoi( _Buffer );
#else
				Int32 _v = 0;
				swscanf(_Buffer, L"%d", &_v);
				return _v;
#endif
			}
			static Int64 AtoL( const CharType * _Buffer ) {
				Int64 _v = 0;
				swscanf(_Buffer, L"%lld", &_v);
				return _v;
			}
			// Return Float Value
			static double AtoF( const CharType * _Buffer ) {
				double _f = 0;
				swscanf(_Buffer, L"%lf", &_f);
				return _f;
			}
			// Bool Value
			static bool BoolValue( const CharType * _Buffer, Uint32 _Length ) {
				if ( _Length == 5 ) {
					CharType _tempFalse[6] = { 0 };
					std::locale loc;
					for ( Uint32 i = 0; i < 5; ++i ) {
						_tempFalse[i] = std::tolower(_Buffer[i], loc);
					}
					return wcsncmp( _tempFalse, L"false", 5 ) != (int)0;
				}
				return true;
			}
			static Uint32 Find( const CharType * _WBuffer, Uint32 _WLength, 
								const CharType * _WData, Uint32 _WDLength )
			{
				if ( _WLength < _WDLength ) return (Uint32)-1;
				if ( _WLength == _WDLength ) 
					return ( wcsncmp( _WBuffer, _WData, _WLength ) == (int)0 ) ? 0 : (Uint32)-1;
				
				const char * _Buffer = (const char *)_WBuffer;
				Uint32 _Length = sizeof(CharType) * _WLength;
				const char * _Data = (const char *)_WData;
				Uint32 _DLength = sizeof(CharType) * _WDLength;
				
				// Build BitMap;
				Uint16 _InData[256] = { (Uint16)-1 };
				for ( Uint16 i = 0; i < (Uint16)_DLength; ++i ) {
					_InData[(Uint8)_Data[i]] = i;
				}
				
				for ( Uint32 i = 0; ; /*Nothing to do, all in for*/) {
					if ( _Buffer[i] == _Data[0] ) {
						for ( Uint32 k = 0; k < _DLength; ++k ) {
							if ( _Buffer[i + k] != _Data[k] ) break;
						}
						return (Uint32)i;
					}
					i += _DLength;
					while ( (Uint16)-1 != _InData[ (Uint8)_Buffer[i] ] && i < _Length ) ++i;
					if ( i == _Length ) return (Uint32)-1;
					i -= _InData[ (Uint8)_Buffer[i] ];
				}
				return (Uint32)-1;
			}		
		};	
				
		typedef _RString< _Basic_Char >		String;
		typedef _RString< _Basic_WChar >	WString;

	}
}

#endif // plib.basic.string.hpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */

