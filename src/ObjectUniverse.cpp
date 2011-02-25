/*
 * Calcium Object Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ObjectUniverse_Base.h"

namespace ca {

class ObjectUniverse : public ObjectUniverse_Base
{
public:
	ObjectUniverse()
	{
	}

	virtual ~ObjectUniverse()
	{
	}
	
	void dummy()
	{}

protected:
	ca::IObjectModel* getReceptacleModel()
	{
		return NULL;
	}

	void setReceptacleModel( ca::IObjectModel* model )
	{
	}

private:
};

} // namespace ca

CORAL_EXPORT_COMPONENT( ca::ObjectUniverse, ObjectUniverse )
