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
#include <ca/INamed.h>
#include <ca/IUniverse.h>
#include <ca/ModelException.h>
#include <ca/NoSuchObjectException.h>
#include <cstdio>
#include <sqlite3.h>

class SpaceSaverTest : public ERMSpace {};

TEST_F( SpaceSaverTest, testNewFileSetup )
{
	int modelVersion = 1;

	co::RefPtr<co::IObject> spaceObj = co::newInstance( "ca.Space" );
	ca::ISpace* space = spaceObj->getService<ca::ISpace>();
	
	startWithExtendedERM();

	spaceObj->getService("universe");

	ca::IUniverse* universe = static_cast<ca::IUniverse*>( spaceObj->getService( "universe" ) );
	assert( universe );

	ca::IModel* model = static_cast<ca::IModel*>( universe->getProvider()->getService( "model" ) );

	co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();
	spaceObj->setService( "universe", universe );

	universeObj->setService("model", _model.get());
	
	space->setRootObject(_erm->getProvider());
	
	co::RefPtr<co::IObject> obj = co::newInstance( "ca.SpaceSaverSQLite3" );
	co::RefPtr<ca::ISpaceSaver> spaceSav = obj->getService<ca::ISpaceSaver>();

	std::string fileName = "SimpleSpaceSave.db";
	remove( fileName.c_str() );

	ca::INamed* file = (obj->getService<ca::INamed>());
	file->setName( fileName );
	
	spaceSav->setModel( _model.get() );
	spaceSav->setModelVersion(modelVersion);
	spaceSav->setSpace( space );

	EXPECT_NO_THROW(spaceSav->setup());
	
	FILE* dbFile = 0;
	EXPECT_NO_THROW(dbFile = fopen(fileName.c_str(), "r"));
	EXPECT_TRUE(dbFile != 0);

	sqlite3* db; 
	sqlite3_open( fileName.c_str(), &db );

	sqlite3_stmt *statement;

	sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type = 'table' and name <> 'sqlite_sequence' ORDER BY name", -1, &statement, 0);

	char* tables[] = { "CALCIUM_MODEL",
					    "ENTITY",
						"FIELD",
						"FIELD_VALUES",
						"OBJECT"
					 };

	int i = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		EXPECT_TRUE(strcmp((char*)sqlite3_column_text(statement, 0), tables[i])==0) ;
		i++;
	}
	EXPECT_EQ( i, 5);

	sqlite3_prepare_v2(db, "SELECT * FROM CALCIUM_MODEL", -1, &statement, 0);

	int cols = sqlite3_column_count(statement);

	sqlite3_step(statement);
	
	int caModelId = atoi((char*)sqlite3_column_text(statement, 0));

	EXPECT_EQ(caModelId, 1);

	//Model name
	EXPECT_TRUE(strcmp((char*)sqlite3_column_text(statement, 1), _model->getName().c_str()) == 0);

	int caModelVersion = atoi((char*)sqlite3_column_text(statement, 0));
	EXPECT_EQ(caModelVersion, 1);

	sqlite3_close(db);
}
