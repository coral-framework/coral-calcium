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

inline bool changesStdCompare( const ca::IObjectChangesRef& a,
							   const ca::IObjectChangesRef& b )
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

co::TSlice<co::IObject*> GraphChanges::getAddedObjects()
{
	return _addedObjects;
}

co::TSlice<co::IObject*> GraphChanges::getRemovedObjects()
{
	return _removedObjects;
}

co::TSlice<ca::IObjectChanges*> GraphChanges::getChangedObjects()
{
	return _changedObjects;
}

co::int32 GraphChanges::findAddedObject( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Slice<co::IObject*>( _addedObjects ),
		object, objectCompare, pos ) ? static_cast<co::int32>( pos ) : -1;
}

co::int32 GraphChanges::findRemovedObject( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Slice<co::IObject*>( _removedObjects ),
		object, objectCompare, pos ) ? static_cast<co::int32>( pos ) : -1;
}

co::int32 GraphChanges::findChangedObject( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Slice<ca::IObjectChanges*>( _changedObjects ),
		object, changesKeyCompare, pos ) ? static_cast<co::int32>( pos ) : -1;
}

void revertFieldChanges( IGraph* graph, IServiceChanges* changes )
{
	co::IService* service = changes->getService();
	graph->addChange( service );

	co::Any instance( service );

	// revert all Ref fields
	co::TSlice<ChangedRefField> refFields = changes->getChangedRefFields();
	for( ; refFields; refFields.popFirst() )
	{
		const ChangedRefField& cur = refFields.getFirst();
		cur.field->getOwner()->getReflector()->setField( instance, cur.field.get(), cur.previous.get() );
	}

	// revert all RefVec fields
	co::TSlice<ChangedRefVecField> refVecFields = changes->getChangedRefVecFields();
	for( ; refVecFields; refVecFields.popFirst() )
	{
		// force a downcast of the IService[] to its real element type
		const ChangedRefVecField& cur = refVecFields.getFirst();
		co::IField* field = cur.field.get();
		co::Any value( true, field->getType(), &cur.previous[0], cur.previous.size() );
		cur.field->getOwner()->getReflector()->setField( instance, field, value );
	}

	// revert all Value fields
	co::TSlice<ChangedValueField> valueFields = changes->getChangedValueFields();
	for( ; valueFields; valueFields.popFirst() )
	{
		const ChangedValueField& cur = valueFields.getFirst();
		cur.field->getOwner()->getReflector()->setField( instance, cur.field.get(), cur.previous );
	}
}

void GraphChanges::revertChanges()
{
	// for each changed object
	for( co::Slice<IObjectChanges*> objects( _changedObjects ); objects; objects.popFirst() )
	{
		// revert connection changes
		co::TSlice<ChangedConnection> connections = objects.getFirst()->getChangedConnections();
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
		co::TSlice<IServiceChanges*> services = objects.getFirst()->getChangedServices();
		for( ; services; services.popFirst() )
			revertFieldChanges( _graph.get(), services.getFirst() );
	}
}

CORAL_EXPORT_COMPONENT( GraphChanges, GraphChanges );

} // namespace ca
