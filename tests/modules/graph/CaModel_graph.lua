-- CaModel where Nodes have only a single ref to another node

Type "graph.Node"
{
	node = "graph.INode"
}

Type "graph.INode"
{
	refs = "graph.INode[]",
}
