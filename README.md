Calcium - Object Model Framework
================================

Calcium is a flexible and minimally intrusive domain object model framework.

Architecture at a Glance
------------------------

- **Objects** contain a set of _attributes_ partitioned across a set of interfaces.
- **Object Models** are object graph schemas, defined through a set of types and a list of attributes for each type.
- **Object Graphs** are directed acyclic graphs (DAGs), where the nodes are component instances and the edges are interface-type attributes.
- **Object Universes** are a unified context for objects under the same object model.
- **Object Spaces** are subsets of an object universe. Each space has its own set of object graphs, which may include unique objects as well as objects shared with other spaces in the same universe.

Features
--------

* Object models are described in a Lua-based DSL.
* Object models are _decentralized_, so several modules may contribute to their description.
* Object spaces track all changes made to their graphs and offer a very flexible change notification system.
* Support for multiple independent object universes.
* Support for persistent object spaces.
* Support for undoing/redoing changes made to an object space.
* Can be used as basis for a distributed object system.
