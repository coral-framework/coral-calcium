/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "GraphChanges.h"
#include <algorithm>

namespace ca {

GraphChanges::GraphChanges()
{
	// empty
}

GraphChanges::GraphChanges( ca::IGraph* graph, GraphChanges& o ) : _graph( graph )
{
	std::swap( _addedObjects, o._addedObjects );
	std::swap( _removedObjects, o._removedObjects );
	std::swap( _changedObjects, o._changedObjects );
}

GraphChanges::~GraphChanges()
{
	// empty
}

inline int objectCompare( const co::IObject* a, const co::IObject* b )
{
	return ( a < b ? -1 : ( a == b ? 0 : 1 ) );
}

inline bool changesStdCompare( const co::RefPtr<ca::IObjectChanges>& a,
							   const co::RefPtr<ca::IObjectChanges>& b )
{
	return static_cast<ObjectChanges*>( a.get() )->getObjectInl()
			< static_cast<ObjectChanges*>( b.get() )->getObjectInl();
}

inline int changesKeyCompare( const co::IObject* key, ca::IObjectChanges* changes )
{
	co::IObject* object = static_cast<ObjectChanges*>( changes )->getObjectInl();
	return ( key == object ? 0 : ( key < object ? -1 : 1 ) );
}

IGraphChanges* GraphChanges::finalize( ca::IGraph* graph )
{
	assert( !_graph.isValid() );

	// sort all arrays
	std::sort( _addedObjects.begin(), _addedObjects.end() );
	std::sort( _removedObjects.begin(), _removedObjects.end() );
	std::sort( _changedObjects.begin(), _changedObjects.end(), changesStdCompare );

	// return a clone with which we swap our state
	return new GraphChanges( graph, *this );
}

ca::IGraph* GraphChanges::getGraph()
{
	return _graph.get();
}

co::Range<co::IObject* const> GraphChanges::getAddedObjects()
{
	return _addedObjects;
}

co::Range<co::IObject* const> GraphChanges::getRemovedObjects()
{
	return _removedObjects;
}

co::Range<ca::IObjectChanges* const> GraphChanges::getChangedObjects()
{
	return _changedObjects;
}

co::int32 GraphChanges::findAddedObject( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Range<co::IObject* const>( _addedObjects ),
		object, objectCompare, pos ) ? static_cast<co::int32>( pos ) : -1;
}

co::int32 GraphChanges::findRemovedObject( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Range<co::IObject* const>( _removedObjects ),
		object, objectCompare, pos ) ? static_cast<co::int32>( pos ) : -1;
}

co::int32 GraphChanges::findChangedObject( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Range<ca::IObjectChanges* const>( _changedObjects ),
		object, changesKeyCompare, pos ) ? static_cast<co::int32>( pos ) : -1;
}

void revertFieldChanges( IGraph* graph, IServiceChanges* changes )
{
	co::IService* service = changes->getService();
	graph->addChange( service );

	co::Any instance( service ), value;

	// revert all Ref fields
	co::Range<ChangedRefField const> refFields = changes->getChangedRefFields();
	for( ; refFields; refFields.popFirst() )
	{
		co::IField* field = refFields.getFirst().field.get();
		co::IInterface* type = static_cast<co::IInterface*>( field->getType() );
		value.setService( refFields.getFirst().previous.get(), type );
		field->getOwner()->getReflector()->setField( instance, field, value );
	}

	// revert all RefVec fields
	co::Range<ChangedRefVecField const> refVecFields = changes->getChangedRefVecFields();
	for( ; refVecFields; refVecFields.popFirst() )
	{
		co::IField* field = refVecFields.getFirst().field.get();
		co::IType* elemType = static_cast<co::IArray*>( field->getType() )->getElementType();
		const co::uint32 flags = co::Any::VarIsPointer | co::Any::VarIsPointerConst;
		value.setArray( co::Any::AK_RefVector, elemType, flags, const_cast<void*>(
			reinterpret_cast<const void*>( &refVecFields.getFirst().previous ) ) );
		field->getOwner()->getReflector()->setField( instance, field, value );
	}

	// revert all Value fields
	co::Range<ChangedValueField const> valueFields = changes->getChangedValueFields();
	for( ; valueFields; valueFields.popFirst() )
	{
		co::IField* field = valueFields.getFirst().field.get();
		field->getOwner()->getReflector()->setField( instance, field, valueFields.getFirst().previous );
	}
}

void GraphChanges::revertChanges()
{
	// for each changed object
	for( co::Range<IObjectChanges* const> objects( _changedObjects ); objects; objects.popFirst() )
	{
		// revert connection changes
		co::Range<ChangedConnection const> connections = objects.getFirst()->getChangedConnections();
		if( connections )
		{
			co::IObject* object = objects.getFirst()->getObject();
			for( ; connections; connections.popFirst() )
			{
				object->setServiceAt( connections.getFirst().receptacle.get(),
								    connections.getFirst().previous.get() );
			}
			_graph->addChange( object );
		}

		// revert field changes for each changed service
		co::Range<IServiceChanges* const> services = objects.getFirst()->getChangedServices();
		for( ; services; services.popFirst() )
			revertFieldChanges( _graph.get(), services.getFirst() );
	}
}

CORAL_EXPORT_COMPONENT( GraphChanges, GraphChanges );

} // namespace ca
