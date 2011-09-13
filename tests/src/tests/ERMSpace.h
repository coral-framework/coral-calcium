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
#include <ca/ISpaceChanges.h>
#include <ca/ISpaceObserver.h>
#include <ca/IObjectChanges.h>
#include <ca/IObjectObserver.h>
#include <ca/IServiceChanges.h>
#include <ca/IServiceObserver.h>
#include <ca/ChangedRefField.h>
#include <ca/ChangedRefVecField.h>
#include <ca/ChangedValueField.h>
#include <ca/ChangedConnection.h>
#include <ca/NoSuchObjectException.h>

#include <erm/IModel.h>
#include <erm/IEntity.h>
#include <erm/IRelationship.h>
#include <erm/Multiplicity.h>

class ERMSpace : public ::testing::Test,
	public ca::ISpaceObserver,
	public ca::IObjectObserver,
	public ca::IServiceObserver
{
public:
	virtual ~ERMSpace() {;}

	void onSpaceChanged( ca::ISpaceChanges* changes );
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

	co::RefPtr<co::IObject> _modelObj;
	co::RefPtr<co::IObject> _universeObj;
	co::RefPtr<co::IObject> _spaceObj;

	co::RefPtr<ca::IModel> _model;
	co::RefPtr<ca::IUniverse> _universe;
	co::RefPtr<ca::ISpace> _space;

	co::RefPtr<ca::ISpaceChanges> _changes;
	co::RefVector<ca::IObjectChanges> _objectChanges;
	co::RefVector<ca::IServiceChanges> _serviceChanges;

	co::RefPtr<erm::IEntity> _entityA;
	co::RefPtr<erm::IEntity> _entityB;
	co::RefPtr<erm::IEntity> _entityC;
	co::RefPtr<erm::IRelationship> _relAB;
	co::RefPtr<erm::IRelationship> _relBC;
	co::RefPtr<erm::IRelationship> _relCA;
	co::RefPtr<erm::IModel> _erm;
};

#endif // _ERMSPACE_H_
