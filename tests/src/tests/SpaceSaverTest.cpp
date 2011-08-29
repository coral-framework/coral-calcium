/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include <gtest/gtest.h>

#include <co/Coral.h>
#include <co/IObject.h>
#include <ca/ISpaceSaver.h>
#include <co/RefPtr.h>
#include <co/Coral.h>
#include <co/IObject.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/IUniverse.h>
#include <ca/ModelException.h>
#include <ca/NoSuchObjectException.h>
#include <stdio.h>



TEST( SpaceSaverTest, simpleTest )
{
	co::IInterface* someInterface = co::cast<co::IInterface>( co::getType( "camodels.SomeInterface" ) );
	co::RefPtr<co::IObject> ermObj = co::newInstance( "erm.Entity" );

	co::RefPtr<co::IObject> spaceObj = co::newInstance( "ca.Space" );
	ca::ISpace* space = spaceObj->getService<ca::ISpace>();

	co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();
	spaceObj->setService( "universe", universe );

	co::RefPtr<co::IObject> modelObj = co::newInstance( "ca.Model" );
	ca::IModel* model = modelObj->getService<ca::IModel>();
	universeObj->setService( std::string( "model" ), model );
	model->setName( "erm" );

	space->addRootObject( ermObj.get() );

	co::RefPtr<co::IObject> obj = co::newInstance( "ca.SpaceSaverSQLite3" );
	co::RefPtr<ca::ISpaceSaver> spaceSav = obj->getService<ca::ISpaceSaver>();
	
	spaceSav->setModel( model );
	spaceSav->setSpace( space );
	spaceSav->setup();
}


