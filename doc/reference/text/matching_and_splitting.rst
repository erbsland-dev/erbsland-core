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
``StringPattern::fromDivider()`` parses a syntax supporting literal text, ``?`` for one decoded character, ``[a-z]``
character sets, and one ``*`` divider.
A bracket set may start with ``^`` to invert its characters, such as ``[^a-z]``.
Subsequent carets are literal; ``[^^]`` excludes caret and ``[^]`` accepts every Unicode scalar value.
This changes the meaning of bracket sets that previously began with a literal caret.
The divider separates front and back matching parts; it is not a repeated wildcard.

Parsed patterns support backslash escapes for ``?``, ``*``, ``[``, ``]`` and ``\``.
The typed ``fromDivider()`` overload uses the ``erbsland::text::pattern`` element namespace to build immutable compiled
pattern data directly without parsing pattern syntax.

Full-string Globs
-----------------

``StringPattern::fromGlob()`` compiles a case-sensitive Erbsland Core glob.
It uses the same character-set syntax and backslash escapes as ``fromDivider()``, including ``[^a-z]`` for negation.
Unlike a Divider pattern, it matches the complete candidate and permits any number of ``*`` wildcards.
Malformed brackets, invalid ranges, and unsupported escapes raise ``ParseError``.

Both glob kinds operate on decoded characters across UTF-8, UTF-16, and UTF-32 strings without allocating during
matching.
Stars match zero or more characters, including directory separators, leading dots, and newlines.
Adjacent stars have the same meaning as one star, and ``?`` matches exactly one decoded character.
An empty glob matches only empty text; a standalone star matches any text.

The typed ``fromGlob()`` factory accepts the existing literal, character, range, and set elements plus ``Star``.
``Star`` may appear multiple times.
``Divider`` is rejected at compile time by ``fromGlob()``, and ``Star`` is rejected by ``fromDivider()``.
No arguments creates an empty Glob.
Typed patterns retain the existing fixed-capacity limits: 16 compiled elements, 16 total ranges, and eight ranges per
set.
Each star occupies one element after adjacent stars are coalesced.
Parsed patterns use dynamic storage, limited to 65535 pooled ranges.

.. code-block:: cpp

    const auto sources = el::StringPattern::fromGlob("src/*/file?.[ch]pp"_el);
    const auto matches = sources.matches("src/text/impl/file1.hpp"_el);

    using namespace el::text::pattern;
    static const auto logName = el::StringPattern::fromGlob(
        Text{U"log-"}, Star{}, Text{U"-"}, Range{U'0', U'9'}, Star{});

Python-style Globs
------------------

``StringPattern::fromPosixGlob()`` preserves the original Python-style glob dialect.
The name identifies this dialect; it does not promise complete POSIX conformance or filesystem traversal.
Its full-string matching and extraction behavior is identical to ``Glob``, while parsing differs:

* ``[abc]`` and ``[a-z]`` match a character or ascending range; ``[!a-z]`` negates the set. A caret is literal.
* A closing bracket immediately after ``[`` or ``[!`` is a set member. ``[[]`` matches an opening bracket.
  Unclosed brackets are literal. A leading or trailing hyphen is literal; descending ranges contribute no matches.
* Backslashes are literal, not escapes.

For example, ``fromGlob("[^a-c]*"_el)`` and ``fromPosixGlob("[!a-c]*"_el)`` both reject a candidate beginning with
``b``.
Use ``fromPosixGlob()`` when retaining the original parser's behavior.
``kind()`` reports ``Divider``, ``Glob``, or ``PosixGlob`` according to the factory used.


Matching and Extraction
-----------------------

All three kinds support every matching and extraction method.
``Divider`` patterns retain prefix/suffix boundaries; both glob kinds match and consume the complete candidate.
All comparisons are case-sensitive.

.. list-table::
    :header-rows: 1
    :widths: 15 45 40

    * - Method
      - ``Divider`` on a match
      - ``Glob`` / ``PosixGlob`` on a match
    * - ``matches()``
      - Accepts a matching prefix without ``*``, or non-overlapping anchored parts around one ``*``.
      - Requires the complete text to match all wildcard elements.
    * - ``trim()`` / ``trimmed()``
      - Removes the matched prefix and/or suffix, retaining the middle text.
      - Removes the complete text.
    * - ``split()``
      - Returns ``{prefix, remainder}`` without a suffix, ``{prefix, middle}`` with both parts,
        or ``{leading text, suffix}`` for a suffix-only pattern.
      - Returns ``{text, empty}``.
    * - ``length()``
      - Returns the prefix length, or the suffix length for a suffix-only pattern.
      - Returns the complete text length.
    * - ``index()``
      - Returns the index after the prefix, or the start of the suffix for a suffix-only pattern.
      - Returns the end index.

On a mismatch, ``matches()`` and ``trim()`` return false, ``trimmed()`` returns the original text, ``split()`` returns
``{empty, text}``, ``length()`` returns zero, and ``index()`` returns no-index.
``trim()`` reports whether text was removed, so a successful empty glob match returns false.
That empty match has length zero and end index zero, distinguishing it from a mismatch through ``index()`` or
``matches()``.
``trim()`` accepts only string editors; use ``trimmed()`` to obtain a narrowed read-only string.
Lengths and indexes use bytes for UTF-8, code units for UTF-16, and code points for UTF-32.

Pattern creation uses named factories, including ``fromDivider()`` with no arguments for an empty Divider pattern.
Copying and moving retain the compiled kind in the destination; a moved-from value behaves as an empty Divider.


Example
-------

.. code-block:: cpp

    using namespace el::text::literals;

    const auto parsed = el::StringPattern::fromDivider("http?://*"_el);
    const auto ok = parsed.matches("https://example.test"_el);

    using namespace el::text::pattern;

    static const auto staticPattern = el::StringPattern::fromDivider(
        Text{U"http"},
        OneChar{},
        Text{U"://"},
        Divider{});

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
:cpp:type:`CpCount <erbsland::unit::CpCount>` edit distance.
``MatchList`` is an :cpp:class:`util::List <erbsland::util::List>` of matches.

Example
-------

.. code-block:: cpp

    auto matcher = el::text::fuzzy::Matcher{"verbsoe"_el};
    matcher.setMaximumDistance(el::CpCount{2U}).setMaximumResults(el::ItemCount{3U});
    const auto matches = matcher.findMatches(el::StringList{"verbose"_el, "version"_el, "quiet"_el});

String Splitters
================

For a single first or last separator, use the string methods described in
:ref:`string-separator-splitting`.
They return both halves and distinguish an absent separator from empty fields.

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
``splitCount()`` counts consumed parts, including empty parts and parts discarded by ``skip()``.
``splitIndex()`` is the zero-based index of the next part and equals that count.
Calls at the end leave both values unchanged; ``reset()`` returns both to zero.

``remaining()`` returns the unread suffix as another shared slice, and ``reset()`` restarts the splitter at the
beginning.

An empty source has one empty part.
This preserves ordinary split semantics and lets a caller observe the source once before ``isAtEnd()`` becomes true.

The UTF-8 implementation has a direct byte-search path for a single ASCII separator.
ASCII bytes cannot occur inside a UTF-8 multibyte sequence, so this optimization preserves tolerant decoding behavior.
Other separator sets use decoded character matching.
The width-specific backends can use independent search optimizations while keeping the same public contract.

Callback Processing
-------------------

``forEach(function)`` processes all remaining parts with the current separator mode.
The callback accepts either ``const String &part`` or ``const String &part, ItemIndex index``, using the splitter's
string width.
The index is the zero-based source part index and continues from ``splitIndex()`` after earlier reads or skips.
Each part is consumed before its callback runs.

Return ``LoopStatus::Continue`` to process the next part, ``LoopStatus::Stop`` to return ``LoopResult::Stopped``, or
``LoopStatus::Error`` to return ``LoopResult::Error``.
Reaching the end returns ``LoopResult::Success``.
After a stop, error, or callback exception, processing can resume with the next unread part.
Callback exceptions propagate to the caller.
A null callback returns success without consuming any parts.
Calling ``forEach()`` at the end returns success without invoking the callback.

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
.. doxygenstruct:: erbsland::text::pattern::Star
    :members:
.. doxygenclass:: erbsland::text::pattern::Text
    :members:
.. doxygenclass:: erbsland::text::StringPattern
    :members:
.. doxygenenum:: erbsland::text::StringPatternKind
.. doxygenenum:: erbsland::text::StringSplitMode
.. doxygentypedef:: erbsland::text::StringSplitter
.. doxygentypedef:: erbsland::text::U16StringSplitter
.. doxygentypedef:: erbsland::text::U32StringSplitter
.. doxygentypedef:: erbsland::text::U8StringSplitter
