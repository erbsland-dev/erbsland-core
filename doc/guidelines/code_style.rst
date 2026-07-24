***********************
Erbsland C++ Code Style
***********************

This document defines the portable code-style baseline for Erbsland C++ libraries and applications.
Project-specific guidelines may refine or override semantic rules for their domain.
The project's ``.clang-format`` file is authoritative for mechanical formatting.

In this document, **must** marks a requirement, **should** marks the expected default unless there is a concrete reason
to deviate, and **may** marks an optional choice.

Formatting
==========

Format every changed C++ file.
Accept ``.clang-format`` decisions for indentation, line length, braces, wrapping, spacing, and includes.

Formatting that requires manual attention:

* Use empty lines to separate logical blocks.
* Do not add empty lines between adjacent documented declarations or inline definitions in a header.
* Use one empty line around namespace-scope class, struct, enum, and function definitions.
* Run the project's validation command after making changes, if one exists.

Naming
======

* Types and public type aliases use ``PascalCase``; methods, free functions, local variables, and parameters use
  ``camelCase``; member variables use ``_camelCase``.
* Namespace-scope and static constants use ``cCamelCase``.
  Enum-like static constants may use ``PascalCase``, as in ``Color::Red``.
* A single straightforward template parameter may use ``T``; multiple or descriptive parameters use ``tCamelCase``.
* Namespaces use lowercase nested names, such as ``erbsland::unittest``.
* Preprocessor macros use ``UPPER_CASE`` and an ``ERBSLAND_<LIBRARY>_`` prefix in Erbsland libraries and applications.
  Do not use macros for constants.
* Treat initialisms as words in identifiers, such as ``HttpServer`` and ``parseUtf8``.
  Keep the documented spelling of domain-specific abbreviations.

Files and Includes
==================

* Begin each C++ file with the project's two-line copyright block.
  Put ``#pragma once`` directly after it in headers.
* Use ``.hpp`` for headers, ``.cpp`` for implementations, and ``.tpp`` for extracted templates.
* Each ``hpp/cpp`` module should have one primary class, struct, enum, alias, or logical method collection.
  Closely related implementation helpers may share a module.
* Match the primary type and filename, and mirror namespaces in the source directory structure.
  Directories may subdivide a large namespace without adding another namespace.
* Put private implementation details in an ``impl`` directory and matching ``impl`` namespace.
* Directly include every declaration a file uses; do not rely on unrelated transitive includes.
  A ``cpp`` file includes its corresponding header first.
* Split implementations beyond roughly 500 lines by logical responsibility.
  Name parts ``Class_part.cpp``, ``Class_part.hpp``, or ``Class_part.tpp``.
  Include ``tpp`` parts at the bottom of the owning header, without an include back to that header.
* Do not edit generated files directly.
  Modify their source or generator and regenerate them.

CMake
=====

* Use one ``CMakeLists.txt`` in each directory that contains source files.
* Add only local files with one flat ``target_sources(<target> PRIVATE ...)`` block.
  Add nested directories with ``add_subdirectory(...)`` before ``target_sources``.
* Sort files and subdirectories alphabetically.

Comments and API Documentation
==============================

Comment Format
--------------

* Use ``///`` and Doxygen ``@`` commands for API documentation, without empty comment lines.
* Use ``//`` for short implementation notes.
  Use ``/* ... */`` only when an inline annotation or generated layout makes it clearer than a line comment.
* ``@seedoc{/path}`` links to a documentation page, and ``@seeref{id}`` links to a reference target.
* ``@wip`` marks work in progress; ask the project owner before modifying the marked API.

Required Documentation
----------------------

* In public and internal APIs, document every class, struct, enum, public type alias, public constant, namespace-scope
  function, and public method.
* Start with one brief line and document every parameter, non-void return value, thrown exception, and relevant edge or
  error case.
* Move extensive explanations to linked reference or topic documentation.
* Give every data member and enum member a brief trailing ``///<`` description.
* Group undocumented, explicitly defaulted or deleted special members under ``// defaults`` or
  ``// defaults/deletions``.
* A trivial getter or setter needs only a one-line description without ``@param`` or ``@return``.

Test Status
-----------

End every documented class, struct, and namespace-scope function API block with exactly one test-status marker.
Do not mark constructors, methods, operators, or other members.

* ``@tested{ExampleTest OtherTest}`` lists one or more test-suite class names separated by spaces.
  Names must end in ``Test``; paths and method selectors are invalid.
* ``@notest{reason}`` explains in one line why a test is not applicable.
* ``@needtest{reason}`` identifies missing coverage in one line.

Class Organization
==================

Group a class with repeated access specifiers: one empty line before each section, none between its declarations, and an
optional lowercase ``//`` label.
Simple value structs and dependency constraints may require a smaller or different layout.

The usual section order is:

1.  Private friends, nested types, enums, and aliases in dependency order.
2.  Public types in dependency order.
3.  Default and other constructors, destructor, copy and move constructors, then copy and move assignment.
    Put explicitly defaulted or deleted members in a final defaults group.
4.  Main public operations.
5.  Overrides, using one ``public: // implement Base`` section per base.
6.  Operators: comparison, arithmetic, logical, then other operators.
7.  Accessors: condition tests first, then each attribute's accessors together.
8.  Other public tools, grouped only when this improves navigation.
9.  Conversions: ``to...`` methods followed by static ``from...`` and other factories.
10. Private and protected methods.
11. Data members, grouped by access.

Modern C++
==========

* Use portable C++20 features.
  Prefer concepts, structured bindings, designated initialization, and range-based loops when clearer.
* Use trailing return types for non-void functions where the syntax permits, such as
  ``auto create() -> std::string``.
  Use ``void function()`` for ordinary void functions and explicit ``-> void`` on non-generic lambdas.
  Use concrete return and parameter types for non-generic functions and lambdas.
* Use ``auto`` for values when the type is apparent or clearer; use ``const`` for immutable values and ``constexpr``
  when usable at compile time.
* Add ``[[nodiscard]]`` when silently discarding a result is likely to be a mistake.
  Add ``noexcept`` only when the operation is guaranteed not to propagate an exception.
* Mark intentionally unused named parameters ``[[maybe_unused]]``; omit an unused private overload-disambiguation tag's
  name.
* Mark overriding functions ``override`` and classes deliberately closed to extension ``final``.
* Express ownership explicitly.
  Use values or references by default, ``std::unique_ptr`` for unique ownership, ``std::shared_ptr`` only for shared
  ownership, raw pointers for deliberate non-owning or native boundaries, and ``std::optional`` for an absent value.
* Expose raw pointers through public APIs only at unavoidable interoperability boundaries.
  Use an explicitly named ``Unsafe...`` alias or wrapper and document ownership, lifetime, nullability, and mutability.
* Use lazy initialization for immutable or expensive data whose construction should be deferred.

Erbsland Core Integration
=========================

* Prefer ``String`` for read-only strings, ``""_el`` for literals, and ``StringFormat`` for formatting.
* Prefer ``StringEditor`` for construction, and ``AnyStringBuilder`` for width agnostic construction.
* Prefer regular UTF-8 types; use U8, U16, or U32 types when an API boundary or algorithm requires them.
* Prefer Erbsland Core types and algorithms before ``std::`` types and algorithms.
* In portable unit tests, do not construct UTF-8 text with ``\x??`` escapes because their interpretation differs
  between compilers.
  When using Erbsland Unit Test, use ``th::stdStringFromHex()`` when raw bytes are required.
