*********************************
Regular Expression API Guidelines
*********************************

Core Semantics
==============

Matching Model
--------------

.. code-block:: text

    match = test only at the start of the input
    full match = require the complete input
    find first = scan for the first matching position
    find all = lazily scan successive non-overlapping matches
    collect all = materialize all successive non-overlapping matches
    group zero = complete match
    capture position = native UTF-8 byte, UTF-16 code-unit, or UTF-32 code-unit offset
    captured text = shared owning slice that retains the complete subject storage

Primary Types
=============

.. code-block:: text

    RegEx // immutable eagerly or lazily compiled regular expression
    Match, Match16, Match32 // immutable UTF-8, UTF-16, and UTF-32 match results

Secondary Types
===============

.. code-block:: text

    Input, Input16, Input32 // custom input extension points for each text width
    InputBase, MatchBase // width-independent input and result interfaces
    CaptureGroup, CaptureRange, CaptureGroupIndex, InputPosition // captured identity and native-unit bounds
    Settings // feature, complexity, and execution limits
    Flag, Flags // matching modes
    Feature, Features // optional syntax and compatibility features
    RegExError, RegExErrorContext, ErrorCategory // structured failures and categories

Pattern Definitions
===================

.. code-block:: text

    Mp = MatchPtr/Match16Ptr/Match32Ptr // match pointer corresponding to the input width
    S = text::String/text::U16String/text::U32String // string corresponding to the match width

Compilation and Matching Patterns
=================================

.. code-block:: text

    T::compile(pattern[, flags, settings]) -> RegExPtr // compile immediately
    T::lazyCompile(pattern[, flags, settings]) -> RegExPtr // compile on first use
    o.pattern() -> text::AnyString // inspect the retained pattern
    o.isCompiled() -> bool // test whether lazy compilation has completed
    o.compileNow() // force lazy compilation
    o.match(input) -> Mp // match at the start
    o.fullMatch(input) -> Mp // require the complete input
    o.findFirst(input) -> Mp // scan for the first match
    o.findAll(input) -> util::CoGenerator❮Mp❯ // lazily scan all matches
    o.collectAll(input) -> util::List❮Mp❯ // materialize all matches
    o.replaceAll(text, replacement-or-callback) -> text::String // replace every UTF-8 match

Match Result Patterns
=====================

.. code-block:: text

    o.begin/end/range([group]) -> T // inspect native-unit bounds of the whole match or one capture
    o.group(index-or-name) -> CaptureGroup // inspect capture identity and bounds
    o.content([index-or-name]) -> S // return an owning slice for the whole match or one capture
    o.groups() -> CaptureGroupList // inspect all captured groups
    o.isMatched() -> bool // test whether a capture participated

Settings Patterns
=================

.. code-block:: text

    o.❮limit❯()/set❮Limit❯(value) // inspect or tighten a pattern or execution limit
    o.timeout()/setTimeout(value) // inspect or set the per-operation timeout
    o.enableFeature/disableFeature(feature) // change accepted syntax or compatibility behavior
    o.hasFeature(feature) -> bool // test whether a syntax feature is enabled

Custom Input Patterns
=====================

.. code-block:: text

    o.read(position) -> CharAndPosition // decode one character and the next native-unit position
    o.isAtEnd(position) -> bool // test the custom input boundary
    o.slice(range) -> S // create retained input content for a captured range
