/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ERMSpace.h"

#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>

#include <ca/IUndoManager.h>
#include <ca/NoChangeException.h>

class UndoRedoTests : public ERMSpace
{
public:
	virtual ~UndoRedoTests() {;}

protected:
	void SetUp()
	{
		ERMSpace::SetUp();

		// create an undoManager for our space
		_undoManagerObj = co::newInstance( "ca.UndoManager" );
		_undoManager = _undoManagerObj->getService<ca::IUndoManager>();
		assert( _undoManager.isValid() );

		_undoManagerObj->setService( "graph", _space.get() );
	}

	void TearDown()
	{				
		ERMSpace::TearDown();

		_undoManager = NULL;
		_undoManagerObj = NULL;
	}

protected:
	co::IObjectRef _undoManagerObj;
	ca::IUndoManagerRef _undoManager;
};


TEST_F( UndoRedoTests, setup )
{
	co::IObjectRef undoManagerObj = co::newInstance( "ca.UndoManager" );
	ca::IUndoManager* undoManager = undoManagerObj->getService<ca::IUndoManager>();
	ASSERT_TRUE( undoManager != NULL );

	// no space
	EXPECT_THROW( undoManager->undo(), co::IllegalStateException );

	// cannot bind a null space
	EXPECT_THROW( undoManagerObj->setService( "graph", NULL ), co::IllegalArgumentException );

	// bind the space
	undoManagerObj->setService( "graph", _space.get() );

	// cannot change the bound space
	EXPECT_THROW( undoManagerObj->setService( "graph", _space.get() ), co::IllegalStateException );

	// nothing to undo/redo
	EXPECT_THROW( undoManager->undo(), ca::NoChangeException );
	EXPECT_THROW( undoManager->redo(), ca::NoChangeException );

	// no currently recording a changeset
	EXPECT_THROW( undoManager->endChange(), co::IllegalStateException );

	EXPECT_NO_THROW( undoManager->clear() );
}

TEST_F( UndoRedoTests, basicUndoRedo )
{
	startWithSimpleERM();

	// perform a non-undoable change
	_relAB->setRelation( "111" );

	// check the graph's state
	EXPECT_EQ( "111", _relAB->getRelation() );
	EXPECT_EQ( "Entity A", _entityA->getName() );
	EXPECT_EQ( "Entity B", _entityB->getName() );
	EXPECT_EQ( NULL, _entityA->getParent() );

	// start an undoable change block
	_undoManager->beginChange( "Rename Entity A" );

	_entityA->setName( "AAA" );
	_space->addChange( _entityA.get() );

	// nested change block (is ignored/merged with the top-level block)
	{
		_undoManager->beginChange( "Set B as parent of A" );
		_entityA->setParent( _entityB.get() );
		_space->addChange( _entityA.get() );
		_undoManager->endChange();
	}

	// nothing to undo yet...
	EXPECT_FALSE( _undoManager->getCanUndo() );

	// end of the first undoable change block
	_undoManager->endChange();

	// check the undoManager's state
	{
		co::TSlice<std::string> undoStack = _undoManager->getUndoStack();
		co::TSlice<std::string> redoStack = _undoManager->getRedoStack();
		EXPECT_TRUE( _undoManager->getCanUndo() );
		EXPECT_FALSE( _undoManager->getCanRedo() );
		ASSERT_EQ( 1, undoStack.getSize() );
		EXPECT_EQ( "Rename Entity A", undoStack[0] );
		EXPECT_EQ( 0, redoStack.getSize() );

		// check the graph's state
		EXPECT_EQ( "111", _relAB->getRelation() );
		EXPECT_EQ( "AAA", _entityA->getName() );
		EXPECT_EQ( "Entity B", _entityB->getName() );
		EXPECT_EQ( _entityB.get(), _entityA->getParent() );

		// perform a second non-undoable change
		_relAB->setRelation( "222" );

		// perform a second undoable change
		_undoManager->beginChange( "Rename Entity B" );
		_entityB->setName( "BBB" );
		_space->addChange( _entityB.get() );
		_undoManager->endChange();
	}

	// check the undoManager's state
	{
		co::TSlice<std::string> undoStack = _undoManager->getUndoStack();
		co::TSlice<std::string> redoStack = _undoManager->getRedoStack();
		EXPECT_TRUE( _undoManager->getCanUndo() );
		EXPECT_FALSE( _undoManager->getCanRedo() );
		ASSERT_EQ( 2, undoStack.getSize() );
		EXPECT_EQ( "Rename Entity A", undoStack[0] );
		EXPECT_EQ( "Rename Entity B", undoStack[1] );
		EXPECT_EQ( 0, redoStack.getSize() );

		// check the graph's state
		EXPECT_EQ( "222", _relAB->getRelation() );
		EXPECT_EQ( "AAA", _entityA->getName() );
		EXPECT_EQ( "BBB", _entityB->getName() );
		EXPECT_EQ( _entityB.get(), _entityA->getParent() );

		// undo the last change
		_undoManager->undo();
	}

	// check the undoManager's state
	{
		co::TSlice<std::string> undoStack = _undoManager->getUndoStack();
		co::TSlice<std::string> redoStack = _undoManager->getRedoStack();
		EXPECT_EQ( true, _undoManager->getCanUndo() );
		EXPECT_EQ( true, _undoManager->getCanRedo() );	
		ASSERT_EQ( 1, undoStack.getSize() );
		EXPECT_EQ( "Rename Entity A", undoStack[0] );
		ASSERT_EQ( 1, redoStack.getSize() );
		EXPECT_EQ( "Rename Entity B", redoStack[0] );

		// check the graph's state
		EXPECT_EQ( "222", _relAB->getRelation() );
		EXPECT_EQ( "AAA", _entityA->getName() );
		EXPECT_EQ( "Entity B", _entityB->getName() );
		EXPECT_EQ( _entityB.get(), _entityA->getParent() );

		// modify a field that's just been 'undone'
		_entityB->setName( "FOO" );
		_space->addChange( _entityB.get() );

		// redo the last undone change
		_undoManager->redo();
	}

	// check the undoManager's state
	{
		co::TSlice<std::string> undoStack = _undoManager->getUndoStack();
		co::TSlice<std::string> redoStack = _undoManager->getRedoStack();
		EXPECT_TRUE( _undoManager->getCanUndo() );
		EXPECT_FALSE( _undoManager->getCanRedo() );	
		ASSERT_EQ( 2, undoStack.getSize() );
		EXPECT_EQ( "Rename Entity A", undoStack[0] );
		EXPECT_EQ( "Rename Entity B", undoStack[1] );
		ASSERT_EQ( 0, redoStack.getSize() );

		// check the graph's state
		EXPECT_EQ( "222", _relAB->getRelation() );
		EXPECT_EQ( "AAA", _entityA->getName() );
		EXPECT_EQ( "BBB", _entityB->getName() );
		EXPECT_EQ( _entityB.get(), _entityA->getParent() );

		// undo the change again...
		_undoManager->undo();
	}

	// check the undoManager's state
	{
		co::TSlice<std::string> undoStack = _undoManager->getUndoStack();
		co::TSlice<std::string> redoStack = _undoManager->getRedoStack();
		EXPECT_EQ( true, _undoManager->getCanUndo() );
		EXPECT_EQ( true, _undoManager->getCanRedo() );	
		ASSERT_EQ( 1, undoStack.getSize() );
		EXPECT_EQ( "Rename Entity A", undoStack[0] );
		ASSERT_EQ( 1, redoStack.getSize() );
		EXPECT_EQ( "Rename Entity B", redoStack[0] );

		// check the graph's state
		EXPECT_EQ( "222", _relAB->getRelation() );
		EXPECT_EQ( "AAA", _entityA->getName() );
		EXPECT_EQ( "FOO", _entityB->getName() );
		EXPECT_EQ( _entityB.get(), _entityA->getParent() );

		// undo the last remaining changeset
		_undoManager->undo();
	}

	// check the undoManager's state
	{
		co::TSlice<std::string> undoStack = _undoManager->getUndoStack();
		co::TSlice<std::string> redoStack = _undoManager->getRedoStack();
		EXPECT_FALSE( _undoManager->getCanUndo() );
		EXPECT_TRUE( _undoManager->getCanRedo() );	
		ASSERT_EQ( 0, undoStack.getSize() );
		ASSERT_EQ( 2, redoStack.getSize() );
		EXPECT_EQ( "Rename Entity B", redoStack[0] );
		EXPECT_EQ( "Rename Entity A", redoStack[1] );

		// check the graph's state
		EXPECT_EQ( "222", _relAB->getRelation() );
		EXPECT_EQ( "Entity A", _entityA->getName() );
		EXPECT_EQ( "FOO", _entityB->getName() );
		EXPECT_EQ( NULL, _entityA->getParent() );
	}
}

TEST_F( UndoRedoTests, newChangeResetsRedoStack )
{
	startWithSimpleERM();

	// perform an undoable change
	_undoManager->beginChange( "Rename Entity A" );
	_entityA->setName( "AAA" );
	_space->addChange( _entityA.get() );
	_undoManager->endChange();

	// check the undoManager's state
	co::TSlice<std::string> undoStack = _undoManager->getUndoStack();
	co::TSlice<std::string> redoStack = _undoManager->getRedoStack();
	EXPECT_TRUE( _undoManager->getCanUndo() );
	EXPECT_FALSE( _undoManager->getCanRedo() );
	ASSERT_EQ( 1, undoStack.getSize() );
	EXPECT_EQ( "Rename Entity A", undoStack[0] );
	EXPECT_EQ( 0, redoStack.getSize() );

	// undo the change and check the undoManager's state
	{
		_undoManager->undo();
		co::TSlice<std::string> undoStack = _undoManager->getUndoStack();
		co::TSlice<std::string> redoStack = _undoManager->getRedoStack();
		EXPECT_FALSE( _undoManager->getCanUndo() );
		EXPECT_TRUE( _undoManager->getCanRedo() );
		ASSERT_EQ( 0, undoStack.getSize() );
		EXPECT_EQ( 1, redoStack.getSize() );
		EXPECT_EQ( "Rename Entity A", redoStack[0] );

		// perform a second undoable change (resets the redo stack)
		_undoManager->beginChange( "Rename Entity B" );
		_entityB->setName( "BBB" );
		_space->addChange( _entityB.get() );
		_undoManager->endChange();
	}

	// check the undoManager's state
	{
		co::TSlice<std::string> undoStack = _undoManager->getUndoStack();
		co::TSlice<std::string> redoStack = _undoManager->getRedoStack();
		EXPECT_TRUE( _undoManager->getCanUndo() );
		EXPECT_FALSE( _undoManager->getCanRedo() );
		ASSERT_EQ( 1, undoStack.getSize() );
		EXPECT_EQ( "Rename Entity B", undoStack[0] );
		EXPECT_EQ( 0, redoStack.getSize() );
	}
}

TEST_F( UndoRedoTests, arrayTest )
{
	startWithSimpleERM();

	erm::IEntity* testEntity = co::newInstance( "erm.Entity" )->getService<erm::IEntity>();
	testEntity->setName( "Test Entity" );

	// remove the only relationship so we can add it again
	// this tests going from an empty array to a non-empty array (and undoing)
	_erm->removeRelationship( _relAB.get() );
	ASSERT_TRUE( _erm->getRelationships().isEmpty() );

	_undoManager->beginChange( "Add Dependency and Entity" );
	_erm->addEntity( testEntity );
	_erm->addRelationship( _relAB.get() );
	ASSERT_FALSE( _erm->getRelationships().isEmpty() );
	_space->addChange( _erm.get() );
	_undoManager->endChange();

	EXPECT_NO_THROW( _undoManager->undo() );
	EXPECT_NO_THROW( _undoManager->redo() );
	EXPECT_NO_THROW( _undoManager->undo() );
}
