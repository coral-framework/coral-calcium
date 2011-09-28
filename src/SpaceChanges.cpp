/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SpaceChanges.h"
#include <algorithm>

namespace ca {

SpaceChanges::SpaceChanges()
{
	// empty
}

SpaceChanges::SpaceChanges( ca::ISpace* space, SpaceChanges& o ) : _space( space )
{
	std::swap( _addedObjects, o._addedObjects );
	std::swap( _removedObjects, o._removedObjects );
	std::swap( _changedObjects, o._changedObjects );
}

SpaceChanges::~SpaceChanges()
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

ISpaceChanges* SpaceChanges::finalize( ca::ISpace* space )
{
	assert( !_space.isValid() );

	// sort all arrays
	std::sort( _addedObjects.begin(), _addedObjects.end() );
	std::sort( _removedObjects.begin(), _removedObjects.end() );
	std::sort( _changedObjects.begin(), _changedObjects.end(), changesStdCompare );

	// return a clone with which we swap our state
	return new SpaceChanges( space, *this );
}

ca::ISpace* SpaceChanges::getSpace()
{
	return _space.get();
}

co::Range<co::IObject* const> SpaceChanges::getAddedObjects()
{
	return _addedObjects;
}

co::Range<co::IObject* const> SpaceChanges::getRemovedObjects()
{
	return _removedObjects;
}

co::Range<ca::IObjectChanges* const> SpaceChanges::getChangedObjects()
{
	return _changedObjects;
}

co::int32 SpaceChanges::findAddedObject( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Range<co::IObject* const>( _addedObjects ),
		object, objectCompare, pos ) ? static_cast<co::int32>( pos ) : -1;
}

co::int32 SpaceChanges::findRemovedObject( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Range<co::IObject* const>( _removedObjects ),
		object, objectCompare, pos ) ? static_cast<co::int32>( pos ) : -1;
}

co::int32 SpaceChanges::findChangedObject( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Range<ca::IObjectChanges* const>( _changedObjects ),
		object, changesKeyCompare, pos ) ? static_cast<co::int32>( pos ) : -1;
}

void revertFieldChanges( ISpace* space, IServiceChanges* changes )
{
	co::IService* service = changes->getService();
	space->addChange( service );

	co::Any instance( service ), value;

	// revert all Ref fields
	co::Range<ChangedRefField const> refFields = changes->getChangedRefFields();
	for( ; refFields; refFields.popLast() )
	{
		co::IField* field = refFields.getLast().field.get();
		co::IInterface* type = static_cast<co::IInterface*>( field->getType() );
		value.setService( refFields.getLast().previous.get(), type );
		field->getOwner()->getReflector()->setField( instance, field, value );
	}

	// revert all RefVec fields
	co::Range<ChangedRefVecField const> refVecFields = changes->getChangedRefVecFields();
	for( ; refVecFields; refVecFields.popLast() )
	{
		co::IField* field = refVecFields.getLast().field.get();
		co::IType* elemType = static_cast<co::IArray*>( field->getType() )->getElementType();
		const co::uint32 flags = co::Any::VarIsPointer | co::Any::VarIsPointerConst;
		value.setArray( co::Any::AK_RefVector, elemType, flags, const_cast<void*>(
			reinterpret_cast<const void*>( &refVecFields.getLast().previous ) ) );
		field->getOwner()->getReflector()->setField( instance, field, value );
	}

	// revert all Value fields
	co::Range<ChangedValueField const> valueFields = changes->getChangedValueFields();
	for( ; valueFields; valueFields.popLast() )
	{
		co::IField* field = valueFields.getLast().field.get();
		field->getOwner()->getReflector()->setField( instance, field, valueFields.getLast().previous );
	}
}

void SpaceChanges::revertChanges()
{
	// for each changed object
	for( co::Range<IObjectChanges* const> objects( _changedObjects ); objects; objects.popLast() )
	{
		// revert connection changes
		co::Range<ChangedConnection const> connections = objects.getLast()->getChangedConnections();
		if( connections )
		{
			co::IObject* object = objects.getLast()->getObject();
			for( ; connections; connections.popLast() )
			{
				object->setService( connections.getLast().receptacle.get(),
								    connections.getLast().previous.get() );
			}
			_space->addChange( object );
		}

		// revert field changes for each changed service
		co::Range<IServiceChanges* const> services = objects.getLast()->getChangedServices();
		for( ; services; services.popLast() )
			revertFieldChanges( _space.get(), services.getLast() );
	}
}

CORAL_EXPORT_COMPONENT( SpaceChanges, SpaceChanges );

} // namespace ca
