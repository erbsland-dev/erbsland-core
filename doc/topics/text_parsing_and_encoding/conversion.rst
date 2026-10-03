.. index::
    !single: String Conversion
    single: StringConverter
    single: StringEncoder
    single: StringDecoder
    single: AnyStringEditor
    single: AnyString
    single: AnyStringBuilder
    single: TextInputStream
    single: TextOutputStream
    single: StringEncoding
    single: StringBomMode
    single: UTF-8
    single: UTF-16
    single: UTF-32
    single: BOM
    single: Encoding
    single: Byte Sequences
    single: Cross-Encoding Conversion
    single: Encoding Strings
    single: Passing Strings

******************
Converting Strings
******************

Conversion becomes much easier once you identify the boundary you are crossing.
Changing a UTF-8 value into UTF-16 is a string conversion; writing UTF-16 bytes with a chosen byte order is encoding;
reading text from a stream adds buffering and error handling; and accepting any string width may require no conversion
at all.

This page separates those concerns before introducing the corresponding tools.
You will learn how to convert between the library and standard string types, how strict and tolerant decoding differ,
how byte-order marks affect encoded output, and how to keep an API independent of a concrete string width.

String Representations and Encoded Byte Data
============================================

The source and destination tell you which abstraction belongs in the middle.
Use the table as a map, then follow the relevant section for its error and allocation behavior.

.. list-table::
   :header-rows: 1
   :widths: 60 40

   * - Use case
     - Recommended API
   * - Convert between UTF-8, UTF-16, and UTF-32 string types
     - :cpp:class:`StringConverter <erbsland::text::StringConverter>`
   * - Convert between library strings and standard-library strings
     - :cpp:class:`StringConverter <erbsland::text::StringConverter>`
   * - Encode a string into a UTF byte sequence
     - :cpp:class:`StringEncoder <erbsland::text::StringEncoder>`
   * - Decode a UTF byte sequence into a string
     - :cpp:class:`StringDecoder <erbsland::text::StringDecoder>`
   * - Read or write encoded text files and streams
     - :cpp:class:`TextInputStream <erbsland::stream::TextInputStream>` /
       :cpp:class:`TextOutputStream <erbsland::stream::TextOutputStream>`
   * - Store a read-only string without committing to a specific encoding
     - :cpp:class:`AnyString <erbsland::text::AnyString>`

``StringConverter`` stays in the world of string values.
``StringEncoder`` and ``StringDecoder`` cross between strings and byte blocks, while text streams own the corresponding
work at an I/O boundary.
If the width merely needs to travel through an API unchanged, ``AnyString`` stores the completed read-only value without
forcing an eager conversion.

.. design-rationale::

    Erbsland Core intentionally focuses on UTF-8, UTF-16, and UTF-32.

    Legacy encodings such as ISO-8859 variants, Windows code pages, or Big5
    significantly increase implementation complexity while providing limited
    value for modern applications. By concentrating on the Unicode encodings
    most commonly used today, the library remains smaller, easier to maintain,
    and easier to understand.

    If your application needs to process legacy encodings, convert them to one
    of the supported UTF encodings at the system boundary and use Unicode
    throughout the rest of the application.

Converting String Representations with StringConverter
======================================================

:cpp:class:`StringConverter <erbsland::text::StringConverter>` is the primary tool for converting between
string encodings.

It supports all library string and editor types and the corresponding standard-library string types.
All conversion methods accept an :cpp:enum:`EncodingMode <erbsland::text::EncodingMode>` argument and default to
``Tolerant``.
In tolerant mode, compatible representations may be copied unchanged without validation; transcoding replaces malformed
sequences with the Unicode replacement character.

.. important::

    ``Strict`` conversion can be **much slower** than ``Tolerant`` conversion, even when the input is valid.
    The difference is especially large when tolerant conversion can reuse compatible string storage while strict
    conversion must examine the entire source.
    Keep the tolerant default unless encoding validity **must be enforced at this conversion boundary**.

A security-sensitive operation or a protocol receiving untrusted network text may require malformed encoding to be
rejected before that text is interpreted.
That is a reason to choose ``Strict`` and accept the additional decoding time: the validity requirement takes priority
over throughput at that boundary.
If text has already been validated and has not been modified, ordinary internal conversions should remain tolerant
instead of repeating strict validation.
The same choice applies when decoding byte blocks with :cpp:class:`StringDecoder <erbsland::text::StringDecoder>` or
reading text; see :doc:`/topics/stream/text_input_and_output`.

To perform a conversion, construct a temporary converter from the source string and call one of the available ``to...``
methods:

.. code-block:: cpp

    auto u16Text = el::StringConverter{u8Text}.toU16String();
    auto stdText = el::StringConverter{u16Text}.toStdString();

Unmarked C++ literals such as ``"text"`` are intentionally not accepted directly.
This avoids accidental interpretation of narrow string literals as UTF-8 text.

If you want to use literals with Erbsland Core string APIs, use the ``"_el"`` literal suffix instead:

.. code-block:: cpp

    auto text = "Hello 🌍"_el;

.. erbsland-demo::
    :source: text/StringConverter/CrossConvertStrings.cpp
    :exec: text/string_converter --demo CrossConvertStrings
    :sha256: 662d02f7a69d6503108a1d89b156b87026215bf1ffbbbd9c9dcd77769f4e0fc8

.. code-block:: cpp

    /// Demonstrates how to convert between library strings and standard string views using `StringConverter`.
    void crossConvertStrings() {
        // Convert between all library string types.
        auto u8String = el::U8String{u8"Bonjour, forêt 🌲"_el};
        auto u16String = el::StringConverter{u8String}.toU16String();
        auto u32String = el::StringConverter{u16String}.toU32String();
        auto backToU8String = el::StringConverter{u32String}.toU8String();
        el::io::printLine("After conversion: ", backToU8String);

        // Convert library strings to standard library strings.
        auto stdString = el::StringConverter{u32String}.toStdString();
        el::io::printLine("Converted stdString: ", stdString);

        // Convert standard string views.
        constexpr auto stdStringView = std::string_view{"Hello, river"};
        constexpr auto stdU8StringView = std::u8string_view{u8"Bonjour, forêt 🌲"};
        constexpr auto stdU16StringView = std::u16string_view{u"Hola, río"};
        constexpr auto stdU32StringView = std::u32string_view{U"Ciao, sole ☀"};
        constexpr auto stdWStringView = std::wstring_view{L"Hej, skog"};

        u8String = el::StringConverter{stdStringView}.toU8String();
        el::io::printLine("Converted 'stdStringView': ", u8String);

        u8String = el::StringConverter{stdU8StringView}.toU8String();
        el::io::printLine("Converted 'stdU8StringView': ", u8String);

        u8String = el::StringConverter{stdU16StringView}.toU8String();
        el::io::printLine("Converted 'stdU16StringView': ", u8String);

        u8String = el::StringConverter{stdU32StringView}.toU8String();
        el::io::printLine("Converted 'stdU32StringView': ", u8String);

        u8String = el::StringConverter{stdWStringView}.toU8String();
        el::io::printLine("Converted 'stdWStringView': ", u8String);

        // Convert a standard string.
        stdString = std::string{"Hello, wind"};
        u8String = el::StringConverter{stdString}.toU8String();
        el::io::printLine("Converted 'stdString': ", u8String);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAfter conversion: Bonjour, forêt 🌲
        Converted stdString: Bonjour, forêt 🌲
        Converted 'stdStringView': Hello, river
        Converted 'stdU8StringView': Bonjour, forêt 🌲
        Converted 'stdU16StringView': Hola, río
        Converted 'stdU32StringView': Ciao, sole ☀
        Converted 'stdWStringView': Hej, skog
        Converted 'stdString': Hello, wind␛[0m

.. erbsland-demo-end::

Encoding Strings as UTF Byte Sequences
======================================

In some cases, you want to encode strings into byte sequences.
For this use case, the :cpp:class:`StringEncoder <erbsland::text::StringEncoder>` class exists.

It allows you to encode all strings and editors from this library into UTF-8, UTF-16 and UTF-32 byte sequences.
Additionally you can choose if the encoded strings shall be little or big-endian encoded.
Also, you can put a BOM in front of the encoded byte sequence.
The BOM is encoding infrastructure at the byte boundary, not string content.
Erbsland Core reserves ``U+FEFF`` for this purpose: a raw ``U+FEFF`` encountered during transcoding is invalid and is
written as a replacement character.

.. erbsland-demo::
    :source: text/StringEncoder/EncodeStrings.cpp
    :exec: text/string_encoder --demo EncodeStrings
    :sha256: a5df7e0b209cc2525b0fa2bdcb0045d6a519e3ef4ada9f21d209cb359da0e339

.. code-block:: cpp

    /// The `StringEncoder` class converts text into a selected Unicode byte encoding.
    /// It supports UTF-8, UTF-16, and UTF-32 output, optional byte order marks, and
    /// explicit little-endian or big-endian byte order for encodings where this matters.
    void encodeStrings() {
        // Encode a short Unicode text into UTF-32 little-endian bytes with a BOM.
        const auto observation = el::U8String{"Sternbild: Orion ✨"_el};
        const auto encodedObservation =
            el::StringEncoder{observation}.encode(el::StringEncoding::Utf32LittleEndian, el::StringBomMode::Require);

        el::io::printLine("Observation: \""_el, observation, "\""_el);
        el::io::printLine("Encoded as UTF-32 little-endian with BOM:");
        el::io::printLine(el::ByteFormat::memoryDump(), encodedObservation);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mObservation: "Sternbild: Orion ✨"
        Encoded as UTF-32 little-endian with BOM:
        00000000 | fffe0000 53000000 74000000 65000000 72000000 6e000000 62000000 69000000
        00000020 | 6c000000 64000000 3a000000 20000000 4f000000 72000000 69000000 6f000000
        00000040 | 6e000000 20000000 28270000␛[0m

.. erbsland-demo-end::

Target Encoding and Byte Order
------------------------------

When encoding strings, ``Utf16`` and ``Utf32`` select the most common little-endian byte order.

.. erbsland-demo::
    :source: text/StringEncoder/ByteOrder.cpp
    :exec: text/string_encoder --demo ByteOrder
    :sha256: e260961a974cbc1d554bbe0f55da5140008758a20ee3928c8e1abe6a03a9b158

.. code-block:: cpp

    /// Byte order determines how multi-byte character encodings store data in memory.
    /// UTF-16 and UTF-32 store characters as sequences of bytes, and the byte order
    /// (little-endian or big-endian) affects how those sequences are laid out.
    /// Little-endian stores the least significant byte first, while big-endian stores
    /// the most significant byte first. This demo shows how the same text produces
    /// different byte sequences depending on the chosen byte order.
    void byteOrder() {
        // Encode a marine biology text in both UTF-16 byte orders.
        const auto oceanText = el::String{u8"🐋 Meerjungfrau 🌊"_el};
        el::io::printLine("Marine text: \""_el, oceanText, "\"\n"_el);

        // Encode as UTF-16 little-endian (least significant byte first).
        auto bytes = el::StringEncoder{oceanText}.encode(el::StringEncoding::Utf16LittleEndian, el::StringBomMode::Reject);
        el::io::printLine("Encoded as UTF-16 little-endian:\n", el::ByteFormat::memoryDump(), bytes);

        // Encode as UTF-16 big-endian (most significant byte first).
        bytes = el::StringEncoder{oceanText}.encode(el::StringEncoding::Utf16BigEndian, el::StringBomMode::Reject);
        el::io::printLine("Encoded as UTF-16 big-endian:\n", el::ByteFormat::memoryDump(), bytes);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMarine text: "🐋 Meerjungfrau 🌊"

        Encoded as UTF-16 little-endian:
        00000000 | 3dd80bdc 20004d00 65006500 72006a00 75006e00 67006600 72006100 75002000
        00000020 | 3cd80adf

        Encoded as UTF-16 big-endian:
        00000000 | d83ddc0b 0020004d 00650065 0072006a 0075006e 00670066 00720061 00750020
        00000020 | d83cdf0a␛[0m

.. erbsland-demo-end::

Byte-Order Mark Policies
------------------------

The enum values in :cpp:enum:`StringBomMode <erbsland::text::StringBomMode>` are not that straightforward to understand
when encoding strings.
``Automatic`` will choose adding a BOM depending on the encoding.
On decoding, these modes apply only to an initial encoded signature.
A second or embedded BOM sequence is invalid content and is either replaced with ``U+FFFD`` or rejected according to
:cpp:enum:`EncodingMode <erbsland::text::EncodingMode>` at the byte-decoding boundary.
When encoding an Erbsland Core string, a raw ``U+FEFF`` is copied unchanged for a matching representation and becomes
``U+FFFD`` when transcoding.

.. erbsland-demo::
    :source: text/StringEncoder/BomHandling.cpp
    :exec: text/string_encoder --demo BomHandling
    :sha256: e4d0267119b2adcfa9990a38b8c44824e151e409fc7e47118ccdf8abba8370ff

.. code-block:: cpp

    /// The `StringEncoder` class handles byte order marks (BOM) during encoding, giving you full control
    /// over how multi-byte Unicode encodings represent their byte order. A BOM is a special marker placed
    /// at the start of a byte stream that identifies both the encoding and the byte order.
    ///
    /// Different encodings treat BOMs differently: UTF-8 rarely uses them, while UTF-16 and UTF-32
    /// conventionally include one. The `StringBomMode` enum provides three modes — `Automatic` follows
    /// convention, `Require` forces a BOM, and `Reject` forbids one entirely.
    void bomHandling() {
        // Encode a nature observation in multiple encodings, each with a different BOM strategy.
        const auto observation = el::String{u8"🌲 Waldlichtung im Morgennebel 🌫️"_el};
        el::io::printLine("Beobachtung: \"", observation, "\"\n"_el);

        // `Automatic` follows encoding conventions: no BOM for UTF-8, BOM for UTF-16 and UTF-32.
        auto bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf8, el::StringBomMode::Automatic);
        el::io::printLine("UTF-8 (automatic, no BOM):\n"_el, el::ByteFormat::memoryDump(), bytes);

        bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf16LittleEndian, el::StringBomMode::Automatic);
        el::io::printLine("UTF-16 LE (automatic, with BOM):\n"_el, el::ByteFormat::memoryDump(), bytes);

        bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf32, el::StringBomMode::Automatic);
        el::io::printLine("UTF-32 (automatic, with BOM):\n"_el, el::ByteFormat::memoryDump(), bytes);

        // Force a BOM even when the encoding convention does not use one.
        bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf8, el::StringBomMode::Require);
        el::io::printLine("UTF-8 with forced BOM:\n"_el, el::ByteFormat::memoryDump(), bytes);

        // Explicitly suppress the BOM, even when the encoding normally includes one.
        bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf32, el::StringBomMode::Reject);
        el::io::printLine("UTF-32 without BOM:\n"_el, el::ByteFormat::memoryDump(), bytes);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mBeobachtung: "🌲 Waldlichtung im Morgennebel 🌫️"

        UTF-8 (automatic, no BOM):
        00000000 | f09f8cb2 2057616c 646c6963 6874756e 6720696d 204d6f72 67656e6e 6562656c
        00000020 | 20f09f8c abefb88f

        UTF-16 LE (automatic, with BOM):
        00000000 | fffe3cd8 32df2000 57006100 6c006400 6c006900 63006800 74007500 6e006700
        00000020 | 20006900 6d002000 4d006f00 72006700 65006e00 6e006500 62006500 6c002000
        00000040 | 3cd82bdf 0ffe

        UTF-32 (automatic, with BOM):
        00000000 | fffe0000 32f30100 20000000 57000000 61000000 6c000000 64000000 6c000000
        00000020 | 69000000 63000000 68000000 74000000 75000000 6e000000 67000000 20000000
        00000040 | 69000000 6d000000 20000000 4d000000 6f000000 72000000 67000000 65000000
        00000060 | 6e000000 6e000000 65000000 62000000 65000000 6c000000 20000000 2bf30100
        00000080 | 0ffe0000

        UTF-8 with forced BOM:
        00000000 | efbbbff0 9f8cb220 57616c64 6c696368 74756e67 20696d20 4d6f7267 656e6e65
        00000020 | 62656c20 f09f8cab efb88f

        UTF-32 without BOM:
        00000000 | 32f30100 20000000 57000000 61000000 6c000000 64000000 6c000000 69000000
        00000020 | 63000000 68000000 74000000 75000000 6e000000 67000000 20000000 69000000
        00000040 | 6d000000 20000000 4d000000 6f000000 72000000 67000000 65000000 6e000000
        00000060 | 6e000000 65000000 62000000 65000000 6c000000 20000000 2bf30100 0ffe0000␛[0m

.. erbsland-demo-end::

Width-Independent Text with AnyString and AnyStringEditor
=========================================================

Sometimes an API should accept text without forcing callers to convert it to a specific string type first.

For example, a logging function, parser, formatter, or configuration API often does not care whether the caller provides
UTF-8, UTF-16, or UTF-32 text.
Requiring an explicit conversion at every call site would create unnecessary overhead and clutter.

:cpp:class:`AnyString <erbsland::text::AnyString>` solves this problem by storing an owning read-only value of any
supported string width.
Like the concrete string types, its contained value can retain shared storage rather than copying character data.

:cpp:class:`AnyStringEditor <erbsland::text::AnyStringEditor>` provides the owning variant
and stores text in any supported encoding while converting lazily when a specific representation is requested.

.. erbsland-demo::
    :source: text/AnyStringEditor/AcceptAny.cpp
    :exec: text/any_string_editor --demo AcceptAny
    :sha256: 1f64026e9d2ad86c99c0ee37318d75696dc8728c58bf35b64420f907aa7495d6

.. code-block:: cpp

    /// `AnyString` accepts any string type — UTF-8, UTF-16, or UTF-32 — through a
    /// single unified interface. Use it to write functions that receive strings regardless
    /// of their underlying encoding, then inspect the kind, length, or convert to the
    /// format you need for further processing.
    void acceptAny() {
        // Prepare sound-wave labels in three different encodings.
        const auto u8Label = el::U8String{"Vlnová frekvence 🌊"_el};
        const auto u16Label = el::U16String{u"Hmotnostní spektrum 🎵"_el};
        const auto u32Label = el::U32String{U"Amplituda vlnění 🎶"_el};

        // An empty AnyString carries no kind information.
        processAnyString({});
        processAnyString(u8Label);
        processAnyString(u16Label);
        processAnyString(u32Label);
    }

    void processAnyString(const el::AnyString &str) {
        el::io::printLine("Signal analysis:"_el);

        if (str.kind().has_value()) {
            el::io::printLine("  Type: "_el, el::toString(str.kind().value()));
        } else {
            el::io::printLine("  Type: (empty)"_el);
        }

        el::io::printLine("  Character length: "_el, str.characterLength());
        el::io::printLine("  Is empty: "_el, str.isEmpty());

        auto u8Str = el::U8StringEditor{str.toU8String()};
        u8Str.replaceAll("vlnění"_el, "vlny"_el);
        el::io::printLine("  Result: "_el, u8Str);
        el::io::printLine();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSignal analysis:
          Type: (empty)
          Character length: 0
          Is empty: true
          Result:

        Signal analysis:
          Type: U8
          Character length: 18
          Is empty: false
          Result: Vlnová frekvence 🌊

        Signal analysis:
          Type: U16
          Character length: 21
          Is empty: false
          Result: Hmotnostní spektrum 🎵

        Signal analysis:
          Type: U32
          Character length: 18
          Is empty: false
          Result: Amplituda vlny 🎶␛[0m

.. erbsland-demo-end::

.. note::

    When a generic function creates text rather than receives it, let the caller provide an
    :cpp:class:`AnyStringBuilder <erbsland::text::AnyStringBuilder>`.
    The caller selects the target width, and the function writes directly into that representation instead of building
    through :cpp:class:`AnyStringEditor <erbsland::text::AnyStringEditor>` and converting afterward.

Choosing String Conversion, Byte Encoding, or Text Streams
==========================================================

Keep each conversion close to the boundary that makes it necessary.
``StringConverter`` is the direct choice when both sides are string values.
When one side is a byte block, ``StringEncoder`` or ``StringDecoder`` makes the encoding mode and byte order explicit.
For files and other streams, the text stream should own that work so data can be converted incrementally instead of
materialized twice.

A public API that accepts every supported width can retain input in ``AnyString``.
If that API produces text, an ``AnyStringBuilder`` reference lets the caller choose the destination width before any
characters are written.
