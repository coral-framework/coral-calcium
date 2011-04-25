/*
 * Calcium Sample
 * Entity Relationship Model (ERM)
 */

#include "Entity_Base.h"

namespace erm {

class Entity : public Entity_Base
{
public:
	Entity() : _parent( NULL )
	{
		// empty
	}

	virtual ~Entity()
	{
		// empty
	}

	const std::string& getName() { return _name; }
	void setName( const std::string& name ) { _name = name; }

	erm::IEntity* getParent()
	{
		return _parent;
	}

	void setParent( erm::IEntity* parent )
	{
		_parent = parent;
	}

	co::Range<erm::IRelationship* const> getRelationships()
	{
		return co::Range<erm::IRelationship* const>();
	}

private:
	std::string _name;
	erm::IEntity* _parent;
};
	
CORAL_EXPORT_COMPONENT( Entity, Entity )

} // namespace erm
