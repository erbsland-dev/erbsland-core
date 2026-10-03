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
* Renaming an anonymous namespace while leaving its helpers hidden in a ``cpp`` file does not provide an accessible
  header interface for unit tests.

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

Otherwise, create a new logical unit below an ``impl`` directory with a separate ``hpp`` and optional ``cpp`` file.
Place the free functions in ``erbsland::<domain>::impl`` or a named helper namespace beneath it.
The helper namespace may share its parent's directory when it owns one logical unit; follow :doc:`../cpp_files` for
directory layout.

This is often preferable even to private helper methods because the functions remain independently unit-testable.

**Implementation-only types (classes, enums, enum classes):**

Never place implementation-only types in anonymous namespaces.
Instead, create a dedicated logical unit below an ``impl`` directory with a ``hpp`` and optional ``cpp`` file, define
the type there, and include it where needed.

Mechanical Detection
====================

The scanner detects ``namespace {`` outside comments and literals.
