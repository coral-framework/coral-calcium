/*
 * Calcium Sample
 * Entity Relationship Model (ERM)
 */

#include "Model_Base.h"
#include <erm/IEntity.h>
#include <erm/IRelationship.h>
#include <co/RefVector.h>

namespace erm {

class Model : public Model_Base
{
public:
	Model()
	{
		// empty
	}

	virtual ~Model()
	{
		// empty
	}

	co::ArrayRange<IEntity* const> getEntities()
	{
		return _entities;
	}

	void setEntities( co::ArrayRange<IEntity* const> entities )
	{
		entities.assignTo( _entities );
	}

	co::ArrayRange<IRelationship* const> getRelationships()
	{
		return _relationships;
	}

	void setRelationships( co::ArrayRange<IRelationship* const> relationships )
	{
		relationships.assignTo( _relationships );
	}

private:
	co::RefVector<IEntity> _entities;
	co::RefVector<IRelationship> _relationships;
};

} // namespace erm

CORAL_EXPORT_COMPONENT( erm::Model, Model )
