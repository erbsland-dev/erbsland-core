..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Unicode Normalization
    single: NFC
    single: NFD
    single: NFKC
    single: NFKD
    single: NormalizationForm

***************************
Normalizing Unicode Strings
***************************

Unicode can represent text that looks identical with different code-point sequences.
An accented letter may be one composed code point or a base letter followed by a combining mark.
Without a normalization policy, those spellings can compare differently, occupy different map keys, or produce an
unexpected result when fragments meet.

Normalization gives a boundary one deliberate representation.
This page helps you choose between canonical and compatibility forms, apply that choice to a read-only value or an
editing workflow, and recognize the concatenation boundary where a second normalization pass is necessary.

Choose the Equivalence Your Data Needs
======================================

NFC and NFD preserve canonical meaning.
NFC composes eligible sequences and is the usual choice for stored or exchanged text.
NFD leaves the result decomposed, which can be useful when an algorithm works directly with combining sequences.

NFKC and NFKD additionally replace compatibility variants with their underlying representation.
For example, they can turn full-width letters into ordinary ASCII letters or a ligature into its component letters.
That is useful for search keys and identifier preparation, but it can discard distinctions that matter in the original
text.
NFKC composes the result; NFKD leaves it decomposed.

.. erbsland-demo::
    :source: text/String/NormalizationForms.cpp
    :exec: text/string --demo NormalizationForms
    :source-sha256: 1eaae6aca1abcacd4987b4fcf5c1a9aeb40d2f66f5877f15acee31f60befc0e9

.. code-block:: cpp

    /// Select canonical or compatibility normalization according to the purpose of the text.
    void normalizationForms() {
        const auto decomposed = el::String{"Cafe\u0301"_el};
        const auto nfc = decomposed.normalized(el::NormalizationForm::Nfc);
        const auto nfd = nfc.normalized(el::NormalizationForm::Nfd);

        const auto fullWidth = el::String{"ＳＥＮＳＯＲ－７"_el};
        const auto nfkc = fullWidth.normalized(el::NormalizationForm::Nfkc);
        const auto nfkd = fullWidth.normalized(el::NormalizationForm::Nfkd);

        el::io::printLine("Canonical forms:"_el);
        el::io::printLine("  source : "_el, decomposed, " (", decomposed.characterLength(), " code points)"_el);
        el::io::printLine("  NFC ...: "_el, nfc, " (", nfc.characterLength(), " code points)"_el);
        el::io::printLine("  NFD ...: "_el, nfd, " (", nfd.characterLength(), " code points)"_el);
        el::io::printLine("Compatibility forms:"_el);
        el::io::printLine("  source : "_el, fullWidth);
        el::io::printLine("  NFKC ..: "_el, nfkc);
        el::io::printLine("  NFKD ..: "_el, nfkd);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Canonical forms:
      source : Café (5 code points)
      NFC ...: Café (4 code points)
      NFD ...: Café (5 code points)
    Compatibility forms:
      source : ＳＥＮＳＯＲ－７
      NFKC ..: SENSOR-7
      NFKD ..: SENSOR-7

.. erbsland-demo-end::

Normalize a Value or a Mutable Workflow
=======================================

For an ordinary transformation, call ``String::normalized(form)`` and keep the source unchanged.
The returned value has the same string width, so UTF-16 and UTF-32 workflows do not need an intermediate UTF-8
conversion.

When normalization is one step in a genuine multi-stage edit, ``StringEditor::normalize(form)`` changes the working
value in place.
This keeps the mutable intent local; once editing is complete, expose the result as a read-only ``String`` again.

.. erbsland-demo::
    :source: text/StringEditor/NormalizeText.cpp
    :exec: text/string_editor --demo NormalizeText
    :source-sha256: b7ef58bf0458c383cd8590fa14237b9ed411d4c1542511aeb4d6bd6e502da882

.. code-block:: cpp

    /// `normalize()` canonicalizes an editor in place. This decomposed Japanese
    /// element name uses a combining voiced mark, which NFC composes with the
    /// preceding katakana character.
    void normalizeText() {
        auto elementName = el::StringEditor{"カ\u3099リウム"_el};

        el::io::printLine("Before NFC: "_el, elementName);
        elementName.normalize(el::NormalizationForm::Nfc);
        el::io::printLine("After NFC : "_el, elementName);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Before NFC: カリウム
    After NFC : ガリウム

.. erbsland-demo-end::

If malformed input must be rejected, validate its encoding before normalization.
Like other decoded string operations, normalization otherwise replaces malformed encoded units with the Unicode
replacement character.
The precise behavior for malformed and unusually long combining sequences belongs in the
:doc:`technical string reference </reference/text/strings>`.

Normalize After Joining Text
============================

Two independently normalized strings do not necessarily remain normalized after concatenation.
A starter at the end of one fragment and a combining mark at the beginning of the next can form a new canonical
sequence, and adjacent combining marks may need a new order.

Normalize the completed value when the result must satisfy a normalization invariant.
There is no need to normalize every fragment first unless those fragments also cross a boundary on their own.

.. erbsland-demo::
    :source: text/String/NormalizeJoinedText.cpp
    :exec: text/string --demo NormalizeJoinedText
    :source-sha256: bd765f619bedc94281eeef10367cf22c9455bcb8e709a61ea941161b153f7e9a

.. code-block:: cpp

    /// Normalize after joining when a new canonical sequence can form at a fragment boundary.
    void normalizeJoinedText() {
        const auto station = el::String{"Meetpunt A"_el}.normalized(el::NormalizationForm::Nfc);
        const auto ring = el::String{"\u030A"_el}.normalized(el::NormalizationForm::Nfc);
        const auto joined = el::String::fromJoined({station, ring});
        const auto normalized = joined.normalized(el::NormalizationForm::Nfc);

        el::io::printLine("Separately normalized : "_el, joined);
        el::io::printLine("After joining .........: "_el, normalized);
        el::io::printLine("Code points ..........: "_el, joined.characterLength(), " -> ", normalized.characterLength());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Separately normalized : Meetpunt Å
    After joining .........: Meetpunt Å
    Code points ..........: 11 -> 10

.. erbsland-demo-end::
