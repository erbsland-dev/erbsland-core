.. index::
    single: RegEx
    single: regular expression matching

********************************
The Regular Expression Interface
********************************

Creating an Expression
======================

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
================

``RegEx::lazyCompile`` retains the pattern, flags and settings without parsing the pattern immediately.
The first matching, replacement or diagnostic operation compiles the engine.
Copies of a lazy expression share this state, and concurrent first use compiles one engine for all copies.

Use ``isCompiled()`` to inspect the state without triggering compilation.
Use ``compileNow()`` when an application needs to validate a lazy pattern before matching.
Invalid lazy patterns throw :cpp:class:`RegExError <erbsland::re::RegExError>` at that point.
A failed attempt leaves the expression uncompiled, and a later operation retries compilation.

Matching Operations
===================

Each supported string width provides the same five operations:

*   ``match`` starts at the beginning of the subject.
*   ``fullMatch`` requires the complete subject to match.
*   ``findFirst`` searches for the first match.
*   ``findAll`` lazily yields matches from a coroutine generator.
*   ``collectAll`` eagerly returns all matches in a ``std::vector``.

``match``, ``fullMatch`` and ``findFirst`` return ``nullptr`` when no match exists.
The generator and list operations return an empty result.

Subjects and Results
====================

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
========================

All Core-string entry points decode malformed source units as U+FFFD.
This applies to patterns, UTF-8/16/32 subjects, assembler source and replacement expressions.
Replacement characters participate in parsing and matching like any other character, including CRLF lookahead and
zero-width advancement paths.

Custom :doc:`input` implementations control their own decoding policy and may throw encoding or other runtime errors.
Such exceptions propagate unchanged.

Text Streams
============

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
===============

By default, regular expression patterns reject U+0000, including raw pattern text and character escapes.
Enable
:cpp:enumerator:`Feature::AcceptNullInPattern <erbsland::re::Feature::AcceptNullInPattern>` only when a pattern needs
to match a null character.

Matching input accepts U+0000 by default and processes all following input normally.
To reject U+0000 at the point it is read, disable
:cpp:enumerator:`Feature::AcceptNullInInput <erbsland::re::Feature::AcceptNullInInput>`; matching then throws
:cpp:class:`text::EncodingError <erbsland::text::EncodingError>`.

Replacement
===========

``replaceAll`` accepts either a replacement expression or a callback receiving
:cpp:type:`MatchPtr <erbsland::re::MatchPtr>`. The callback returns an owning
:cpp:class:`text::String <erbsland::text::U8String>`, allowing it to reuse unchanged input or match content
without creating an intermediate string copy.
Use :cpp:func:`text::U8String::toEscaped <erbsland::text::U8String::toEscaped>` with
:cpp:enumerator:`text::EscapeFormat::RegEx <erbsland::text::EscapeFormat::RegEx>` to escape text for insertion as a
literal pattern.

Interface
=========

.. doxygenenum:: erbsland::re::Flag

.. doxygenfunction:: erbsland::re::toString(const Flag flag) -> text::String
.. doxygenclass:: erbsland::re::Flags
    :members:
.. doxygenclass:: erbsland::re::RegEx
    :members:

.. doxygentypedef:: erbsland::re::RegExPtr

.. doxygentypedef:: erbsland::re::ConstRegExPtr
