***********************************************
Classes and Structs in the Wrong Files or Units
***********************************************

:Rule ID: ``type_in_wrong_unit``
:Severity: high

Classes and structs must be declared in accessible headers.
A type defined only in a ``cpp`` file has no independently accessible API and cannot be tested in isolation.

Correct Placement
=================

Declare every class and struct in an ``hpp`` file.
This rule applies to namespace-scope implementation-only helper types.
Move an implementation-only helper into its own header below an ``impl`` directory and declare it in the matching
``impl`` namespace.

An inline type declared in a class, struct, or function belongs to its enclosing declaration and is not a separate
placement violation.
The size and complexity of a type nested in an owning class or struct are governed by
:doc:`oversized_nested_type`.
An out-of-line nested-type definition in a ``cpp`` file, such as ``struct Owner::State``, is still a violation.

The relationship between a namespace and its directory is covered by :doc:`namespace_in_wrong_unit`.
The relationship between a type name and its filename is a separate concern covered by
:doc:`multiple_types_in_header` and the :doc:`file-structure guidelines <../cpp_files>`.
The prohibition against hiding helpers in implementation files is also related to :doc:`anonymous_namespace`.

Exceptions
==========

Demos and unit tests may keep example-local or test-local types beside the code that uses them.

Mechanical Detection
====================

The scanner reports namespace-scope class and struct definitions in ``cpp`` files.
