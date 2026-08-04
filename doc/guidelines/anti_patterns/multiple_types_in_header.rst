***********************************************
Multiple Classes, Structs, or Enums in a Header
***********************************************

:Rule ID: ``multiple_types_in_header``
:Severity: medium

Our coding guidelines have a strict relationship between type and file name.
A library user expects to include ``<Name>.hpp`` to use the type ``Name``.

Scope and Exceptions
====================

* **Public API**: Each class or enum must have its own header unless it belongs to one of the logical groups described
  below.
* **Internal API**: Each class that contains functions must have its own header and optional implementation file. Enum
  classes used outside the compilation unit must have their own header. Small functionless helper classes with a
  declaration size of less than 12 lines and used only in the compilation unit can share its header.

The following declarations count as one logical header type:

* A primary class template and its explicit or partial specializations, provided they have the same name and namespace.
* Multiple thematically related trait types in a header ending in ``Traits.hpp``.
* A coherent collection of micro-types in a header ending in ``Types.hpp``. Typical micro-types are aliases, concepts,
  or lightweight shells derived from one detailed base and adding only one or two overrides.

Supported ``std::hash`` and ``std::formatter`` specializations can stay with the Erbsland type they extend.

Correct Solution
================

Move each type into its own header and correct subdirectory.

For example, ``erbsland::example::ExampleType``:

* **requires** ``src/erbsland/example/ExampleType.hpp``;
* **should** have ``src/erbsland/example/ExampleType.cpp`` if it has function implementations exceeding five lines;
* **can** have ``src/erbsland/example/ExampleType_fwd.hpp`` if other code requires a forward declaration; and
* **requires** ``src/erbsland/example/ExampleType.tpp`` if the type or header contains complex template function
  implementations exceeding five lines.

Mechanical Detection
====================

The scanner reports multiple namespace-scope type definitions in one header, except for configured and documented
exceptions.
