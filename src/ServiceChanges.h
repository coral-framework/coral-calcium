/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_SERVICECHANGES_H_
#define _CA_SERVICECHANGES_H_

#include "ServiceChanges_Base.h"
#include <ca/ChangedRefField.h>
#include <ca/ChangedRefVecField.h>
#include <ca/ChangedValueField.h>

namespace ca {

class ServiceChanges : public ServiceChanges_Base
{
public:
	ServiceChanges();

	inline ServiceChanges( co::IService* service ) : _service( service )
	{;}

	virtual ~ServiceChanges();

	// ------ Internal Methods ------ //

	inline co::IService* getServiceInl() { return _service.get(); }

	inline ChangedRefField& addChangedRefField()
	{
		_changedRefFields.push_back( ChangedRefField() );
		return _changedRefFields.back();
	}

	inline ChangedRefVecField& addChangedRefVecField()
	{
		_changedRefVecFields.push_back( ChangedRefVecField() );
		return _changedRefVecFields.back();
	}

	inline ChangedValueField& addChangedValueField()
	{
		_changedValueFields.push_back( ChangedValueField() );
		return _changedValueFields.back();
	}

	// ------ ca.IServiceChanges Methods ------ //

	co::IService* getService();
	co::Range<ChangedRefField> getChangedRefFields();
	co::Range<ChangedRefVecField> getChangedRefVecFields();
	co::Range<ChangedValueField> getChangedValueFields();

private:
	co::RefPtr<co::IService> _service;
	std::vector<ChangedRefField> _changedRefFields;
	std::vector<ChangedRefVecField> _changedRefVecFields;
	std::vector<ChangedValueField> _changedValueFields;
};

} // namespace ca

#endif
