.. index::
    single: Convenience Collections

***********************
Convenience Collections
***********************

Shared Empty Storage
====================

Default-constructed collections reuse one empty raw container for each eligible raw container type.
The first operation that requests mutable access detaches and creates private storage, leaving other empty collections
unchanged.

Lists always use shared empty storage.
Sets and maps use it when their comparison policy is stateless; hash sets and hash maps use it when both their hash and
equality policies are stateless.
A policy is considered stateless when ``std::is_empty_v`` is true.
Collections with stateful policies keep independently constructed empty storage so each policy instance retains its own
state.

Interface
=========

.. doxygenclass:: erbsland::util::HashMap
    :members:
.. doxygenclass:: erbsland::util::HashSet
    :members:
.. doxygenclass:: erbsland::util::List
    :members:
.. doxygenclass:: erbsland::util::Map
    :members:
.. doxygenclass:: erbsland::util::Set
    :members:
