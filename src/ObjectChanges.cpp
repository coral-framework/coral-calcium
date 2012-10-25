/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ObjectChanges.h"

namespace ca {

ObjectChanges::ObjectChanges()
{
	// empty
}

ObjectChanges::~ObjectChanges()
{
	// empty
}

// ------ ca.IObjectChanges Methods ------ //

co::IObject* ObjectChanges::getObject()
{
	return _object.get();
}

co::TSlice<IServiceChanges*> ObjectChanges::getChangedServices()
{
	return _changedServices;
}

co::TSlice<ChangedConnection> ObjectChanges::getChangedConnections()
{
	return _changedConnections;
}

CORAL_EXPORT_COMPONENT( ObjectChanges, ObjectChanges );

} // namespace ca
