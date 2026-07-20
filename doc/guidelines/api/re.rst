*********************************
Regular Expression API Guidelines
*********************************

Core Semantics
==============

Core strings own their backing storage.
Regular expression subjects, matches and captured slices may therefore share storage without a separate borrowed-view
API.
Names identify the encoding of the result, not its ownership mode.

Primary Types
=============

.. code-block:: text

    RegEx // immutable eager or lazy regular expression
    Match, Match16, Match32 // UTF-8, UTF-16 and UTF-32 results
    Input, Input16, Input32 // custom-source extension points for each result family
    RegExError, RegExErrorContext // structured RE failures and diagnostic context
    Flag, Flags // regular-expression matching modes
    Feature, Features // optional syntax and compatibility features

Compilation Patterns
====================

.. code-block:: text

    RegEx.compile(text::String) -> RegExPtr
    RegEx.compile(text::U16String) -> RegExPtr
    RegEx.compile(text::U32String) -> RegExPtr
    RegEx.lazyCompile(text::AnyString) -> RegExPtr
    o.pattern() -> text::String // normalized UTF-8 source pattern
    o.isCompiled() -> bool
    o.compileNow() -> void

All pattern overloads share the same reader-based compilation path.
Pattern limits are code-point based.
All APIs that receive Core strings use tolerant decoding and treat malformed units as U+FFFD.
Compiled expressions retain that normalized UTF-8 source pattern independently of the input string width.
Custom inputs are exception-transparent and may provide strict decoding instead.
Lazy expressions retain their pattern, flags and settings without validation.
Their first matching, replacement or diagnostic operation compiles a shared engine exactly once.
Explicit validation uses ``compileNow()`` and preserves ``RegExError`` diagnostics.

Error Message Patterns
======================

All :cpp:class:`RegExError <erbsland::re::RegExError>` messages separate operation from cause:

.. code-block:: text

    title = what operation failed
    description = why it failed, including relevant values or limits
    category and location = structured context only

Use stable operation titles such as ``Failed to parse regular expression``, ``Failed to compile regular expression``,
``Failed to assemble regular expression`` and ``Failed to match regular expression``.

The regular-expression API never throws C++ standard-library exception types.
Use
:cpp:class:`err::ParameterError <erbsland::err::ParameterError>` for invalid API arguments and selectors, and
:cpp:class:`RegExError <erbsland::re::RegExError>` for parsing, compilation, diagnostics and matching failures.
Exceptions raised by user callbacks propagate unchanged.

Subject and Match Patterns
==========================

.. code-block:: text

    o.match(text::String) -> MatchPtr
    o.match(text::U16String) -> Match16Ptr
    o.match(text::U32String) -> Match32Ptr
    o.❮operation❯(Input❮width❯Ptr) -> Match❮width❯Ptr // custom source, same result family
    Match.content() -> text::String
    Match16.content() -> text::U16String
    Match32.content() -> text::U32String

The same mapping applies to ``fullMatch``, ``findFirst``, ``findAll`` and ``collectAll``.
Do not add ``...View`` method, input or match names: ownership is already part of the Core view semantics.

Capture Group Patterns
======================

.. code-block:: text

    o.group(CaptureGroupIndex) -> CaptureGroup
    o.group(text::String) -> CaptureGroup
    o.content(CaptureGroupIndex) -> text::U❮width❯String
    o.content(text::String) -> text::U❮width❯String

Use :cpp:type:`CaptureGroupIndex <erbsland::re::CaptureGroupIndex>` for group selectors.
Keep
:cpp:type:`InputPosition <erbsland::re::InputPosition>` and :cpp:class:`CaptureRange <erbsland::re::CaptureRange>`
encoding-agnostic because custom inputs define their coordinate system.
