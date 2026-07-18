..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: String Attributes
    single: String
    single: StringEditor
    single: StringLiteral
    single: U8String
    single: U8StringEditor
    single: U16String
    single: U16StringEditor
    single: U32String
    single: U32StringEditor
    single: isEmpty
    single: isValidUtf8
    single: isValidUtf16
    single: isValidUtf32
    single: length
    single: characterLength
    single: toHash
    single: toHashCI
    single: storageId
    single: Native Length
    single: Code-Point Length
    single: Encoding Validation
    single: Storage Identifier

*****************
String Attributes
*****************

String attributes answer simple questions about a string without changing it.
They tell you whether a string is empty, whether its stored data is valid UTF, how long the string is in storage units
or decoded code points, which hash value represents its decoded text, and which storage range a view refers to.

The examples on this page use :cpp:type:`String <erbsland::text::String>`, the common UTF-8 read-only string
type.
The same attribute model is available for :cpp:type:`StringEditor <erbsland::text::StringEditor>`,
:cpp:type:`StringLiteral <erbsland::text::StringLiteral>`,
:cpp:class:`U8String <erbsland::text::U8String>`,
:cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>`,
:cpp:class:`U16String <erbsland::text::U16String>`,
:cpp:class:`U16StringEditor <erbsland::text::U16StringEditor>`,
:cpp:class:`U32String <erbsland::text::U32String>`, and
:cpp:class:`U32StringEditor <erbsland::text::U32StringEditor>`.

Use the attribute that matches the question you are asking:

.. list-table::
    :header-rows: 1
    :widths: 35 35 30

    *   - Question
        - Method
        - Cost model
    *   - Does this string contain any stored text?
        - :cpp:func:`isEmpty() <erbsland::text::U8String::isEmpty>`
        - Constant time.
    *   - Is the stored UTF data well-formed?
        - :cpp:func:`isValidUtf8() <erbsland::text::U8String::isValidUtf8>`,
          :cpp:func:`isValidUtf16() <erbsland::text::U16String::isValidUtf16>`, or
          :cpp:func:`isValidUtf32() <erbsland::text::U32String::isValidUtf32>`
        - Scans the stored data.
    *   - How large is the native storage range?
        - :cpp:func:`length() <erbsland::text::U8String::length>`
        - Constant time.
    *   - How many decoded code points does it contain?
        - :cpp:func:`characterLength() <erbsland::text::U8String::characterLength>`
        - Scans UTF-8 and UTF-16.
    *   - Which hash represents the decoded text?
        - :cpp:func:`toHash() <erbsland::text::U8String::toHash>` or
          :cpp:func:`toHashCI() <erbsland::text::U8String::toHashCI>`
        - Scans decoded code points.
    *   - Does an index belong to the same visible storage range?
        - :cpp:func:`storageId() <erbsland::text::U8String::storageId>`
        - Constant time.

Empty Strings and Valid Encodings
=================================

Use :cpp:func:`isEmpty() <erbsland::text::U8String::isEmpty>` when you need to know whether a string contains any
stored data.
This is clearer than comparing a length to zero, and it keeps the code independent of whether the string is stored as
UTF-8 bytes, UTF-16 code units, or UTF-32 code points.

Encoding validation is a different question.
Use :cpp:func:`isValidUtf8() <erbsland::text::U8String::isValidUtf8>` for UTF-8 strings,
:cpp:func:`isValidUtf16() <erbsland::text::U16String::isValidUtf16>` for UTF-16 strings, and
:cpp:func:`isValidUtf32() <erbsland::text::U32String::isValidUtf32>` for UTF-32 strings.
Validation has to inspect the complete stored range, so it is proportional to the length of the input.

Most string algorithms in Erbsland Core handle invalid encoding data deterministically.
When a malformed sequence is decoded, the string APIs use the Unicode replacement character instead of walking into
undefined behavior.
This makes ordinary processing robust, but it does not replace validation at trust boundaries.

For security-sensitive input, validate the encoding when text enters your application.
Then validate the allowed character set with
:cpp:func:`containsOnly() <erbsland::text::U8String::containsOnly>` before treating the text as an identifier,
path component, protocol token, or command.

.. erbsland-demo::
    :source: text/String/BasicTests.cpp
    :exec: text/string --demo BasicTests
    :source-sha256: 33c9b69703ba4ffb03816392bb7b7c3bc26652068c697783fd667a87bcab0b84

.. code-block:: cpp

    /// Create a string that deliberately contains invalid UTF-8 bytes.
    ///
    /// This is only for demonstrating error handling. Do not construct strings this
    /// way in application code.
    auto createTextWithInvalidUtf8() -> el::String {
        constexpr auto bytes = std::array<const char, 15>{
            'S', 'u', 'n', 'n', 'y', ' ', static_cast<char>(0x82U), ' ', 'W', 'e', 'a', 't', 'h', 'e', 'r'};

        return el::U8StringEditor{std::string_view{bytes.data(), bytes.size()}};
    }

    /// This demo shows basic state checks for `String`.
    ///
    /// It demonstrates:
    /// - checking whether a string is empty,
    /// - checking whether its byte data is valid UTF-8,
    /// - safely printing strings even when invalid UTF-8 is present.
    ///
    /// Invalid UTF-8 data is handled safely. Encoding errors are represented with
    /// the Unicode replacement character U+FFFD.
    void basicTests() {
        const auto station = el::String{"🌦️ Station Süd: Nebel über dem Fjord"_el};
        const auto empty = el::String{};
        const auto invalidUtf8 = createTextWithInvalidUtf8();

        el::io::printLine("Input strings:"_el);
        el::io::printLine("  station     : \""_el, station, "\""_el);
        el::io::printLine("  empty       : \""_el, empty, "\""_el);
        el::io::printLine("  invalidUtf8 : \""_el, invalidUtf8, "\""_el);

        // `isEmpty()` checks whether the view contains no bytes.
        el::io::printLine("\nEmpty checks:"_el);
        el::io::printLine("  station.isEmpty()     : "_el, station.isEmpty());
        el::io::printLine("  empty.isEmpty()       : "_el, empty.isEmpty());
        el::io::printLine("  invalidUtf8.isEmpty() : "_el, invalidUtf8.isEmpty());

        // `isValidUtf8()` checks whether all bytes form valid UTF-8 sequences.
        el::io::printLine("\nUTF-8 validity checks:"_el);
        el::io::printLine("  station.isValidUtf8()     : "_el, station.isValidUtf8());
        el::io::printLine("  empty.isValidUtf8()       : "_el, empty.isValidUtf8());
        el::io::printLine("  invalidUtf8.isValidUtf8() : "_el, invalidUtf8.isValidUtf8());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Input strings:
      station     : "🌦️ Station Süd: Nebel über dem Fjord"
      empty       : ""
      invalidUtf8 : "Sunny � Weather"

    Empty checks:
      station.isEmpty()     : false

.. erbsland-demo-end::

Native Length and Code-Point Length
===================================

:cpp:func:`length() <erbsland::text::U8String::length>` returns the native storage length.
For UTF-8 strings, this is a :cpp:type:`ByteLength <erbsland::unit::ByteLength>`.
For UTF-16 strings, this is a :cpp:type:`U16DataLength <erbsland::unit::U16DataLength>`.
For UTF-32 strings, this is a :cpp:type:`CpLength <erbsland::unit::CpLength>`, because one UTF-32 storage element is one
decoded code point.

Native length is the right value for slicing, reserving storage, reporting byte sizes, and working with native indexes.
It is stored with the string range and is therefore fast to query.

:cpp:func:`characterLength() <erbsland::text::U8String::characterLength>` returns the number of decoded Unicode
code points as :cpp:type:`CpLength <erbsland::unit::CpLength>`.
For UTF-8 and UTF-16 text this requires decoding the string because one character may use multiple storage units.
For large strings, avoid calling it repeatedly inside loops.
If an algorithm needs to inspect each character anyway, count while iterating or use
:cpp:class:`StringCharReader <erbsland::text::StringCharReader>`.

.. erbsland-demo::
    :source: text/String/LengthAttributes.cpp
    :exec: text/string --demo LengthAttributes
    :source-sha256: 32ade239042c1883832a8732d3d3481c24b2f5944019ce1f838b18b4b7210c60

.. code-block:: cpp

    /// `length()` returns the native storage length, while `characterLength()`
    /// returns the decoded Unicode code-point length.
    ///
    /// UTF-8 counts bytes, UTF-16 counts 16-bit data units, and UTF-32 counts code
    /// points directly. For non-ASCII text these values often differ. Use the native
    /// length for storage ranges and indexes, and use the character length only when
    /// a user-visible code-point count is the actual question.
    void lengthAttributes() {
        const auto u8Reading = el::String{"温度計📡: 21℃"_el};
        const auto u16Reading = el::U16String{u"温度計📡: 21℃"_el};
        const auto u32Reading = el::U32String{U"温度計📡: 21℃"_el};
        const auto asciiLabel = el::String{"sensor-21"_el};

        el::io::printLine("Measurement label: "_el, u8Reading);
        el::io::printLine("UTF-8 native length ....: "_el, u8Reading.length());
        el::io::printLine("UTF-8 code points ......: "_el, u8Reading.characterLength());
        el::io::printLine("UTF-16 native length ...: "_el, u16Reading.length());
        el::io::printLine("UTF-16 code points .....: "_el, u16Reading.characterLength());
        el::io::printLine("UTF-32 code points .....: "_el, u32Reading.length());

        el::io::printLine();
        el::io::printLine("ASCII label: "_el, asciiLabel);
        el::io::printLine("ASCII bytes ............: "_el, asciiLabel.length());
        el::io::printLine("ASCII code points ......: "_el, asciiLabel.characterLength());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Measurement label: 温度計📡: 21℃
    UTF-8 native length ....: 20
    UTF-8 code points ......: 9
    UTF-16 native length ...: 10
    UTF-16 code points .....: 9
    UTF-32 code points .....: 9

    ASCII label: sensor-21
    ASCII bytes ............: 9
    ASCII code points ......: 9

.. erbsland-demo-end::

The different length types are intentionally not interchangeable.
This prevents accidental comparisons such as “byte length equals character length” from becoming a hidden Unicode bug.

Hashing Decoded Text
====================

Use :cpp:func:`toHash() <erbsland::text::U8String::toHash>` when a string is used as a key in hash-based containers.
The hash is calculated from decoded code points, not from raw storage bytes.
UTF-8, UTF-16, and UTF-32 strings that contain the same decoded text therefore produce the same regular hash value.

Use :cpp:func:`toHashCI() <erbsland::text::U8String::toHashCI>` when the matching rule is Unicode simple case-folded
text.
Pair it with a matching case-folded comparison rule.
Do not mix case-insensitive hashes with exact comparisons because equal hash buckets alone do not define equality.

The library also provides ``std::hash`` specializations for the string and string-view types.
Those specializations call ``toHash()`` and therefore use exact decoded text.

.. erbsland-demo::
    :source: text/String/Hashing.cpp
    :exec: text/string --demo Hashing
    :source-sha256: 95f8df37f8e3c8ea03135cbdd3de4bde0796df80232efec7efcb879bbeea6c3b

.. code-block:: cpp

    /// This demo shows how to make use of `String`s hash functions.
    void hashing() {
        // Here we create a set of different read-only string values.
        // For real code use the literals directly, like `const auto x = "abc"_el;`
        const auto titlecase = el::String{"Fichte"_el};
        const auto lowercase = el::String{"fichte"_el};
        const auto greek = el::String{"Σκιά"_el};
        const auto greekLower = el::String{"σκιά"_el};

        el::io::printLine("titlecase ............: "_el, titlecase);
        el::io::printLine("lowercase ............: "_el, lowercase);
        el::io::printLine("greek ................: "_el, greek);
        el::io::printLine("greekLower ...........: "_el, greekLower);
        el::io::printLine();

        // String hashes let you use read-only values in sets and unordered maps.
        el::io::printLine("\nRegular Hash Values:"_el);
        auto hash = titlecase.toHash();
        el::io::printLine("  titlecase.toHash()     → "_el, hashToString(hash));
        hash = lowercase.toHash();
        el::io::printLine("  lowercase.toHash()     → "_el, hashToString(hash));
        hash = greek.toHash();
        el::io::printLine("  greek.toHash()         → "_el, hashToString(hash));
        hash = greekLower.toHash();
        el::io::printLine("  greekLower.toHash()    → "_el, hashToString(hash));

        el::io::printLine("\nCase-Insensitive Hash Values:"_el);
        hash = titlecase.toHashCI();
        el::io::printLine("  titlecase.toHashCI()   → "_el, hashToString(hash));
        hash = lowercase.toHashCI();
        el::io::printLine("  lowercase.toHashCI()   → "_el, hashToString(hash));
        hash = greek.toHashCI();
        el::io::printLine("  greek.toHashCI()       → "_el, hashToString(hash));
        hash = greekLower.toHashCI();
        el::io::printLine("  greekLower.toHashCI()  → "_el, hashToString(hash));
    }

.. erbsland-ansi::
    :escape-char: ␛

    titlecase ............: Fichte
    lowercase ............: fichte
    greek ................: Σκιά
    greekLower ...........: σκιά


    Regular Hash Values:
      titlecase.toHash()     → 0x29b5533011ce1d3c
      lowercase.toHash()     → 0x29b554d5e70ddac6
      greek.toHash()         → 0x00028253e6edaba4
      greekLower.toHash()    → 0x00028253df6bdd02

    Case-Insensitive Hash Values:
      titlecase.toHashCI()   → 0x29b554d5e70ddac6
      lowercase.toHashCI()   → 0x29b554d5e70ddac6
      greek.toHashCI()       → 0x00028253df6bdd02
      greekLower.toHashCI()  → 0x00028253df6bdd02

.. erbsland-demo-end::

Storage Identifiers
===================

Use :cpp:func:`storageId() <erbsland::text::U8String::storageId>` only for low-level code that stores native indexes
outside the string object.
The returned :cpp:class:`StorageIdentifier <erbsland::mem::StorageIdentifier>` identifies the visible storage range of
the string or view.
It changes when a string detaches, reallocates, or when a view selects a different range.

This matters because a native index is meaningful only for the storage it came from.
A :cpp:type:`ByteIndex <erbsland::unit::ByteIndex>` into a UTF-8 view must not be reused with another UTF-8 view just
because the text looks similar.
The same applies to :cpp:type:`U16DataIndex <erbsland::unit::U16DataIndex>` for UTF-16 strings.

Store the storage identifier together with the native index when an index outlives the immediate operation:

.. erbsland-demo::
    :source: text/String/StorageIdentifier.cpp
    :exec: text/string --demo StorageIdentifier
    :source-sha256: db7ad310659cca32732d59a35d5df1dc884681ae0c0a21e9cad1c8b149e56dc1

.. code-block:: cpp

    /// `storageId()` lets low-level code verify that a cached native index still
    /// belongs to the same visible storage range.
    ///
    /// This is useful when a byte index outlives the immediate operation that
    /// produced it. Even when two strings contain the same decoded text, a native
    /// index from one storage range must not be applied to another one.
    void storageIdentifier() {
        struct CachedRange final {
            el::StorageIdentifier storageId;
            el::ByteIndex index;
            el::ByteLength length;
        };

        const auto report = el::StringEditor{"温度計A: 21℃; 気圧計B: 1012hPa; 湿度計C: 45%"_el};
        const auto reportView = el::String{report};
        const auto token = el::String{"気圧計"_el};
        const auto cachedToken = CachedRange{reportView.storageId(), reportView.find(token), token.length()};
        const auto booleanFormat = el::BooleanFormat::yesNo();

        const auto tryUseCachedRange = [&](const el::String &label, const el::String &candidate) -> void {
            const auto sameStorage = candidate.storageId() == cachedToken.storageId;
            el::io::printLine(label, ":"_el);
            el::io::printLine("  same visible storage range: "_el, booleanFormat, sameStorage);
            if (sameStorage && !cachedToken.index.isNoIndex()) {
                el::io::printLine(
                    "  cached range reads ........: "_el,
                    candidate.slice(el::ByteRange{cachedToken.index, cachedToken.length}));
            } else {
                el::io::printLine("  cached range reads ........: <not used>"_el);
            }
        };

        const auto copiedReport = reportView.copy();
        const auto copiedView = el::String{copiedReport};
        const auto tailView = reportView.slice(el::ByteRange{reportView.find(token), el::ByteLength::infinite()});

        el::io::printLine("Report: "_el, reportView);
        el::io::printLine("Cached token: "_el, token);
        el::io::printLine("Cached byte index: "_el, cachedToken.index);
        el::io::printLine();

        tryUseCachedRange("Original view"_el, reportView);
        tryUseCachedRange("Copied text"_el, copiedView);
        tryUseCachedRange("Tail slice"_el, tailView);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Report: 温度計A: 21℃; 気圧計B: 1012hPa; 湿度計C: 45%
    Cached token: 気圧計
    Cached byte index: 19

    Original view:
      same visible storage range: yes
      cached range reads ........: 気圧計
    Copied text:
      same visible storage range: no
      cached range reads ........: <not used>
    Tail slice:
      same visible storage range: no
      cached range reads ........: <not used>

.. erbsland-demo-end::

For ordinary string processing, prefer keeping indexes local, using
:cpp:class:`StringCharReader <erbsland::text::StringCharReader>`, or using slices returned by the string API.
Those patterns avoid manual storage identity checks.
