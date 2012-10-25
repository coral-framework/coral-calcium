/*
 * Calcium - domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "CompanySpace.h"
#include <co/Log.h>

void CompanySpace::SetUp()
{
	// create an object model
	_modelObj = co::newInstance( "ca.Model" );
	_model = _modelObj->getService<ca::IModel>();
	assert( _model.isValid() );

	_model->setName( "dom" );

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

	_space->initialize( createCompanyGraph() );
	_space->notifyChanges();

}

void CompanySpace::TearDown()
{
	//_space->removeSpaceObserver( this );

	_modelObj = NULL;
	_spaceObj = NULL;
	_universeObj = NULL;

	_model = NULL;
	_space = NULL;
	_universe = NULL;

	_manager1 = NULL;
	_manager2 = NULL;

	_developer1 = NULL;
	_developer2 = NULL;
	_developer3 = NULL;
	_developer4 = NULL;

	_software = NULL;
	_dataMaintain = NULL;
	
	_company = NULL;
}

co::IObject* CompanySpace::createCompanyGraph()
{
	//co::IType* devType = co::getType( "dom.Developer" );

	// create a simple object graph with 2 entities and 1 relationship
	_developer1 = co::newInstance( "dom.Developer" )->getService<dom::IDeveloper>();
	_developer1->setName( "John Cplusplus Experienced" );
	_developer1->setSalary( 5000 );
	
	_developer2 = co::newInstance( "dom.Developer" )->getService<dom::IDeveloper>();
	_developer2->setName( "Joseph Java Newbie" );
	_developer2->setSalary( 1000 );

	_developer3 = co::newInstance( "dom.Developer" )->getService<dom::IDeveloper>();
	_developer3->setName( "Michael CSharp Senior" );
	_developer3->setSalary( 4000 );

	_developer4 = co::newInstance( "dom.Developer" )->getService<dom::IDeveloper>();
	_developer4->setName( "Jacob Lua Junior" );
	_developer4->setSalary( 3000 );

	_manager1 = co::newInstance( "dom.Manager" )->getService<dom::IManager>();
	_manager1->setName( "Richard Scrum Master" );
	_manager1->setSalary( 10000 );

	std::vector<dom::IDeveloperRef> richardDevs;
	richardDevs.push_back( _developer2 );
	richardDevs.push_back( _developer3 );

	_manager1->setWorkers( richardDevs );

	_manager2 = co::newInstance( "dom.Manager" )->getService<dom::IManager>();
	_manager2->setName( "Wiliam Kanban Expert" );
	_manager2->setSalary( 9000 );

	_software = co::newInstance( "dom.Project" )->getService<dom::IProject>();
	_software->setName( "Software2.0" );
	_software->setEarnings( 1000000 );
	_software->setIsService( false );

	_developer2->setProject( _software.get() );
	_developer3->setProject( _software.get() );

	_manager1->setProject( _software.get() );
	_manager1->setWorkers( richardDevs );
	_software->setManager( _manager1.get() );
	_software->setDevelopers( richardDevs );

	_dataMaintain = co::newInstance( "dom.Project" )->getService<dom::IProject>();
	_dataMaintain->setName( "Software1.0 Maintenance" );
	_dataMaintain->setEarnings( 50000 );
	_dataMaintain->setIsService( true );

	std::vector<dom::IDeveloperRef> maintainers;
	maintainers.push_back( _developer1.get() );
	maintainers.push_back( _developer4.get() );

	_developer1->setProject( _dataMaintain.get() );
	_developer4->setProject( _dataMaintain.get() );

	_dataMaintain->setManager( _manager2.get() );
	_dataMaintain->setDevelopers( maintainers );
	_manager2->setWorkers( maintainers );

	_company = co::newInstance( "dom.Company" )->getService<dom::ICompany>();

	_company->addDeveloper( _developer1.get() );
	_company->addDeveloper( _developer2.get() );
	_company->addDeveloper( _developer3.get() );
	_company->addDeveloper( _developer4.get() );

	_company->addManager( _manager1.get() );
	_company->addManager( _manager2.get() );

	_company->addProject( _software.get() );
	_company->addProject( _dataMaintain.get() );

	return _company->getProvider();
}

