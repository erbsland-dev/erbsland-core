********************
Anonymous Namespaces
********************

:Rule ID: ``anonymous_namespace``
:Severity: high

Anonymous namespaces may seem like a convenient place for helper functions or constants in implementation files,
avoiding linker conflicts.
In a large library like Erbsland Core, however, they are almost always bad practice.

Why We Do Not Tolerate Anonymous Namespaces
===========================================

* Functions in anonymous namespaces are **untestable** because they expose no accessible API for granular unit testing.
* Erbsland Core supports unity builds, where multiple implementation files are combined into a single compilation unit.
  Anonymous namespaces are merged as well, which can lead to unpredictable name conflicts.

Unacceptable Solutions
======================

* Only removing the anonymous namespace is no solution, because the helper or constant still exists, is still hidden in
  the implementation, and is still untestable.
* Converting the anonymous namespace into a real namespace with a unique name is no solution, because this is only an
  anonymous namespace in disguise.

Correct Solutions for Common Problems
=====================================

**Small free helper functions:**

Before writing a helper function, ask yourself:

* Does this functionality already exist in the library? If so, use the existing implementation.
* Does it operate on a library type? If the functionality is closely related to that type and could be useful elsewhere,
  extend the type's API instead. Prefer ``object.myAction()`` over ``myAction(object)``.
* Is it specific to the implementation of a public API? If so, a private or private static member function is often the
  best choice.

  Exceptions include:

  * Template-heavy implementations that would unnecessarily increase the include surface.
  * Platform-specific code.
  * Circular dependencies.

Otherwise, create a new compilation unit in an ``impl`` subdirectory with a separate ``hpp`` and optional ``cpp`` file.
Place the free functions in the ``erbsland::<domain>::impl`` namespace.

This is often preferable even to private helper methods because the functions remain independently unit-testable.

**Implementation-only types (classes, enums, enum classes):**

Never place implementation-only types in anonymous namespaces.
Instead, create a dedicated compilation unit in an ``impl`` subdirectory with a ``hpp`` and optional ``cpp`` file,
define the type there, and include it where needed.

Mechanical Detection
====================

The scanner detects ``namespace {`` outside comments and literals.
