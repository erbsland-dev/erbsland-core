..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: String Patterns
    single: StringPattern
    single: Pattern Matching
    single: Shell Patterns
    single: Prefix Matching
    single: Suffix Matching
    single: text::pattern

*************************
Recognizing Text Patterns
*************************

Some text formats are more expressive than a fixed prefix but far smaller than a grammar.
A record may begin with a literal marker, require two letters and a digit, and allow an arbitrary payload before a known
suffix.
Writing a parser for that boundary would be excessive, while a chain of manual searches can hide the shape you meant to
validate.

:cpp:class:`StringPattern <erbsland::text::StringPattern>` fills this narrow space.
It combines literals, decoded characters, ranges, and one front/back divider into a reusable recognizer for UTF-8,
UTF-16, or UTF-32 text.
This page shows both the compact pattern language and the typed construction API, then uses the resulting pattern to
match, trim, split, and recover native positions.

Keep the boundary of this tool in mind as you read.
A fixed prefix remains clearer as ``startsWith()``, and a sequential grammar with optional branches, values, quoting, or
rich diagnostics belongs in :cpp:class:`StringCharReader <erbsland::text::StringCharReader>`.
``StringPattern`` is at its best when the complete small shape remains easy to see.

Describe a Small Shape in One Pattern
=====================================

The parsed constructor accepts the same string-view types as the rest of the text module.
For the common UTF-8 case, construct a pattern from a ``""_el`` literal:

.. code-block:: cpp

    const auto pattern = el::StringPattern{"probe-??[0-9]*"_el};

The pattern is parsed and validated during construction.
Invalid syntax throws
:cpp:class:`ParseError <erbsland::err::ParseError>`.

This form is well suited for configuration files and for application code where a compact pattern is easier to read than
an equivalent sequence of typed elements.

.. erbsland-demo::
    :source: text/StringPattern/PatternConstruction.cpp
    :exec: text/string_pattern --demo PatternConstruction
    :source-sha256: b7ecf2aa54075fc206fa7d6c458d7bef823fd2c9fc1aa2758be0a02152dd7ba7

.. code-block:: cpp

    /// `StringPattern` parses a compact shell-like pattern once and reuses it for many text checks.
    ///
    /// The parsed syntax is useful for small library-level recognizers: a literal prefix, one decoded character with `?`,
    /// bracket character sets, and one `*` divider for front/back matching. Backslash escapes keep special characters
    /// literal when they are part of the text format itself.
    void patternConstruction() {
        // A pattern can describe the small fixed shell around a laboratory record.
        const auto probeRecord = el::StringPattern{"probe-??[0-9]*"_el};

        const auto probeLines = el::StringList{{
            "probe-AB7 temperatur=21.4C"_el,
            "probe-A7 temperatur=21.4C"_el,
        }};
        const auto lineFormat = el::StringFormat{"{:30}: {}"};
        for (const auto &line : probeLines) {
            el::io::printLine(lineFormat.build(line, probeRecord.matches(line)));
        }

        // Escapes make the pattern syntax available for literal diagnostic markers.
        const auto marker = el::StringPattern{"messung\\[\\?\\]\\*"_el};
        el::io::printLine("escaped marker                : "_el, marker.matches("messung[?]* archiviert"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    probe-AB7 temperatur=21.4C    : true
    probe-A7 temperatur=21.4C     : false
    escaped marker                : true

.. erbsland-demo-end::

Pattern Syntax
--------------

A pattern consists of decoded text plus a small set of special characters.
It always matches at the boundary of a string.

Without a divider, the pattern matches the prefix.
With a divider, the text before the divider matches the prefix and the text after the divider matches the suffix.

The syntax intentionally remains small and predictable.

``text``
    Literal decoded characters.

    The pattern ``messung:`` matches the prefix of
    ``messung:temperatur=21.4``.

``?``
    Exactly one decoded Unicode character.

    This matches one character, not one byte.
    The pattern ``probe-??`` matches both ``probe-A7`` and ``probe-µ7``.

``[a-z]``
    A character set using the same compact range syntax as
    :cpp:class:`CharSet <erbsland::text::CharSet>`.
    It matches one decoded character from the set.

``*``
    The divider.

    It does not represent a repeated wildcard.
    Instead, it separates the prefix and suffix portions of the pattern so
    both can be validated without scanning the entire string like a regular
    expression engine.

``\?``, ``\*``, ``\[``, ``\]``, ``\\``
    Escaped special characters.

    Only these escapes are supported.
    Restricting the syntax keeps mistakes visible instead of silently accepting
    unsupported constructs.

A pattern may contain only one divider.

.. list-table::
    :header-rows: 1
    :widths: 24 38 38

    *   - Pattern
        - Meaning
        - Example
    *   - ``abc``
        - Match the prefix ``abc``.
        - ``abcdef`` matches.
    *   - ``abc*``
        - Match the same prefix with an explicit divider.
        - ``abcdef`` matches.
    *   - ``*abc``
        - Match the suffix ``abc``.
        - ``defabc`` matches.
    *   - ``abc*xyz``
        - Match prefix ``abc`` and suffix ``xyz``.
        - ``abcdefxyz`` matches while leaving ``def`` as the middle.

The pattern ``*`` is invalid.
Patterns containing more than one divider, such as ``a*b*c``, are also invalid.

Build the Same Shape from Typed Elements
========================================

For library-internal patterns, the typed element constructor avoids the parsed syntax entirely.
It builds immutable compiled pattern data directly, stores literal text as UTF-32, and makes each pattern element
explicit.

.. erbsland-demo::
    :source: text/StringPattern/StaticConstruction.cpp
    :exec: text/string_pattern --demo StaticConstruction
    :source-sha256: b88ae32d6a926b559824a0fd62fea3595604e02aa15d09d61b2f7ee27830c9c3

.. code-block:: cpp

    /// Typed construction builds a `StringPattern` from explicit pattern elements.
    ///
    /// This form skips parsed pattern syntax. Literal text is supplied as UTF-32 through `Text`, and character choices are
    /// expressed with `OneChar`, `Range`, and `Set`. `Divider` is the single front/back divider that corresponds to `*` in
    /// parsed patterns.
    void staticConstruction() {
        using namespace el::pattern;

        static const auto acceptedRecord = el::StringPattern{
            Text{U"probe-"},
            Range{U'A', U'Z'},
            Set{{U'0', U'9'}},
            OneChar{},
            Divider{},
            Text{U";ok"},
        };

        const auto probeLines = el::StringList{{
            "probe-A7b wert=0.42;ok"_el,
            "probe-a7b wert=0.42;ok"_el,
            "probe-A7b wert=0.42;prüfen"_el,
        }};
        const auto lineFormat = el::StringFormat{"{:30}: {}"};
        for (const auto &line : probeLines) {
            el::io::printLine(lineFormat.build(line, acceptedRecord.matches(line)));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    probe-A7b wert=0.42;ok        : true
    probe-a7b wert=0.42;ok        : false
    probe-A7b wert=0.42;prüfen    : false

.. erbsland-demo-end::

Pattern Elements
----------------

The elements are defined in ``erbsland::text::pattern``.

Most static patterns begin with:

.. code-block:: cpp

    using namespace el::pattern;

``Text``
    Literal UTF-32 text.

    The type stores a ``std::u32string_view``, so literals use the ``U``
    prefix.

``OneChar``
    One decoded character.
    Equivalent to ``?`` in the parsed syntax.

``Range``
    One inclusive
    :cpp:class:`CharRange <erbsland::text::CharRange>`.

``Set``
    One character set built from several ranges or from an existing
    :cpp:class:`CharSet <erbsland::text::CharSet>`.

    The API uses the name ``Set``.
    The public header is named ``PatternSet.hpp`` to avoid filename
    collisions with other public ``Set`` headers.

``Divider``
    The prefix/suffix divider.
    Equivalent to ``*`` in the parsed syntax.

The number of pattern elements and character ranges is intentionally limited.
A string pattern is designed to describe a small boundary shape rather than a general-purpose grammar.

Ask Whether the Boundary Matches
================================

``matches()`` answers the most common question: does this text have the expected shape?

Use it whenever the surrounding code only needs a yes/no decision.

.. erbsland-demo::
    :source: text/StringPattern/Matches.cpp
    :exec: text/string_pattern --demo Matches
    :source-sha256: da9cd7770bbf1d1ca5d9c7aa97a26012095358d793fcd2c44fd663321585584c

.. code-block:: cpp

    /// `matches()` answers whether a text has the expected decoded-character shape.
    ///
    /// A pattern without `*` matches the front of the string. A trailing `*` keeps that prefix behavior explicit.
    /// A leading `*` checks the end. A pattern with text on both sides of `*` requires both the prefix and suffix without
    /// overlap.
    void matches() {
        const auto nameOnly = el::StringPattern{"messung:"_el};
        const auto withUnit = el::StringPattern{"messung:*;C"_el};
        const auto finished = el::StringPattern{"*;fertig"_el};

        const auto temperature = el::String{"messung:temperatur=21.4;C"_el};
        const auto completed = el::String{"messung:leitwert=0.42;fertig"_el};

        el::io::printLine("front only ..............: "_el, nameOnly.matches(temperature));
        el::io::printLine("front and back ..........: "_el, withUnit.matches(temperature));
        el::io::printLine("suffix only .............: "_el, finished.matches(completed));
        el::io::printLine("wrong suffix ............: "_el, finished.matches(temperature));
        el::io::printLine("missing suffix ..........: "_el, withUnit.matches("messung:temperatur=21.4"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    front only ..............: true
    front and back ..........: true
    suffix only .............: true
    wrong suffix ............: false
    missing suffix ..........: false

.. erbsland-demo-end::

The method does not return captures or explain why matching failed.

If you need diagnostics or want to parse several fields after validating the boundary, continue with
:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` or use the
reader directly.

Remove a Recognized Boundary
============================

``trimmed()`` returns a view with the matching shell removed.

``trim()`` narrows an owning read-only string to the same range and reports whether the operation succeeded.
It changes only the visible range; it does not modify character data.

For a prefix pattern, the prefix is removed.
For a suffix pattern, the suffix is removed.
For a pattern containing both a prefix and a suffix, both are removed and only the middle remains.

This provides a compact way to unwrap small protocol-like envelopes.

.. erbsland-demo::
    :source: text/StringPattern/TrimAndTrimmed.cpp
    :exec: text/string_pattern --demo TrimAndTrimmed
    :source-sha256: 4640b457858e3cd35d936da8f54b8cab8fadd29d383604020ce7620800a1c7c6

.. code-block:: cpp

    /// `trimmed()` and `trim()` remove the matched front or back shell of a string pattern.
    ///
    /// For a pattern with text on both sides of the divider, the divider represents the part that stays. This is useful
    /// when a protocol-like string wraps the meaningful value in a predictable prefix and suffix.
    void trimAndTrimmed() {
        const auto envelope = el::StringPattern{"messung:*;ok"_el};
        const auto original = el::String{"messung:temperatur=21.4C;ok"_el};

        el::io::printLine("original ................: "_el, original);
        el::io::printLine("trimmed .................: "_el, envelope.trimmed(original));

        auto narrowed = el::String{"messung:ph=7.1;ok"_el};
        if (envelope.trim(narrowed)) {
            el::io::printLine("narrowed ................: "_el, narrowed);
        }

        auto unchanged = el::String{"messung:ph=7.1;prüfen"_el};
        el::io::printLine("changed .................: "_el, envelope.trim(unchanged));
        el::io::printLine("kept ....................: "_el, unchanged);
    }

.. erbsland-ansi::
    :escape-char: ␛

    original ................: messung:temperatur=21.4C;ok
    trimmed .................: temperatur=21.4C
    narrowed ................: ph=7.1
    changed .................: false
    kept ....................: messung:ph=7.1;prüfen

.. erbsland-demo-end::

Trimming is deliberately conservative.

If the pattern does not match, the original text remains unchanged and ``trim()`` returns ``false``.

These methods are intended for removing a single matching shell.
To remove repeated occurrences throughout a string, use the normal string editing and search functions instead.

Split Where the Pattern Places Its Divider
==========================================

``split()`` returns two views.

The first contains the text matched by the prefix portion of the pattern.
The second contains everything after that boundary.

This is particularly useful for UTF-8 text.
A pattern such as ``probe-??`` may consume different numbers of bytes depending on the matched Unicode characters.
``split()`` calculates the correct native split position automatically.

.. erbsland-demo::
    :source: text/StringPattern/Split.cpp
    :exec: text/string_pattern --demo Split
    :source-sha256: 849c15527027c147f0db09d40a804d5e10a1c4d49683bddc2d15d23fbd16da0b

.. code-block:: cpp

    /// `split()` returns the matched text and the remaining text at the pattern boundary.
    ///
    /// The method is especially handy when `?` or bracket sets make the matched prefix variable in bytes. For a pattern
    /// with both a prefix and suffix, the suffix is validated and then dropped from the second result.
    void split() {
        const auto recordPattern = el::StringPattern{"probe-??*;ok"_el};
        const auto record = el::String{"probe-A7 temperatur=21.4C;ok"_el};

        const auto [recordId, valueText] = recordPattern.split(record);
        el::io::printLine("record id ...............: "_el, recordId);
        el::io::printLine("value ...................: "_el, valueText);

        const auto suffixPattern = el::StringPattern{"*;fehler"_el};
        const auto [payload, suffix] = suffixPattern.split("probe-B8 leitwert=0.41;fehler"_el);
        el::io::printLine("payload .................: "_el, payload);
        el::io::printLine("suffix ..................: "_el, suffix);
    }

.. erbsland-ansi::
    :escape-char: ␛

    record id ...............: probe-A7
    value ...................:  temperatur=21.4C
    payload .................: probe-B8 leitwert=0.41
    suffix ..................: ;fehler

.. erbsland-demo-end::

With a suffix-only pattern such as ``*;ok``, the first result contains everything before the suffix and the second
contains the matched suffix.

With a pattern such as ``probe-??*;ok``, the suffix is validated and omitted from the second result, leaving only the
useful middle part.

If the pattern does not match, the first result is empty and the second result contains the original text.

Reuse the Native Match Position
===============================

``length()`` and ``index()`` expose the native boundary position found by the matcher.

For UTF-8 strings, the result is a
:cpp:type:`ByteLength <erbsland::unit::ByteLength>` or
:cpp:type:`ByteIndex <erbsland::unit::ByteIndex>`.

For UTF-16 strings, the values are UTF-16 code-unit positions.

For UTF-32 strings, they are code-point positions.

.. erbsland-demo::
    :source: text/StringPattern/LengthAndIndex.cpp
    :exec: text/string_pattern --demo LengthAndIndex
    :source-sha256: 1681622778250b30616c905e14cde36572e74b1d0ac79e9d87844c7532250deb

.. code-block:: cpp

    /// `length()` and `index()` return native string positions for the matching part.
    ///
    /// For UTF-8 text this means byte positions, for UTF-16 it means data-unit positions, and for UTF-32 it means
    /// code-point positions. The values are meant for slicing and diagnostics near string boundaries, not for scanning an
    /// entire grammar.
    void lengthAndIndex() {
        const auto microSensor = el::StringPattern{u8"µ?*"_el};
        const auto u8Measurement = el::String{u8"µA=0.42"_el};

        el::io::printLine("UTF-8 split index .......: "_el, microSensor.index(u8Measurement));
        el::io::printLine("UTF-8 match bytes .......: "_el, microSensor.length(u8Measurement));

        const auto suffix = el::StringPattern{"*;ok"_el};
        const auto accepted = el::String{"temperatur=21.4;ok"_el};

        el::io::printLine("suffix index ............: "_el, suffix.index(accepted));
        el::io::printLine("suffix length ...........: "_el, suffix.length(accepted));
    }

.. erbsland-ansi::
    :escape-char: ␛

    UTF-8 split index .......: 3
    UTF-8 match bytes .......: 3
    suffix index ............: 15
    suffix length ...........: 3

.. erbsland-demo-end::

These methods are useful when the next operation performs native string slicing, reports diagnostics near the validated
boundary, or stores native string positions.

They are not intended to replace iteration.
If your algorithm needs to process every character or token, a search loop or
:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` is usually the
better tool.
