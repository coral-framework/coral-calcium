/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _ANY_ARRAY_UTIL_
#define _ANY_ARRAY_UTIL_


#include <co/IReflector.h>

/*--------------------------------------------------------------------

Any with values of kind TK_ARRAY help functions. Must be removed after
Any arrays refactoring

--------------------------------------------------------------------*/

class AnyArrayUtil
{

public:
	template< typename T>
	void setArrayElement( const co::Any& array, co::uint32 index, T element )
	{
		co::IType* elementType = array.getType();
		co::uint32 elementSize = elementType->getReflector()->getSize();

		void* ptr = getArrayPtr( array ) + elementSize * index;
			
		T* elementTypePointer = reinterpret_cast<T*>(ptr);
		*elementTypePointer = element;
	}

	void setArrayComplexTypeElement( const co::Any& array, co::uint32 index, co::Any& element )
	{
		assert( element.getKind() == co::TK_STRUCT ||
				element.getKind() == co::TK_NATIVECLASS ||
				element.getKind() == co::TK_INTERFACE );

		co::IType* elementType = array.getType();
		co::IReflector* reflector = elementType->getReflector();
		co::uint32 elementSize = reflector->getSize();

		void* ptr = getArrayPtr( array ) + elementSize * index;
		if( elementType->getKind() != co::TK_INTERFACE )
		{
			reflector->copyValues( element.getState().data.ptr, ptr, 1 );
		}
		else
		{
			co::RefPtr<co::IService>& refPtr = *reinterpret_cast<co::RefPtr<co::IService>*>( ptr );
			refPtr = element.getState().data.service;
		}
	}

	void getArrayElement( const co::Any& array, co::uint32 index, co::Any& element )
	{
		co::IType* elementType = array.getType();
		co::uint32 elementSize = elementType->getReflector()->getSize();

		co::TypeKind ek = elementType->getKind();
		bool isPrimitive = ( ( ek >= co::TK_BOOLEAN && ek <= co::TK_DOUBLE ) || ek == co::TK_ENUM );
		co::uint32 flags = isPrimitive ? co::Any::VarIsValue : co::Any::VarIsConst|co::Any::VarIsReference;

		void* ptr = getArrayPtr( array ) + elementSize * index;

		element.setVariable( elementType, flags, ptr );
	}

	co::uint32 getArraySize( const co::Any& array )
	{
		assert( array.getKind() == co::TK_ARRAY );

		const co::Any::State& s = array.getState();
		if( s.arrayKind == co::Any::State::AK_Range )
			return s.arraySize;

		co::Any::PseudoVector& pv = *reinterpret_cast<co::Any::PseudoVector*>( s.data.ptr );
		return static_cast<co::uint32>( pv.size() ) / array.getType()->getReflector()->getSize();
	}

private:

	co::uint8* getArrayPtr( const co::Any& array )
	{
		assert( array.getKind() == co::TK_ARRAY );

		const co::Any::State& s = array.getState();
		if( s.arrayKind == co::Any::State::AK_Range )
			return reinterpret_cast<co::uint8*>( s.data.ptr );

		co::Any::PseudoVector& pv = *reinterpret_cast<co::Any::PseudoVector*>( s.data.ptr );
		return &pv[0];
	}
};
#endif