/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include <gtest/gtest.h>

#include <co/Coral.h>
#include <co/IObject.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/IUniverse.h>
#include <ca/ModelException.h>
#include <ca/NoSuchObjectException.h>


TEST( BasicTests, spaceUniverseAndModelSetup )
{
	// create a plain space
	co::RefPtr<co::IObject> spaceObj = co::newInstance( "ca.Space" );
	ca::ISpace* space = spaceObj->getService<ca::ISpace>();
	assert( space );

	// try using a space without an universe
	EXPECT_THROW( space->addRootObject( spaceObj.get() ), co::IllegalStateException );

	// try binding a null universe to a space
	EXPECT_THROW( spaceObj->setService( "universe", NULL ), co::IllegalArgumentException );

	// create a plain universe and bind it to the space
	co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();
	spaceObj->setService( "universe", universe );

	// cannot bind another universe to a space after one is bound
	EXPECT_THROW( spaceObj->setService( "universe", universe ), co::IllegalStateException );

	// we should already be able to call methods that don't require a model
	co::RefVector<co::IObject> rootObjs;
	EXPECT_NO_THROW( space->getRootObjects( rootObjs ) );

	// but we should not be able to call methos that do require a model
	EXPECT_THROW( space->addRootObject( spaceObj.get() ), co::IllegalStateException );

	// create a plain model and bind it to the universe
	co::RefPtr<co::IObject> modelObj = co::newInstance( "ca.Model" );
	ca::IModel* model = modelObj->getService<ca::IModel>();
	universeObj->setService( std::string( "model" ), model );

	// we should not be able to use the model because it doesn't have a name
	EXPECT_THROW( space->addRootObject( spaceObj.get() ), co::IllegalStateException );

	model->setName( "test" );

	// now the whole setup should be ready for use
	EXPECT_THROW( space->addRootObject( spaceObj.get() ), ca::ModelException );
	EXPECT_THROW( space->beginChange( spaceObj.get() ), ca::NoSuchObjectException );

	// and we cannot change the model's name after it's been set
	EXPECT_THROW( model->setName( "test" ), co::IllegalStateException );	
}
