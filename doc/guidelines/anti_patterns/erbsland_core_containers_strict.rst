*******************************
Erbsland Core Containers Strict
*******************************

:Rule ID: ``erbsland_core_containers_strict``
:Severity: medium

Use Erbsland Core's string containers when storing Core ``String`` values or using them as keys.
They provide the library's typed counts, indexes, string comparison, and list operations without repeated conversions at
call sites.

Correct Solution
================

* Replace ``std::vector<String>`` with ``StringList``. Use ``StringList::fromSplit`` for ordinary string splitting.
* Replace ``std::set<String>`` with ``StringSet``.
* Replace ``std::map<String, T>`` with ``StringMap<T>`` when ``T`` satisfies the map's value requirements.
* Follow changes to function parameters, return types, and callers. Use ``append``, ``count``, ``getRef``, ``set``,
  and the other Core container operations.

Common Issues and their Solutions
=================================

* **Can't write to container values through mutating references:** Wrap the type in a shared pointer like
  ``using FooPtr = std::shared_ptr<Foo>;`` the use it like ``StringMap<FooPtr>``.
* **Can't store types without default ctor:** Wrap the type in a shared pointer like
  ``using FooPtr = std::shared_ptr<Foo>;`` the use it like ``StringMap<FooPtr>``.

Scope and Exceptions
====================

A standard container can remain when its semantics are required.
Required means, the type is required to be used with an external or low-level API outside our control that
only takes a standard container as argument.

Mechanical Detection
====================

The scanner detects ``std::vector``, ``std::set``, and ``std::map`` declarations whose element or first key type is
``String``.
It does not inspect whether a custom comparator, allocator, or mutable standard-container interface is necessary;
inspect each finding before changing its type.
