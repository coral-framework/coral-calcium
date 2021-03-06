/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_OBJECTCHANGES_H_
#define _CA_OBJECTCHANGES_H_

#include "ServiceChanges.h"
#include "ObjectChanges_Base.h"
#include <co/IObject.h>
#include <ca/IServiceChanges.h>
#include <ca/ChangedConnection.h>

namespace ca {

class ObjectChanges : public ObjectChanges_Base
{
public:
	ObjectChanges();

	inline ObjectChanges( co::IObject* object ) : _object( object )
	{;}

	virtual ~ObjectChanges();

	// ------ Internal Methods ------ //

	inline co::IObject* getObjectInl() const { return _object.get(); }

	inline void addChangedService( ServiceChanges* sc )
	{
		_changedServices.push_back( sc );
	}

	inline ChangedConnection& addChangedConnection()
	{
		_changedConnections.push_back( ChangedConnection() );
		return _changedConnections.back();
	}

	// ------ ca.IObjectChanges Methods ------ //
	
	co::IObject* getObject();
	co::TSlice<IServiceChanges*> getChangedServices();
	co::TSlice<ChangedConnection> getChangedConnections();

private:
	co::IObjectRef _object;
	std::vector<IServiceChangesRef> _changedServices;
	std::vector<ChangedConnection> _changedConnections;
};

} // namespace ca

#endif
