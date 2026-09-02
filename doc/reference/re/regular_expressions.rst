..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Regular Expressions; Reference
    single: RegEx
    single: regular expression matching
    single: Input
    single: Match
    single: Match16
    single: Match32
    single: capture group
    single: RegExError
    single: Error Handling
    single: Assembler
    single: Disassembler

*******************
Regular Expressions
*******************

The Regular Expression Interface
================================

Creating an Expression
----------------------

:cpp:func:`RegEx::compile <erbsland::re::RegEx::compile>` accepts owning UTF-8, UTF-16 and UTF-32 Core strings.
Every overload returns :cpp:type:`RegExPtr <erbsland::re::RegExPtr>` and produces equivalent compiled behaviour.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    const auto expression8 = re::RegEx::compile("(A)(😀)(B)"_el);
    const auto expression16 = re::RegEx::compile(text::U16String{u"(A)(😀)(B)"_el});
    const auto expression32 = re::RegEx::compile(text::U32String{U"(A)(😀)(B)"_el});

Invalid syntax and configured limits throw :cpp:class:`RegExError <erbsland::re::RegExError>`.
Pattern length is measured in decoded code points, independently of the source encoding.
The compiled object retains the normalized UTF-8 source text returned by
:cpp:func:`pattern() <erbsland::re::RegEx::pattern>`.
UTF-16 and UTF-32 patterns therefore produce the same stored representation as an equivalent UTF-8 pattern.

Lazy Compilation
----------------

``RegEx::lazyCompile`` retains the pattern, flags and settings without parsing the pattern immediately.
The first matching, replacement or diagnostic operation compiles the engine.
Copies of a lazy expression share this state, and concurrent first use compiles one engine for all copies.

Use ``isCompiled()`` to inspect the state without triggering compilation.
Use ``compileNow()`` when an application needs to validate a lazy pattern before matching.
Invalid lazy patterns throw :cpp:class:`RegExError <erbsland::re::RegExError>` at that point.
A failed attempt leaves the expression uncompiled, and a later operation retries compilation.

Matching Operations
-------------------

Each supported string width provides the same five operations:

*   ``match`` starts at the beginning of the subject.
*   ``fullMatch`` requires the complete subject to match.
*   ``findFirst`` searches for the first match.
*   ``findAll`` lazily yields matches from a coroutine generator.
*   ``collectAll`` eagerly returns all matches in a ``std::vector``.

``match``, ``fullMatch`` and ``findFirst`` return ``nullptr`` when no match exists.
The generator and list operations return an empty result.

Subjects and Results
--------------------

The subject parameter determines the result family:

*   :cpp:class:`text::String <erbsland::text::U8String>` returns
    :cpp:class:`Match <erbsland::re::Match>`.
*   :cpp:class:`text::U16String <erbsland::text::U16String>` returns
    :cpp:class:`Match16 <erbsland::re::Match16>`.
*   :cpp:class:`text::U32String <erbsland::text::U32String>` returns
    :cpp:class:`Match32 <erbsland::re::Match32>`.

Core strings own their backing storage.
Inputs, returned matches and lazy generators therefore remain valid after the caller's original string or temporary has
been destroyed.

Tolerant String Decoding
------------------------

All Core-string entry points decode malformed source units as U+FFFD.
This applies to patterns, UTF-8/16/32 subjects, assembler source and replacement expressions.
Replacement characters participate in parsing and matching like any other character, including CRLF lookahead and
zero-width advancement paths.

Custom input implementations control their own decoding policy and may throw encoding or other runtime errors.
Such exceptions propagate unchanged.

Text Streams
------------

The matching operations also accept :cpp:type:`stream::TextInputStreamPtr <erbsland::stream::TextInputStreamPtr>`.
The stream must support positioning because the matching engine stores only byte ranges for capture groups.
After a successful match, the API seeks to each captured range and copies its text into the returned
:cpp:class:`Match <erbsland::re::Match>` object. The returned match therefore remains valid after the stream has been
reused or destroyed.

Capture copying restores the stream's logical position before the next matching operation continues.
Streams that do not support positioning cause :cpp:class:`err::ParameterError <erbsland::err::ParameterError>`.
Read or positioning timeouts cause :cpp:class:`stream::StreamError <erbsland::stream::StreamError>` and are never
treated as end-of-input.

Null Characters
---------------

By default, regular expression patterns reject U+0000, including raw pattern text and character escapes.
Enable
:cpp:enumerator:`Feature::AcceptNullInPattern <erbsland::re::Feature::AcceptNullInPattern>` only when a pattern needs
to match a null character.

Matching input accepts U+0000 by default and processes all following input normally.
To reject U+0000 at the point it is read, disable
:cpp:enumerator:`Feature::AcceptNullInInput <erbsland::re::Feature::AcceptNullInInput>`; matching then throws
:cpp:class:`text::EncodingError <erbsland::text::EncodingError>`.

Replacement
-----------

``replaceAll`` accepts either a replacement expression or a callback receiving
:cpp:type:`MatchPtr <erbsland::re::MatchPtr>`. The callback returns an owning
:cpp:class:`text::String <erbsland::text::U8String>`, allowing it to reuse unchanged input or match content
without creating an intermediate string copy.
Use :cpp:func:`text::U8String::toEscaped <erbsland::text::U8String::toEscaped>` with
:cpp:enumerator:`text::EscapeFormat::RegEx <erbsland::text::EscapeFormat::RegEx>` to escape text for insertion as a
literal pattern.

Settings
========

The :cpp:class:`Settings <erbsland::re::Settings>` class allows you to control which features are accepted and which
limits apply when compiling and executing regular expressions.

Settings are applied during compilation and define the behaviour of the resulting
:cpp:class:`RegEx <erbsland::re::RegEx>` instance.
They are especially important when your application processes patterns from external or untrusted sources, such as user
configuration files or plug-in systems.

Limiting the Run-Time
---------------------

When working with externally provided patterns, it is strongly recommended to limit the maximum run-time of matching
operations.

Using :cpp:func:`setTimeout <erbsland::re::Settings::setTimeout>`, you can define a per-call time limit for all matching
operations performed by a
:cpp:class:`RegEx <erbsland::re::RegEx>` instance.

If a matching operation exceeds the configured timeout, it is aborted and an
:cpp:class:`RegExError <erbsland::re::RegExError>` exception with the error category
``Timeout`` is thrown.

This mechanism protects your application against excessively complex patterns or pathological input that would otherwise
lead to long or unbounded execution times.

Setting Pattern Complexity Limits
---------------------------------

In addition to time limits, the library provides a set of limits that restrict individual aspects of pattern complexity,
such as nesting depth, repetition counts, or internal resource usage.

These limits can only be *tightened*, never extended.
The library ships with a carefully chosen set of safe default limits that work well for most use cases.

Reducing these limits is useful when you want to:

*   Prevent excessive memory consumption
*   Guard against denial-of-service scenarios
*   Enforce predictable performance characteristics

All limits are checked during compilation or execution and result in a well-defined error if exceeded.

Enabling or Disabling Features
------------------------------

By default, the regular expression support in Erbsland Core enables a number of compatibility features to ease migration
from existing regular expression engines.

While convenient, some of these features come with ambiguities, surprising edge cases, or performance costs.
For applications that accept patterns from external sources, it is often desirable to restrict the accepted syntax more
strictly.

The feature flags in :cpp:class:`Settings <erbsland::re::Settings>` allow you to explicitly control which parts of the
pattern syntax are enabled.

A sensible starting point for untrusted patterns is disabling ``AllCompatibility``, thereby accepting only the core,
well-defined syntax.

Certain particularly problematic features are disabled by default:

*   ``EmptyAlternatives``
*   ``EmptyGroups``

Both can lead to unintuitive matches and make patterns harder to reason about.
If you enable them, you should do so consciously and only when their behaviour is clearly understood and required.

Choosing the Right Settings
---------------------------

For internal, fully controlled patterns, the default settings are usually sufficient and provide maximum convenience.

For externally supplied patterns, we recommend:

*   Enabling a strict timeout
*   Tightening complexity limits where possible
*   Disabling compatibility features that are not explicitly required

This layered approach keeps your application robust while still allowing powerful and expressive regular expressions.

The Input Interface
===================

The :cpp:class:`Input <erbsland::re::Input>` interface allows you to provide custom input sources to the regular
expression engine.

Because the regular expression engine in Erbsland Core is based on a Thompson NFA, input is consumed sequentially and
processed in a highly efficient streaming fashion.
This makes it possible to match patterns not only against built-in Core strings, but also against custom iterators or
application-specific sources.

The input interface is a low-level extension point intended for advanced use cases.
If you only need to match against strings, the built-in string overloads are usually the better and simpler choice.
They also provide a defined tolerant decoding policy: malformed units are replaced with U+FFFD.

How to Implement Your Source
----------------------------

To implement a custom input source, derive from one of the following classes:

*   :cpp:class:`Input <erbsland::re::Input>` for UTF-8 input
*   :cpp:class:`Input16 <erbsland::re::Input16>` for UTF-16 input
*   :cpp:class:`Input32 <erbsland::re::Input32>` for UTF-32 input

The chosen base class determines which unified match family is returned: ``Match``, ``Match16`` or ``Match32``.
A custom input implements ``createMatch`` and decides whether captured content is copied or retained as a slice of its
source.

Your implementation must override the abstract methods defined by
:cpp:class:`InputBase <erbsland::re::InputBase>`. These methods form the
contract between your input source and the matching engine.

Implementation Requirements
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The most important method is
:cpp:func:`read <erbsland::re::InputBase::read>`. It is invoked in the hot loop
of the matching engine and must therefore be implemented as efficiently as possible.

When implementing an input source, the following rules must be respected:

*   :cpp:func:`read <erbsland::re::InputBase::read>` must return the next
    character as :cpp:class:`text::Char <erbsland::text::Char>` together with
    its position. Every returned character must be a valid Unicode scalar value.

*   When the end of the input is reached,
    :cpp:func:`text::Char::endOfData <erbsland::text::Char::endOfData>` must be
    returned.

*   Repeated calls to :cpp:func:`read <erbsland::re::InputBase::read>` after the
    end of the input must continue to succeed and keep returning the end-of-data
    signal.

*   Reserved :cpp:class:`text::Char <erbsland::text::Char>` signals other than
    the end-of-data signal, surrogate code points and values above U+10FFFF must
    never be returned.

*   The returned position must advance monotonically and must uniquely identify
    the character within the input stream.

If line-break folding (CRLF handling) is enabled for the regular expression, your input source must additionally
implement:

*   :cpp:func:`peek <erbsland::re::InputBase::peek>` to look ahead without
    consuming input
*   :cpp:func:`skip <erbsland::re::InputBase::skip>` to advance by a :cpp:type:`unit::CpLength
    <erbsland::unit::CpLength>`

These methods allow the engine to treat ``CRLF`` sequences as a single logical line break while preserving correct
positional information.

The matching engine relies on this contract and does not repeat Unicode validity checks in its hot loop.
Validate or decode custom data before returning it.
Error-tolerant inputs should replace invalid source values with
:cpp:func:`text::Char::replacement <erbsland::text::Char::replacement>`.
Incorrect or incomplete implementations may lead to incorrect matches or undefined behavior.

Exception Propagation
~~~~~~~~~~~~~~~~~~~~~

A custom input may choose strict decoding and throw a Core encoding error.
Exceptions thrown by ``read()``, ``peek()``, ``skip()`` or ``createMatch()`` propagate unchanged through every matching
operation.
The engine does not replace, wrap or translate these exceptions into :cpp:class:`RegExError <erbsland::re::RegExError>`.

Example Implementation
----------------------

The following example shows a complete implementation of a custom input source that reads characters from a
``std::vector``.
While simplified, it demonstrates all required methods and lifetime rules.
The example treats the vector as an error-tolerant source and replaces invalid UTF-32 values while reading and capturing
its content.

.. literalinclude:: files/input.hpp
    :language: cpp
    :linenos:

The Match Interface
===================

Match Families
--------------

A successful match is represented by one encoding-specific type:

*   :cpp:class:`Match <erbsland::re::Match>` returns :cpp:class:`text::String
    <erbsland::text::U8String>` content.
*   :cpp:class:`Match16 <erbsland::re::Match16>` returns :cpp:class:`text::U16String
    <erbsland::text::U16String>` content.
*   :cpp:class:`Match32 <erbsland::re::Match32>` returns :cpp:class:`text::U32String
    <erbsland::text::U32String>` content.

There is no separate view-result family.
Core strings are read-only owning values, so matches retain their subject storage and return copy-free slices from it.

Lifetime and Ownership
----------------------

Match objects are shared pointers and are immutable after creation.
The match keeps the complete subject string alive; every value returned by ``content()`` also owns the referenced
storage.
It is safe to retain a match or captured string after the source variable, temporary subject or generator has been
destroyed.

Groups and Positions
--------------------

:cpp:class:`MatchBase <erbsland::re::MatchBase>` provides ``begin``, ``end``, ``range`` and ``group`` access. Overloads
without a selector address capture group zero, which represents the whole match.
Other groups are selected with
:cpp:type:`CaptureGroupIndex <erbsland::re::CaptureGroupIndex>` or a :cpp:class:`text::String
<erbsland::text::U8String>` name.

Positions are coordinates in the original encoding:

*   UTF-8 positions count bytes.
*   UTF-16 positions count ``char16_t`` code units, including both units of a surrogate pair.
*   UTF-32 positions count ``char32_t`` code units.

``end()`` identifies the first unit after the captured range.
``content()`` converts the stored coordinates into a native Core string slice without copying.

Regular Expression Errors
=========================

Regular-expression parsing, diagnostics and execution failures are reported with
:cpp:class:`RegExError <erbsland::re::RegExError>`. The exception derives from
:cpp:class:`err::RuntimeError <erbsland::err::RuntimeError>` and carries a
:cpp:class:`RegExErrorContext <erbsland::re::RegExErrorContext>`.

.. code-block:: cpp

    try {
        const auto expression = re::RegEx::compile(pattern);
    } catch (const re::RegExError &error) {
        log(error.diagnostic()->toString());
    }

Error Context
-------------

The context separates information that applications commonly need to inspect:

*   ``title`` is a concise statement of what failed. The inherited ``reason()`` and ``what()`` contain only this title.
*   ``description`` optionally explains why the operation failed, including limits or relevant values.
*   ``category`` identifies the subsystem or failure kind without duplicating it in the title.
*   ``location`` stores optional, zero-based typed line, column and code-point indices.

The corresponding convenience accessors are available directly on ``RegExError``.
Missing location components return the respective ``noIndex`` value.
``withLineNumber()`` returns a copy with a replaced zero-based line index and preserves every other context field.

Every RE subsystem follows the same message split.
The title states which operation failed, such as ``Failed to parse regular expression`` or
``Failed to assemble regular expression``.
The description states the concrete cause, including relevant values and limits.
Category and location are never duplicated in either text field.

Diagnostics
-----------

``diagnostic()`` returns a structured Core diagnostic whose ``toString()`` method renders the complete plain-text
diagnostic, including the title, optional description, category and every available location component.
Human-readable locations are rendered one-based, while the inspection API remains zero-based.
``RegExError::toString()`` provides a compact single-line summary in the form ``<category>: <title>. <description>`` and
deliberately omits structured location details.

Core Strings and Encoding
-------------------------

Every RE API that receives a Core string uses tolerant decoding.
Malformed UTF-8, UTF-16 or UTF-32 units become U+FFFD and participate normally in pattern parsing, assembler parsing,
replacement parsing and matching.
These string overloads therefore do not throw encoding errors for malformed units.

Custom input implementations remain exception-transparent.
An encoding error or another runtime error thrown by ``read()``, ``peek()``, ``skip()`` or ``createMatch()`` propagates
unchanged and is not converted into ``RegExError``.

Categories
----------

``Parser`` and ``Format`` identify invalid pattern and replacement syntax.
``Assembler`` identifies invalid diagnostic program text.
``Limit`` and ``Timeout`` identify configured resource boundaries.
``Engine`` and ``Internal`` identify execution or invariant failures.

Diagnostics
===========

The regular expression support in Erbsland Core includes a low-level assembler and disassembler that allow you to
inspect, analyze, and even manually construct the internal program executed by the matching engine.

These tools are primarily intended for diagnostics, debugging, testing, and advanced experimentation.
They are not required for normal use of the library, but they provide valuable insight into how patterns are translated
into executable instructions.

Disassemble Compiled Patterns
-----------------------------

The disassembler can be used to inspect the program generated by the compiler for a given regular expression pattern.

.. code-block:: cpp

    const auto reTag = RegEx::compile(R"((?is)<([a-z]+)([^>]*)>)");

    for (const auto &line : diagnostics::Disassembler{reTag}.disassemble()) {
        std::cout << line << '\n';
    }

This produces a textual representation of the compiled program, including character classes, control flow, and capture
group handling:

.. code-block:: text

    ; Character classes
    ; ============================================================================
    .section &class
    $0000:                                  .class              ; [a-z]
    $0000:                                  .data $000061-$00007A
    ; Program
    ; ============================================================================
    .section &program
    $0000:            0900003c              CI CHAR '<'
    $0001:            86000000              START CAPTURE 0
    $0002:            0c000000              CI CLASS $0000
    $0003:            81000002 00000005     SPLIT $0002, $0005
    $0005:            a6000000              STOP CAPTURE 0
    $0006:            86000001              START CAPTURE 1
    $0007:            81000009 0000000b     SPLIT $0009, $000B
    $0009:            2900003e              NOT CI CHAR '>'
    $000A:            c2000007              JUMP $0007
    $000B:            a6000001              STOP CAPTURE 1
    $000C:            0900003e              CI CHAR '>'
    $000D:            83000000              MATCH

Reading this output is helpful when:

*   debugging unexpected matching behaviour,
*   analyzing performance characteristics,
*   learning how specific pattern constructs are compiled.

Write Custom Programs
---------------------

For advanced use cases, you can also write custom matching programs directly using the assembler.

This allows you to bypass the regular expression syntax entirely and construct a program manually using the engine’s
instruction set.

.. code-block:: cpp

    using namespace erbsland::text::literals;
    const auto program = text::StringList{
        "; my custom program"_el,
        "loop: CHAR 'a'"_el,
        "      SPLIT %loop, %end"_el,
        "end:  MATCH"_el,
    };

    auto reCustom = diagnostics::Assembler().compile(program);
    auto match = reCustom->match("aaaaaa"_el);

    std::cout << "Result: " << match->content() << '\n';

.. code-block:: text

    Result: aaaaaa

Custom programs are useful for testing the engine, experimenting with new instruction sequences, or creating minimal
reproducible examples when investigating bugs.

The assembler accepts :cpp:type:`text::StringList <erbsland::text::U8StringList>`.
The disassembler returns the same owning Core list type, so listings remain valid independently of the disassembler
object.
Assembler source uses tolerant Core-string decoding.
Malformed UTF-8 units become U+FFFD before tokenization.
Syntax and limit failures are reported as :cpp:class:`RegExError <erbsland::re::RegExError>` with an ``Assembler``
category and a zero-based structured source location; the rendered diagnostic presents locations one-based.

Interface
=========

.. doxygenclass:: erbsland::re::CaptureGroup
    :members:

.. doxygentypedef:: erbsland::re::CaptureGroupList
.. doxygentypedef:: erbsland::re::CaptureGroupIndex
.. doxygenclass:: erbsland::re::CaptureRange
    :members:
.. doxygenstruct:: erbsland::re::CharAndPosition
    :members:
.. doxygenclass:: erbsland::re::diagnostics::Assembler
    :members:
.. doxygenclass:: erbsland::re::diagnostics::Disassembler
    :members:
.. doxygenenum:: erbsland::re::ErrorCategory

.. doxygenfunction:: erbsland::re::toString(const ErrorCategory category) noexcept -> text::String
.. doxygenenum:: erbsland::re::Feature

.. doxygenfunction:: erbsland::re::toString(const Feature feature) -> text::String
.. doxygenclass:: erbsland::re::Features
    :members:
.. doxygenenum:: erbsland::re::Flag

.. doxygenfunction:: erbsland::re::toString(const Flag flag) -> text::String
.. doxygenclass:: erbsland::re::Flags
    :members:
.. doxygenclass:: erbsland::re::Input
    :members:

.. doxygentypedef:: erbsland::re::InputPtr
.. doxygenclass:: erbsland::re::Input16
    :members:

.. doxygentypedef:: erbsland::re::Input16Ptr
.. doxygenclass:: erbsland::re::Input32
    :members:

.. doxygentypedef:: erbsland::re::Input32Ptr
.. doxygenclass:: erbsland::re::InputBase
    :members:

.. doxygentypedef:: erbsland::re::InputBasePtr
.. doxygentypedef:: erbsland::re::InputPosition
.. doxygenclass:: erbsland::re::Match
    :members:

.. doxygentypedef:: erbsland::re::MatchPtr

.. doxygentypedef:: erbsland::re::MatchGenerator

.. doxygentypedef:: erbsland::re::MatchList
.. doxygenclass:: erbsland::re::Match16
    :members:

.. doxygentypedef:: erbsland::re::Match16Ptr

.. doxygentypedef:: erbsland::re::Match16Generator

.. doxygentypedef:: erbsland::re::Match16List
.. doxygenclass:: erbsland::re::Match32
    :members:

.. doxygentypedef:: erbsland::re::Match32Ptr

.. doxygentypedef:: erbsland::re::Match32Generator

.. doxygentypedef:: erbsland::re::Match32List
.. doxygenclass:: erbsland::re::MatchBase
    :members:

.. doxygentypedef:: erbsland::re::MatchBasePtr
.. doxygenclass:: erbsland::re::RegEx
    :members:

.. doxygentypedef:: erbsland::re::RegExPtr

.. doxygentypedef:: erbsland::re::ConstRegExPtr
.. doxygenclass:: erbsland::re::RegExError
    :members:
.. doxygenclass:: erbsland::re::RegExErrorContext
    :members:
.. doxygenclass:: erbsland::re::Settings
    :members:
