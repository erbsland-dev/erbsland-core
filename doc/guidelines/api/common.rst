
*********************
Common API Guidelines
*********************

This document describes common API guidelines for the Erbsland Core library.
These guidelines are additional clarifications on top of the Erbsland C++ Code Style.

Pattern Placeholder Definitions
===============================

.. code-block:: text

    o = the object
    T = the object type of the interface
    U, V = generic types that are not T
    R = the raw underlying type
    TPtr = std::shared_ptr<T>
    a, b, ... = value parameters
    v = commonly used for "the value"
    <U> = marks a template
    ❮name❯ = pattern placeholder

Common Patterns
===============

.. code-block:: text

    o.❮value❯(...) -> V  // get a value
    o.get❮Type❯(...) -> V  // get a value for a special type
    o.get(...) -> V  // get index/key based values, containers
    o.is❮Condition❯() -> bool  // test for a condition
    o.set❮Value❯(v) -> void  // setter on functional object, e.g. parser or objects that used via shared_ptr
    o.set❮Value❯(v) -> T&  // setter on editor and option-like types for chaining setters.
    o.set(a, b) -> void/T&  // set index/key based values, containers
    o.❮modify❯(...) -> void  // in-place modify, verb in present tense, void = chaining more such ops is unlikely
    o.❮modify❯(...) -> T&  // in-place modify, T& = chaining more such ops is likely
    o.❮modified❯(...) -> T  // return modified copy, past-participle or result-state name, const method, nodiscard
    o.to❮Value❯() -> U  // convert this type into another one, const method, copy/const-ref, nodiscard
    T::from❮Value❯(v) -> T  // factory method to create an instance from some other data
    T::create(...) -> TPtr  // factory method to create a shared_ptr, usually combined with private ctor
    T::❮special❯() -> T  // factory method to create a special or predefined state

    // alternative variants that throw an exception, only in combination with non throwing methods
    o.❮value❯OrThrow(...) -> V  // try to get a value, but throw an exception if something goes wrong.
    o.❮modify❯OrThrow(...) -> void  // try to do something, throw if it isn't possible.
    T::from❮Value❯OrThrow(v) -> T  // try to convert from v, throw on any error.

    // frequently used
    o.toRawValue() -> R  // access a the raw underlying type, const method, copy/const-ref, nodiscard
    o.toHash() -> std::size_t  // get std hash from a type
    o.toString() -> String  // create a string representation of a type
    o.swap(T) -> void  // swap this instance with another one

Additional Name Principles
==========================

.. code-block::

    // template parameters
    T               // single template parameter, straightforward use case
    tCamelCase      // multiple parameters, non-generic arg, or verbose naming, like `tValue`, `tSize`
    tFirst, tSecond // classic a/b operations

    // static constexpr variables
    cCamelCase      // special states/numbers
    T::cCamelCase   // should match method name, e.g. cExample -> T::example() -> T

    // static constexpr variables for enum like types
    T::CamelCase    // to allow enum class like usage (Color::Red)

Exception and Error Handling
============================

Object APIs shall be of two types:

-   **Default No Throw:** Preferred where practical. Regular methods do not throw exceptions.
    Methods or method variants that do throw exceptions are marked with the suffix ``...OrThrow()``.
-   **Default May Throw:** The class-level API documentation defines a set of exceptions all methods may throw,
    unless a method is marked with ``noexcept``.

We prefer the first variant where it makes sense, as this leads to more readable user code.
The second variant makes sense for APIs like parsers, where e.g. embedded IO objects may throw on read errors at any
time and, therefore, the caller must expect exceptions with every call.

All thrown exceptions shall be listed in the API documentation.

Test Status Markers
===================

Every documented class or struct and every namespace-scope free function shall end its API documentation block with one
test-status marker.
The marker points to the test suite where a maintainer can find the relevant coverage; it does not describe individual
test methods.

.. code-block:: text

    @tested{PathContentTest StreamSettingsTest}
    @notest{Abstract interface; concrete implementations own the behavior tests.}
    @needtest{Add coverage for the platform-specific failure path.}

``@tested`` lists one or more suite class names, separated by one space.
Each name must end in ``Test``.
Do not use commas, paths, backticks, or ``Suite::testMethod`` selectors.
``@notest`` and ``@needtest`` require a compact single-line explanation.
Do not place any test-status marker on constructors, methods, operators, or other members; the containing class or
struct marker covers its complete public API.

Domain-Specific Page Guidelines
===============================

Domain-specific guideline pages should define a finite vocabulary and explicit name-composition patterns.
They define the following sections:

*   **Core Semantics:**
    Domain specific information that is required to understand the following sections.
    Optional, at most 60 lines and four subsections.
    Compact, minimal/no-prosa, noise-free technical definitions.
*   **Primary Types:**
    Mandatory first type section.
    Text block, plain list of type names with ``// short description``, one line per collapsed type.
*   **[❮name❯ Types]:**
    Zero to many blocks defining additional types in this domain; with exactly two type sections, the second is
    **Secondary Types**.
    Text block, plain list of type names with ``// short description``, one line per collapsed type.
*   **[Pattern Definitions]:**
    Optional text block defining compact one- or two-character pattern shortcuts.
*   **❮name❯ Patterns**
    Zero to many blocks defining API patterns.
    Text block, plain, collapsed patterns with ``// short description``, one line per collapsed pattern.
    Patterns omit keywords like ``const``, ``auto``, ``static``.
    The patterns reduce noise and unnecessary repetitions.
    Only if necessary, a block clarifies a used placeholder ``

Related Guidelines
==================

* Read :doc:`../api_guidelines` on how to write domain guidelines in more detail.
* Read :doc:`../rst_style` to learn how to format documentation pages correctly, choose appropriate heading levels, and create links to code symbols.
* Read :doc:`../writing_style` for the tone, vocabulary, and writing style used throughout the documentation.
