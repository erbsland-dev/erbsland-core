**********************************
Implementations in the Wrong Units
**********************************

:Rule ID: ``implementation_in_wrong_unit``
:Severity: high

A class member must be implemented in the source unit owned by that class.
Keeping implementations with their declaration makes the unit predictable to navigate and prevents unrelated source
files from accumulating hidden responsibilities.

Correct Placement
=================

Implement members of ``Foo`` in ``Foo.cpp``.
If the implementation is split by responsibility, use ``Foo_<part>.cpp``, such as ``Foo_parsing.cpp``.
For a nested type such as ``Foo::State``, ``Foo`` remains the primary owning unit.

Exceptions
==========

Files whose names contain ``Windows`` or ``Posix`` may implement a platform-neutral base-class factory alongside the
platform implementation.
Demos and unit tests may organize implementations around their complete examples or fixtures.

Mechanical Detection
====================

The scanner detects namespace-scope member-function definitions in ``cpp`` files whose owner does not match the source
unit.
