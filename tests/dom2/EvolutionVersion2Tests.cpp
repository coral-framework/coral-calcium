/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "persistence/sqlite/sqlite3.h"

#include <gtest/gtest.h>

#include <co/Coral.h>
#include <co/IObject.h>
#include <co/RefPtr.h>

#include <dom/ICompany.h>
#include <dom/IEmployee.h>
#include <dom/IProduct.h>
#include <dom/IService.h>

#include <ca/ISpacePersister.h>
#include <ca/ISpace.h>
#include <ca/INamed.h>
#include <ca/IModel.h>
#include <ca/IUniverse.h>
#include <ca/ISpaceStore.h>
#include <ca/IOException.h>


class EvolutionVersion2Tests : public ::testing::Test
{
public:

	void SetUp()
	{
		
	}

	ca::ISpacePersisterRef createPersister( const std::string& fileName, const std::string& modelName )
	{
		co::IObjectRef persisterObj = co::newInstance( "ca.SpacePersister" );
		ca::ISpacePersister* persister = persisterObj->getService<ca::ISpacePersister>();

		co::IObjectRef spaceFileObj = co::newInstance( "ca.SQLiteSpaceStore" );
		spaceFileObj->getService<ca::INamed>()->setName( fileName );

		co::IObject* universeObj = co::newInstance( "ca.Universe" );

		co::IObject* modelObj = co::newInstance( "ca.Model" );
		ca::IModel* model = modelObj->getService<ca::IModel>();
		model->setName( modelName );


		_universe = universeObj->getService<ca::IUniverse>();

		universeObj->setService( "model", model );

		persisterObj->setService( "store", spaceFileObj->getService<ca::ISpaceStore>() );
		persisterObj->setService( "universe", _universe.get() );
		
		return persister;
	}

	ca::IUniverseRef _universe;

};

TEST_F( EvolutionVersion2Tests, testScriptNotFound )
{
	std::string fileName = "CompanyV1.db";

	ca::ISpacePersisterRef persister = createPersister( fileName, "notfound" );

	ASSERT_THROW( persister->restore(), ca::IOException );

}

TEST_F( EvolutionVersion2Tests, testSyntaxErrorUpdate )
{
	std::string fileName = "CompanyV1.db";

	ca::ISpacePersisterRef persister = createPersister( fileName, "syntaxerror" );

	ASSERT_THROW( persister->restore(), ca::IOException );
}

TEST_F( EvolutionVersion2Tests, testScriptWithoutUpdateFunc )
{
	std::string fileName = "CompanyV1.db";

	ca::ISpacePersisterRef persister = createPersister( fileName, "scriptNoUpdate" );

	ASSERT_THROW( persister->restore(), ca::IOException );
}

TEST_F( EvolutionVersion2Tests, restoreV2SpaceFromV1FilePreviousRevision )
{
	std::string fileName = "CompanyV1.db";

	ca::ISpacePersisterRef persister = createPersister( fileName, "dom" );

	ASSERT_NO_THROW( persister->restoreRevision( 1 ) ); // restore allowed

	ca::ISpace * spaceRestored = persister->getSpace();

	co::IObject* objRest = spaceRestored->getRootObject();

	dom::ICompany* company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	co::TSlice<dom::IProduct*> products = company->getProducts();
	ASSERT_EQ( 1, products.getSize() );

	EXPECT_EQ( "Software2.0", products[0]->getName() );
	EXPECT_EQ( 1000000, products[0]->getValue() );

	{
		co::TSlice<dom::IEmployee*> devs = products[0]->getDevelopers();

		EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
		EXPECT_EQ( 1000, devs[0]->getSalary() );
		EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
		EXPECT_EQ( 4000, devs[1]->getSalary() );

		EXPECT_EQ( "Developer", devs[0]->getRole() );
		EXPECT_EQ( "Developer", devs[1]->getRole() );
	}

	dom::IEmployee* manager = products[0]->getLeader();

	EXPECT_EQ( "Richard Scrum Master", manager->getName() );
	EXPECT_EQ( 10000, manager->getSalary() );

	EXPECT_EQ( "Manager", manager->getRole() );

	co::TSlice<dom::IService*> services = company->getServices();
	ASSERT_EQ( 1, services.getSize() );

	EXPECT_EQ( "Software1.0 Maintenance", services[0]->getName() );
	EXPECT_EQ( 50000, services[0]->getMonthlyIncome() );

	{
		co::TSlice<dom::IEmployee*> devs = services[0]->getMantainers();

		ASSERT_EQ( 2, devs.getSize() );

		EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
		EXPECT_EQ( 5000, devs[0]->getSalary() );
		EXPECT_EQ( "Developer", devs[0]->getRole() );

		EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
		EXPECT_EQ( 3000, devs[1]->getSalary() );
		EXPECT_EQ( "Developer", devs[1]->getRole() );
	}

	ASSERT_THROW( persister->save(), ca::IOException ); //save not allowed
}

// i'll put some effort to keep both tests working, for this, it'll be needed two different version 1 databases. 
// For now, the most complicated one will be run

TEST_F( EvolutionVersion2Tests, restoreV2SpaceFromV1FileLastRevision )
{
	std::string fileName = "CompanyV1.db";

	ca::ISpacePersisterRef persister = createPersister( fileName, "dom" );

	ASSERT_NO_THROW( persister->restore() );

	ca::ISpace* spaceRestored = persister->getSpace();
	
	co::IObject* objRest = spaceRestored->getRootObject();

	dom::ICompany* company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	{
		co::TSlice<dom::IProduct*> products = company->getProducts();
		ASSERT_EQ( 1, products.getSize() );

		EXPECT_EQ( "Software2.0", products[0]->getName() );
		EXPECT_EQ( 1000000, products[0]->getValue() );

		co::TSlice<dom::IEmployee*> devs = products[0]->getDevelopers();

		EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
		EXPECT_EQ( 1000, devs[0]->getSalary() );
		EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
		EXPECT_EQ( 5000, devs[1]->getSalary() );

		EXPECT_EQ( "Developer", devs[0]->getRole() );
		EXPECT_EQ( "Developer", devs[1]->getRole() );

		dom::IEmployee* manager = products[0]->getLeader();
		EXPECT_EQ( "Richard Scrum Master", manager->getName() );
		EXPECT_EQ( 10000, manager->getSalary() );
		EXPECT_EQ( "Manager", manager->getRole() );
	}
	{
		co::TSlice<dom::IService*> services = company->getServices();
		ASSERT_EQ( 1, services.getSize() );

		EXPECT_EQ( "Software1.0 Maintenance", services[0]->getName() );
		EXPECT_EQ( 50000, services[0]->getMonthlyIncome() );

		co::TSlice<dom::IEmployee*> devs = services[0]->getMantainers();
		ASSERT_EQ( 2, devs.getSize() );

		EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
		EXPECT_EQ( 5000, devs[0]->getSalary() );
		EXPECT_EQ( "Developer", devs[0]->getRole() );

		EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
		EXPECT_EQ( 3000, devs[1]->getSalary() );
		EXPECT_EQ( "Developer", devs[1]->getRole() );
	}

	ASSERT_NO_THROW( persister->save() ); //save allowed

	ca::ISpacePersisterRef persisterToRestore = createPersister( fileName, "dom" );

	ASSERT_NO_THROW( persisterToRestore->restore() );

	spaceRestored = persisterToRestore->getSpace();

	objRest = spaceRestored->getRootObject();

	company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	{
		co::TSlice<dom::IProduct*> products = company->getProducts();
		
		ASSERT_EQ( 1, products.getSize() );

		EXPECT_EQ( "Software2.0", products[0]->getName() );
		EXPECT_EQ( 1000000, products[0]->getValue() );

		co::TSlice<dom::IEmployee*> devs = products[0]->getDevelopers();

		EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
		EXPECT_EQ( 1000, devs[0]->getSalary() );
		EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
		EXPECT_EQ( 5000, devs[1]->getSalary() );

		EXPECT_EQ( "Developer", devs[0]->getRole() );
		EXPECT_EQ( "Developer", devs[1]->getRole() );

		dom::IEmployee* manager = products[0]->getLeader();
		EXPECT_EQ( "Richard Scrum Master", manager->getName() );
		EXPECT_EQ( 10000, manager->getSalary() );
		EXPECT_EQ( "Manager", manager->getRole() );
		manager->setRole("Development Manager");

		spaceRestored->addChange( manager );
		spaceRestored->notifyChanges();
	}
	{
		co::TSlice<dom::IService*> services = company->getServices();
		ASSERT_EQ( 1, services.getSize() );

		EXPECT_EQ( "Software1.0 Maintenance", services[0]->getName() );
		EXPECT_EQ( 50000, services[0]->getMonthlyIncome() );

		co::TSlice<dom::IEmployee*> devs = services[0]->getMantainers();

		ASSERT_EQ( 2, devs.getSize() );

		EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
		EXPECT_EQ( 5000, devs[0]->getSalary() );
		EXPECT_EQ( "Developer", devs[0]->getRole() );

		EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
		EXPECT_EQ( 3000, devs[1]->getSalary() );
		EXPECT_EQ( "Developer", devs[1]->getRole() );

		devs[1]->setSalary( 4000 );
		spaceRestored->addChange( devs[1] );
		spaceRestored->notifyChanges();

		ASSERT_NO_THROW( persisterToRestore->save() );

		ca::ISpacePersisterRef persisterToRestore2 = createPersister( fileName, "dom" );

		ASSERT_NO_THROW( persisterToRestore2->restore() );

		spaceRestored = persisterToRestore2->getSpace();

		objRest = spaceRestored->getRootObject();

		company = objRest->getService<dom::ICompany>();
		ASSERT_TRUE( company != NULL );
	}
	{
		co::TSlice<dom::IProduct*> products = company->getProducts();

		ASSERT_EQ( 1, products.getSize() );

		EXPECT_EQ( "Software2.0", products[0]->getName() );
		EXPECT_EQ( 1000000, products[0]->getValue() );
		co::TSlice<dom::IEmployee*> devs = products[0]->getDevelopers();

		EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
		EXPECT_EQ( 1000, devs[0]->getSalary() );
		EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
		EXPECT_EQ( 5000, devs[1]->getSalary() );

		EXPECT_EQ( "Developer", devs[0]->getRole() );
		EXPECT_EQ( "Developer", devs[1]->getRole() );

		dom::IEmployee* manager = products[0]->getLeader();

		EXPECT_EQ( "Richard Scrum Master", manager->getName() );
		EXPECT_EQ( 10000, manager->getSalary() );

		EXPECT_EQ( "Development Manager", manager->getRole() );
	}
	{
		co::TSlice<dom::IService*> services = company->getServices();
		ASSERT_EQ( 1, services.getSize() );

		EXPECT_EQ( "Software1.0 Maintenance", services[0]->getName() );
		EXPECT_EQ( 50000, services[0]->getMonthlyIncome() );
		co::TSlice<dom::IEmployee*> devs = services[0]->getMantainers();

		ASSERT_EQ( 2, devs.getSize() );

		EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
		EXPECT_EQ( 5000, devs[0]->getSalary() );
		EXPECT_EQ( "Developer", devs[0]->getRole() );

		EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
		EXPECT_EQ( 4000, devs[1]->getSalary() );
		EXPECT_EQ( "Developer", devs[1]->getRole() );
	}
}

TEST_F( EvolutionVersion2Tests, restoreV2SpaceFromV1FileLastRevisionInv )
{
	std::string fileName = "CompanyV1Inv.db";

	ca::ISpacePersisterRef persister = createPersister( fileName, "domInv" );
	
	ASSERT_NO_THROW( persister->restore() );
	
	ca::ISpace * spaceRestored = persister->getSpace();
	
	co::IObject* objRest = spaceRestored->getRootObject();
	
	dom::ICompany* company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	dom::IEmployee* ceo = co::cast<dom::IEmployee>( objRest->getService( "ceo" ) );
	ASSERT_TRUE( ceo != NULL );

	EXPECT_EQ( "James CEO Boss", ceo->getName() );
	EXPECT_EQ( 100000, ceo->getSalary() );
	EXPECT_EQ( "CEO", ceo->getRole() );
	EXPECT_EQ( NULL, ceo->getLeading() );
	EXPECT_EQ( 0, ceo->getWorking().getSize() );

	{
		co::TSlice<dom::IEmployee*> employees = company->getEmployees();
		ASSERT_EQ( 5, employees.getSize() );
		
		EXPECT_EQ( "Joseph Java Newbie", employees[0]->getName() );
		EXPECT_EQ( 1000, employees[0]->getSalary() );
		EXPECT_EQ( "Developer", employees[0]->getRole() );
		EXPECT_EQ( NULL, employees[0]->getLeading() );

		ASSERT_EQ( 1, employees[0]->getWorking().getSize() );

		dom::IProduct* devProduct;
		ASSERT_NO_THROW( devProduct = co::cast<dom::IProduct>( employees[0]->getWorking()[0] ) );
		EXPECT_EQ( "Software2.0", devProduct->getName() );
		EXPECT_EQ( 1000000, devProduct->getValue() );

		EXPECT_EQ( "Michael CSharp Senior", employees[1]->getName() );
		EXPECT_EQ( 5000, employees[1]->getSalary() );
		EXPECT_EQ( "Developer", employees[1]->getRole() );
		EXPECT_EQ( NULL, employees[1]->getLeading() );

		ASSERT_EQ( 1, employees[1]->getWorking().getSize() );
		ASSERT_NO_THROW( devProduct = co::cast<dom::IProduct>( employees[1]->getWorking()[0] ) );
		EXPECT_EQ( "Software2.0", devProduct->getName() );
		EXPECT_EQ( 1000000, devProduct->getValue() );

		EXPECT_EQ( "Richard Scrum Master", employees[2]->getName() );
		EXPECT_EQ( 10000, employees[2]->getSalary() );
		EXPECT_EQ( "Manager", employees[2]->getRole() );

		EXPECT_EQ( 0, employees[2]->getWorking().getSize() );
		ASSERT_TRUE( employees[2]->getLeading() != NULL );
		ASSERT_NO_THROW( devProduct = co::cast<dom::IProduct>( employees[2]->getLeading() ) );

		EXPECT_EQ( "Software2.0", devProduct->getName() );
		EXPECT_EQ( 1000000, devProduct->getValue() );

		EXPECT_EQ( "John Cplusplus Experienced", employees[3]->getName() );
		EXPECT_EQ( 5000, employees[3]->getSalary() );
		EXPECT_EQ( "Developer", employees[3]->getRole() );
		EXPECT_EQ( NULL, employees[3]->getLeading() );

		dom::IService* devService; 

		ASSERT_EQ( 1, employees[3]->getWorking().getSize() );
		ASSERT_NO_THROW( devService = co::cast<dom::IService>( employees[3]->getWorking()[0] ) );
		EXPECT_EQ( "Software1.0 Maintenance", devService->getName() );
		EXPECT_EQ( 50000.0/1.90, devService->getMonthlyIncome() );

		EXPECT_EQ( "Jacob Lua Junior", employees[4]->getName() );
		EXPECT_EQ( 3000, employees[4]->getSalary() );
		EXPECT_EQ( "Developer", employees[4]->getRole() );
		EXPECT_EQ( NULL, employees[4]->getLeading() );

		ASSERT_EQ( 1, employees[4]->getWorking().getSize() );
		ASSERT_NO_THROW( devService = co::cast<dom::IService>( employees[4]->getWorking()[0] ) );
		EXPECT_EQ( "Software1.0 Maintenance", devService->getName() );
		EXPECT_NEAR( 26315.79, devService->getMonthlyIncome(), 0.01 );
	}

	ASSERT_NO_THROW( persister->save() );

	ca::ISpacePersisterRef persisterToRestore = createPersister( fileName, "domInv" );

	ASSERT_NO_THROW( persisterToRestore->restore() );

	spaceRestored = persisterToRestore->getSpace();

	objRest = spaceRestored->getRootObject();

	company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	{
		co::TSlice<dom::IEmployee*> employees = company->getEmployees();
		ASSERT_EQ( 5, employees.getSize() );

		EXPECT_EQ( "Joseph Java Newbie", employees[0]->getName() );
		EXPECT_EQ( 1000, employees[0]->getSalary() );
		EXPECT_EQ( "Developer", employees[0]->getRole() );
		EXPECT_EQ( NULL, employees[0]->getLeading() );

		dom::IProduct* devProduct;

		ASSERT_EQ( 1, employees[0]->getWorking().getSize() );
		ASSERT_NO_THROW( devProduct = co::cast<dom::IProduct>( employees[0]->getWorking()[0] ) );
		EXPECT_EQ( "Software2.0", devProduct->getName() );
		EXPECT_EQ( 1000000, devProduct->getValue() );

		EXPECT_EQ( "Michael CSharp Senior", employees[1]->getName() );
		EXPECT_EQ( 5000, employees[1]->getSalary() );
		EXPECT_EQ( "Developer", employees[1]->getRole() );
		EXPECT_EQ( NULL, employees[1]->getLeading() );

		ASSERT_EQ( 1, employees[1]->getWorking().getSize() );
		ASSERT_NO_THROW( devProduct = co::cast<dom::IProduct>( employees[1]->getWorking()[0] ) );
		EXPECT_EQ( "Software2.0", devProduct->getName() );
		EXPECT_EQ( 1000000, devProduct->getValue() );

		EXPECT_EQ( "Richard Scrum Master", employees[2]->getName() );
		EXPECT_EQ( 10000, employees[2]->getSalary() );
		EXPECT_EQ( "Manager", employees[2]->getRole() );

		EXPECT_EQ( 0, employees[2]->getWorking().getSize() );
		ASSERT_TRUE( employees[2]->getLeading() != NULL );
		ASSERT_NO_THROW( devProduct = co::cast<dom::IProduct>( employees[2]->getLeading() ) );

		EXPECT_EQ( "Software2.0", devProduct->getName() );
		EXPECT_EQ( 1000000, devProduct->getValue() );

		EXPECT_EQ( "John Cplusplus Experienced", employees[3]->getName() );
		EXPECT_EQ( 5000, employees[3]->getSalary() );
		EXPECT_EQ( "Developer", employees[3]->getRole() );
		EXPECT_EQ( NULL, employees[3]->getLeading() );

		dom::IService* devService;

		ASSERT_EQ( 1, employees[3]->getWorking().getSize() );
		ASSERT_NO_THROW( devService = co::cast<dom::IService>( employees[3]->getWorking()[0] ) );
		EXPECT_EQ( "Software1.0 Maintenance", devService->getName() );
		EXPECT_NEAR( 26315.79, devService->getMonthlyIncome(), 0.01 );

		EXPECT_EQ( "Jacob Lua Junior", employees[4]->getName() );
		EXPECT_EQ( 3000, employees[4]->getSalary() );
		EXPECT_EQ( "Developer", employees[4]->getRole() );
		EXPECT_EQ( NULL, employees[4]->getLeading() );

		ASSERT_EQ( 1, employees[4]->getWorking().getSize() );
		ASSERT_NO_THROW( devService = co::cast<dom::IService>( employees[4]->getWorking()[0] ) );
		EXPECT_EQ( "Software1.0 Maintenance", devService->getName() );
		EXPECT_NEAR( 26315.79, devService->getMonthlyIncome(), 0.01 );

		devService->setMonthlyIncome( 60000 );
		spaceRestored->addChange( devService );
		spaceRestored->notifyChanges();
		ASSERT_NO_THROW( persister->save() );
	}
}