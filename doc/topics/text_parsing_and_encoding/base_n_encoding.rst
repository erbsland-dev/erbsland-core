..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Base-N; Encoding
    single: BaseNEncoder
    single: BaseNFormat

****************************
Writing Bytes as Base-N Text
****************************

You have a byte block and need to place it in a text field.
The field's format determines how the bytes should look as text: perhaps uppercase hexadecimal, perhaps URL-safe Base64,
or perhaps Base64 wrapped into lines.

:cpp:class:`BaseNEncoder <erbsland::text::base_n::BaseNEncoder>` handles the conversion.
:cpp:class:`BaseNFormat <erbsland::text::base_n::BaseNFormat>` describes the alphabet and layout it should write.
This page starts with a simple encoding, then shows how to choose and adjust that format.
For the underlying idea and a full round trip, see
:doc:`/topics/text_parsing_and_encoding/encoding_byte_data_as_base_n`.

Start with a Byte Block
=======================

The encoder takes a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
It does not need to know whether those bytes came from a file, a digest, or some other source.
If you omit the format argument, it uses canonical padded Base64.

Here the input bytes spell ``kaiku``, the Finnish word for an echo.
The example writes their Base64 representation and shows the available output string widths.

.. erbsland-demo::
    :source: text/BaseN/BaseNDemos.cpp
    :show: f:encodeSoundTag
    :exec: text/base_n --demo EncodeSoundTag
    :sha256: ef26014d5cddc92eba8747fdcd622900a777d7e90f265c38bd5ba1190db37976

.. code-block:: cpp

    void encodeSoundTag() {
        // Encode the UTF-8 bytes of a short Finnish sound label.
        const auto bytes = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
        const auto encoded = el::base_n::BaseNEncoder{bytes}.toString();
        el::io::printLine("Base64: "_el, encoded);

        // The same encoder can produce UTF-16 and UTF-32 text when that is the destination's string width.
        const auto encoder = el::base_n::BaseNEncoder{bytes};
        el::io::printLine("UTF-16 characters: "_el, encoder.toU16String().characterLength().toSizeT());
        el::io::printLine("UTF-32 characters: "_el, encoder.toU32String().characterLength().toSizeT());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mBase64: a2Fpa3U=
        UTF-16 characters: 8
        UTF-32 characters: 8␛[0m

.. erbsland-demo-end::

``toString()`` returns the ordinary UTF-8 :cpp:type:`String <erbsland::text::String>`.
If the receiving API needs a specific width, use ``toU8String()``, ``toU16String()``, or ``toU32String()``.
The Base-N characters stay the same; the destination string type changes.
You can inspect the encoder's selected format through ``format()``.

Choose the Alphabet
===================

A standard format factory is the clearest starting point when a protocol names a familiar encoding.
``BaseNFormat::base16()`` writes uppercase hexadecimal digits.
``base32()`` and ``base32Hex()`` select the two RFC 4648 Base32 alphabets.
``base64()`` and ``base64Url()`` select ordinary and URL-safe Base64. The default constructor and ``defaultFormat()``
both select ``base64()``.

The next example uses two bytes that expose the difference between the Base64 alphabets.
Ordinary Base64 produces ``+/8=``; Base64url produces ``-_8=``.
The bytes are identical, but the text follows different transport rules.

.. erbsland-demo::
    :source: text/BaseN/BaseNDemos.cpp
    :show: f:chooseAlphabet
    :exec: text/base_n --demo ChooseAlphabet
    :sha256: 5b74cc42cda94f490d9fe9359c1d1f1750c3f6db3e60b42046c88d9c65be2525

.. code-block:: cpp

    void chooseAlphabet() {
        const auto bytes = el::ByteBlock{el::Byte{0xfbU}, el::Byte{0xffU}};
        const auto base64 = el::base_n::BaseNFormat::base64();
        const auto url = el::base_n::BaseNFormat::base64Url();
        el::io::printLine("Base64: "_el, el::base_n::BaseNEncoder{bytes, base64}.toString());
        el::io::printLine("Base64url: "_el, el::base_n::BaseNEncoder{bytes, url}.toString());

        // A custom Base16 alphabet changes the text representation of the same bytes.
        const auto custom = el::base_n::BaseNFormat{el::U32String{U"FEDCBA9876543210"_el}};
        el::io::printLine("Custom Base16: "_el, el::base_n::BaseNEncoder{bytes, custom}.toString());
        el::io::printLine("Bits per character: "_el, custom.bitsPerCharacter());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mBase64: +/8=
        Base64url: -_8=
        Custom Base16: 0400
        Bits per character: 4␛[0m

.. erbsland-demo-end::

If another protocol defines its own alphabet, pass a :cpp:class:`U32String <erbsland::text::U32String>` to the
``BaseNFormat`` constructor.
You can also change an existing format with ``setAlphabet()``.
The alphabet must contain exactly 16, 32, or 64 distinct Unicode scalar values.
The custom-alphabet constructor starts without padding and accepts ASCII whitespace while decoding.

``alphabet()`` returns the chosen characters.
``bitsPerCharacter()`` reports whether each one stands for four, five, or six bits.
For inspecting an external format, ``valueFor()`` finds the digit value assigned to a character, while
``characterFor()`` performs the reverse lookup.
These answers follow the selected alphabet; they are not universal Base-N values.

Choose the Padding Character
============================

Some encodings fill their last group with padding.
For example, the Base64 representation of ``kaiku`` ends with ``=``.
The standard Base32 and Base64 format factories use that character.

``padding()`` reports the configured character, and ``setPadding()`` changes it.
The character must be a valid Unicode scalar outside the alphabet and the decoder's ignored whitespace set.
The example changes it to ``~`` and confirms that the matching decoder recovers the original bytes.

.. erbsland-demo::
    :source: text/BaseN/BaseNDemos.cpp
    :show: f:choosePadding
    :exec: text/base_n --demo ChoosePadding
    :sha256: 554197e8bb6c9907608aa5ea75ff896f471888521f3ddfced655d1cf2837a791

.. code-block:: cpp

    void choosePadding() {
        const auto bytes = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
        auto format = el::base_n::BaseNFormat::base64();
        format.setPadding(el::Char{U'~'});
        const auto encoded = el::base_n::BaseNEncoder{bytes, format}.toString();
        el::io::printLine("Alternate padding: "_el, encoded);
        el::io::printLine("Round trip: "_el, el::base_n::BaseNDecoder{encoded, format}.toDataOrThrow() == bytes);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAlternate padding: a2Fpa3U~
        Round trip: true␛[0m

.. erbsland-demo-end::

The padding character and the decision to emit it are separate settings.
:cpp:enumerator:`EmitPadding <erbsland::text::base_n::BaseNFormatFlag::EmitPadding>` tells the encoder to write
canonical padding.
Clearing that flag produces an unpadded value.
If you built a format from a custom alphabet, assign a padding character before enabling this flag.

Control the Format Flags
========================

The :cpp:enum:`BaseNFormatFlag <erbsland::text::base_n::BaseNFormatFlag>` set also contains ``RequirePadding`` and
``WrapLines``.
``RequirePadding`` affects decoding; ``WrapLines`` affects encoded layout.
The flags can be selected independently as long as the resulting format is valid.

``flags()`` returns the :cpp:type:`BaseNFormatFlags <erbsland::text::base_n::BaseNFormatFlags>` set.
``hasFlag()`` asks about one flag.
``setFlags()`` replaces the entire set, whereas ``addFlags()`` and ``clearFlags()`` modify a format you already have.

The example removes both padding flags from Base64. The encoder then omits ``=``, and the decoder accepts the unpadded
value.
Adding ``RequirePadding`` again makes that same text invalid for decoding.

.. erbsland-demo::
    :source: text/BaseN/BaseNDemos.cpp
    :show: f:chooseFlags
    :exec: text/base_n --demo ChooseFlags
    :sha256: 65ca642558587c200348c72445f593a7c3178887656adefa22c3e66d01ef205e

.. code-block:: cpp

    void chooseFlags() {
        const auto bytes = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
        auto format = el::base_n::BaseNFormat::base64();
        format.clearFlags(el::base_n::BaseNFormatFlag::EmitPadding | el::base_n::BaseNFormatFlag::RequirePadding);
        const auto unpadded = el::base_n::BaseNEncoder{bytes, format}.toString();
        el::io::printLine("Unpadded: "_el, unpadded);
        el::io::printLine("Unpadded accepted: "_el, el::base_n::BaseNDecoder{unpadded, format}.toData().has_value());
        format.addFlags(el::base_n::BaseNFormatFlag::RequirePadding);
        el::io::printLine("Padding now required: "_el, el::base_n::BaseNDecoder{unpadded, format}.toData().has_value());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mUnpadded: a2Fpa3U
        Unpadded accepted: true
        Padding now required: false␛[0m

.. erbsland-demo-end::

This separation can help when reading an older unpadded format while writing a newer padded form.
The receiving side still needs an explicit policy, described in
:doc:`/topics/text_parsing_and_encoding/base_n_decoding`.

Wrap Long Output
================

Some text formats store Base64 as lines rather than one continuous field.
``BaseNFormat::base64Pem()`` is the ready-made choice for 64-character lines separated by LF.
It supplies the payload layout, without PEM header or footer lines.

For a different layout, call ``setLineLength()``.
The length counts encoded characters, including padding.
Call ``setLineSeparator()`` to choose the text between lines, then enable ``WrapLines``.
The ``lineLength()`` and ``lineSeparator()`` accessors report the chosen values.
Without that flag, the stored length and separator do not change the output.

.. erbsland-demo::
    :source: text/BaseN/BaseNDemos.cpp
    :show: f:wrapLines
    :exec: text/base_n --demo WrapLines
    :sha256: fcbf3b8869e08d2736039dedc663f315aae50e3e4853ad9ed6e1b469e5f8c9ca

.. code-block:: cpp

    void wrapLines() {
        const auto bytes = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
        auto format = el::base_n::BaseNFormat::base64();
        format.setLineLength(el::CpCount{4U})
            .setLineSeparator(el::U32String{U"\n"_el})
            .addFlags(el::base_n::BaseNFormatFlag::WrapLines);
        const auto wrapped = el::base_n::BaseNEncoder{bytes, format}.toString();
        el::io::printLine("Wrapped:\n"_el, wrapped);
        el::io::printLine("Round trip: "_el, el::base_n::BaseNDecoder{wrapped, format}.toDataOrThrow() == bytes);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mWrapped:
        a2Fp
        a3U=
        Round trip: true␛[0m

.. erbsland-demo-end::

The encoder inserts a separator *between* lines, not after the last line.
With wrapping enabled, the length must be finite and greater than zero, and the separator must be nonempty.
Every separator character must also be in the decoder's ignored whitespace set.
That rule lets the same format read the output it writes.

Format setters validate each resulting combination and throw
:cpp:class:`ParameterFailure <erbsland::err::ParameterFailure>` if it is invalid.
Build and validate a format before encoding data, especially when its settings came from configuration.
