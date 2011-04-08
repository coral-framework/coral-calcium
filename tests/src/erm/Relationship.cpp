/*
 * Calcium Sample
 * Entity Relationship Model (ERM)
 */

#include "Relationship_Base.h"
#include <erm/Multiplicity.h>

namespace erm {

class Relationship : public Relationship_Base
{
public:
	Relationship() : _entityA( NULL ), _entityB( NULL )
	{
		// empty
	}

	virtual ~Relationship()
	{
		// empty
	}

	IEntity* getEntityA() { return _entityA; }
	void setEntityA( IEntity* entity ) { _entityA = entity; }

	IEntity* getEntityB() { return _entityB; }
	void setEntityB( IEntity* entity ) { _entityB = entity; }

	const Multiplicity& getMultiplicityA() { return _multiplicityA; }
	void setMultiplicityA( const Multiplicity& multiplicity ) { _multiplicityA = multiplicity; }

	const Multiplicity& getMultiplicityB() { return _multiplicityB; }
	void setMultiplicityB( const Multiplicity& multiplicity ) { _multiplicityB = multiplicity; }

	const std::string& getRelation() { return _relation; }
	void setRelation( const std::string& relation ) { _relation = relation; }

private:
	IEntity* _entityA;
	IEntity* _entityB;
	Multiplicity _multiplicityA;
	Multiplicity _multiplicityB;
	std::string _relation;
};

CORAL_EXPORT_COMPONENT( Relationship, Relationship )

} // namespace erm
