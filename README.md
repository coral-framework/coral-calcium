Calcium - Domain Model Framework
================================

Calcium is a versatile and minimally intrusive domain object model framework.

Architecture at a Glance
------------------------

- **Objects** contain a set of _fields_ partitioned across a set of interfaces.
- **Object Graphs** are directed acyclic graphs (DAGs), where the nodes are objects and the edges are interface-type fields.
- **Models** are object graph schemas, defined through a set of types and a list of fields for each type.
- **Universes** are a unified context for objects under the same model.
- **Spaces** are subsets of a universe. Each space has its own set of object graphs, which may include unique objects as well as objects shared with other spaces in the same universe.

Features
--------

* Models are defined using a Lua-based DSL.
* Models are _decentralized_, so several modules may contribute to their definition.
* Spaces track all changes made to their graphs and offer a very flexible change notification system.
* Support for multiple independent universes.
* Support for persistent spaces.
* Support for undoing/redoing changes to a space.
* Basis for a distributed shared object system.
