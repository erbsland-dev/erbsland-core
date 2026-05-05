.. index::
    !single: String Conversion
    single: StringConverter
    single: StringEncoder
    single: StringDecoder
    single: AnyString
    single: AnyStringView
    single: StringBuilder
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

Erbsland Core supports text in UTF-8, UTF-16, and UTF-32. The library provides several tools for converting between
these encodings, working with encoded byte sequences, and writing APIs that are independent of a specific string
representation.

This page explains when to use each approach and demonstrates the most important string conversion workflows.

Overview
========

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
   * - Accept strings without committing to a specific encoding
     - :cpp:class:`AnyString <erbsland::text::AnyString>` /
       :cpp:class:`AnyStringView <erbsland::text::AnyStringView>`

As a general guideline:

- Use :cpp:class:`StringConverter <erbsland::text::StringConverter>` whenever
  you need to convert between string types.
- Use :cpp:class:`StringEncoder <erbsland::text::StringEncoder>` and
  :cpp:class:`StringDecoder <erbsland::text::StringDecoder>` when working with
  encoded bytes stored in memory.
- Use :cpp:class:`TextInputStream <erbsland::stream::TextInputStream>` and
  :cpp:class:`TextOutputStream <erbsland::stream::TextOutputStream>` when
  reading or writing encoded text through files or other byte streams.
- Use :cpp:class:`AnyString <erbsland::text::AnyString>` or
  :cpp:class:`AnyStringView <erbsland::text::AnyStringView>` when an API should
  accept text regardless of its underlying encoding.

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

Cross-Encoding Conversion with ``StringConverter``
==================================================

:cpp:class:`StringConverter <erbsland::text::StringConverter>` is the primary tool for converting between
string encodings.

It supports all library string types, string views, and the corresponding standard-library string types.
The converter validates the source text and produces a correctly encoded target string.

To perform a conversion, construct a temporary converter from the source string and call one of the available ``to...``
methods:

.. code-block:: cpp

    auto u16Text = el::StringConverter{u8Text}.toU16String();
    auto stdText = el::StringConverter{u16Text}.toStdString();

String literals such as ``"text"`` are intentionally not accepted directly.
This avoids accidental interpretation of narrow string literals as UTF-8 text.

If you want to use literals with Erbsland Core string APIs, use the ``"_el"`` literal suffix instead:

.. code-block:: cpp

    auto text = "Hello 🌍"_el;

.. erbsland-demo::
    :source: text/StringConverter/CrossConvertStrings.cpp
    :exec: string_converter --demo CrossConvertStrings
    :source-sha256: 267f2e5b93b20167dd9e0478b6f962c9d09e6d363f259ef3b0c902ed8123a8f4

.. code-block:: cpp

    /// Demonstrates how to convert between library strings and standard string views using `StringConverter`.
    void crossConvertStrings() {
        // Convert between all library string types.
        auto u8String = el::U8String{u8"Bonjour, forêt 🌲"};
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

.. erbsland-ansi::
    :escape-char: ␛

    After conversion: Bonjour, forêt 🌲
    Converted stdString: Bonjour, forêt 🌲
    Converted 'stdStringView': Hello, river
    Converted 'stdU8StringView': Bonjour, forêt 🌲
    Converted 'stdU16StringView': Hola, río
    Converted 'stdU32StringView': Ciao, sole ☀
    Converted 'stdWStringView': Hej, skog
    Converted 'stdString': Hello, wind

.. erbsland-demo-end::

Encoding Strings into Byte Sequences using ``StringEncoder``
============================================================

In some cases, you want to encode strings into byte sequences.
For this use case, the :cpp:class:`StringEncoder <erbsland::text::StringEncoder>` class exists.

It allows you to encode all strings and string views from this library into UTF-8, UTF-16 and UTF-32 byte sequences.
Additionally you can choose if the encoded strings shall be little or big-endian encoded.
Also, you can put a BOM in front of the encoded byte sequence.

.. erbsland-demo::
    :source: text/StringEncoder/EncodeStrings.cpp
    :exec: string_encoder --demo EncodeStrings
    :source-sha256: a990ae22641c9a089bbdd08b76c9cc366fcf772d782ec80315443c8beb3c7bc2

.. code-block:: cpp

    /// The `StringEncoder` class converts text into a selected Unicode byte encoding.
    /// It supports UTF-8, UTF-16, and UTF-32 output, optional byte order marks, and
    /// explicit little-endian or big-endian byte order for encodings where this matters.
    void encodeStrings() {
        // Encode a short Unicode text into UTF-32 little-endian bytes with a BOM.
        const auto observation = el::U8StringView{"Sternbild: Orion ✨"_el};
        const auto encodedObservation =
            el::StringEncoder{observation}.encode(el::StringEncoding::Utf32LittleEndian, el::StringBomMode::Require);

        el::io::printLine("Observation: \"", observation, "\"");
        el::io::printLine("Encoded as UTF-32 little-endian with BOM:");
        el::io::printLine(el::ByteFormat::memoryDump(), encodedObservation);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Observation: "Sternbild: Orion ✨"
    Encoded as UTF-32 little-endian with BOM:
    00000000 | fffe0000 53000000 74000000 65000000 72000000 6e000000 62000000 69000000
    00000020 | 6c000000 64000000 3a000000 20000000 4f000000 72000000 69000000 6f000000
    00000040 | 6e000000 20000000 28270000

.. erbsland-demo-end::

``StringEncoding`` for Encoding Strings
---------------------------------------

When encoding strings, ``Utf16`` and ``Utf32`` select the most common little-endian byte order.

.. erbsland-demo::
    :source: text/StringEncoder/ByteOrder.cpp
    :exec: string_encoder --demo ByteOrder
    :source-sha256: 95d6c6beb7f22971ba47184008f72684a7f3a962abf292f231a2d401a1896081

.. code-block:: cpp

    /// Byte order determines how multi-byte character encodings store data in memory.
    /// UTF-16 and UTF-32 store characters as sequences of bytes, and the byte order
    /// (little-endian or big-endian) affects how those sequences are laid out.
    /// Little-endian stores the least significant byte first, while big-endian stores
    /// the most significant byte first. This demo shows how the same text produces
    /// different byte sequences depending on the chosen byte order.
    void byteOrder() {
        // Encode a marine biology text in both UTF-16 byte orders.
        const auto oceanText = el::StringView{u8"🐋 Meerjungfrau 🌊"_el};
        el::io::printLine("Marine text: \"", oceanText, "\"\n");

        // Encode as UTF-16 little-endian (least significant byte first).
        auto bytes = el::StringEncoder{oceanText}.encode(el::StringEncoding::Utf16LittleEndian, el::StringBomMode::Reject);
        el::io::printLine("Encoded as UTF-16 little-endian:\n", el::ByteFormat::memoryDump(), bytes);

        // Encode as UTF-16 big-endian (most significant byte first).
        bytes = el::StringEncoder{oceanText}.encode(el::StringEncoding::Utf16BigEndian, el::StringBomMode::Reject);
        el::io::printLine("Encoded as UTF-16 big-endian:\n", el::ByteFormat::memoryDump(), bytes);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Marine text: "🐋 Meerjungfrau 🌊"

    Encoded as UTF-16 little-endian:
    00000000 | 3dd80bdc 20004d00 65006500 72006a00 75006e00 67006600 72006100 75002000
    00000020 | 3cd80adf

    Encoded as UTF-16 big-endian:
    00000000 | d83ddc0b 0020004d 00650065 0072006a 0075006e 00670066 00720061 00750020
    00000020 | d83cdf0a

.. erbsland-demo-end::

The ``StringBomMode`` for Encoding Strings
------------------------------------------

The enum values in :cpp:enum:`StringBomMode <erbsland::text::StringBomMode>` are not that straightforward to understand
when encoding strings.
``Automatic`` will choose adding a BOM depending on the encoding.

.. erbsland-demo::
    :source: text/StringEncoder/BomHandling.cpp
    :exec: string_encoder --demo BomHandling
    :source-sha256: 8869abb94dfedd6e396b27f90626ee16d28d5a44846bcfe76f0697faedcb46ad

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
        const auto observation = el::StringView{u8"🌲 Waldlichtung im Morgennebel 🌫️"_el};
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

.. erbsland-ansi::
    :escape-char: ␛

    Beobachtung: "🌲 Waldlichtung im Morgennebel 🌫️"

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
    00000060 | 6e000000 65000000 62000000 65000000 6c000000 20000000 2bf30100 0ffe0000

.. erbsland-demo-end::

Passing Strings Independent of Their Encoding
=============================================

Sometimes an API should accept text without forcing callers to convert it to a specific string type first.

For example, a logging function, parser, formatter, or configuration API often does not care whether the caller provides
UTF-8, UTF-16, or UTF-32 text.
Requiring an explicit conversion at every call site would create unnecessary overhead and clutter.

:cpp:class:`AnyStringView <erbsland::text::AnyStringView>` solves this problem
by providing a lightweight view that can reference any supported string type.

:cpp:class:`AnyString <erbsland::text::AnyString>` provides the owning variant
and stores text in any supported encoding while converting lazily when a specific representation is requested.

.. erbsland-demo::
    :source: text/AnyString/AcceptAny.cpp
    :exec: any_string --demo AcceptAny
    :source-sha256: 7aca60919bfc8972276b0f3052ce3d49061539573648653fa759c06793efef18

.. code-block:: cpp

    /// `AnyStringView` accepts any string type — UTF-8, UTF-16, or UTF-32 — through a
    /// single unified interface. Use it to write functions that receive strings regardless
    /// of their underlying encoding, then inspect the kind, length, or convert to the
    /// format you need for further processing.
    void acceptAny() {
        // Prepare sound-wave labels in three different encodings.
        const auto u8Label = el::U8StringView{"Vlnová frekvence 🌊"_el};
        const auto u16Label = el::U16StringView{u"Hmotnostní spektrum 🎵"_el};
        const auto u32Label = el::U32StringView{U"Amplituda vlnění 🎶"_el};

        // An empty AnyStringView carries no kind information.
        processAnyString({});
        processAnyString(u8Label);
        processAnyString(u16Label);
        processAnyString(u32Label);
    }

    void processAnyString(const el::AnyStringView &str) {
        el::io::printLine("Analýza signálu:"_el);

        if (str.kind().has_value()) {
            el::io::printLine("  Typ: "_el, el::toString(str.kind().value()));
        } else {
            el::io::printLine("  Typ: (prázdný)"_el);
        }

        el::io::printLine("  Délka znaků: "_el, str.characterLength());
        el::io::printLine("  Je prázdný: "_el, str.isEmpty() ? "ano" : "ne");

        auto u8Str = str.toU8String();
        u8Str.replaceAll("vlnění"_el, "vlny"_el);
        el::io::printLine("  Výsledek: "_el, u8Str);
        el::io::printLine();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Analýza signálu:
      Typ: (prázdný)
      Délka znaků: 0
      Je prázdný: ano
      Výsledek: 

    Analýza signálu:
      Typ: U8
      Délka znaků: 18
      Je prázdný: ne
      Výsledek: Vlnová frekvence 🌊

    Analýza signálu:
      Typ: U16
      Délka znaků: 21
      Je prázdný: ne
      Výsledek: Hmotnostní spektrum 🎵

    Analýza signálu:
      Typ: U32
      Délka znaků: 18
      Je prázdný: ne
      Výsledek: Amplituda vlny 🎶

.. erbsland-demo-end::

.. note::

    For creating strings in a generic way, create a function that takes a reference to a
    :cpp:class:`StringBuilder <erbsland::text::StringBuilder>` instead.
    The caller can construct the builder with a fitting underlying string type, and the function builds the string.
    This is more efficient that convert between string types using :cpp:class:`AnyString <erbsland::text::AnyString>`.

Choosing the Right Tool
=======================

If you already have text and only need another string type, use
:cpp:class:`StringConverter <erbsland::text::StringConverter>`.

If you need to transform text into bytes or bytes into text, use
:cpp:class:`StringEncoder <erbsland::text::StringEncoder>` and
:cpp:class:`StringDecoder <erbsland::text::StringDecoder>`.

If you are reading or writing files, prefer
:cpp:class:`TextInputStream <erbsland::stream::TextInputStream>` and
:cpp:class:`TextOutputStream <erbsland::stream::TextOutputStream>` because they
perform encoding and decoding while data is streamed.

If you are designing a public API and want to support all string encodings, consider
:cpp:class:`AnyStringView <erbsland::text::AnyStringView>` for input parameters and
:cpp:class:`StringBuilder <erbsland::text::StringBuilder>` for building output efficiently.
