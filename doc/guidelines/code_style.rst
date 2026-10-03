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
* Bodies of ``if``, ``else``, ``while``, ``for``, and ``do`` statements **must** be enclosed in ``{}``.
* There **must** be empty lines around namespace opening and closing lines, between function definitions, and after
  ``#pragma once``.
* Data blocks **should** use manual formatting when this improves readability.
  Surround such blocks with ``// clang-format off`` and ``// clang-format on``.
* Classes **must** use logically ordered access sections as described under `Class Organization`_.
* Run the project's validation command after making changes, if one exists.

Naming
======

* Types and public type aliases use ``PascalCase``; methods, free functions, local variables, and parameters use
  ``camelCase``; member variables use ``_camelCase``.
* Namespace-scope and static constants use ``cCamelCase``.
  Enum-like static constants may use ``PascalCase``, as in ``Color::Red``.
* A single straightforward template parameter may use ``T``; multiple or descriptive parameters use ``tCamelCase``.
* Namespaces use lowercase nested names, such as ``erbsland::unittest``.
* Preprocessor macros use ``UPPER_CASE`` and an ``ERBSLAND_<LIBRARY>_`` prefix in Erbsland Core libraries and applications.
  Do not use macros for constants.
* Treat initialisms as words in identifiers, such as ``HttpServer`` and ``parseUtf8``.
  Keep the documented spelling of domain-specific abbreviations.

Files and Includes
==================

* Begin each C++ file with the project's two-line copyright block.
  Spell the copyright marker ``(c)`` in project headers and generator defaults.
  Put ``#pragma once`` directly after it in headers.
* Use ``.hpp`` for headers, ``.cpp`` for implementations, and ``.tpp`` for extracted templates.
* Each ``hpp/cpp`` module **must** have only one primary class, struct, enum, alias, or logical free-function collection.
  Type traits may share one header, and hash or format helpers may follow a class or struct.
  Place implementation helpers in individual modules in an ``impl`` namespace.
* Match the primary type and filename, and mirror domain namespaces and the ``impl`` boundary in the source directories.
  Directories may subdivide a large namespace without adding another namespace.
  A helper namespace within ``impl`` does not require its own directory when it owns one logical unit.
* Put private implementation details in an ``impl`` directory and matching ``impl`` namespace.
* Directly include every declaration a file uses; do not rely on unrelated transitive includes.
  A ``cpp`` file includes its corresponding header first.
* Split implementations beyond roughly 500 code lines, excluding comments, by logical responsibility.
  Name parts ``Class_part.cpp``, ``Class_part.hpp``, or ``Class_part.tpp``.
  Include ``tpp`` parts at the bottom of the owning header, without an include back to that header.
* Do not edit generated files directly.
  Modify their source or generator and regenerate them.

Erbsland Core's additional file-structure rules, module exceptions, and stricter file-size limit are defined in
:doc:`cpp_files`.

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
* Document class members at their declarations inside the class.
  Separate out-of-class definitions in headers or template implementation files must not repeat API documentation.
* Use ``//`` for short implementation notes.
* Do **not** use ``/* ... */``, except when required for generated code.
* ``@seedoc{/path}`` links to a documentation page, and ``@seeref{id}`` links to a reference target.
* ``@wip`` marks work in progress; ask the project owner before modifying the marked API.

Required Documentation
----------------------

* In public and internal APIs, you **must** document every class, struct, enum, public type alias, public constant,
  namespace-scope function, and public method, except comparison operators and methods or operators that are overridden,
  explicitly defaulted, or deleted. Exceptions are also ``main`` and ``wmain`` functions.
* Every required API documentation block **must** contain:

  * One brief opening line.
  * A description of every parameter (``@param``) and template parameter (``@tparam``), except for trivial setters.
  * A description of the non-void return value (``@return``), except for getters and fluent methods.
  * A description of *concrete* runtime exceptions (``@throws``), excluding logic errors (classes ending in
    ``...Failure``).

* For non-trivial methods, you **should** also document applicable parameter ranges, thread safety, limits, edge cases,
  error conditions, and side effects.
* Move extensive explanations to linked reference or topic documentation.
* Give every namespace-scope or static class constant a one-line ``///`` description immediately before its declaration.
  Explain its purpose at the use site: which option it identifies, which setting it controls, or how its value is used.
  Generic descriptions such as "Configuration key" or comments that only repeat the identifier are insufficient.
* Give every instance data member and enum member a brief trailing ``///<`` description.
  If this would make the line too long, place a regular ``///`` description immediately before the member instead.

Cryptographic Implementations
-----------------------------

Cryptographic code must be written for transparent security review as well as functional correctness.

* Keep the implementation in the same logical order as the defining specification wherever practical.
  Prefer direct, readable transformations over compact or clever formulations.
* API documentation for an algorithm implementation must name the governing specification and the relevant section.
* Add inline comments before each substantive algorithm step that identify the corresponding specification section and
  relate the specification's notation or formula to the variables and operations in the code.
* Document security-relevant bounds, representation choices, precomputations, and deviations from optional parts of the
  specification where they are enforced.
* Keep secret-state lifetime and erasure behavior visible at the point where secret intermediates are created,
  transferred, or released.

Test Status
-----------

Every documented class, struct, free function and namespace-scope function API block **must** end with exactly one
test-status marker.
This applies to both public and internal library APIs, unless they are part of one of the following exceptions.

**Don't** mark inline types, constructors, methods, operators, or other members.
**Don't** mark unit tests.
**Don't** mark demos and examples.
This is reader-facing teaching material and would distract the reader.
**Only mark** elements in build and maintenance tools **if** these have dedicated unittests.

* ``@tested{ExampleTest OtherTest}`` lists one or more test-suite class names separated by spaces.
  Names must end in ``Test``; paths and method selectors are invalid.
* ``@notest{reason}`` explains in one line why a test is not applicable.
  It means the type is not or cannot be tested for the stated reason.
* ``@needtest{reason}`` identifies missing coverage in one line.

Prefer ``@needtest`` over ``@notest`` when missing coverage is temporary.
Prefer ``@tested`` over ``@notest`` when a type is implicitly tested through another type.

Class Organization
==================

Divide a class into logical sections, repeating access specifiers as needed.
Use one empty line before each section and none between its declarations, except before a defaults group.
Methods and data members **must** occupy separate sections, even when they have the same access.
Before the first data member after methods, add one empty line and repeat the appropriate ``public:``, ``protected:``,
or ``private:`` specifier.
Add lowercase ``//`` labels according to these rules:

* Type, constant, and data-member sections **must not** have a label, regardless of access.
* Public method sections **must** have a label, except for construction and operator sections.
* Protected method sections **should** have a label.
* Private method sections **may** have a label only when the class declaration exceeds 100 lines and the label improves
  navigation.

Structs with no methods, or whose entire declaration is fewer than eight lines excluding comments, **must not** use this
section layout.

The usual section order is:

1.  Private friends, then private constants at the beginning of the class, followed by private nested types, enums, and
    aliases in dependency order.
    A type needed by a constant precedes that constant.
    Use the initial implicit private section of a ``class`` or an explicit ``private:`` section in a ``struct``.
    Do not place private constants after methods or among instance data members.
2.  Public types in dependency order.
3.  Default constructor, other constructors, destructor, copy constructor, move constructor, copy assignment, then move
    assignment.
    Put explicitly defaulted or deleted members in a final defaults group.
4.  Operators immediately after construction and defaults: comparison, arithmetic, logical, then other operators.
5.  Accessors and condition tests: condition tests first, then each attribute's accessors together.
6.  Public overrides, grouped in one ``public: // implement Base`` section per base class.
    This grouping **must** take precedence over operator and accessor sections.
    Keep constructors and destructors in the construction section.
    Overrides **should** omit API documentation to avoid duplicating the base class's documentation, and **should**
    follow the declaration order of methods in that base class.
7.  Remaining public methods that do not match other sections.
    Divide them into additional logical groups only when this improves navigation.
8.  Public ``to...`` methods, followed by public static ``from...`` methods and other factories.
9.  Remaining public static helper methods.
10. Repeat points 6, 7, and 9 for protected methods first, then private methods, using the corresponding access specifiers.
    Protected methods **must** have API documentation unless exempt under `Required Documentation`_.
    Private helper methods **should** have API documentation.
11. Instance data members, grouped by access in the order public, protected, then private, each in its own access
    section separated from methods.

Precede the defaults group with one empty line and one of these comment lines:

* ``// defaults`` if every declaration in the group uses ``= default``.
* ``// defaults/deletions`` if any declaration in the group uses ``= delete``.

When rules conflict, choose the most readable layout; use maintainability as the next criterion.

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
* Do not use ``static_cast<void>(...)`` merely to silence ``[[nodiscard]]``.
  Select an operation whose contract matches the intended use, handle the result, or remove ``[[nodiscard]]`` when
  discarding the result is genuinely a normal and safe use of that API.
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

Common Rules
------------

* Prefer ``String`` for read-only strings, ``""_el`` for literals, and ``StringFormat`` for formatting.
* In consumer code, including maintenance tools and demos, use ``using namespace el::text::literals;`` inside the
  owning namespace, with an empty line after the namespace opening, and prefer ``"paths"_el`` over explicit
  ``el::StringLiteral{"paths"}`` construction.
* Prefer ``StringEditor`` as a local mutable working value for explicit in-place editing or small construction tasks.
  Do not use it as the default parameter or read-only storage type.
* For joining fixed string segments, use ``el::join(a, b, ...)``; for a dynamic collection, prepare and join a
  ``StringList``.
  Use ``AnyStringBuilder`` for width-independent construction and direct formatted-value appends.
* Prefer regular UTF-8 types; use U8, U16, or U32 types when an API boundary or algorithm requires them.
* Prefer Erbsland Core types and algorithms before ``std::`` types and algorithms.
* In portable unit tests, do not construct UTF-8 text with ``\x??`` escapes because their interpretation differs
  between compilers.
  When using Erbsland Unit Test, use ``th::stdStringFromHex()`` when raw bytes are required.

Regular Expressions
-------------------

* Write fixed patterns longer than 40 characters in multiline ``(?x)`` mode, with comments explaining each logical part.
* Use verbose mode for shorter patterns too when it makes their constraints clearer.
