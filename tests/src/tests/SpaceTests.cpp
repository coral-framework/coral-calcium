/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include <gtest/gtest.h>

#include <co/Coral.h>
#include <co/IObject.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/IUniverse.h>
#include <ca/NoSuchObjectException.h>

#include <erm/IModel.h>
#include <erm/IEntity.h>
#include <erm/IRelationship.h>


class SpaceTests : public ::testing::Test
{
protected:
	virtual void SetUp()
	{
		// create an object model
		_modelObj = co::newInstance( "ca.Model" );
		_model = _modelObj->getService<ca::IModel>();
		assert( _model );

		_model->setName( "erm" );

		// create an object universe and bind the model
		_universeObj = co::newInstance( "ca.Universe" );
		_universe = _universeObj->getService<ca::IUniverse>();
		assert( _universe );

		_universeObj->setService( "model", _model.get() );

		// create an object space and bind it to the universe
		_spaceObj = co::newInstance( "ca.Space" );
		_space = _spaceObj->getService<ca::ISpace>();
		assert( _space );

		_spaceObj->setService( "universe", _universe.get() );
	}

	virtual void TearDown()
	{
		_modelObj = NULL;
		_spaceObj = NULL;
		_universeObj = NULL;

		_model = NULL;
		_space = NULL;
		_universe = NULL;
	}

protected:
	co::RefPtr<co::IObject> _modelObj;
	co::RefPtr<co::IObject> _universeObj;
	co::RefPtr<co::IObject> _spaceObj;

	co::RefPtr<ca::IModel> _model;
	co::RefPtr<ca::IUniverse> _universe;
	co::RefPtr<ca::ISpace> _space;
};

TEST_F( SpaceTests, basics )
{
	// the space is empty, so beginChange() should always fail
	EXPECT_THROW( _space->beginChange( _model.get() ), ca::NoSuchObjectException );

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
	EXPECT_THROW( _space->beginChange( ermModel ), ca::NoSuchObjectException );
	EXPECT_THROW( _space->beginChange( entityA ), ca::NoSuchObjectException );
	EXPECT_THROW( _space->beginChange( entityB ), ca::NoSuchObjectException );
	EXPECT_THROW( _space->beginChange( rel ), ca::NoSuchObjectException );

	// add the graph's root object (the erm.Model) to the space
	_space->addRootObject( ermModelObj.get() );

	// now the space should contain the whole graph
	EXPECT_EQ( 0, _space->beginChange( ermModel ) );
	EXPECT_NO_THROW( _space->endChange( 0 ) );

	EXPECT_EQ( 0, _space->beginChange( entityA ) );
	EXPECT_NO_THROW( _space->endChange( 0 ) );

	EXPECT_EQ( 0, _space->beginChange( entityB ) );
	EXPECT_NO_THROW( _space->endChange( 0 ) );

	EXPECT_EQ( 0, _space->beginChange( rel ) );
	EXPECT_NO_THROW( _space->endChange( 0 ) );
}
