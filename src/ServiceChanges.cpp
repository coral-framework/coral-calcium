/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ServiceChanges.h"
#include <algorithm>

namespace ca {

ServiceChanges::ServiceChanges()
{
	// empty
}

ServiceChanges::~ServiceChanges()
{
	// empty
}

// ------ ca.IServiceChanges Methods ------ //

co::IService* ServiceChanges::getService()
{
	return _service.get();
}

co::Range<ChangedRefField> ServiceChanges::getChangedRefFields()
{
	return _changedRefFields;
}

co::Range<ChangedRefVecField> ServiceChanges::getChangedRefVecFields()
{
	return _changedRefVecFields;
}

co::Range<ChangedValueField> ServiceChanges::getChangedValueFields()
{
	return _changedValueFields;
}

CORAL_EXPORT_COMPONENT( ServiceChanges, ServiceChanges );

} // namespace ca
