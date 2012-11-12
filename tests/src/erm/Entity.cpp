/*
 * Calcium Sample
 * Entity Relationship Model (ERM)
 */

#include "Entity_Base.h"
#include <erm/IRelationship.h>

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

	std::string getName() { return _name; }
	void setName( const std::string& name ) { _name = name; }

	erm::IEntity* getParent()
	{
		return _parent;
	}

	void setParent( erm::IEntity* parent )
	{
		_parent = parent;
	}

	co::TSlice<erm::IRelationship*> getRelationships()
	{
		return _rels;
	}

	void addRelationship( erm::IRelationship* rel )
	{
		_rels.push_back( rel );
	}

	void removeRelationship( erm::IRelationship* rel )
	{
		_rels.erase( std::remove( _rels.begin(), _rels.end(), rel ), _rels.end() );
	}

	co::TSlice<std::string> getAdjacentEntityNames()
	{
		std::vector<std::string> names;
		for( auto it = _rels.begin(); it != _rels.end(); ++it )
		{
			erm::IRelationship* r = *it;
			erm::IEntity* e = r->getEntityA();
			if( e == this )
				e = r->getEntityB();
			names.push_back( e->getName() );
		}
		std::sort( names.begin(), names.end() );
		names.erase( std::unique( names.begin(), names.end() ), names.end() );
		return co::moveToSlice<std::string>( names );
	}

private:
	std::string _name;
	erm::IEntity* _parent;
	std::vector<erm::IRelationship*> _rels;
};
	
CORAL_EXPORT_COMPONENT( Entity, Entity )

} // namespace erm
