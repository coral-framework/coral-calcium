/*
 * Calcium Sample
 * Entity Relationship Model (ERM)
 */

#include "Entity_Base.h"

namespace erm {

class Entity : public Entity_Base
{
public:
	Entity()
	{
		// empty
	}

	virtual ~Entity()
	{
		// empty
	}

	const std::string& getName() { return _name; }
	void setName( const std::string& name ) { _name = name; }

	co::Range<erm::IRelationship* const> getRelationships()
	{
		return co::Range<erm::IRelationship* const>();
	}

private:
	std::string _name;
};
	
CORAL_EXPORT_COMPONENT( Entity, Entity )

} // namespace erm
