/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _ERMSPACE_H_
#define _ERMSPACE_H_

#include "TestUtils.h"

#include <co/Coral.h>
#include <co/IField.h>
#include <co/IObject.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/IUniverse.h>
#include <ca/IGraphChanges.h>
#include <ca/IGraphObserver.h>
#include <ca/IObjectChanges.h>
#include <ca/IObjectObserver.h>
#include <ca/IServiceChanges.h>
#include <ca/IServiceObserver.h>
#include <ca/ChangedRefField.h>
#include <ca/ChangedRefVecField.h>
#include <ca/ChangedValueField.h>
#include <ca/ChangedConnection.h>

#include <erm/IModel.h>
#include <erm/IEntity.h>
#include <erm/IRelationship.h>
#include <erm/Multiplicity.h>

class ERMSpace : public ::testing::Test,
	public ca::IGraphObserver,
	public ca::IObjectObserver,
	public ca::IServiceObserver
{
public:
	virtual ~ERMSpace() {;}

	void onGraphChanged( ca::IGraphChanges* changes );
	void onObjectChanged( ca::IObjectChanges* changes );
	void onServiceChanged( ca::IServiceChanges* changes );

	co::IInterface* getInterface();
	co::IObject* getProvider();
	co::IPort* getFacet();
	void serviceRetain();
	void serviceRelease();

protected:
	virtual const char* getModelName();

	void SetUp();
	void TearDown();

	co::IObject* createSimpleERM();

	void startWithSimpleERM();

	void extendSimpleERM();

	void startWithExtendedERM();

protected:
	std::string _modelName;

	co::IObjectRef _modelObj;
	co::IObjectRef _universeObj;
	co::IObjectRef _spaceObj;

	ca::IModelRef _model;
	ca::IUniverseRef _universe;
	ca::ISpaceRef _space;

	ca::IGraphChangesRef _changes;
	std::vector<ca::IObjectChangesRef> _objectChanges;
	std::vector<ca::IServiceChangesRef> _serviceChanges;

	erm::IEntityRef _entityA;
	erm::IEntityRef _entityB;
	erm::IEntityRef _entityC;
	erm::IRelationshipRef _relAB;
	erm::IRelationshipRef _relBC;
	erm::IRelationshipRef _relCA;
	erm::IModelRef _erm;
};

#endif // _ERMSPACE_H_
