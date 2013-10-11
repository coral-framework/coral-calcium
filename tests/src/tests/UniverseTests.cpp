/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "TestUtils.h"

#include <co/Coral.h>
#include <co/IObject.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/IUniverse.h>
#include <ca/IGraphChanges.h>
#include <ca/IGraphObserver.h>

#include <graph/INode.h>
#include <graph/IDestructionObserver.h>

#include <set>

template<typename Itf>
class PseudoComponent : public Itf
{
public:
	co::IInterface* getInterface() { return co::typeOf<Itf>::get(); }
	co::IObject* getProvider() { return 0; }
	co::IPort* getFacet() { return 0; }
	void serviceRetain() {;}
	void serviceRelease() {;}
};

class GraphDestructionObserver :
	public PseudoComponent<ca::IGraphObserver>,
	public PseudoComponent<graph::IDestructionObserver>
{
public:
	size_t getNumObservedObjects() { return _observedSet.size(); }
	bool hasObserved( co::IObject* obj ) { return _observedSet.find( obj ) != _observedSet.end(); }

	size_t getNumDestroyedObjects() { return _destroyedSet.size(); }
	bool wasDestroyed( co::IObject* obj ) { return _destroyedSet.find( obj ) != _destroyedSet.end(); }

	void onGraphChanged( ca::IGraphChanges* changes )
	{
		co::TSlice<co::IObject*> addedObjects = changes->getAddedObjects();
		for( ; addedObjects; addedObjects.popFirst() )
		{
			co::IObject* obj = addedObjects.getFirst();
			if( hasObserved( obj ) )
				continue;
			obj->setService( "destObs", static_cast<graph::IDestructionObserver*>( this ) );
			_observedSet.insert( obj );
		}
	}

	void onDestruction( co::IObject* obj )
	{
		_destroyedSet.insert( obj );
	}

private:
	std::set<co::IObject*> _observedSet;
	std::set<co::IObject*> _destroyedSet;
};

class SpaceObjectSetObserver : public PseudoComponent<ca::IGraphObserver>
{
public:
	size_t getNumObjects() { return _objectSet.size(); }
	bool contains( co::IObject* obj ) { return _objectSet.find( obj ) != _objectSet.end(); }

	void onGraphChanged( ca::IGraphChanges* changes )
	{
		co::TSlice<co::IObject*> addedObjects = changes->getAddedObjects();
		for( ; addedObjects; addedObjects.popFirst() )
			_objectSet.insert( addedObjects.getFirst() );

		co::TSlice<co::IObject*> removedObjects = changes->getRemovedObjects();
		for( ; removedObjects; removedObjects.popFirst() )
			_objectSet.erase( removedObjects.getFirst() );
	}

private:
	std::set<co::IObject*> _objectSet;
};

class UniverseTests : public ::testing::Test
{
protected:
	void SetUp()
	{
		// create a calcium model
		co::IObjectRef modelObj = co::newInstance( "ca.Model" );
		_model = modelObj->getService<ca::IModel>();
		assert( _model.isValid() );

		_model->setName( "graph" );

		// create a universe and bind the model
		co::IObjectRef universeObj = co::newInstance( "ca.Universe" );
		_universe = universeObj->getService<ca::IUniverse>();
		assert( _universe.isValid() );

		universeObj->setService( "model", _model.get() );
		_universe->addGraphObserver( &_graphObserver );

		// create the 'R' (root) space and bind it to the universe
		co::IObjectRef spaceObj = co::newInstance( "ca.Space" );
		spaceObj->setService( "universe", _universe.get() );
		_spaceR = spaceObj->getService<ca::ISpace>();
		_spaceR->addGraphObserver( &_spaceRObserver );
		assert( _spaceR.isValid() );
		
		// create the 'A' space and bind it to the universe
		spaceObj = co::newInstance( "ca.Space" );
		spaceObj->setService( "universe", _universe.get() );
		_spaceA = spaceObj->getService<ca::ISpace>();
		_spaceA->addGraphObserver( &_spaceAObserver );
		assert( _spaceA.isValid() );

		// create the 'B' space and bind it to the universe
		spaceObj = co::newInstance( "ca.Space" );
		spaceObj->setService( "universe", _universe.get() );
		_spaceB = spaceObj->getService<ca::ISpace>();
		_spaceB->addGraphObserver( &_spaceBObserver );
		assert( _spaceB.isValid() );

		_nodeR = co::newInstance( "graph.Node" )->getService<graph::INode>();
		_nodeA = co::newInstance( "graph.Node" )->getService<graph::INode>();
		_nodeB = co::newInstance( "graph.Node" )->getService<graph::INode>();
		_nodeC = co::newInstance( "graph.Node" )->getService<graph::INode>();
		_nodeD = co::newInstance( "graph.Node" )->getService<graph::INode>();
	}

	void TearDown()
	{
		_universe->removeGraphObserver( &_graphObserver );
		if( _spaceR.isValid() ) _spaceR->removeGraphObserver( &_spaceRObserver );
		if( _spaceA.isValid() ) _spaceA->removeGraphObserver( &_spaceAObserver );
		if( _spaceB.isValid() ) _spaceB->removeGraphObserver( &_spaceBObserver );

		_model = NULL;
		_universe = NULL;
		_spaceR = NULL;
		_spaceA = NULL;
		_spaceB = NULL;

		_nodeR = NULL;
		_nodeA = NULL;
		_nodeB = NULL;
		_nodeC = NULL;
		_nodeD = NULL;
	}

protected:
	ca::IModelRef _model;
	ca::IUniverseRef _universe;
	ca::ISpaceRef _spaceR;
	ca::ISpaceRef _spaceA;
	ca::ISpaceRef _spaceB;

	GraphDestructionObserver _graphObserver;
	SpaceObjectSetObserver _spaceRObserver;
	SpaceObjectSetObserver _spaceAObserver;
	SpaceObjectSetObserver _spaceBObserver;

	graph::INodeRef _nodeR;
	graph::INodeRef _nodeA;
	graph::INodeRef _nodeB;
	graph::INodeRef _nodeC;
	graph::INodeRef _nodeD;
};

/*
	We test a universe with 3 spaces rooted at different branches of a DAG:

                       --> nodeA (spaceA) ------> nodeC
    nodeR (spaceR) ---|                      /
                       --> nodeB (spaceB) __/
											\---> nodeD
 
	Space R contains: R, A, B, C, D
	Space A contains: A, C
	Space B contains: B, C, D

	Our main goal is to test cross-space node sharing and reference counting.
 */
TEST_F( UniverseTests, multipleSpaces )
{
	graph::INode* nodesB[] = { _nodeB.get() };
	graph::INode* nodesAB[] = { _nodeA.get(), _nodeB.get() };
	graph::INode* nodesC[] = { _nodeC.get() };
	graph::INode* nodesCD[] = { _nodeC.get(), _nodeD.get() };

	_nodeR->setRefs( nodesAB );
	_nodeA->setRefs( nodesC );
	_nodeB->setRefs( nodesCD );

	co::IObject* objR = _nodeR->getProvider();
	co::IObject* objA = _nodeA->getProvider();
	co::IObject* objB = _nodeB->getProvider();
	co::IObject* objC = _nodeC->getProvider();
	co::IObject* objD = _nodeD->getProvider();

	_spaceR->initialize( objR );
	_spaceA->initialize( objA );
	_spaceB->initialize( objB );

	_spaceR->notifyChanges();
	_spaceA->notifyChanges();
	_spaceB->notifyChanges();

	// do the spaces contain the expected graphs?
	EXPECT_EQ( _spaceRObserver.getNumObjects(), 5 );
	EXPECT_TRUE( _spaceRObserver.contains( objR ) );
	EXPECT_TRUE( _spaceRObserver.contains( objA ) );
	EXPECT_TRUE( _spaceRObserver.contains( objB ) );
	EXPECT_TRUE( _spaceRObserver.contains( objC ) );
	EXPECT_TRUE( _spaceRObserver.contains( objD ) );

	EXPECT_EQ( _spaceAObserver.getNumObjects(), 2 );
	EXPECT_FALSE( _spaceAObserver.contains( objR ) );
	EXPECT_TRUE( _spaceAObserver.contains( objA ) );
	EXPECT_FALSE( _spaceAObserver.contains( objB ) );
	EXPECT_TRUE( _spaceAObserver.contains( objC ) );
	EXPECT_FALSE( _spaceAObserver.contains( objD ) );

	EXPECT_EQ( _spaceBObserver.getNumObjects(), 3 );
	EXPECT_FALSE( _spaceBObserver.contains( objR ) );
	EXPECT_FALSE( _spaceBObserver.contains( objA ) );
	EXPECT_TRUE( _spaceBObserver.contains( objB ) );
	EXPECT_TRUE( _spaceBObserver.contains( objC ) );
	EXPECT_TRUE( _spaceBObserver.contains( objD ) );

	// our graph destruction observer should also have all nodes, none destroyed
	EXPECT_EQ( _graphObserver.getNumObservedObjects(), 5 );
	EXPECT_EQ( _graphObserver.getNumDestroyedObjects(), 0 );

	// destroying the space A at first wont change anything
	_nodeA = nullptr;
	_spaceA->removeGraphObserver( &_spaceAObserver );
	_spaceA = nullptr;
	EXPECT_EQ( _graphObserver.getNumDestroyedObjects(), 0 );

	// but now if we remove reference R->A, node A should be destroyed
	_nodeR->setRefs( nodesB );
	_spaceR->addChange( _nodeR.get() );
	_spaceR->notifyChanges();
	EXPECT_EQ( _spaceRObserver.getNumObjects(), 4 );
	EXPECT_FALSE( _spaceRObserver.contains( objA ) );
	EXPECT_EQ( _graphObserver.getNumDestroyedObjects(), 1 );
	EXPECT_TRUE( _graphObserver.wasDestroyed( objA ) );

	// destroying space R while space B still exists should only destroy node R
	_nodeR = nullptr;
	_spaceR->removeGraphObserver( &_spaceRObserver );
	_spaceR = nullptr;
	// note: the universe will hold a ref to R until the next notification!
	EXPECT_EQ( _graphObserver.getNumDestroyedObjects(), 1 );
	_universe->notifyChanges(); // releases the universe's ref to R
	EXPECT_EQ( _graphObserver.getNumDestroyedObjects(), 2 );
	EXPECT_TRUE( _graphObserver.wasDestroyed( objR ) );

	// now by destroying space B, nodes B, C and D should be destroyed
	_nodeB = nullptr;
	_nodeC = nullptr;
	_nodeD = nullptr;
	_spaceB->removeGraphObserver( &_spaceBObserver );
	_spaceB = nullptr;
	// once again, the universe will hold refs until the next notification!
	EXPECT_EQ( _graphObserver.getNumDestroyedObjects(), 2 );
	_universe->notifyChanges(); // releases the universe's ref to B, C and D
	EXPECT_EQ( _graphObserver.getNumDestroyedObjects(), 5 );
	EXPECT_TRUE( _graphObserver.wasDestroyed( objB ) );
	EXPECT_TRUE( _graphObserver.wasDestroyed( objC ) );
	EXPECT_TRUE( _graphObserver.wasDestroyed( objD ) );
}
