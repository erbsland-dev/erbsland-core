..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Base-N Encoding
    single: Base64
    single: Base32
    single: Base16
    single: ByteBlock; Text Encoding

*********************************
Encoding Byte Data as Base-N Text
*********************************

Imagine a sound sample identifier stored as bytes.
You want to put it in a configuration file, but the file contains text.
Writing the bytes as if they were characters would mix two different things: the value of the bytes and the encoding
used for text.

Base-N encoding gives the bytes a reversible text representation.
It takes groups of bits and writes each group as a character from a chosen alphabet.
The reader uses the same alphabet to recover the original bytes.

This introduction explains the idea and shows one complete round trip.
The next pages look more closely at :doc:`writing Base-N text <base_n_encoding>` and
:doc:`reading it safely <base_n_decoding>`.

Why the Base Matters
====================

The ``N`` tells you how many symbols an alphabet contains.
Base16 has 16 symbols, so each symbol can represent four bits.
Base32 represents five bits per symbol, and Base64 represents six.
That is why a five-byte value takes ten Base16 characters but only eight characters in padded Base64.

The alphabet is only part of the format.
Standard Base64 uses ``+`` and ``/`` for its last two values.
Base64url uses ``-`` and ``_`` instead, which makes those characters suitable for URL fields.
Some formats require padding at the end; some omit it.
Line-oriented formats may also wrap the encoded text.

These details must come from the field or protocol you are working with.
If one side writes Base64url and the other expects ordinary Base64, the text may be rejected.
The same applies to padding and line layout.

One Complete Round Trip
=======================

The example starts with a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` containing the bytes of ``kaiku``.
It writes those bytes as Base64 text, as if the result were placed in a text-only field.
It then decodes the text with the same :cpp:class:`BaseNFormat <erbsland::text::base_n::BaseNFormat>` and checks that
the recovered bytes match the original.

.. erbsland-demo::
    :source: text/BaseN/BaseNDemos.cpp
    :function-blocks: roundTripSoundTag
    :function-blocks-sha256: 6da09d9325a19c80e9182c4de90c50a5d9813dc9542b222ee282845ea17127ca
    :exec: text/base_n --demo RoundTripSoundTag
    :source-sha256: 25ad05fcdf84c76b9b65f271db932811662bf5f38574ef658c1444b657225e11

.. code-block:: cpp

    void roundTripSoundTag() {
        // Treat the sound label as bytes that need to pass through a text field.
        const auto original = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
        const auto format = el::base_n::BaseNFormat::base64();
        const auto encoded = el::base_n::BaseNEncoder{original, format}.toString();

        // Decode with the same format and compare the recovered bytes.
        const auto recovered = el::base_n::BaseNDecoder{encoded, format}.toDataOrThrow();
        el::io::printLine("Text field: "_el, encoded);
        el::io::printLine("Bytes recovered: "_el, recovered == original);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Text field: a2Fpa3U=
    Bytes recovered: true

.. erbsland-demo-end::

The encoded value is text, but it still represents bytes.
Base-N does not interpret arbitrary bytes as Unicode text, and it does not hide their contents.
Anyone who knows the format can decode them.

When you write a field, start with :doc:`the encoding workflow <base_n_encoding>`.
When you accept a field from elsewhere, :doc:`the decoding workflow <base_n_decoding>` explains validation, size limits,
and error handling.
