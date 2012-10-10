/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_GRAPHCHANGES_H_
#define _CA_GRAPHCHANGES_H_

#include "Model.h"
#include "ObjectChanges.h"
#include "GraphChanges_Base.h"
#include <co/RefVector.h>
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
	co::Range<co::IObject*> getAddedObjects();	
	co::Range<co::IObject*> getRemovedObjects();
	co::Range<ca::IObjectChanges*> getChangedObjects();
	co::int32 findAddedObject( co::IObject* object );
	co::int32 findRemovedObject( co::IObject* object );
	co::int32 findChangedObject( co::IObject* object );
	void revertChanges();

private:
	GraphChanges( ca::IGraph* graph, GraphChanges& other );

private:
	co::RefPtr<ca::IGraph> _graph;
	co::RefVector<co::IObject> _addedObjects;
	co::RefVector<co::IObject> _removedObjects;
	co::RefVector<ca::IObjectChanges> _changedObjects;
};

} // namespace ca

#endif
