.. index::
    !single: Comparing Whole and Partial Strings
    single: Whole-String Comparison
    single: Character Comparison Rules
    single: Partial-String Comparison
    single: Binary String Comparison
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
    single: compareBinary
    single: compareCaseFolded
    single: compareAsciiFolded
    single: compareIdentifier
    single: startsWith
    single: endsWith
    single: contains
    single: count
    single: isEncodingValid
    single: encodingValidOrThrow
    single: Case-Insensitive Comparison
    single: Identifier Comparison
    single: Partial StringEditor Tests
    single: Encoding Validation

***********************************
Comparing Whole and Partial Strings
***********************************

Two strings can be “the same” in several useful but incompatible ways.
A protocol token may require exact character matching, a user-facing lookup may ignore case, and an index may need to
distinguish the bytes used to store the text.
This page explains how to choose a comparison rule for your data, apply it to whole strings or selected parts, and
handle malformed text at an untrusted boundary.

The examples use the common UTF-8 :cpp:type:`String <erbsland::text::String>`; the same model applies to the matching
width-specific values, editors, and literals.

Decoded String Equality and Ordering
====================================

When you compare strings with ``==``, ``<``, or the other comparison operators, you compare their decoded Unicode code
points.
This gives you a consistent character order across UTF-8, UTF-16, and UTF-32, which is useful for keys, configuration
values, and protocol text.
Uppercase and lowercase letters remain distinct, as do accented and unaccented characters.
Comparison does not normalize text or apply the sorting rules of a particular language.

The :cpp:func:`compare() <erbsland::text::U8String::compare>` method provides the same comparison as a
``std::strong_ordering`` result when you need to distinguish less, equal, and greater explicitly.

For UTF-8 code, compare a :cpp:type:`String <erbsland::text::String>` directly with another ``String``, a
:cpp:type:`StringEditor <erbsland::text::StringEditor>`, or a ``"_el"`` literal.

The UTF-16 and UTF-32 variants follow the same pattern with values of the same width:
:cpp:class:`U16String <erbsland::text::U16String>` works with
:cpp:class:`U16StringEditor <erbsland::text::U16StringEditor>` or ``u"..."_el``, while
:cpp:class:`U32String <erbsland::text::U32String>` works with
:cpp:class:`U32StringEditor <erbsland::text::U32StringEditor>` or ``U"..."_el``.

.. erbsland-demo::
    :source: text/String/WholeStringComparison.cpp
    :exec: text/string --demo WholeStringComparison
    :sha256: 2afea925708028c080dee00c9d38033efed4386ed057a979d26122a103ad9913

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mtag == sameString .............: yes
        tag == "lišejník"_el ..........: yes
        tag != "tuleň"_el .............: yes
        tag <=> "tuleň"_el ............: less
        tag.compare("LIŠEJNÍK"_el) ....: greater
        tag < "tuleň"_el ..............: yes
        tag <= sameString .............: yes
        tag >= "arka"_el ..............: yes
        u8Habitat == u8 literal .......: yes
        u16Habitat == u16 literal .....: yes
        u32Habitat == u32 literal .....: yes␛[0m

.. erbsland-demo-end::

When text arrives in different encodings, you can compare it through
:cpp:class:`AnyString <erbsland::text::AnyString>`.
Its :cpp:func:`compare() <erbsland::text::AnyString::compare>` method decodes each string in its own encoding, so the
UTF-8 and UTF-16 versions of ``ledová kra`` compare equal without creating converted strings.
This is useful when your code accepts text from several sources but only needs to compare its characters.

Custom Character Comparison Rules
=================================

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
    :sha256: 16f4d22e152980f19d45d3bae455b707d6e206b3dde57f56b5be77997e9091f4

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0msnowyOwl.compare(snowyOwlLower) .........................: less
        snowyOwl.compare(..., Char::compareCaseFolded) ..........: equal
        asciiLabel.compare(..., Char::compareAsciiFolded) .......: equal
        snowyOwl.compare(..., Char::compareAsciiFolded) .........: less
        identifier.compare(..., Char::compareIdentifier) ........: equal␛[0m

.. erbsland-demo-end::

Prefix, Suffix, Substring, and Occurrence Tests
===============================================

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
    :sha256: e63f929527861d84fed9c21a204940a8c2c571bab9a4d4aaa47c9f2e0aa5f49c

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mObservation Log:
        Lachtan: LEDOVÁ KRA; lachtan: tiché moře; tuleň: ledová kra; mrož: severní útes

        startsWith("Lachtan"_el) ................: yes
        endsWith("severní útes"_el) .............: yes
        contains("tiché moře"_el) ...............: yes
        count("ledová kra"_el) ..................: 1

        startsWith("lachtan"_el, case-folded) ...: yes
        contains("LEDOVÁ KRA"_el, case-folded) ..: yes
        count("lachtan"_el, case-folded) ........: 2
        count("ledová kra"_el, case-folded) .....: 2
        count(""_el) ............................: 0␛[0m

.. erbsland-demo-end::

Comparing Stored Bytes
======================

Some tasks depend on how text is stored.
For example, an index of encoded values may need to keep distinct byte sequences as separate entries even when they
decode to the same text.
The :cpp:func:`compareBinary() <erbsland::text::U8String::compareBinary>` method gives you a fast ordering for this
purpose by comparing bytes directly without decoding.
Like ``compare()``, it returns ``std::strong_ordering`` and is available on all three string widths and their editors.
The same-width string, editor, and literal combinations shown earlier also work for binary comparison.

Bytes are compared as unsigned values, with the first difference deciding the order.
When one byte sequence is a prefix of the other, the shorter sequence comes first: ``abc`` precedes ``abcd``.
Only the content of the string or slice participates.
An embedded NUL is part of that content and does not stop comparison; a storage terminator or unused editor capacity
does not affect the result.

For valid UTF-8, byte order agrees with decoded code-point order.
UTF-16 and UTF-32 store each code unit in several bytes, so their binary ordering depends on the machine's native byte
order and can differ from character ordering.
If you need sorted output to be reproducible across machines or independent of the encoding, ordinary decoded comparison
provides that guarantee.

For mixed encodings, :cpp:func:`AnyString::compareBinary() <erbsland::text::AnyString::compareBinary>` and
:cpp:func:`AnyStringEditor::compareBinary() <erbsland::text::AnyStringEditor::compareBinary>` also distinguish the
encoding itself.
The UTF-8 and UTF-16 versions of ``ledová kra`` therefore compare unequal, although they compare equal with
``compare()``.
Nonempty strings are grouped by encoding in the order UTF-8, UTF-16, UTF-32, then ordered by their bytes within each
group.
Empty strings compare equal regardless of their encoding and sort before every nonempty string.

Validating Untrusted Text
=========================

Text received from a file or another application may contain malformed UTF-8, UTF-16, or UTF-32 sequences.
Ordinary comparison and search can still inspect it: decoding substitutes the Unicode replacement character ``U+FFFD``,
available as :cpp:func:`Char::replacement() <erbsland::text::Char::replacement>`, for malformed input.
This tolerant behavior is useful when you want to display or inspect damaged text.

The replacement also means that different malformed byte sequences can compare equal after decoding.
Binary comparison preserves those byte differences, but it does not check whether either string is valid text.
When an identifier or protocol value must contain valid Unicode, validation belongs before comparison, whichever
comparison rule you choose.

For the common :cpp:type:`String <erbsland::text::String>` alias and other
:cpp:class:`U8String <erbsland::text::U8String>` values,
:cpp:func:`isEncodingValid() <erbsland::text::U8String::isEncodingValid>` lets you reject malformed input at that boundary.
The same method is available for UTF-16 and UTF-32 strings.
Use ``encodingValidOrThrow()`` when validation failures should propagate as exceptions with an error position.
Once the input has passed validation, you can apply the matching rule your application requires without accepting
malformed text through replacement-character equality.
