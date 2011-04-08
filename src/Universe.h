/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_UNIVERSE_H_
#define _CA_UNIVERSE_H_

#include "Model.h"

namespace ca {

template<typename Callback>
void traverseReceptacles( ObjectRecord* object, Callback& cb )
{
	ComponentRecord* model = object->model;
	for( co::uint8 i = 0; i < model->numReceptacles; ++i )
		cb( object, model->ports[i], object->getRef( i ) );
}

template<typename Callback>
void traverseFacetRefs( ObjectRecord* object, PortRecord& facet, Callback& cb )
{
	RefField* ref = object->getRef( facet.firstRef );
	for( co::uint16 i = 0; i < facet.typeRec->numRefs; ++i, ++ref )
		cb( object, facet.typeRec->fields[i], ref );
}

template<typename Callback>
void traverseFacetRefVecs( ObjectRecord* object, PortRecord& facet, Callback& cb )
{
	RefVecField* refVec = object->getRefVec( facet.firstRefVec );
	for( co::uint16 i = facet.typeRec->numRefs; i < facet.typeRec->firstValue; ++i, ++refVec )
		cb( object, facet.typeRec->fields[i], refVec );
}

template<typename Callback>
void traverseFacetValues( ObjectRecord* object, PortRecord& facet, Callback& cb )
{
	ValueField* value = object->getValue( facet.firstValue );
	for( co::uint16 i = facet.typeRec->firstValue; i < facet.typeRec->numFields; ++i, ++value )
		cb( object, facet.typeRec->fields[i], value );
}

// Traverses all fields (both references and values) within an object's facet.
template<typename Callback>
void traverseFacet( ObjectRecord* object, co::uint8 portId, Callback& cb )
{
	PortRecord& facet = object->model->ports[portId];

	if( facet.typeRec->numRefs > 0 )
		traverseFacetRefs( object, facet, cb );

	if( facet.typeRec->numRefVecs > 0 )
		traverseFacetRefVecs( object, facet, cb );

	if( facet.typeRec->numValues > 0 )
		traverseFacetValues( object, facet, cb );
}

// Traverses all object receptacles and fields (both references and values).
template<typename Callback>
void traverseObject( ObjectRecord* object, Callback& cb )
{
	traverseReceptacles( object, cb );

	ComponentRecord* model = object->model;
	for( co::uint8 i = model->numReceptacles; i < model->numPorts; ++i )
		traverseFacet( object, i, cb );
}

// Traverses all object receptacles and fields (both references and values).
template<typename Callback>
void traverseObjectRefs( ObjectRecord* object, Callback& cb )
{
	traverseReceptacles( object, cb );

	ComponentRecord* model = object->model;
	for( co::uint8 i = model->numReceptacles; i < model->numPorts; ++i )
	{
		PortRecord& facet = object->model->ports[i];
		if( facet.typeRec->numRefs > 0 )
			traverseFacetRefs( object, facet, cb );

		if( facet.typeRec->numRefVecs > 0 )
			traverseFacetRefVecs( object, facet, cb );
	}
}

} // namespace ca

#endif // _CA_UNIVERSE_H_
