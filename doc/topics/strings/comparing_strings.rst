.. index::
    !single: Comparing Whole and Partial Strings
    single: Compare Whole Strings
    single: Choose a Character Comparison Function
    single: Test Prefixes, Suffixes, and Contained Text
    single: Handle Invalid Encodings Before Comparing
    single: String
    single: StringEditor
    single: StringLiteral
    single: U8String
    single: U8StringEditor
    single: U16String
    single: U16StringEditor
    single: U32String
    single: U32StringEditor
    single: compare
    single: compareCaseFolded
    single: compareAsciiFolded
    single: compareIdentifier
    single: startsWith
    single: endsWith
    single: contains
    single: count
    single: isValidUtf8
    single: isValidUtf16
    single: isValidUtf32
    single: Case-Insensitive Comparison
    single: Identifier Comparison
    single: Partial StringEditor Tests
    single: Encoding Validation

***********************************
Comparing Whole and Partial Strings
***********************************

Erbsland Core compares decoded Unicode code points rather than raw bytes.
This means that string operations behave consistently across UTF-8, UTF-16, and UTF-32 text and follow the actual
characters represented by the text.

This page explains how to compare strings, how to perform partial-string searches, and how to choose the right
comparison rule for your use case.

For most code:

- Use ``==`` and ``!=`` for exact whole-string equality.
- Use ``<=>``, ``<``, ``<=``, ``>``, and ``>=`` when you need deterministic ordering.
- Use :cpp:func:`compare() <erbsland::text::U8String::compare>` when you need an explicit
  ``std::strong_ordering`` result or want to supply a custom character comparison function.
- Use :cpp:func:`startsWith() <erbsland::text::U8String::startsWith>`,
  :cpp:func:`endsWith() <erbsland::text::U8String::endsWith>`,
  :cpp:func:`contains() <erbsland::text::U8String::contains>`, and
  :cpp:func:`count() <erbsland::text::U8String::count>` when you need to inspect only part of a string.

The examples on this page use :cpp:type:`String <erbsland::text::String>`, the common UTF-8 read-only string
type.
The same comparison model is available for :cpp:type:`StringEditor <erbsland::text::StringEditor>`,
:cpp:type:`StringLiteral <erbsland::text::StringLiteral>`,
:cpp:class:`U8String <erbsland::text::U8String>`,
:cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>`,
:cpp:class:`U16String <erbsland::text::U16String>`,
:cpp:class:`U16StringEditor <erbsland::text::U16StringEditor>`,
:cpp:class:`U32String <erbsland::text::U32String>`, and
:cpp:class:`U32StringEditor <erbsland::text::U32StringEditor>`.

Compare Whole Strings
=====================

Whole-string comparison is exact by default.

The comparison operators do not fold case, normalize identifiers, ignore accents, or apply locale-specific collation
rules.
This behavior is intentional.
Exact comparison is deterministic, easy to reason about, and suitable for keys, caches, configuration values, protocol
data, tests, and other situations where the stored text must match exactly.

For UTF-8 code, compare a :cpp:type:`String <erbsland::text::String>` directly with another ``String``, a
:cpp:type:`StringEditor <erbsland::text::StringEditor>`, or a ``"_el"`` literal.

For UTF-16 and UTF-32 code, compare values of the same width.
Compare ``U16String`` with ``U16StringEditor`` or ``u"..."_el``, and compare ``U32String`` with ``U32StringEditor`` or
``U"..."_el``.
Convert explicitly when you intentionally compare text stored in different encodings.

.. erbsland-demo::
    :source: text/String/WholeStringComparison.cpp
    :exec: text/string --demo WholeStringComparison
    :source-sha256: 587a702fdd1412a4f8ad36e34d2ad57dca559130db3c51b0008a97a185ea62e5

.. code-block:: cpp

    /// `String` compares whole strings by decoded Unicode code point.
    /// Use the comparison operators for ordinary equality and ordering. Use `compare()`
    /// when you need the `std::strong_ordering` result explicitly or want to pass a
    /// character comparison function. The common UTF-8 aliases compare with other UTF-8
    /// strings, views, and `"_el"` literals; UTF-16 and UTF-32 variants follow the same
    /// same-width pattern.
    void wholeStringComparison() {
        const auto tag = el::String{"lišejník"_el};
        const auto sameView = el::String{"lišejník"_el};
        const auto editableTag = "lišejník"_el;
        const auto booleanFormat = el::BooleanFormat::yesNo();

        // Compare a view with another view, an editable string, and a literal.
        el::io::printLine("tag == sameView ...............: "_el, booleanFormat, tag == sameView);
        el::io::printLine("tag == editableTag ............: "_el, booleanFormat, tag == editableTag);
        el::io::printLine("tag == \"lišejník\"_el ..........: "_el, booleanFormat, tag == "lišejník"_el);
        el::io::printLine("tag != \"tuleň\"_el .............: "_el, booleanFormat, tag != "tuleň"_el);

        // Ordering uses decoded code points, so uppercase and lowercase letters differ.
        auto ordering = tag <=> "tuleň"_el;
        el::io::printLine("tag <=> \"tuleň\"_el ............: "_el, el::toString(ordering));
        ordering = tag.compare("LIŠEJNÍK"_el);
        el::io::printLine("tag.compare(\"LIŠEJNÍK\"_el) ....: "_el, el::toString(ordering));
        el::io::printLine("tag < \"tuleň\"_el ..............: "_el, booleanFormat, tag < "tuleň"_el);
        el::io::printLine("tag <= sameView ...............: "_el, booleanFormat, tag <= sameView);
        el::io::printLine("tag >= \"arka\"_el ..............: "_el, booleanFormat, tag >= "arka"_el);

        // Width-specific strings use the same operations with values of the same width.
        const auto u8Habitat = u8"ledová kra"_el;
        const auto u16Habitat = u"ledová kra"_el;
        const auto u32Habitat = U"ledová kra"_el;

        el::io::printLine("u8Habitat == u8 literal .......: "_el, booleanFormat, u8Habitat == u8"ledová kra"_el);
        el::io::printLine("u16Habitat == u16 literal .....: "_el, booleanFormat, u16Habitat == u"ledová kra"_el);
        el::io::printLine("u32Habitat == u32 literal .....: "_el, booleanFormat, u32Habitat == U"ledová kra"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    tag == sameView ...............: yes
    tag == editableTag ............: yes
    tag == "lišejník"_el ..........: yes
    tag != "tuleň"_el .............: yes
    tag <=> "tuleň"_el ............: less
    tag.compare("LIŠEJNÍK"_el) ....: greater
    tag < "tuleň"_el ..............: yes
    tag <= sameView ...............: yes
    tag >= "arka"_el ..............: yes
    u8Habitat == u8 literal .......: yes
    u16Habitat == u16 literal .....: yes
    u32Habitat == u32 literal .....: yes

.. erbsland-demo-end::

Choose a Character Comparison Function
======================================

The comparison operators always perform exact code-point comparison.

When your application requires different matching rules, use
:cpp:func:`compare() <erbsland::text::U8String::compare>` and provide a character comparison function.

Three comparison functions cover the most common use cases:

- :cpp:func:`Char::compareCaseFolded() <erbsland::text::Char::compareCaseFolded>`
  performs Unicode-aware case-insensitive comparison.
  Characters are compared using Unicode case-folding rules.
  This requires the Unicode database and therefore links the corresponding Unicode data into your application.

- :cpp:func:`Char::compareAsciiFolded() <erbsland::text::Char::compareAsciiFolded>`
  folds only ASCII letters ``A-Z`` to ``a-z``.
  It does not depend on Unicode database data and is often suitable for protocol tokens, file formats, and other
  ASCII-only text.

- :cpp:func:`Char::compareIdentifier() <erbsland::text::Char::compareIdentifier>`
  is designed for identifier-like text.
  It folds ASCII case and treats the space character ``U+0020`` as equal to the underscore ``U+005F``.

Choose the comparison rule that matches the meaning of your data.
Do not use folded comparison when exact identity is required, and do not use ASCII folding for international text unless
ASCII-only behavior is explicitly desired.

.. erbsland-demo::
    :source: text/String/ComparisonFunctions.cpp
    :exec: text/string --demo ComparisonFunctions
    :source-sha256: 04a7e20599b2c3768572a68078cf8276a0d63ed2bc674d5ecb9eb66eb54838dd

.. code-block:: cpp

    /// `compare()` accepts character comparison functions for specialized matching rules.
    /// Use `Char::compareCaseFolded` for Unicode-aware case-insensitive text, use
    /// `Char::compareAsciiFolded` when ASCII-only folding is enough, and use
    /// `Char::compareIdentifier` for identifier-like names where ASCII case and spaces
    /// versus underscores should compare equally.
    void comparisonFunctions() {
        const auto snowyOwl = "SOVA SNĚŽNÍ"_el;
        const auto snowyOwlLower = "sova sněžní"_el;
        const auto asciiLabel = "Polar Fox"_el;
        const auto asciiLabelLower = "polar fox"_el;
        const auto identifier = "Arctic Fox Trail"_el;
        const auto normalizedIdentifier = "arctic_fox_trail"_el;

        // Regular comparison is exact and uses decoded code points.
        printComparison("snowyOwl.compare(snowyOwlLower) .........................: "_el, snowyOwl.compare(snowyOwlLower));

        // Unicode case folding handles non-ASCII letters such as `Ě` and `ě`.
        printComparison("snowyOwl.compare(..., Char::compareCaseFolded) ..........: "_el,
            snowyOwl.compare(snowyOwlLower, el::Char::compareCaseFolded));

        // ASCII folding is small and fast, but only changes A-Z to a-z.
        printComparison("asciiLabel.compare(..., Char::compareAsciiFolded) .......: "_el,
            asciiLabel.compare(asciiLabelLower, el::Char::compareAsciiFolded));
        printComparison("snowyOwl.compare(..., Char::compareAsciiFolded) .........: "_el,
            snowyOwl.compare(snowyOwlLower, el::Char::compareAsciiFolded));

        // Identifier comparison is useful for normalized keys or configuration-style names.
        printComparison("identifier.compare(..., Char::compareIdentifier) ........: "_el,
            identifier.compare(normalizedIdentifier, el::Char::compareIdentifier));
    }

.. erbsland-ansi::
    :escape-char: ␛

    snowyOwl.compare(snowyOwlLower) .........................: less
    snowyOwl.compare(..., Char::compareCaseFolded) ..........: equal
    asciiLabel.compare(..., Char::compareAsciiFolded) .......: equal
    snowyOwl.compare(..., Char::compareAsciiFolded) .........: less
    identifier.compare(..., Char::compareIdentifier) ........: equal

.. erbsland-demo-end::

Test Prefixes, Suffixes, and Contained Text
===========================================

Many string-related questions concern only part of a string.

Instead of manually slicing text and comparing the pieces, use the dedicated partial-string operations.
These methods make the intent clearer and ensure that matching follows the same Unicode decoding rules as whole-string
comparison.

The four core operations are:

- :cpp:func:`startsWith() <erbsland::text::U8String::startsWith>` for prefixes.
- :cpp:func:`endsWith() <erbsland::text::U8String::endsWith>` for suffixes.
- :cpp:func:`contains() <erbsland::text::U8String::contains>` for containment tests.
- :cpp:func:`count() <erbsland::text::U8String::count>` for counting non-overlapping occurrences.

Each function accepts the same optional character comparison function as
:cpp:func:`compare() <erbsland::text::U8String::compare>`.
This allows case-folded or identifier-style matching for prefix, suffix, containment, and counting operations.

An empty search string always produces a count of zero.
This avoids accidental infinite matches in loops and counting logic.

When you know that a match must occur at the beginning or end of the text, prefer ``startsWith()`` or ``endsWith()``.
These operations only inspect the relevant edge of the string and avoid a full containment search.

.. erbsland-demo::
    :source: text/String/PartialStringComparison.cpp
    :exec: text/string --demo PartialStringComparison
    :source-sha256: 6e75349fb59042cebcc0095231da2f23f8d29abc3110315a1ed043e4ee4878e5

.. code-block:: cpp

    /// Use `startsWith()`, `endsWith()`, `contains()`, and `count()` to test parts of a string.
    /// These methods use the same comparison rules as whole-string comparison and accept the
    /// same optional character comparison function. Prefix and suffix tests are efficient
    /// even for large strings because only the required edge of the string is inspected.
    void partialStringComparison() {
        const auto observationLog = "Lachtan: LEDOVÁ KRA; lachtan: tiché moře; tuleň: ledová kra; mrož: severní útes"_el;
        const auto booleanFormat = el::BooleanFormat::yesNo();

        el::io::printLine("Observation Log:"_el);
        el::io::printLine(observationLog);
        el::io::printLine();

        // Case-sensitive partial tests are direct and predictable.
        el::io::printLine(
            "startsWith(\"Lachtan\"_el) ................: "_el, booleanFormat, observationLog.startsWith("Lachtan"_el));
        el::io::printLine(
            "endsWith(\"severní útes\"_el) .............: "_el, booleanFormat, observationLog.endsWith("severní útes"_el));
        el::io::printLine(
            "contains(\"tiché moře\"_el) ...............: "_el, booleanFormat, observationLog.contains("tiché moře"_el));
        el::io::printLine("count(\"ledová kra\"_el) ..................: "_el, observationLog.count("ledová kra"_el));

        // Pass a comparison function when matching should ignore case.
        el::io::printLine();
        el::io::printLine("startsWith(\"lachtan\"_el, case-folded) ...: "_el,
            booleanFormat,
            observationLog.startsWith("lachtan"_el, el::Char::compareCaseFolded));
        el::io::printLine("contains(\"LEDOVÁ KRA\"_el, case-folded) ..: "_el,
            booleanFormat,
            observationLog.contains("LEDOVÁ KRA"_el, el::Char::compareCaseFolded));
        el::io::printLine("count(\"lachtan\"_el, case-folded) ........: "_el,
            observationLog.count("lachtan"_el, el::Char::compareCaseFolded));
        el::io::printLine("count(\"ledová kra\"_el, case-folded) .....: "_el,
            observationLog.count("ledová kra"_el, el::Char::compareCaseFolded));
        el::io::printLine("count(\"\"_el) ............................: "_el, observationLog.count(""_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Observation Log:
    Lachtan: LEDOVÁ KRA; lachtan: tiché moře; tuleň: ledová kra; mrož: severní útes

    startsWith("Lachtan"_el) ................: yes
    endsWith("severní útes"_el) .............: yes
    contains("tiché moře"_el) ...............: yes
    count("ledová kra"_el) ..................: 1

    startsWith("lachtan"_el, case-folded) ...: yes
    contains("LEDOVÁ KRA"_el, case-folded) ..: yes
    count("lachtan"_el, case-folded) ........: 2
    count("ledová kra"_el, case-folded) .....: 2
    count(""_el) ............................: 0

.. erbsland-demo-end::

Handle Invalid Encodings Before Comparing
=========================================

Comparison operations remain deterministic even when the input contains malformed UTF data.

During decoding, malformed UTF-8, UTF-16, and UTF-32 sequences are replaced with the Unicode replacement character
``U+FFFD``.
All comparison and search operations work on the decoded character stream, including these replacement characters.

This behavior is often useful when processing text from untrusted or partially corrupted sources because operations can
continue without throwing exceptions or failing unexpectedly.

However, different malformed inputs can decode to the same sequence of replacement characters.
As a result, two different invalid strings can compare equal after decoding.

If invalid data must be rejected, validate the encoding before performing comparisons:

- Use :cpp:func:`isValidUtf8() <erbsland::text::U8String::isValidUtf8>` for
  :cpp:class:`U8String <erbsland::text::U8String>` and the common
  :cpp:type:`String <erbsland::text::String>` alias.
- Use :cpp:func:`isValidUtf16() <erbsland::text::U16String::isValidUtf16>` for UTF-16 text.
- Use :cpp:func:`isValidUtf32() <erbsland::text::U32String::isValidUtf32>` for UTF-32 text.

Validation is particularly important for:

- security-sensitive identifiers,
- protocol values,
- file formats,
- authentication data,
- and information crossing trust boundaries.

For display-oriented workflows, tolerant comparison may provide a better user experience.
The important part is making that choice deliberately.

Practical Guidelines
====================

A few simple rules cover most situations:

- Use exact comparison by default.
- Apply case folding only where case-insensitive behavior is required.
- Prefer :cpp:type:`String <erbsland::text::String>` parameters when a function only inspects text.
- Keep string widths consistent whenever possible.
- Use ``startsWith()``, ``endsWith()``, ``contains()``, and ``count()`` instead of hand-written search loops.
- Validate text before comparison whenever malformed input must be rejected.

These guidelines keep comparison code predictable, efficient, and consistent with the rest of the Erbsland Core string
API.
