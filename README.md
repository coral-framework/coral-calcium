Calcium - Domain Model Framework
================================

Calcium is a minimally intrusive object graph and persistence framework.

Architecture at a Glance
------------------------

- **Objects** contain a set of _fields_ arranged in a set of _services_.
- **Graphs** are directed acyclic graphs (DAGs), where the nodes are objects and the edges are interface-type fields.
- **Models** are object graph schemas, defined through a set of Coral types and a list of fields for each type.
- **Universes** are a unified context for shared object graphs under the same model.
- **Spaces** are subsets of a universe. Each space contains a connected graph that spans from a single _root object_, and may include unique objects as well as objects shared with other spaces in the same universe.

Features
--------

* Non-intrusive tracking of changes in object graphs.
* Powerful change notification system.
* Seamless integration with Lua.
* Persistent object graphs.
	* Data format agnostic, with built-in support for SQLite.
	* Model evolution and data migration engine for backwards compatibility.
* Support for undoing/redoing changes to a graph.
* Models are defined using a Lua-based DSL.
* Models are _decentralized_ so any module may extend it.
* Support for multiple independent universes and models.
* Basis for a distributed shared object system.
