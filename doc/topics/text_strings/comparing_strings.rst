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

Two strings can be “the same” in several useful but incompatible ways.
A protocol token may require exact equality, a user-facing lookup may ignore case, and an identifier may need its own
restricted folding rule.
Choosing the comparison deliberately matters more than choosing a clever function name.

Erbsland Core compares decoded Unicode code points, so the rule behaves consistently across UTF-8, UTF-16, and UTF-32.
This page begins with predictable exact equality and ordering, then introduces character comparison functions, partial
tests, and the encoding validation required at an untrusted boundary.
The examples use the common UTF-8 :cpp:type:`String <erbsland::text::String>`; the same model applies to the matching
width-specific values, editors, and literals.

Begin with Exact Equality and Ordering
======================================

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
    :source-sha256: e445d26dfaf14c5f3ab743eb51b359de7e8abe678cf7b922805899cf98630288

.. code-block:: cpp

    /// `String` compares whole strings by decoded Unicode code point.
    /// Use the comparison operators for ordinary equality and ordering. Use `compare()`
    /// when you need the `std::strong_ordering` result explicitly or want to pass a
    /// character comparison function. The common UTF-8 aliases compare with other UTF-8
    /// strings and `"_el"` literals; UTF-16 and UTF-32 variants follow the same same-width pattern.
    void wholeStringComparison() {
        const auto tag = el::String{"lišejník"_el};
        const auto sameString = el::String{"lišejník"_el};
        const auto booleanFormat = el::BooleanFormat::yesNo();

        // Compare a string with another string and a literal.
        el::io::printLine("tag == sameString .............: "_el, booleanFormat, tag == sameString);
        el::io::printLine("tag == \"lišejník\"_el ..........: "_el, booleanFormat, tag == "lišejník"_el);
        el::io::printLine("tag != \"tuleň\"_el .............: "_el, booleanFormat, tag != "tuleň"_el);

        // Ordering uses decoded code points, so uppercase and lowercase letters differ.
        auto ordering = tag <=> "tuleň"_el;
        el::io::printLine("tag <=> \"tuleň\"_el ............: "_el, el::toString(ordering));
        ordering = tag.compare("LIŠEJNÍK"_el);
        el::io::printLine("tag.compare(\"LIŠEJNÍK\"_el) ....: "_el, el::toString(ordering));
        el::io::printLine("tag < \"tuleň\"_el ..............: "_el, booleanFormat, tag < "tuleň"_el);
        el::io::printLine("tag <= sameString .............: "_el, booleanFormat, tag <= sameString);
        el::io::printLine("tag >= \"arka\"_el ..............: "_el, booleanFormat, tag >= "arka"_el);

        // Width-specific strings use the same operations with values of the same width.
        const auto u8Habitat = el::U8String{u8"ledová kra"_el};
        const auto u16Habitat = el::U16String{u"ledová kra"_el};
        const auto u32Habitat = el::U32String{U"ledová kra"_el};

        el::io::printLine("u8Habitat == u8 literal .......: "_el, booleanFormat, u8Habitat == u8"ledová kra"_el);
        el::io::printLine("u16Habitat == u16 literal .....: "_el, booleanFormat, u16Habitat == u"ledová kra"_el);
        el::io::printLine("u32Habitat == u32 literal .....: "_el, booleanFormat, u32Habitat == U"ledová kra"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    tag == sameString .............: yes
    tag == "lišejník"_el ..........: yes
    tag != "tuleň"_el .............: yes
    tag <=> "tuleň"_el ............: less
    tag.compare("LIŠEJNÍK"_el) ....: greater
    tag < "tuleň"_el ..............: yes
    tag <= sameString .............: yes
    tag >= "arka"_el ..............: yes
    u8Habitat == u8 literal .......: yes
    u16Habitat == u16 literal .....: yes
    u32Habitat == u32 literal .....: yes

.. erbsland-demo-end::

Change the Rule, Not the Stored Text
====================================

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
Pass the rule at the comparison point instead of silently changing every stored value; that keeps exceptional matching
semantics visible and lets different fields use the rules their domains require.

.. erbsland-demo::
    :source: text/String/ComparisonFunctions.cpp
    :exec: text/string --demo ComparisonFunctions
    :source-sha256: 7afdca4b37258bdb78cc605681b9a1b3249f6d8f254aa8495861d31f48019782

.. code-block:: cpp

    /// `compare()` accepts character comparison functions for specialized matching rules.
    /// Use `Char::compareCaseFolded` for Unicode-aware case-insensitive text, use
    /// `Char::compareAsciiFolded` when ASCII-only folding is enough, and use
    /// `Char::compareIdentifier` for identifier-like names where ASCII case and spaces
    /// versus underscores should compare equally.
    void comparisonFunctions() {
        const auto snowyOwl = el::String{"SOVA SNĚŽNÍ"_el};
        const auto snowyOwlLower = el::String{"sova sněžní"_el};
        const auto asciiLabel = el::String{"Polar Fox"_el};
        const auto asciiLabelLower = el::String{"polar fox"_el};
        const auto identifier = el::String{"Arctic Fox Trail"_el};
        const auto normalizedIdentifier = el::String{"arctic_fox_trail"_el};

        // Regular comparison is exact and uses decoded code points.
        el::io::printLine(
            "snowyOwl.compare(snowyOwlLower) .........................: "_el,
            el::toString(snowyOwl.compare(snowyOwlLower)));

        // Unicode case folding handles non-ASCII letters such as `Ě` and `ě`.
        el::io::printLine(
            "snowyOwl.compare(..., Char::compareCaseFolded) ..........: "_el,
            el::toString(snowyOwl.compare(snowyOwlLower, el::Char::compareCaseFolded)));

        // ASCII folding is small and fast, but only changes A-Z to a-z.
        el::io::printLine(
            "asciiLabel.compare(..., Char::compareAsciiFolded) .......: "_el,
            el::toString(asciiLabel.compare(asciiLabelLower, el::Char::compareAsciiFolded)));
        el::io::printLine(
            "snowyOwl.compare(..., Char::compareAsciiFolded) .........: "_el,
            el::toString(snowyOwl.compare(snowyOwlLower, el::Char::compareAsciiFolded)));

        // Identifier comparison is useful for normalized keys or configuration-style names.
        el::io::printLine(
            "identifier.compare(..., Char::compareIdentifier) ........: "_el,
            el::toString(identifier.compare(normalizedIdentifier, el::Char::compareIdentifier)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    snowyOwl.compare(snowyOwlLower) .........................: less
    snowyOwl.compare(..., Char::compareCaseFolded) ..........: equal
    asciiLabel.compare(..., Char::compareAsciiFolded) .......: equal
    snowyOwl.compare(..., Char::compareAsciiFolded) .........: less
    identifier.compare(..., Char::compareIdentifier) ........: equal

.. erbsland-demo-end::

Ask a Smaller Question Than Whole-string Equality
=================================================

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
    :source-sha256: c4ddb0e309fb734706e8f0fe3612b08dd88c736d7857ffe8b665b5a91522f1b2

.. code-block:: cpp

    /// Use `startsWith()`, `endsWith()`, `contains()`, and `count()` to test parts of a string.
    /// These methods use the same comparison rules as whole-string comparison and accept the
    /// same optional character comparison function. Prefix and suffix tests are efficient
    /// even for large strings because only the required edge of the string is inspected.
    void partialStringComparison() {
        const auto observationLog =
            el::String{"Lachtan: LEDOVÁ KRA; lachtan: tiché moře; tuleň: ledová kra; mrož: severní útes"_el};
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
        el::io::printLine(
            "startsWith(\"lachtan\"_el, case-folded) ...: "_el,
            booleanFormat,
            observationLog.startsWith("lachtan"_el, el::Char::compareCaseFolded));
        el::io::printLine(
            "contains(\"LEDOVÁ KRA\"_el, case-folded) ..: "_el,
            booleanFormat,
            observationLog.contains("LEDOVÁ KRA"_el, el::Char::compareCaseFolded));
        el::io::printLine(
            "count(\"lachtan\"_el, case-folded) ........: "_el,
            observationLog.count("lachtan"_el, el::Char::compareCaseFolded));
        el::io::printLine(
            "count(\"ledová kra\"_el, case-folded) .....: "_el,
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

Validate Untrusted Encodings Before Comparison
==============================================

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
At an untrusted boundary, decide whether malformed text is acceptable before choosing that tolerant behavior; comparison
must not accidentally become validation.
