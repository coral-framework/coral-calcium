/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ERMSpace.h"

void ERMSpace::onSpaceChanged( ca::ISpaceChanges* changes )
{
	_changes = changes;
}

co::IInterface* ERMSpace::getInterface() { return 0; }
co::IObject* ERMSpace::getProvider() { return 0; }
co::IPort* ERMSpace::getFacet() { return 0; }
void ERMSpace::serviceRetain() {;}
void ERMSpace::serviceRelease() {;}

void ERMSpace::SetUp()
{
	// create an object model
	_modelObj = co::newInstance( "ca.Model" );
	_model = _modelObj->getService<ca::IModel>();
	assert( _model.isValid() );

	_model->setName( "erm" );

	// create an object universe and bind the model
	_universeObj = co::newInstance( "ca.Universe" );
	_universe = _universeObj->getService<ca::IUniverse>();
	assert( _universe.isValid() );

	_universeObj->setService( "model", _model.get() );

	// create an object space and bind it to the universe
	_spaceObj = co::newInstance( "ca.Space" );
	_space = _spaceObj->getService<ca::ISpace>();
	assert( _space.isValid() );

	_spaceObj->setService( "universe", _universe.get() );

	_space->addSpaceObserver( this );
}

void ERMSpace::TearDown()
{
	_modelObj = NULL;
	_spaceObj = NULL;
	_universeObj = NULL;
	
	_model = NULL;
	_space = NULL;
	_universe = NULL;
}

co::IObject* ERMSpace::createSimpleERM()
{
	// create a simple object graph with 2 entities and 1 relationship
	_entityA = co::newInstance( "erm.Entity" )->getService<erm::IEntity>();
	_entityA->setName( "Entity A" );
	
	_entityB = co::newInstance( "erm.Entity" )->getService<erm::IEntity>();
	_entityB->setName( "Entity B" );
	
	_relAB = co::newInstance( "erm.Relationship" )->getService<erm::IRelationship>();
	_relAB->setRelation( "relation A-B" );
	_relAB->setEntityA( _entityA.get() );
	_relAB->setEntityB( _entityB.get() );
	
	_erm = co::newInstance( "erm.Model" )->getService<erm::IModel>();
	_erm->addEntity( _entityA.get() );
	_erm->addEntity( _entityB.get() );
	_erm->addRelationship( _relAB.get() );

	// 1 entity and 2 rels are created but not added to the graph yet
	_entityC = co::newInstance( "erm.Entity" )->getService<erm::IEntity>();
	_entityC->setName( "Entity C" );

	_relBC = co::newInstance( "erm.Relationship" )->getService<erm::IRelationship>();
	_relBC->setRelation( "relation B-C" );

	_relCA = co::newInstance( "erm.Relationship" )->getService<erm::IRelationship>();
	_relCA->setRelation( "relation C-A" );

	return _erm->getProvider();
}

void ERMSpace::startWithSimpleERM()
{
	_space->addRootObject( createSimpleERM() );

	// skip the first notification with the initial object additions
	_space->notifyChanges();
	_changes = NULL;
}

void ERMSpace::extendSimpleERM()
{
	// adds 1 entity and 2 relationships, closing a 'loop' in the ERM
	_relBC->getProvider()->setService( "entityA", _entityB.get() );
	_relBC->getProvider()->setService( "entityB", _entityC.get() );

	_relCA->setEntityA( _entityC.get() );
	_relCA->setEntityB( _entityA.get() );

	_erm->addEntity( _entityC.get() );
	_erm->addRelationship( _relBC.get() );
	_erm->addRelationship( _relCA.get() );
}

void ERMSpace::startWithExtendedERM()
{
	createSimpleERM();
	extendSimpleERM();
	_space->addRootObject( _erm->getProvider() );

	// skip the first notification with the initial object additions
	_space->notifyChanges();
	_changes = NULL;
}
