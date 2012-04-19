/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "UndoManager_Base.h"

#include <co/RefVector.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>

#include <ca/IGraph.h>
#include <ca/IGraphChanges.h>
#include <ca/NoChangeException.h>

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
		_nestingLevel = 0;
		_targetStack = Stack_Invalid;
	}

	virtual ~UndoManager()
	{
		// still recording a changeset?
		assert( _nestingLevel == 0 );

		if( _graph.isValid() )
			_graph->removeGraphObserver( this );
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

	co::Range<std::string const> getUndoStack()
	{
		return _descriptions[Stack_Undo];
	}

	co::Range<std::string const> getRedoStack()
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

		checkHasGraph();

		// separate pre-existing changes from the new ones
		_graph->notifyChanges();

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
		clearStack( Stack_Redo );
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
		checkHasGraph();
		checkNotRecording();
		clearStack( Stack_Undo );
		clearStack( Stack_Redo );
	}

	// ------ ca.IGraphObserver Methods ------ //

	void onGraphChanged( ca::IGraphChanges* changes )
	{
		assert( changes && _graph.get() == changes->getGraph() );

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
	ca::IGraph* getGraphService()
	{
		return _graph.get();
	}

	void setGraphService( ca::IGraph* graph )
	{
		if( _graph.isValid() )
			throw co::IllegalStateException( "once set, the graph of a ca.UndoManager cannot be changed" );

		if( !graph )
			throw co::IllegalArgumentException( "illegal null graph" );

		graph->addGraphObserver( this );
		_graph = graph;
	}

private:
	inline void checkHasGraph()
	{
		if( !_graph )
			throw co::IllegalStateException( "the ca.UndoManager requires a graph for this operation" );
	}

	inline void checkNotRecording()
	{
		if( _nestingLevel )
			throw co::IllegalStateException( "operation forbidden while recording a changeset" );
	}

	void clearStack( Stack s )
	{
		_changes[s].clear();
		_descriptions[s].clear();
	}

	void recordChanges( Stack s )
	{
		_targetStack = s;
		_graph->notifyChanges();
		if( _targetStack != Stack_Invalid )
		{
			_targetStack = Stack_Invalid;
			throw NoChangeException( "no change detected" );
		}
	}

	void revertChanges( Stack s )
	{
		assert( s < Stack_Invalid );

		checkHasGraph();
		checkNotRecording();

		if( _changes[s].empty() )
			throw NoChangeException( s == Stack_Undo ? "nothing to undo" : "nothing to redo" );

		// separate pre-existing changes from the new ones
		_graph->notifyChanges();

		// revert and discard the changes
		_description = _descriptions[s].back();
		_descriptions[s].pop_back();

		_changes[s].back()->revertChanges();
		_changes[s].pop_back();

		// record the "reverse" changes on the "other" stack
		recordChanges( s == Stack_Undo ? Stack_Redo : Stack_Undo );
	}

private:
	co::RefPtr<ca::IGraph> _graph;

	// changeset recording:
	int _nestingLevel;
	Stack _targetStack;
	std::string _description;

	typedef std::vector<std::string> StringStack;
	typedef co::RefVector<IGraphChanges> ChangeStack;

	// undo / redo stacks:
	ChangeStack _changes[Stack_Count];
	StringStack _descriptions[Stack_Count];
};

CORAL_EXPORT_COMPONENT( UndoManager, UndoManager );

} // namespace ca
