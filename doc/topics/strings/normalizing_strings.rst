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
Normalization gives such text a predictable representation for comparisons, identifiers, indexes, and interchange.
Erbsland Core supports the four standard Unicode normalization forms for UTF-8, UTF-16, and UTF-32 without external
dependencies.

Choosing a Normalization Form
=============================

Canonical normalization preserves the distinction between compatible characters.
Use :cpp:enumerator:`NormalizationForm::Nfc <erbsland::text::NormalizationForm::Nfc>` for ordinary stored or exchanged
text: it canonically decomposes characters, orders combining marks, and composes eligible sequences again.
Use :cpp:enumerator:`NormalizationForm::Nfd <erbsland::text::NormalizationForm::Nfd>` when a decomposed representation
is useful for further code-point processing.

Compatibility normalization additionally replaces compatibility variants with their underlying representation.
For example, NFKC and NFKD turn the ligature ``ﬃ`` into the three letters ``ffi``.
This can be useful when preparing identifiers or search keys, but it can discard distinctions that matter to the
original text.
:cpp:enumerator:`NormalizationForm::Nfkc <erbsland::text::NormalizationForm::Nfkc>` composes the result, while
:cpp:enumerator:`NormalizationForm::Nfkd <erbsland::text::NormalizationForm::Nfkd>` leaves it decomposed.

The form is always explicit so the call site records whether compatibility changes are intended.

Normalizing Values and Editors
==============================

Every width-specific value supports ``normalized(form)`` and returns the same string width.
Editors support both the copy form and the in-place ``normalize(form)`` operation.
The common :cpp:type:`String <erbsland::text::String>` and
:cpp:type:`StringEditor <erbsland::text::StringEditor>` aliases therefore normalize UTF-8 text directly.

.. code-block:: cpp

    const auto composed = String{"A\u030A"_el}.normalized(NormalizationForm::Nfc);

    auto key = StringEditor{"ﬃeld"_el};
    key.normalize(NormalizationForm::Nfkc);

If valid input is already in the requested form, no character data is copied.
A value result shares the source allocation, an editor result shares the existing copy-on-write allocation, and an
in-place editor keeps its storage, capacity, aliases, and sensitivity mark unchanged.
Changed UTF-8 output also preserves a sensitive-storage mark from its source.

Normalization processes the input as bounded canonical sequences and uses constant working memory.
It allocates replacement storage only after the first sequence that actually changes, then copies unchanged encoded
ranges directly and encodes only changed ranges.
Consequently, normalizing a large value that is already in the requested form performs no character-data allocation.

Malformed Encoded Text
======================

Normalization uses the same tolerant decoding model as other string operations.
Malformed UTF-8 or UTF-16 units and invalid UTF-32 values are decoded as U+FFFD, the Unicode replacement character.
Because replacing malformed input changes the encoded text, the result receives newly encoded, valid storage even if the
remaining code points were already normalized.

Defensive Combining-Mark Limit
==============================

Normalization accepts at most 30 consecutive non-starters in one canonical sequence.
The count is applied after recursive canonical or compatibility decomposition, so a single source code point cannot hide
additional combining marks through decomposition.

When a 31st non-starter is encountered, the complete canonical sequence, including its starter and all following
non-starters, is replaced with one U+FFFD.
A leading sequence containing only non-starters is replaced in the same way.
Processing resumes at the next starter.

This is a deliberate defensive deviation from unrestricted Unicode normalization for hostile network and identifier
input.
Sequences within the limit follow the selected Unicode 17 normalization form and the official conformance data.

Normalization Boundaries
========================

Normalized strings are not generally closed under concatenation.
Joining two normalized values can place a starter and combining mark next to each other, or put combining marks into a
new order that is not normalized.
Normalize the completed result when concatenated text must satisfy a normalization invariant.

The normalization behavior and generated tables follow Unicode 17 subject to the defensive combining-mark limit above.
The tables are independent of the Unicode Light character database and are linked into an ordinary static-library
consumer only when one of the normalization methods is referenced.
