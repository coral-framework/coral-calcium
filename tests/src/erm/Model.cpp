/*
 * Calcium Sample
 * Entity Relationship Model (ERM)
 */

#include "Model_Base.h"
#include <erm/IEntity.h>
#include <erm/IRelationship.h>
#include <co/Exception.h>
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

	co::Range<IEntity* const> getEntities()
	{
		return _entities;
	}

	void setEntities( co::Range<IEntity* const> entities )
	{
		co::assign( entities, _entities );
	}

	void addEntity( IEntity* entity )
	{
		_entities.push_back( entity );
	}

	co::Range<IRelationship* const> getRelationships()
	{
		return _relationships;
	}

	void setRelationships( co::Range<IRelationship* const> relationships )
	{
		co::assign( relationships, _relationships );
	}
	
	void addRelationship( IRelationship* rel )
	{
		_relationships.push_back( rel );
	}

	co::Range<IModel* const> getDependencies()
	{
		return _dependencies;
	}

	void setDependencies( co::Range<IModel* const> dependencies )
	{
		co::assign( dependencies, _dependencies );
	}

	bool getThrowsOnGet()
	{
		throw co::Exception( "getThrowsOnGet exception" );
	}
	
	void setThrowsOnGet( bool )
	{
		// empty
	}

	bool getThrowsOnGetAndSet()
	{
		throw co::Exception( "getThrowsOnGetAndSet exception" );
	}

	void setThrowsOnGetAndSet( bool )
	{
		throw co::Exception( "setThrowsOnGetAndSet exception" );
	}

	bool getThrowsOnSet()
	{
		return false;
	}
	
	void setThrowsOnSet( bool )
	{
		throw co::Exception( "setThrowsOnSet exception" );
	}

private:
	co::RefVector<IEntity> _entities;
	co::RefVector<IRelationship> _relationships;
	co::RefVector<IModel> _dependencies;
};
	
CORAL_EXPORT_COMPONENT( Model, Model )

} // namespace erm
