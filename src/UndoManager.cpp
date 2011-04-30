/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "UndoManager_Base.h"

#include <ca/ISpaceChanges.h>
#include <ca/NoChangeException.h>

#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>

namespace ca {

class UndoManager : public UndoManager_Base
{
public:
	enum Stack
	{
		Stack_Undo,
		Stack_Redo,
		Stack_Invalid,
		Stack_Count = Stack_Invalid
	};

	UndoManager()
	{
		_space = NULL;
		_nestingLevel = 0;
		_targetStack = Stack_Invalid;
	}

	virtual ~UndoManager()
	{
		// still recording a changeset?
		assert( _nestingLevel == 0 );
		_space = NULL;
	}

	// ------ ca.IUndoManager Methods ------ //

	bool getCanUndo()
	{
		return !_descriptions[Stack_Undo].empty();
	}

	bool getCanRedo()
	{
		return !_descriptions[Stack_Redo].empty();
	}

	co::Range<std::string const> getUndoList()
	{
		return _descriptions[Stack_Undo];
	}

	co::Range<std::string const> getRedoList()
	{
		return _descriptions[Stack_Redo];
	}

	void beginChange( const std::string& description )
	{
		if( _nestingLevel )
		{
			// nested calls are ignored
			++_nestingLevel;
			return;
		}

		checkHasSpace();

		// separate pre-existing changes from the new ones
		_space->notifyChanges();

		// start tracking changes
		++_nestingLevel;
		_description = description;
	}

	void endChange()
	{
		if( _nestingLevel < 1 )
			throw co::IllegalStateException( "not currently recording a changeset" );

		if( --_nestingLevel > 0 )
			return; // nested calls are ignored

		recordChanges( Stack_Undo );
	}

	void undo()
	{
		revertChanges( Stack_Undo );
	}

	void redo()
	{
		revertChanges( Stack_Redo );
	}

	void clear()
	{
		checkHasSpace();
		checkNotRecording();

		for( int s = Stack_Undo; s < Stack_Count; ++s )
		{
			_changes[s].clear();
			_descriptions[s].clear();
		}
	}

	// ------ ca.ISpaceObserver Methods ------ //

	void onSpaceChanged( ca::ISpaceChanges* changes )
	{
		assert( changes && changes->getSpace() == _space );

		// ignore changes if we're not tracking them
		if( _targetStack == Stack_Invalid )
		{
			if( !_nestingLevel ) return;
			throw co::IllegalStateException( "unexpected change notification" );
		}

		/*
			Once we start tracking changes, we expect only one notification,
			which should happen from within the top-level endChange() call.
		 */
		Stack s = _targetStack;
		_targetStack = Stack_Invalid;

		/*
			Changes must contain 'changedObjects', or we'd have nothing to undo.
			A changeset with no changes means a root object was added/removed.
			Root objects should be 'static' with regard to change tracking.
		 */
		if( changes->getChangedObjects().isEmpty() )
			throw NoChangeException( "changes contain no changed object" );

		_changes[s].push_back( changes );
		_descriptions[s].push_back( _description );
	}

protected:
	// ------ Receptacle 'space' (ca.ISpace) ------ //

	ca::ISpace* getSpaceService()
	{
		return _space;
	}

	void setSpaceService( ca::ISpace* space )
	{
		if( _space )
			throw co::IllegalStateException( "once set, the space of a ca.UndoManager cannot be changed" );

		if( !space )
			throw co::IllegalArgumentException( "illegal null space" );

		_space = space;
		_space->addSpaceObserver( this );
	}

private:
	inline void checkHasSpace()
	{
		if( !_space )
			throw co::IllegalStateException( "the ca.UndoManager requires a space for this operation" );
	}

	inline void checkNotRecording()
	{
		if( _nestingLevel )
			throw co::IllegalStateException( "operation forbidden while recording a changeset" );
	}

	void recordChanges( Stack s )
	{
		_targetStack = s;
		_space->notifyChanges();
		if( _targetStack != Stack_Invalid )
		{
			_targetStack = Stack_Invalid;
			throw NoChangeException( "no change detected" );
		}
	}

	void revertChanges( Stack s )
	{
		assert( s < Stack_Invalid );

		checkHasSpace();
		checkNotRecording();

		if( _changes[s].empty() )
			throw NoChangeException( s == Stack_Undo ? "nothing to undo" : "nothing to redo" );

		// separate pre-existing changes from the new ones
		_space->notifyChanges();

		// revert and discard the changes
		_description = _descriptions[s].back();
		_descriptions[s].pop_back();

		_changes[s].back()->revertChanges();
		_changes[s].pop_back();

		// record the "reverse" changes on the "other" stack
		recordChanges( s == Stack_Undo ? Stack_Redo : Stack_Undo );
	}

private:
	ca::ISpace* _space;

	// changeset recording:
	int _nestingLevel;
	Stack _targetStack;
	std::string _description;

	typedef std::vector<std::string> StringStack;
	typedef co::RefVector<ISpaceChanges> ChangeStack;

	// undo / redo stacks:
	ChangeStack _changes[Stack_Count];
	StringStack _descriptions[Stack_Count];
};

CORAL_EXPORT_COMPONENT( UndoManager, UndoManager );

} // namespace ca
