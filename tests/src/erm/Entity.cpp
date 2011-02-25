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

private:
	std::string _name;
};

} // namespace erm

CORAL_EXPORT_COMPONENT( erm::Entity, Entity )
