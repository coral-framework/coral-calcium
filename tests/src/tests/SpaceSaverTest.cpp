/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */
#include "ERMSpace.h"

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
#include <cstdio>

class SpaceSaverTest : public ERMSpace {};

TEST_F( SpaceSaverTest, simpleTest )
{

	co::RefPtr<co::IObject> spaceObj = co::newInstance( "ca.Space" );
	ca::ISpace* space = spaceObj->getService<ca::ISpace>();

	startWithExtendedERM();

	co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();
	spaceObj->setService( "universe", universe );

	universeObj->setService("model", _model.get());
	
	space->setRootObject(_erm->getProvider());
	
	co::RefPtr<co::IObject> obj = co::newInstance( "ca.SpaceSaverSQLite3" );
	co::RefPtr<ca::ISpaceSaver> spaceSav = obj->getService<ca::ISpaceSaver>();
	spaceSav->setModel( _model.get() );
	spaceSav->setSpace( space );
	spaceSav->setup();
}
