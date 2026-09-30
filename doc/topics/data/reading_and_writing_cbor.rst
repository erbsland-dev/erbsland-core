..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: CBOR; Reading and Writing
    single: CBOR; Parsing
    single: CBOR; Serialization
    single: DAG-CBOR

*************************************
Reading and Writing CBOR and DAG-CBOR
*************************************

CBOR lets you exchange a structured value as a compact binary block without converting numbers and bytes to text.
This page shows how to turn a complete block into a value tree, build and edit that tree, and write it back.
It also explains the types Core understands, the limits you can place on input, and how to select DAG-CBOR when a peer
requires its deterministic encoding.

What Is CBOR?
=============

The *Concise Binary Object Representation* is a binary format for individual data items.
An item may be a number, text or byte string, array, map, or tagged value; the same encoding can nest these items.
Unlike a text document, a byte string stays distinct from text, and a number carries a numeric representation on the
wire.
CBOR is useful in binary protocols, stored records, and exchanges where both sides agree on the meaning of the data.
For example, a sculpture catalog can keep a title as text, a year as a number, and a preview as bytes inside the same
record.
The full format is defined by `RFC 8949 <https://www.rfc-editor.org/rfc/rfc8949.html>`_.

Core reads one *complete* CBOR item per :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
That root can be a scalar, array, or map; it does not need a document wrapper.
Ordinary CBOR permits indefinite-length byte strings, text strings, arrays, and maps, where a break marker ends the
item.
Core accepts those forms but writes definite lengths, so reading and writing the same value need not reproduce the
original bytes.
Its :cpp:class:`CborValue <erbsland::data::cbor::CborValue>` model covers the types in `Supported Values and Format
Boundaries`_ rather than every extension defined for CBOR.

Parsing a Complete Item
=======================

Use :cpp:func:`CborValue::fromByteBlock <erbsland::data::cbor::CborValue::fromByteBlock>` when invalid input is an
expected validation result.
It returns an optional value, empty if parsing fails.
If you need a diagnostic exception, use
:cpp:func:`CborValue::fromByteBlockOrThrow <erbsland::data::cbor::CborValue::fromByteBlockOrThrow>`; malformed or
unsupported input raises :cpp:class:`ParseError <erbsland::err::ParseError>`.
Both forms require exactly one item and reject truncated input, a second trailing item, duplicate map keys, and
unsupported types.

This block holds a small Portuguese sculpture record, ``{"obra": "Lua"}``.
The example passes its complete bytes to the optional parser, then shows the throwing form on a break marker that cannot
stand alone.
It also reads an indefinite array and shows that writing it again gives it a definite length.

.. erbsland-demo::
    :source: data/DataFormats/CborExamples.cpp
    :function-blocks: parseCborBlock
    :function-blocks-sha256: a1de74090b93d18f169dbe842a13f0a5b3128528c920a652be8a1c3f8f818159
    :exec: data/data_formats --demo ParseCborBlock
    :source-sha256: 63ad7d16ab3c36c5d50ad02474dbf039e4d4274aa3f120b1d8bb2a5131588e74

.. code-block:: cpp

    void parseCborBlock() {
        // This block contains the map {"obra": "Lua"}.
        const auto bytes =
            el::ByteBlock::fromVector(std::vector<uint8_t>{0xa1, 0x64, 'o', 'b', 'r', 'a', 0x63, 'L', 'u', 'a'});
        if (const auto parsed = CborValue::fromByteBlock(bytes)) {
            el::io::printLine("Work: "_el, parsed->getOrThrow("obra"_el).getText().value());
        }
        // Ordinary CBOR may use an indefinite array; the writer gives it a definite length.
        const auto indefinite = el::ByteBlock::fromVector(std::vector<uint8_t>{0x9f, 0x01, 0x02, 0xff});
        const auto array = CborValue::fromByteBlockOrThrow(indefinite);
        el::io::printLine("Array items: "_el, array.itemCount().toSizeT());
        el::io::printLine("Rewritten bytes: "_el, array.toByteBlock().length().toRawValue());
        try {
            (void)CborValue::fromByteBlockOrThrow(el::ByteBlock{el::Byte{0xffU}});
        } catch (const el::err::ParseError &) {
            el::io::printLine("Invalid CBOR item"_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Work: Lua
    Array items: 2
    Rewritten bytes: 3
    Invalid CBOR item

.. erbsland-demo-end::

Building a Value Tree
=====================

A default ``CborValue`` is null.
Construct other scalar values directly, or begin with :cpp:type:`CborMap <erbsland::data::cbor::CborMap>` and
:cpp:type:`CborArray <erbsland::data::cbor::CborArray>` for containers.
``set(key, value)`` adds or replaces a map member, while ``append(value)`` adds an array item.
``set(index, value)`` replaces an existing array item or appends at exactly the current size; a larger index throws an
out-of-range error.
Calling either mutator on the wrong container type throws a logic error.

The exhibition below has a title, a year, and an ordered list of works.
It shows how you can construct a nested tree without first creating a CBOR byte sequence.

.. erbsland-demo::
    :source: data/DataFormats/CborExamples.cpp
    :function-blocks: makeExhibition buildCborValue
    :function-blocks-sha256: afca94cd5742c2825652dd71f39a7ad966b5ff4eb68a4d688c371231b596d077
    :exec: data/data_formats --demo BuildCborValue
    :source-sha256: 63ad7d16ab3c36c5d50ad02474dbf039e4d4274aa3f120b1d8bb2a5131588e74

.. code-block:: cpp

    auto makeExhibition() -> CborValue {
        auto works = CborValue{CborArray{}};
        works.append(CborValue{"Maré"_el}).append(CborValue{"Horizonte"_el});
        auto exhibition = CborValue{CborMap{}};
        exhibition.set("mostra"_el, CborValue{"Esculturas do Mar"_el});
        exhibition.set("obras"_el, std::move(works));
        exhibition.set("ano"_el, CborValue{2026});
        return exhibition;
    }

    void buildCborValue() {
        const auto exhibition = makeExhibition();
        el::io::printLine("Fields: "_el, exhibition.itemCount().toSizeT());
        el::io::printLine("Works: "_el, exhibition.getOrThrow("obras"_el).itemCount().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Fields: 3
    Works: 2

.. erbsland-demo-end::

Serializing a Value
===================

:cpp:func:`toByteBlock <erbsland::data::cbor::CborValue::toByteBlock>` serializes any supported root value as one
complete CBOR item.
The result is binary data; store or transmit it as bytes, not as a string.
The default :cpp:class:`CborFormatOptions <erbsland::data::cbor::CborFormatOptions>` writes ordinary CBOR with definite
containers and strings, shortest integer and length headers, and 64-bit floating-point numbers.
If a value cannot be represented, such as an invalid date or CID link, serialization raises a parameter error.

The example writes the exhibition, reads its block, and retrieves the title from the received tree.
Notice what happens to a positive value constructed as ``int64_t``: CBOR encodes it as an unsigned integer, so its
semantic type is ``Unsigned`` after parsing.
The numeric value is unchanged, and ``getSigned()`` can still read it while it fits in ``int64_t``.

.. erbsland-demo::
    :source: data/DataFormats/CborExamples.cpp
    :function-blocks: serializeCborValue
    :function-blocks-sha256: 128a820ec2eb38222cb4ea0ff693a3c4ed7978f201edd1f81c0cd6908327d32e
    :exec: data/data_formats --demo SerializeCborValue
    :source-sha256: 63ad7d16ab3c36c5d50ad02474dbf039e4d4274aa3f120b1d8bb2a5131588e74

.. code-block:: cpp

    void serializeCborValue() {
        const auto original = makeExhibition();
        const auto bytes = original.toByteBlock();
        const auto received = CborValue::fromByteBlockOrThrow(bytes);
        el::io::printLine("Encoded bytes: "_el, bytes.length().toRawValue());
        el::io::printLine("Exhibition: "_el, received.getOrThrow("mostra"_el).getText().value());
        const auto positiveSigned = CborValue{int64_t{2026}};
        const auto roundTrip = CborValue::fromByteBlockOrThrow(positiveSigned.toByteBlock());
        el::io::printLine("Positive integer reads as unsigned: "_el, roundTrip.is(CborType::Unsigned));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Encoded bytes: 56
    Exhibition: Esculturas do Mar
    Positive integer reads as unsigned: true

.. erbsland-demo-end::

Finding and Editing Children
============================

``get(key)`` and ``get(index)`` return a null ``CborValue`` if the requested child is absent.
Since a real CBOR null looks the same, use ``getOrThrow`` when absence must be an error.
It raises an out-of-range error for a missing key or index and a logic error when the parent is the wrong container
type.
``itemCount()`` returns a map's field count or an array's element count, and zero for a scalar.

Check :cpp:func:`type <erbsland::data::cbor::CborValue::type>` or ``is(type)`` before interpreting a value.
Typed getters such as ``getText()``, ``getBytes()``, and ``getFloat()`` return an optional result.
``getSigned()`` can also read an unsigned value through ``INT64_MAX``; ``getUnsigned()`` can read a nonnegative signed
value.
Other getters need their matching semantic type.
``getArray()`` and ``getMap()`` return copies of the containers.

Child access returns a value, not an editor into the parent.
To change a nested child, edit the copy and set it back.
``CborValue`` uses copy-on-write storage, so editing a copy of the parsed exhibition leaves the original alone.

.. erbsland-demo::
    :source: data/DataFormats/CborExamples.cpp
    :function-blocks: inspectCborValue
    :function-blocks-sha256: d4c9a700bbfc30dd8ac74b934a62c85bfb521a7199271d2edbf60a4eb9528e3d
    :exec: data/data_formats --demo InspectCborValue
    :source-sha256: 63ad7d16ab3c36c5d50ad02474dbf039e4d4274aa3f120b1d8bb2a5131588e74

.. code-block:: cpp

    void inspectCborValue() {
        const auto original = CborValue::fromByteBlockOrThrow(makeExhibition().toByteBlock());
        const auto works = original.getOrThrow("obras"_el);
        el::io::printLine("First work: "_el, works.getOrThrow(el::ItemIndex{0U}).getText().value());
        el::io::printLine("Missing is null: "_el, original.get("artista"_el).is(CborType::Null));

        auto revised = original;
        auto revisedWorks = revised.getOrThrow("obras"_el);
        revisedWorks.append(CborValue{"Vento"_el});
        revised.set("obras"_el, revisedWorks);
        el::io::printLine("Original works: "_el, original.getOrThrow("obras"_el).itemCount().toSizeT());
        el::io::printLine("Revised works: "_el, revised.getOrThrow("obras"_el).itemCount().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    First work: Maré
    Missing is null: true
    Original works: 2
    Revised works: 3

.. erbsland-demo-end::

Supported Values and Format Boundaries
======================================

:cpp:enum:`CborType <erbsland::data::cbor::CborType>` identifies the semantic type stored by Core.
This table shows how it relates to the CBOR item you exchange.

.. list-table:: CBOR values in Erbsland Core
    :header-rows: 1
    :widths: 25 32 43

    * - CBOR wire item
      - Core type and getter
      - Notes
    * - Null and Boolean
      - ``Null``; ``Bool`` / ``getBool()``
      - A default value is null.
    * - Unsigned integer
      - ``Unsigned`` / ``getUnsigned()``
      - Full CBOR ``uint64_t`` range is accepted.
    * - Negative integer
      - ``Signed`` / ``getSigned()``
      - Limited to the ``int64_t`` range.
    * - Half-, single-, or double-precision float
      - ``Float`` / ``getFloat()``
      - Read as ``double``; written as 64-bit float.
    * - Text and byte string
      - ``Text`` / ``getText()``; ``Bytes`` / ``getBytes()``
      - DAG-CBOR validates UTF-8 text; binary bytes remain distinct.
    * - Array and map
      - ``Array`` / ``getArray()``; ``Map`` / ``getMap()``
      - Maps require unique text keys.
    * - Date/time tags 0 and 1
      - ``DateTime`` / ``getDateTime()``
      - Ordinary CBOR only; written with tag 0 as UTC text.
    * - CID link tag 42
      - ``Link`` / ``getLinkBytes()``
      - The getter returns binary CID bytes without the wire prefix.

CBOR itself allows more map-key types, simple values, and application tags.
Core rejects nontext map keys, unsupported simple values, and tags other than 0, 1, and 42. It also rejects a negative
CBOR integer below ``INT64_MIN``.
The in-memory map is keyed by Core strings, so map order from an ordinary CBOR input is not retained through a round
trip.
Ordinary parsing does not validate every text string as UTF-8; use DAG-CBOR parsing when that validation is required by
the exchange profile.

Dates and CID Links
-------------------

In ordinary CBOR, tag 0 carries a date/time as text and tag 1 carries seconds relative to the Unix epoch.
Core reads either as :cpp:class:`DateTime <erbsland::time::DateTime>` and writes a ``DateTime`` with tag 0 in UTC.
Malformed or unrepresentable dates fail parsing; an invalid date fails serialization.
Date/time tags are excluded from DAG-CBOR.

Tag 42 carries a CID link in both modes.
On the wire, its byte string begins with a zero byte followed by a valid binary CID.
Core exposes only the CID bytes as :cpp:struct:`CborLink <erbsland::data::cbor::CborLink>` or through
``getLinkBytes()``; you do not include the zero prefix when constructing a link.
Parsing and serialization validate the CID structure.
The next demo recognizes a tagged date and a CID link without interpreting the link as text.
It then writes the date as tag 0 and constructs a new link value from the CID bytes for a DAG-CBOR round trip.

.. erbsland-demo::
    :source: data/DataFormats/CborExamples.cpp
    :function-blocks: readCborTags
    :function-blocks-sha256: 7ee8bc4f393a6f52e0a696e900b8b07ebb39a3a27eeefa90f8b19043c9e8e8b8
    :exec: data/data_formats --demo ReadCborTags
    :source-sha256: 63ad7d16ab3c36c5d50ad02474dbf039e4d4274aa3f120b1d8bb2a5131588e74

.. code-block:: cpp

    void readCborTags() {
        // Tag 1 wraps the number 1.5: one and a half seconds after the Unix epoch.
        const auto timeBytes = el::ByteBlock::fromVector(std::vector<uint8_t>{0xc1, 0xfb, 0x3f, 0xf8, 0, 0, 0, 0, 0, 0});
        const auto date = CborValue::fromByteBlockOrThrow(timeBytes);
        el::io::printLine("Date/time value: "_el, date.is(CborType::DateTime));
        el::io::printLine(
            "Written date/time: "_el, CborValue::fromByteBlockOrThrow(date.toByteBlock()).is(CborType::DateTime));

        // Tag 42 wraps a zero prefix and a binary CIDv0 (SHA-256 multihash).
        auto linkWire = std::vector<uint8_t>{0xd8, 0x2a, 0x58, 0x23, 0x00, 0x12, 0x20};
        linkWire.insert(linkWire.end(), 32U, 0U);
        const auto link =
            CborValue::fromByteBlockOrThrow(el::ByteBlock::fromVector(linkWire), CborParseOptions{}.setDagCbor(true));
        el::io::printLine("CID bytes: "_el, link.getLinkBytes()->length().toRawValue());
        const auto rewritten =
            CborValue{el::cbor::CborLink{link.getLinkBytes().value()}}.toByteBlock(CborFormatOptions{}.setDagCbor(true));
        el::io::printLine("Link round trip: "_el, rewritten == el::ByteBlock::fromVector(linkWire));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Date/time value: true
    Written date/time: true
    CID bytes: 34
    Link round trip: true

.. erbsland-demo-end::

Exchanging DAG-CBOR
===================

DAG-CBOR is a restricted CBOR profile for content-addressed data, where equivalent values must have a predictable byte
representation.
Select it on *both* sides: pass ``CborFormatOptions{}.setDagCbor(true)`` to ``toByteBlock()`` and
``CborParseOptions{}.setDagCbor(true)`` to the parser.
The `DAG-CBOR specification <https://ipld.io/specs/codecs/dag-cbor/spec/>`_ defines the profile.

Core writes definite forms, shortest integer and length headers, text keys ordered by their encoded bytes, and 64-bit
floats.
The DAG-CBOR parser checks those wire rules; it also rejects invalid UTF-8, indefinite forms, nonfinite floats, negative
zero, and tags other than the CID link tag 42. The writer rejects invalid UTF-8, nonfinite floats, and date/time values,
and normalizes negative zero to positive zero.
An ordinary CBOR decoder may accept a different encoding for the same logical value, so use the DAG-CBOR option when
validating bytes received under this profile.

.. erbsland-demo::
    :source: data/DataFormats/CborExamples.cpp
    :function-blocks: useDagCbor
    :function-blocks-sha256: 29771f6dcd3837c4e7ee4efb970b7e0f7a1425fbbe679dc5a437d0839d98b245
    :exec: data/data_formats --demo UseDagCbor
    :source-sha256: 63ad7d16ab3c36c5d50ad02474dbf039e4d4274aa3f120b1d8bb2a5131588e74

.. code-block:: cpp

    void useDagCbor() {
        const auto exhibition = makeExhibition();
        const auto bytes = exhibition.toByteBlock(CborFormatOptions{}.setDagCbor(true));
        const auto parsed = CborValue::fromByteBlockOrThrow(bytes, CborParseOptions{}.setDagCbor(true));
        el::io::printLine("DAG-CBOR works: "_el, parsed.getOrThrow("obras"_el).itemCount().toSizeT());

        // The integer 1 encoded in two bytes is valid ordinary CBOR, but not DAG-CBOR.
        const auto longOne = el::ByteBlock::fromVector(std::vector<uint8_t>{0x18, 0x01});
        el::io::printLine("Ordinary accepts: "_el, CborValue::fromByteBlock(longOne).has_value());
        el::io::printLine(
            "DAG-CBOR accepts: "_el, CborValue::fromByteBlock(longOne, CborParseOptions{}.setDagCbor(true)).has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    DAG-CBOR works: 2
    Ordinary accepts: true
    DAG-CBOR accepts: false

.. erbsland-demo-end::

Limiting Untrusted Input
========================

:cpp:class:`CborParseOptions <erbsland::data::cbor::CborParseOptions>` combines the profile switch with four
independent input limits.
An exceeded limit is a parse failure in both parser forms.
Defaults are useful for ordinary records, but a known schema can often use smaller limits.
This demo applies each one to the same exhibition block so you can see what it protects.

.. erbsland-demo::
    :source: data/DataFormats/CborExamples.cpp
    :function-blocks: limitCborInput
    :function-blocks-sha256: 0cbd7443a084dad05374eb9f9c6d4c065d99cca3a44054fad89ac972a1d69111
    :exec: data/data_formats --demo LimitCborInput
    :source-sha256: 63ad7d16ab3c36c5d50ad02474dbf039e4d4274aa3f120b1d8bb2a5131588e74

.. code-block:: cpp

    void limitCborInput() {
        const auto bytes = makeExhibition().toByteBlock();
        const auto input = CborParseOptions{}.setMaximumInputLength(el::ByteLength{10U});
        const auto nesting = CborParseOptions{}.setMaximumNesting(el::ItemCount{1U});
        const auto values = CborParseOptions{}.setMaximumValueCount(el::ItemCount{3U});
        const auto strings = CborParseOptions{}.setMaximumStringLength(el::ByteLength{4U});
        el::io::printLine("Input accepted: "_el, CborValue::fromByteBlock(bytes, input).has_value());
        el::io::printLine("Nesting accepted: "_el, CborValue::fromByteBlock(bytes, nesting).has_value());
        el::io::printLine("Values accepted: "_el, CborValue::fromByteBlock(bytes, values).has_value());
        el::io::printLine("Text accepted: "_el, CborValue::fromByteBlock(bytes, strings).has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Input accepted: false
    Nesting accepted: false
    Values accepted: false
    Text accepted: false

.. erbsland-demo-end::

Maximum Input Length
--------------------

``setMaximumInputLength(ByteLength{...})`` bounds the complete byte block before parsing begins.
The default is 16 MiB.
In the demo, a ten-byte limit rejects the whole exhibition block even though its individual strings are small.

Maximum Nesting
---------------

``setMaximumNesting(ItemCount{...})`` bounds array and map depth; the default is 64. The root container counts as one
level, so the demo's limit of one permits a flat map but rejects its nested ``obras`` array.
This limit is useful when a peer can supply deeply nested containers.

Maximum Value Count
-------------------

``setMaximumValueCount(ItemCount{...})`` bounds the total number of parsed values, including the root, map keys, and
nested items.
The default is 1,000,000. The demo sets a limit of three, which the exhibition exceeds before the parser reaches the
end.

Maximum String Length
---------------------

``setMaximumStringLength(ByteLength{...})`` bounds the byte length of each text string, including map keys.
The default is 8 MiB.
It is a per-string limit, rather than a total text budget; the complete block still obeys the input-length limit.
The demo's four-byte setting rejects longer text in the exhibition.

The DAG-CBOR switch, ``setDagCbor(bool)``, is also a parse option.
It changes which wire forms are accepted as shown in `Exchanging DAG-CBOR`_; it does not replace any of these four
resource limits.
