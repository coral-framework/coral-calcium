/*
 * Graph - Axuliary Calcium Test Module
 */

#include "Node_Base.h"
#include <graph/IDestructionObserver.h>

namespace graph {

class Node : public Node_Base
{
public:
	Node() : _destObs( nullptr )
	{
		// empty
	}

	virtual ~Node()
	{
		if( _destObs )
			_destObs->onDestruction( this );
	}

	co::TSlice<INode*> getRefs()
	{
		return _refs;
	}

	void setRefs( co::Slice<INode*> refs )
	{
		co::assign( refs, _refs );
	}

protected:
	graph::IDestructionObserver* getDestObsService()
	{
		return _destObs;
	}
	
	void setDestObsService( graph::IDestructionObserver* destObs )
	{
		_destObs = destObs;
	}

private:
	std::vector<INodeRef> _refs;
	IDestructionObserver* _destObs;
};
	
CORAL_EXPORT_COMPONENT( Node, Node )

} // namespace graph
