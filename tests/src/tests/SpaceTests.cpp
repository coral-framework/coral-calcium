/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include <gtest/gtest.h>

#include <co/Coral.h>
#include <co/IObject.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/NoSuchObjectException.h>

#include <erm/IModel.h>
#include <erm/IEntity.h>
#include <erm/IRelationship.h>


TEST( SpaceTests, basics )
{
	// create an object model
	co::RefPtr<co::IObject> modelObj = co::newInstance( "ca.Model" );
	ca::IModel* model = modelObj->getService<ca::IModel>();
	assert( model );
	model->setName( "erm" );

	// create an object universe and bind the model
	co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	co::bindService( universeObj.get(), "model", model );

	// create an object space and bind it to the universe
	co::RefPtr<co::IObject> spaceObj = co::newInstance( "ca.Space" );
	co::bindService( spaceObj.get(), "universe", universeObj.get() );
	ca::ISpace* space = spaceObj->getService<ca::ISpace>();
	assert( space );

	// the space is empty, so beginChange() should always fail
	EXPECT_THROW( space->beginChange( model ), ca::NoSuchObjectException );

	// create a simple object graph with 2 entities and 1 relationship
	erm::IEntity* entityA = co::newInstance( "erm.Entity" )->getService<erm::IEntity>();
	entityA->setName( "Entity A" );

	erm::IEntity* entityB = co::newInstance( "erm.Entity" )->getService<erm::IEntity>();
	entityB->setName( "Entity B" );

	erm::IRelationship* rel = co::newInstance( "erm.Relationship" )->getService<erm::IRelationship>();
	rel->setRelation( "relation name" );
	rel->setEntityA( entityA );
	rel->setEntityB( entityB );

	co::RefPtr<co::IObject> ermModelObj = co::newInstance( "erm.Model" );
	erm::IModel* ermModel = ermModelObj->getService<erm::IModel>();
	ermModel->addEntity( entityA );
	ermModel->addEntity( entityB );
	ermModel->addRelationship( rel );

	// none of the created components are in the space yet
	EXPECT_THROW( space->beginChange( ermModel ), ca::NoSuchObjectException );
	EXPECT_THROW( space->beginChange( entityA ), ca::NoSuchObjectException );
	EXPECT_THROW( space->beginChange( entityB ), ca::NoSuchObjectException );
	EXPECT_THROW( space->beginChange( rel ), ca::NoSuchObjectException );

	// add the graph's root object (the erm.Model) to the space
	space->addRootObject( ermModelObj.get() );

	// now the space should contain the whole graph
	EXPECT_NO_THROW( space->beginChange( ermModel ) );
	EXPECT_NO_THROW( space->endChange( ermModel ) );

	EXPECT_NO_THROW( space->beginChange( entityA ) );
	EXPECT_NO_THROW( space->endChange( entityA ) );

	EXPECT_NO_THROW( space->beginChange( entityB ) );
	EXPECT_NO_THROW( space->endChange( entityB ) );

	EXPECT_NO_THROW( space->beginChange( rel ) );
	EXPECT_NO_THROW( space->endChange( rel ) );
}
