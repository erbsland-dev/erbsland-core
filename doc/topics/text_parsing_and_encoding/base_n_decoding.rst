..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Base-N; Decoding
    single: BaseNDecoder
    single: Base64; Validation

******************************
Reading Base-N Text into Bytes
******************************

An encoded field may come from a file, a network message, or another application's output.
Before using its bytes, you need to know whether the text follows the expected format and whether the decoded value fits
the field's size limit.

:cpp:class:`BaseNDecoder <erbsland::text::base_n::BaseNDecoder>` performs those checks while recovering a
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
This page explains the two failure-handling methods, the accepted whitespace and padding policies, and what strict
decoding rejects.
For the basic idea behind Base-N, start with :doc:`/topics/text_parsing_and_encoding/encoding_byte_data_as_base_n`.

Decode with a Byte Limit
========================

Construct a decoder from the text field.
It accepts the Core string widths and uses padded Base64 by default.
When the protocol specifies another alphabet or layout, pass the same
:cpp:class:`BaseNFormat <erbsland::text::base_n::BaseNFormat>` that describes that field.
The decoder's ``format()`` accessor lets you inspect the selected format.

Both decoding methods accept a maximum decoded byte length.
Set it at an input boundary when the field has a known limit.
The limit applies to the resulting bytes, rather than to the number of characters in the encoded text.

Here a five-byte value fits a five-byte limit.
The same text fails with a four-byte limit, while a separate malformed value fails because it is not valid Base64.

.. erbsland-demo::
    :source: text/BaseN/BaseNDemos.cpp
    :function-blocks: decodeSoundTag
    :function-blocks-sha256: 675dce90b00fcd9712e1b7005756d2159954d6a05033b4df5b8b0532f87c5d39
    :exec: text/base_n --demo DecodeSoundTag
    :source-sha256: 25ad05fcdf84c76b9b65f271db932811662bf5f38574ef658c1444b657225e11

.. code-block:: cpp

    void decodeSoundTag() {
        // Decode a small value with the largest byte count this field permits.
        const auto decoded = el::base_n::BaseNDecoder{"a2Fpa3U="_el}.toData(el::ByteLength{5U});
        el::io::printLine("Decoded bytes: "_el, decoded->length().toSizeT());

        // A malformed payload and an oversized payload both fail in the optional form.
        const auto invalid = el::base_n::BaseNDecoder{"a2Fpa3U!"_el}.toData();
        const auto tooLarge = el::base_n::BaseNDecoder{"a2Fpa3U="_el}.toData(el::ByteLength{4U});
        el::io::printLine("Malformed accepted: "_el, invalid.has_value());
        el::io::printLine("Oversized accepted: "_el, tooLarge.has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Decoded bytes: 5
    Malformed accepted: false
    Oversized accepted: false

.. erbsland-demo-end::

``toData()`` returns an optional byte block.
An empty optional means either the text was malformed or the decoded size was too large.
This is useful when a parser only needs to accept or reject a field.

``toDataOrThrow()`` keeps the reason available.
Malformed text raises :cpp:class:`ParseError <erbsland::err::ParseError>`.
An exceeded byte limit raises :cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>`.
Use this form when diagnostics or different recovery paths depend on the reason.

Decide Which Characters to Ignore
=================================

The format's ``whitespace()`` set tells the decoder which characters it may skip between encoded symbols.
The predefined formats accept ASCII whitespace.
That is why a wrapped Base64 payload can be decoded with its matching format.

``setWhitespace()`` replaces the set.
The example accepts a tilde between digits and then shows that an ordinary space is no longer ignored.

.. erbsland-demo::
    :source: text/BaseN/BaseNDemos.cpp
    :function-blocks: chooseWhitespace
    :function-blocks-sha256: 6a9ca3de4b9c4db992d78761a9860ef5b3e07725affa96030674aa712491fe42
    :exec: text/base_n --demo ChooseWhitespace
    :source-sha256: 25ad05fcdf84c76b9b65f271db932811662bf5f38574ef658c1444b657225e11

.. code-block:: cpp

    void chooseWhitespace() {
        auto format = el::base_n::BaseNFormat::base64();
        format.setWhitespace(el::CharSet{U'~'});
        el::io::printLine("Tilde accepted: "_el, el::base_n::BaseNDecoder{"a2Fp~a3U="_el, format}.toData().has_value());
        el::io::printLine("Space accepted: "_el, el::base_n::BaseNDecoder{"a2Fp a3U="_el, format}.toData().has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Tilde accepted: true
    Space accepted: false

.. erbsland-demo-end::

The ignored set must not overlap the alphabet or the padding character.
If the same character had two meanings, the decoder could not reliably decide whether it was data or a separator.
The format validates this rule when you change the set.

Choose a Padding Policy
=======================

Standard Base32 and Base64 formats require canonical padding when the final group needs it.
For example, the Base64 text for ``kaiku`` ends with one ``=``.
The format's ``padding()`` character tells the decoder what to recognize, and the
:cpp:enumerator:`RequirePadding <erbsland::text::base_n::BaseNFormatFlag::RequirePadding>` flag says whether it must be
present.

Some protocols allow the padding to be omitted.
For such a field, clear ``RequirePadding`` on the matching format.
The next example shows the difference from the default policy.

.. erbsland-demo::
    :source: text/BaseN/BaseNDemos.cpp
    :function-blocks: decodePaddingPolicy
    :function-blocks-sha256: 126326c002310b649e075a25205d57cc234ac3eb9a63db1faf9946b39cd9c480
    :exec: text/base_n --demo DecodePaddingPolicy
    :source-sha256: 25ad05fcdf84c76b9b65f271db932811662bf5f38574ef658c1444b657225e11

.. code-block:: cpp

    void decodePaddingPolicy() {
        auto format = el::base_n::BaseNFormat::base64();
        el::io::printLine(
            "Default accepts a2Fpa3U: "_el, el::base_n::BaseNDecoder{"a2Fpa3U"_el, format}.toData().has_value());

        format.clearFlags(el::base_n::BaseNFormatFlag::RequirePadding);
        el::io::printLine(
            "Optional padding accepts a2Fpa3U: "_el, el::base_n::BaseNDecoder{"a2Fpa3U"_el, format}.toData().has_value());
        el::io::printLine(
            "Incorrect padding accepted: "_el, el::base_n::BaseNDecoder{"a2Fpa3U=="_el, format}.toData().has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Default accepts a2Fpa3U: false
    Optional padding accepts a2Fpa3U: true
    Incorrect padding accepted: false

.. erbsland-demo-end::

Optional padding does not mean arbitrary padding.
If padding is present, the decoder still requires the canonical number of characters in the correct position.
The encoder's ``EmitPadding`` flag is a separate setting; it controls what is written, as explained in
:doc:`/topics/text_parsing_and_encoding/base_n_encoding`.

What Strict Decoding Protects
=============================

After removing the configured ignored characters, the decoder examines the complete encoded value.
It rejects characters outside the selected alphabet and alphabet characters that appear after padding.
It also rejects missing required padding, non-canonical padding, and impossible final group lengths.

The last encoded character may contain a few bit positions that are not used by the recovered bytes.
Those unused positions must be zero.
Rejecting nonzero trailing bits avoids accepting multiple textual spellings for the same bytes.
That matters when an encoded value is stored, signed, or compared with another value later.

The format must match the source.
Ordinary Base64 and Base64url have different alphabets, even though many short encoded values happen to look the same.
For wrapped output, the line separator must be part of the format's ignored whitespace set.
When you control both sides, use the same format object or the same factory and settings for writing and reading.
