..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: BSON; Reading and Writing
    single: BSON; Parsing
    single: BSON; Serialization

************************
Reading and Writing BSON
************************

BSON is useful when a document must travel as binary data while retaining types that JSON cannot express directly, such
as binary bytes and UTC timestamps.
This page takes you from a complete BSON frame to a value tree and back.
It also explains which values Erbsland Core interprets, how to handle other BSON types, and how to set limits before
reading untrusted data.

What BSON Stores
================

BSON means *Binary JSON*, but its values are richer than JSON values.
A BSON document is a sequence of named, typed fields.
Each field carries a type code, a NUL-terminated key, and a value payload; the document starts with its byte length.
Multibyte numbers use little-endian order.
Arrays are stored as documents whose keys are consecutive decimal indexes.
These details matter when you exchange bytes with another BSON implementation, but the value API handles the framing for
you.
The complete wire format is defined by the `BSON specification <https://bsonspec.org/spec.html>`_.

BSON is common in document-oriented systems and in protocols that already exchange BSON documents.
It is a good fit when the receiving side expects BSON's explicit integer widths, binary subtypes, or date type.
The root of a BSON frame is always a document: an array or scalar may appear inside it, but cannot be serialized as the
frame itself.

Core's BSON Model
=================

:cpp:class:`BsonValue <erbsland::data::bson::BsonValue>` is the value tree.
A default-constructed value represents BSON null.
:cpp:type:`BsonDocument <erbsland::data::bson::BsonDocument>` maps field names to values, while
:cpp:type:`BsonArray <erbsland::data::bson::BsonArray>` holds values in order.
``BsonDocument`` is a key-sorted map, so serializing a parsed document may change field order.
Do not rely on byte-for-byte equality of an entire frame when the original document has multiple fields.
Values have copy-on-write behavior: copies may share their contents until one copy is changed.
This is useful when you want to edit a received document while retaining the original.

The library interprets the types most applications use directly and preserves several other recognized wire types as
opaque values.
It does not invent an unsigned 64-bit BSON type or convert special BSON values into unrelated Core types.
See `Supported and Preserved Types`_ for the full mapping.

Parsing a Complete Frame
========================

Pass the complete byte block to :cpp:func:`BsonValue::fromByteBlock <erbsland::data::bson::BsonValue::fromByteBlock>`
when an invalid frame is an ordinary validation outcome.
It returns an optional value, which is empty if parsing fails.
Use :cpp:func:`BsonValue::fromByteBlockOrThrow <erbsland::data::bson::BsonValue::fromByteBlockOrThrow>` when the caller
needs a
:cpp:class:`ParseError <erbsland::err::ParseError>`.
The parser requires exactly one document: truncated input, malformed lengths, duplicate document keys, invalid array
indexes, unknown type codes, and trailing bytes are rejected.

The following frame contains a Turkish plant name, ``{"bitki": "lale"}``.
The byte array is complete, so it can be passed directly to the parser.

.. erbsland-demo::
    :source: data/DataFormats/BsonExamples.cpp
    :show: f:parseBsonDocument
    :exec: data/data_formats --demo ParseBsonDocument
    :sha256: 2bc13a4f79bdea77ddc8bca7e40791dca9c47b85ccceac58af9d7a4d48036e94

.. code-block:: cpp

    void parseBsonDocument() {
        // This complete BSON frame contains {"bitki": "lale"}.
        const auto bytes = el::ByteBlock::fromVector(
            std::vector<uint8_t>{21, 0, 0, 0, 2, 'b', 'i', 't', 'k', 'i', 0, 5, 0, 0, 0, 'l', 'a', 'l', 'e', 0, 0});
        const auto parsed = BsonValue::fromByteBlock(bytes);
        if (parsed) {
            el::io::printLine("Plant: "_el, parsed->getOrThrow("bitki"_el).getText().value());
        }

        try {
            (void)BsonValue::fromByteBlockOrThrow(el::ByteBlock{el::Byte{0U}});
        } catch (const el::err::ParseError &) {
            el::io::printLine("Invalid BSON document"_el);
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mPlant: lale
        Invalid BSON document␛[0m

.. erbsland-demo-end::

Building a Document
===================

Begin with a document value, then use :cpp:func:`set <erbsland::data::bson::BsonValue::set>` to add or replace fields.
For an array, :cpp:func:`append <erbsland::data::bson::BsonValue::append>` adds the next value.
You can also use ``set(index, value)`` to replace an existing array item or append at exactly the current array size; an
index beyond that position throws an out-of-range error.
Calling ``set`` on the wrong container type throws a logic error.

The example groups moisture readings under ``nem`` and stores a small sample as binary data.
:cpp:struct:`BsonBinary <erbsland::data::bson::BsonBinary>` combines payload bytes with their BSON subtype.
If you construct a value from a plain :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`, the subtype is zero.
Use ``BsonBinary`` when the other side needs a specific subtype.

.. erbsland-demo::
    :source: data/DataFormats/BsonExamples.cpp
    :show: f:makePlantRecord, f:buildBsonDocument
    :exec: data/data_formats --demo BuildBsonDocument
    :sha256: 17c884e714ad1ce8e3f4f71e8a4d856f8f1bbf9b72104f23c927c7188e0e94b7

.. code-block:: cpp

    auto makePlantRecord() -> BsonValue {
        auto readings = BsonValue{BsonArray{}};
        readings.append(BsonValue{int32_t{42}}).append(BsonValue{int32_t{45}});
        auto record = BsonValue{BsonDocument{}};
        record.set("bitki"_el, BsonValue{"lale"_el});
        record.set("nem"_el, std::move(readings));
        record.set("etkin"_el, BsonValue{true});
        return record;
    }

    void buildBsonDocument() {
        auto record = makePlantRecord();
        record.set("ornek"_el, BsonValue{BsonBinary{el::ByteBlock{el::Byte{1U}, el::Byte{2U}}, 0x80U}});
        el::io::printLine("Fields: "_el, record.itemCount().toSizeT());
        el::io::printLine("Readings: "_el, record.getOrThrow("nem"_el).itemCount().toSizeT());
        el::io::printLine("Binary subtype: "_el, static_cast<int>(record.getOrThrow("ornek"_el).getBinary()->subtype));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFields: 4
        Readings: 2
        Binary subtype: 128␛[0m

.. erbsland-demo-end::

Serializing a Document
======================

:cpp:func:`toByteBlock <erbsland::data::bson::BsonValue::toByteBlock>` writes a complete BSON frame that you can store
or send to a peer.
It requires a document root and throws :cpp:class:`ParameterFailure <erbsland::err::ParameterFailure>` for an array or
scalar root, a key containing NUL, a value that cannot be represented, or an encoded document that is too long.
The output is binary data; do not treat it as text.

.. erbsland-demo::
    :source: data/DataFormats/BsonExamples.cpp
    :show: f:serializeBsonDocument
    :exec: data/data_formats --demo SerializeBsonDocument
    :sha256: 29e3c4d56e3aa3d8864ee1c1161889d2871e4e916ac66adcf8878dc5aba4ad39

.. code-block:: cpp

    void serializeBsonDocument() {
        const auto record = makePlantRecord();
        const auto bytes = record.toByteBlock();
        el::io::printLine("Encoded bytes: "_el, bytes.length().toRawValue());
        el::io::printLine("Round-trip fields: "_el, BsonValue::fromByteBlockOrThrow(bytes).itemCount().toSizeT());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mEncoded bytes: 53
        Round-trip fields: 3␛[0m

.. erbsland-demo-end::

The byte block uses the default format behavior.
:cpp:class:`BsonFormatOptions <erbsland::data::bson::BsonFormatOptions>` is an extension point and currently has no
settings, so passing ``BsonFormatOptions{}`` explicitly has the same effect as omitting it.

Finding and Editing Values
==========================

After parsing, use ``get(key)`` for an optional field or ``get(index)`` for an array position.
Both return a null :cpp:class:`BsonValue <erbsland::data::bson::BsonValue>` when the child is absent.
That result is also a real BSON null, so use ``getOrThrow`` when absence must be distinguished from a stored null.
``getOrThrow`` raises an out-of-range error for a missing document key or array index and a logic error if the value is
not the requested container type.

Use :cpp:func:`type <erbsland::data::bson::BsonValue::type>` or ``is(type)`` to inspect the kind of a value.
Typed getters such as ``getText()``, ``getInteger()``, and ``getBinary()`` return an optional result, empty when the
type does not match.
``getInteger()`` accepts both BSON integer widths and returns an ``int64_t`` without losing their integer value.
``itemCount()`` reports the number of array items or document fields and zero for a scalar.
``getArray()`` and ``getDocument()`` return copies of their containers.

Child access returns a value, rather than an editor into its parent.
To change a nested array, edit the returned array value and set it back into the document.
The original document in this example keeps its two readings when the copy gains a third.

.. erbsland-demo::
    :source: data/DataFormats/BsonExamples.cpp
    :show: f:inspectBsonDocument
    :exec: data/data_formats --demo InspectBsonDocument
    :sha256: 7bfcb4dde3d531d68c4f4e7c77ce6e2f27fba84f9cf2799879574b80c74e142a

.. code-block:: cpp

    void inspectBsonDocument() {
        const auto original = BsonValue::fromByteBlockOrThrow(makePlantRecord().toByteBlock());
        const auto readings = original.getOrThrow("nem"_el);
        el::io::printLine("First reading: "_el, readings.getOrThrow(el::ItemIndex{0U}).getInteger().value());
        el::io::printLine("Missing is null: "_el, original.get("konum"_el).is(BsonType::Null));

        auto revised = original;
        auto changedReadings = revised.getOrThrow("nem"_el);
        changedReadings.append(BsonValue{int32_t{47}});
        revised.set("nem"_el, changedReadings);
        el::io::printLine("Original readings: "_el, original.getOrThrow("nem"_el).itemCount().toSizeT());
        el::io::printLine("Revised readings: "_el, revised.getOrThrow("nem"_el).itemCount().toSizeT());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFirst reading: 42
        Missing is null: true
        Original readings: 2
        Revised readings: 3␛[0m

.. erbsland-demo-end::

Supported and Preserved Types
=============================

The value's :cpp:enum:`BsonType <erbsland::data::bson::BsonType>` tells you which Core representation you received.
This table separates values that Core interprets from recognized BSON types it preserves for round trips.

.. list-table:: BSON values in Erbsland Core
    :header-rows: 1
    :widths: 25 32 43

    * - BSON wire type
      - Core representation
      - Notes
    * - Null and Boolean
      - ``Null``, ``Bool``
      - A default ``BsonValue`` is null; ``getBool()`` reads a Boolean.
    * - 32-bit and 64-bit signed integers
      - ``Int32``, ``Int64``
      - ``getInteger()`` reads either width as ``int64_t``.
    * - Double and UTF-8 string
      - ``Double``, ``Text``
      - Read with ``getDouble()`` and ``getText()``.
    * - Binary
      - ``Binary``
      - ``getBinary()`` returns bytes and the subtype.
    * - UTC datetime
      - ``DateTime``
      - ``getTimestamp()`` returns a Core ``Timestamp`` at millisecond precision.
    * - Array and embedded document
      - ``Array``, ``Document``
      - Read with ``getArray()`` and ``getDocument()`` or navigate by index and key.
    * - Recognized other BSON types
      - ``Opaque``
      - The original type code and encoded payload are retained without interpretation.

Smaller signed integers and unsigned integers through ``uint32_t`` can be passed to a ``BsonValue`` constructor.
They are stored as ``Int32`` or ``Int64`` as needed.
A ``uint64_t`` is accepted only through ``INT64_MAX``; a larger value throws a parameter error because BSON has no
unsigned 64-bit integer value.
The constructor's chosen integer width matters if another application distinguishes 32-bit from 64-bit values.

BSON UTC datetimes count milliseconds from the Unix epoch.
Core rejects a :cpp:class:`Timestamp <erbsland::time::Timestamp>` with a finer fraction when writing BSON, instead of
silently rounding it.
The datetime must also be valid and representable.
For binary subtype 2, the old binary format includes an extra length field; Core reads and writes that framing while
presenting only the payload bytes in ``BsonBinary``.

Opaque Values
-------------

Recognized types such as ObjectId, regular expression, Decimal128, timestamp, and code-like BSON values are returned as
:cpp:class:`BsonOpaqueValue <erbsland::data::bson::BsonOpaqueValue>`.
You can inspect ``typeCode()`` and the encoded ``payload()``; serializing that value retains its type code and payload.
The containing document's field order may still change, as described above.
Core does not evaluate a script-like payload.
An unknown type code cannot be safely framed and is a parse error.

The example reads an ObjectId as an opaque value and verifies that the complete document survives serialization.

.. erbsland-demo::
    :source: data/DataFormats/BsonExamples.cpp
    :show: f:preserveOpaqueBsonValue
    :exec: data/data_formats --demo PreserveOpaqueBsonValue
    :sha256: 67a378b3effb0717d265bf939f4ff69577ede31fba9856e1983d765ef518c35a

.. code-block:: cpp

    void preserveOpaqueBsonValue() {
        const auto bytes = el::ByteBlock::fromVector(
            std::vector<uint8_t>{21, 0, 0, 0, 7, 'i', 'd', 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0});
        const auto record = BsonValue::fromByteBlockOrThrow(bytes);
        const auto opaque = record.getOrThrow("id"_el).getOpaque().value();
        el::io::printLine("Type code: "_el, static_cast<int>(opaque.typeCode()));
        el::io::printLine("Payload bytes: "_el, opaque.payload().length().toRawValue());
        el::io::printLine("Preserved: "_el, record.toByteBlock() == bytes);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mType code: 7
        Payload bytes: 12
        Preserved: true␛[0m

.. erbsland-demo-end::

Limiting Untrusted Input
========================

:cpp:class:`BsonParseOptions <erbsland::data::bson::BsonParseOptions>` applies four independent safety limits.
Both parser variants accept it as their second argument.
An exceeded limit is a parse failure: the optional parser returns no value, and the throwing parser raises
``ParseError``.
Defaults suit a typical document, but you can lower them to match the size and shape your application expects.

This example applies each limit separately to the same plant record.

.. erbsland-demo::
    :source: data/DataFormats/BsonExamples.cpp
    :show: f:limitBsonDocument
    :exec: data/data_formats --demo LimitBsonDocument
    :sha256: 254e0bc83d94a4bc291147c55bb88d5ff779b2e9efca24015cfb94b2902b8748

.. code-block:: cpp

    void limitBsonDocument() {
        const auto bytes = makePlantRecord().toByteBlock();
        const auto input = BsonParseOptions{}.setMaximumInputLength(el::ByteCount{10U});
        const auto nesting = BsonParseOptions{}.setMaximumNesting(el::ItemCount{1U});
        const auto values = BsonParseOptions{}.setMaximumValueCount(el::ItemCount{2U});
        const auto strings = BsonParseOptions{}.setMaximumStringLength(el::ByteCount{3U});
        el::io::printLine("Input accepted: "_el, BsonValue::fromByteBlock(bytes, input).has_value());
        el::io::printLine("Nesting accepted: "_el, BsonValue::fromByteBlock(bytes, nesting).has_value());
        el::io::printLine("Values accepted: "_el, BsonValue::fromByteBlock(bytes, values).has_value());
        el::io::printLine("Strings accepted: "_el, BsonValue::fromByteBlock(bytes, strings).has_value());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mInput accepted: false
        Nesting accepted: false
        Values accepted: false
        Strings accepted: false␛[0m

.. erbsland-demo-end::

Maximum Input Length
--------------------

``setMaximumInputLength(ByteCount)`` caps the complete frame in bytes, including its length field and terminator.
The default is 16 MiB.
In the demo, a ten-byte limit rejects the document before its fields are read.
Choose this limit from the largest frame your protocol permits.

Maximum Nesting
---------------

``setMaximumNesting(ItemCount)`` caps document and array levels.
The root document counts as one level; a nested array or document adds another.
The default is 64 levels.
In the demo, a limit of one accepts a flat root but rejects the ``nem`` array within it.

Maximum Value Count
-------------------

``setMaximumValueCount(ItemCount)`` caps the total number of fields and array elements encountered across the frame.
The root document itself is not counted as a value; each of its fields is.
The default is 1,000,000 values.
The demo's limit of two rejects a record with three fields and two array elements.

Maximum String Length
---------------------

``setMaximumStringLength(ByteCount)`` caps the UTF-8 byte length of each individual key or string value, excluding its
NUL terminator.
The default is 8 MiB per string or key.
The demo's three-byte limit rejects the four-byte name ``lale`` and the five-byte key ``bitki``.
This is a byte limit, so a short visible word containing multibyte characters can occupy more than its character count.
