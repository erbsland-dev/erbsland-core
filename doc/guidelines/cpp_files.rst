
******************************
Guidelines for File Structures
******************************

Relation File vs. Type
======================

1.  One primary type per ``hpp/cpp`` module.
2.  A primary type can be a ``class``, ``struct``, ``enum class`` or even a ``using`` directive.
    Relaxed ``impl`` directories: a ``hpp/cpp`` module can also contain a logical group of helper functions/templates/types. 
3.  The filename of a source file always matches its type. E.g. the file for the class ``Example`` is ``Example.hpp``.
4.  A ``cpp`` file is added if there are implementation details that cannot/should not be in the header file.

Splitting Implementation over Multiple Files
============================================
 
If the implementation for a class exceeds 500 lines, it should be split into multiple ``cpp`` files.

1.  The pattern for files related to a class is: ``Class_part.cpp``, where ``part`` is a descriptive name for the 
    logical functionality.
    E.g. ``Integer_addition.cpp`` for the addition implementation of the ``Integer`` class.
    Important is to separate the part from the class using an underscore character ``_``.
2.  Template implementations should be placed in ``Class_part.tpp``. Same principle as point 1.
    **Only** include the partial headers at the bottom of ``Class.hpp``.
    Do not add ``#include "Class.hpp"`` in ``Class_part.tpp``.
3.  Inline implementations should be placed in ``Class_part.hpp``. Same principle as point 1.

Forward Declarations
====================

If forward declarations to a nontrivial declared class or template are used from multiple files, put them into a special
header ``Class_fwd.hpp``, that only contains the fwd implementation and all required includes.

The forward header is the authoritative declaration location:

1.  If ``Class_fwd.hpp`` exists, ``Class.hpp`` must include it. This lets the compiler diagnose declaration and
    definition mismatches.
2.  Use the canonical forward header for friends, pointers, references, function declarations with by-value
    parameters or returns, and aliases that accept incomplete types.
3.  Include the full definition for inheritance, by-value data members, inline, template, or ``constexpr`` bodies,
    nested-type access, ``sizeof``, or default arguments that construct or access the type.
4.  Template constraints and default template arguments belong in the canonical forward declaration. The
    implementation header defines the same template without repeating defaults.
5.  A forward header should include other canonical forward headers. A full project header is only appropriate when
    an exposed alias or declaration genuinely requires its definition.

Out-of-Line Implementations
===========================

Keep headers focused on declarations and code that must be visible to callers.
Move non-template, non-``constexpr`` implementations to the matching ``cpp`` file when they contain more than a
defaulted special member or one simple member expression.
Move dependencies used only by the extracted body to the ``cpp`` file as well.

Templates stay in the owning ``tpp`` file and are included through that header.
Do not add a heap-backed PImpl only to reduce compile time.
Representation splitting is appropriate only when an existing heap or shared-storage design allows it without another
allocation or a semantic or runtime regression.

Include Ownership
=================

Each header and source file directly includes the declarations it uses.
After moving an implementation out of a header, remove dependencies that were used only by that implementation.
Source files are reviewed independently for stale includes and direct dependency completeness; they must not rely on
unrelated transitive includes.

Optional Standard-Library Formatters
====================================

The production API uses the Erbsland ``StringFormat`` system.
Optional ``std::formatter`` specializations exist only to improve diagnostics in unit tests and other explicit
standard-library interoperability code.

1.  Keep all specializations for a domain in ``StdFormatFor<Domain>.hpp``.
2.  Do not include these headers from regular domain headers or generated ``all.hpp`` headers.
3.  Include the matching formatter header explicitly in a unit test that compares domain values with
    ``REQUIRE_*`` or ``CHECK_*`` macros.
4.  A formatter based on :cpp:type:`erbsland::text::String <erbsland::text::String>` includes ``StdFormatForText.hpp`` and derives from the
    matching ``std::formatter`` base.

Directories and Namespaces
==========================

1.  The ``src`` directory matches the global namespace.
2.  For each used namespace, a subdirectory is created with the namespace name.
    E.g. the class ``erbsland::math::Integer`` is located in ``src/erbsland/math/Integer.hpp``
3.  *Additional* directories are allowed, to further group files in the same namespace.
    So, ``math/additions`` and ``math/subtractions`` may exist to group files in the ``math`` namespace.
    Such subgroupings make sense if a directory exceeds 30 files. 

Separating Implementation Details from the Public API
=====================================================

1.  All types/classes/etc. declared in regular directories and namespaces are considered part of the 
    **public API**.
2.  The namespace and subdirectory ``impl`` marks the boundary between public API and implementation details.
3.  All types/classes/etc. declared inside a namespace and directory tree that contains ``impl`` is considered as 
    **private implementation detail**.
4.  For classes, it is ok to have a ``impl`` variant. E.g. ``math::Example`` may have a ``math::impl::Example`` if
    this makes sense for hiding implementation details.
5.  If naming clashes must be avoided, adding the suffix ``Impl`` to functions is ok – but discouraged.  
