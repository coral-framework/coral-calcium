/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_GRAPHCHANGES_H_
#define _CA_GRAPHCHANGES_H_

#include "Model.h"
#include "ObjectChanges.h"
#include "GraphChanges_Base.h"
#include <ca/IGraph.h>

namespace ca {

class GraphChanges : public GraphChanges_Base
{
public:
	GraphChanges();
	virtual ~GraphChanges();

	// ------ Internal Methods ------ //

	void addAddedObject( ObjectRecord* object )
	{
		_addedObjects.push_back( object->instance );
	}

	void addRemovedObject( ObjectRecord* object )
	{
		_removedObjects.push_back( object->instance );
	}

	void addChangedObject( ObjectChanges* objectChanges )
	{
		_changedObjects.push_back( objectChanges );
	}

	/*
		Prepares this changeset for dissemination. This creates an immutable
		clone of this object, with 'space' set, and then resets this object.
	 */
	IGraphChanges* finalize( ca::IGraph* graph );

	// ------ ca.IGraphChanges Methods ------ //

	ca::IGraph* getGraph();
	co::TSlice<co::IObject*> getAddedObjects();	
	co::TSlice<co::IObject*> getRemovedObjects();
	co::TSlice<ca::IObjectChanges*> getChangedObjects();
	co::int32 findAddedObject( co::IObject* object );
	co::int32 findRemovedObject( co::IObject* object );
	co::int32 findChangedObject( co::IObject* object );
	void revertChanges();

private:
	GraphChanges( ca::IGraph* graph, GraphChanges& other );

private:
	ca::IGraphRef _graph;
	std::vector<co::IObjectRef> _addedObjects;
	std::vector<co::IObjectRef> _removedObjects;
	std::vector<ca::IObjectChangesRef> _changedObjects;
};

} // namespace ca

#endif
