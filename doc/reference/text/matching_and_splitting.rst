..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Matching; Reference
    single: String Splitting; Reference
    single: String Pattern
    single: Fuzzy Matching
    single: Damerau-Levenshtein Distance
    single: String Splitter
    single: splitting strings

***************************
Text Matching and Splitting
***************************

String Pattern
==============

Introduction
------------

``StringPattern`` is a lightweight pattern matcher for decoded string characters.
It is intended for small internal checks such as protocol or prefix/suffix matching.
The parsed syntax supports literal text, ``?`` for one decoded character, ``[a-z]`` character sets, and one ``*``
divider.
The divider separates front and back matching parts; it is not a repeated wildcard.

Parsed patterns support backslash escapes for ``?``, ``*``, ``[``, ``]`` and ``\``.
Typed construction uses the ``erbsland::text::pattern`` element namespace to build immutable compiled pattern data
directly without parsing pattern syntax.

Example
-------

.. code-block:: cpp

    using namespace el::text::literals;

    const auto parsed = el::StringPattern{"http?://*"_el};
    const auto ok = parsed.matches("https://example.test"_el);

    using namespace el::text::pattern;

    static const auto staticPattern = el::StringPattern{
        Text{U"http"},
        OneChar{},
        Text{U"://"},
        Divider{}};

Fuzzy Matching
==============

Introduction
------------

The ``text::fuzzy`` namespace ranks candidate strings by Damerau-Levenshtein edit distance.
Matching operates on decoded Unicode code points, and an adjacent transposition counts as one edit.
It is suitable for short suggestion lists, command names, option names, and other user-entered identifiers.

Matcher
-------

``Matcher`` stores the reference pattern, an optional maximum distance, an optional result limit, and a
:cpp:type:`CharCompareFn <erbsland::text::CharCompareFn>`.
Distance and result limits are unbounded by default, and matching is case-sensitive unless a comparison callback is
configured.

``findMatches()`` accepts a :cpp:type:`StringList <erbsland::text::StringList>` and returns a ``MatchList`` ordered by
distance and then original candidate order.
Equivalent candidates are deduplicated using the configured comparison callback.

Match Results
-------------

``Match`` exposes the original candidate and its
:cpp:type:`CpLength <erbsland::unit::CpLength>` edit distance.
``MatchList`` is an :cpp:class:`util::List <erbsland::util::List>` of matches.

Example
-------

.. code-block:: cpp

    auto matcher = el::text::fuzzy::Matcher{"verbsoe"_el};
    matcher.setMaximumDistance(el::CpLength{2U}).setMaximumResults(el::ItemCount{3U});
    const auto matches = matcher.findMatches(el::StringList{"verbose"_el, "version"_el, "quiet"_el});

String Splitters
================

String splitters read an owning Core string sequentially and return copy-on-write slices without copying their text.
``StringSplitter`` is the common UTF-8 alias.
The ``U8StringSplitter``, ``U16StringSplitter`` and ``U32StringSplitter`` variants provide the same interface for each
supported width.

Separators
----------

A splitter accepts either one :cpp:class:`Char <erbsland::text::Char>` or a
:cpp:class:`CharSet <erbsland::text::CharSet>`. Each call to ``next()`` reads through the next matching separator.
Malformed encoded data is handled tolerantly like the underlying string type.

:cpp:enumerator:`StringSplitMode::DiscardSeparator <erbsland::text::StringSplitMode::DiscardSeparator>` returns only
the text between separators.
Consecutive separators therefore return empty parts, and a trailing separator produces a final empty part.
:cpp:enumerator:`StringSplitMode::KeepSeparator <erbsland::text::StringSplitMode::KeepSeparator>` includes the
separator at the end of each part.
In this mode a trailing separator completes the preceding part without producing another empty part, which is useful for
line-oriented processing.

Sequential State
----------------

``isAtEnd()`` distinguishes an empty part from the end of the sequence.
Calling ``next()`` after the end safely returns an empty string.
Calling ``skip()`` consumes the same next part as ``next()`` without constructing its shared slice; calling it after the
end has no effect.
``remaining()`` returns the unread suffix as another shared slice, and ``reset()`` restarts the splitter at the
beginning.

An empty source has one empty part.
This preserves ordinary split semantics and lets a caller observe the source once before ``isAtEnd()`` becomes true.

The UTF-8 implementation has a direct byte-search path for a single ASCII separator.
ASCII bytes cannot occur inside a UTF-8 multibyte sequence, so this optimization preserves tolerant decoding behavior.
Other separator sets use decoded character matching.
The width-specific backends can use independent search optimizations while keeping the same public contract.

Interface
=========

.. doxygenclass:: erbsland::text::fuzzy::Match
    :members:

.. doxygentypedef:: erbsland::text::fuzzy::MatchList
.. doxygenclass:: erbsland::text::fuzzy::Matcher
    :members:
.. doxygenstruct:: erbsland::text::pattern::Divider
    :members:
.. doxygenstruct:: erbsland::text::pattern::OneChar
    :members:
.. doxygenclass:: erbsland::text::pattern::Set
    :members:
.. doxygenclass:: erbsland::text::pattern::Range
    :members:
.. doxygenclass:: erbsland::text::pattern::Text
    :members:
.. doxygenclass:: erbsland::text::StringPattern
    :members:
.. doxygenenum:: erbsland::text::StringSplitMode
.. doxygentypedef:: erbsland::text::StringSplitter
.. doxygentypedef:: erbsland::text::U16StringSplitter
.. doxygentypedef:: erbsland::text::U32StringSplitter
.. doxygentypedef:: erbsland::text::U8StringSplitter
