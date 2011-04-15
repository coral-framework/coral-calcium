/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ServiceChanges_Base.h"
#include <ca/IFieldChange.h>

namespace ca {

class ServiceChanges : public ServiceChanges_Base
{
public:
	ServiceChanges()
	{
		// empty constructor
	}

	virtual ~ServiceChanges()
	{
		// empty destructor
	}

	// ------ ca.IServiceChanges Methods ------ //

	co::Range<ca::IFieldChange* const> getChangedFields()
	{
		// TODO: implement this method.
		return co::Range<ca::IFieldChange* const>();
	}

	co::IService* getService()
	{
		// TODO: implement this method.
		return NULL;
	}

private:
	// member variables go here
};

CORAL_EXPORT_COMPONENT( ServiceChanges, ServiceChanges );

} // namespace ca
