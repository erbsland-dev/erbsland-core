.. index::
    single: Memory; Byte buffers
    single: Byte buffer; Reusable storage
    single: Binary data; Mutable buffers

******************
Using Byte Buffers
******************

A :cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` is reusable working storage for a sequence of bytes whose length
changes at runtime.
It is useful when you repeatedly assemble, resize, or revise binary data and want to retain an allocation between
iterations.
This page explains how to create and reuse a buffer, access bytes and integers safely, reshape ranges, and cross the
boundary to standard contiguous storage.

Choosing a Mutable Working Buffer
=================================

The defining property of :cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` is unique ownership.
Copying a buffer makes an independent copy of its visible bytes, so later changes to either value cannot affect the
other.
That behavior is easy to reason about for scratch space and long-lived mutable state.

For data that has finished changing and now needs to travel through an application, prefer
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
Blocks use copy-on-write storage and are inexpensive to pass and retain.
For a short sequence of edits that immediately becomes a block,
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` is often the more direct builder.
A :cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` solves a different problem: it queues a continuous FIFO
stream, where reads consume the oldest bytes.

Creating Independent Buffers
============================

A buffer can start empty, with a chosen number of repeated bytes, from an initializer list, or by copying a
:cpp:type:`ConstByteSpan <erbsland::mem::ConstByteSpan>`.
The span constructor is particularly useful at an ownership boundary: once construction returns, the source may move or
disappear without invalidating the buffer.

Copy construction and copy assignment are deep-copy operations.
This differs deliberately from :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`, whose copies normally share storage.
Moving a buffer transfers its allocation and is the natural choice when a completed working buffer changes owners.

.. erbsland-demo::
    :source: mem/ByteBuffer/CreatingBuffers.cpp
    :exec: mem/byte_buffer --demo CreatingBuffers
    :source-sha256: 448ce27c494cad0341f672512c1ecb5f4306bbe4562f63a10fdc88248e195bcd

.. code-block:: cpp

    /// Create independent, reusable byte buffers from common inputs.
    ///
    /// A `ByteBuffer` owns mutable storage and copies both its construction input
    /// and other buffers. It is a good fit for working memory that changes several
    /// times before its contents become a stable application value.
    void creatingBuffers() {
        // Start with empty, repeated, and explicitly listed forest measurements.
        const auto emptySurvey = el::ByteBuffer{};
        const auto clearings = el::ByteBuffer{el::ByteLength{4U}, el::Byte{0U}};
        const auto canopy = el::ByteBuffer{el::Byte{18U}, el::Byte{42U}, el::Byte{67U}};

        // Copy a borrowed span into a buffer with an independent lifetime.
        const auto soilSamples = el::ByteArray{el::Byte{7U}, el::Byte{11U}, el::Byte{15U}};
        auto copiedSamples = el::ByteBuffer{soilSamples.span()};

        // Copies are deep: changing one buffer never changes the other.
        auto independentCopy = copiedSamples;
        independentCopy.setOrThrow(el::ByteIndex{0U}, el::Byte{99U});

        el::io::printLine("Orman             : Karadeniz ormanı"_el);
        el::io::printLine("Empty survey      : "_el, el::BooleanFormat::yesNo(), emptySurvey.isEmpty());
        el::io::printLine(
            "Clearing samples  : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(clearings.span()));
        el::io::printLine("Canopy samples    : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(canopy.span()));
        el::io::printLine(
            "Original soil     : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(copiedSamples.span()));
        el::io::printLine(
            "Changed copy      : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(independentCopy.span()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Orman             : Karadeniz ormanı
    Empty survey      : yes
    Clearing samples  : 00 00 00 00
    Canopy samples    : 12 2a 43
    Original soil     : 07 0b 0f
    Changed copy      : 63 0b 0f

.. erbsland-demo-end::

Controlling Length and Capacity
===============================

:cpp:func:`length() <erbsland::mem::ByteBuffer::length>` is the number of visible bytes, while
:cpp:func:`endIndex() <erbsland::mem::ByteBuffer::endIndex>` is the position immediately after the last one.
:cpp:func:`isEmpty() <erbsland::mem::ByteBuffer::isEmpty>` is the clearest test when only the presence of data matters.

:cpp:func:`resize() <erbsland::mem::ByteBuffer::resize>` changes that visible sequence.
Growing a buffer adds zero-valued bytes, which provides a predictable starting state for fields that will be filled
later.
Shrinking removes bytes from the end.

Capacity describes the allocation behind the visible sequence.
:cpp:func:`reserve() <erbsland::mem::ByteBuffer::reserve>` prepares at least the requested capacity without changing the
length, making it useful when you can estimate a future result.
:cpp:func:`clear() <erbsland::mem::ByteBuffer::clear>` removes all visible bytes but retains the allocation for another
cycle.
When reuse has ended, :cpp:func:`shrinkToFit() <erbsland::mem::ByteBuffer::shrinkToFit>` reduces capacity to the current
length; :cpp:func:`reset() <erbsland::mem::ByteBuffer::reset>` empties the buffer and releases its storage directly.

Capacity is an implementation resource rather than part of the byte value.
Do not depend on an exact growth amount after ``reserve()`` or an append; only depend on capacity being sufficient for
the requested operation.

.. erbsland-demo::
    :source: mem/ByteBuffer/ManagingStorage.cpp
    :exec: mem/byte_buffer --demo ManagingStorage
    :source-sha256: 337d6014f3393c8ae4f8aee4580440aea3357a847aa7d55f850eea672a2d2a33

.. code-block:: cpp

    /// Reuse a byte buffer while controlling its visible size and allocation.
    ///
    /// `resize()` changes the byte sequence and zero-fills growth. `reserve()` only
    /// prepares storage, while `clear()` preserves that storage for the next cycle
    /// and `shrinkToFit()` releases capacity that is no longer useful.
    void managingStorage() {
        auto samples = el::ByteBuffer{el::Byte{21U}, el::Byte{34U}};

        // Prepare room for a larger survey without adding visible bytes.
        samples.reserve(el::ByteLength{16U});
        const auto reservedCapacity = samples.capacity();

        // Extend the visible data; all new sample positions start at zero.
        samples.resize(el::ByteLength{5U});
        const auto grownSamples = el::ByteBlock::fromSpan(samples.span());

        // Reuse the allocation for a new survey, then trim it to the final result.
        samples.clear().append(el::Byte{8U}).append(el::Byte{13U});
        const auto capacityAfterClear = samples.capacity();
        samples.shrinkToFit();

        el::io::printLine("Gözlem            : Meşe yenilenmesi"_el);
        el::io::printLine("Reserved capacity : "_el, reservedCapacity.toSizeT());
        el::io::printLine("Grown samples     : "_el, el::ByteFormat::separated(), grownSamples);
        el::io::printLine("Length / end index: "_el, samples.length().toSizeT(), " / "_el, samples.endIndex().toSizeT());
        el::io::printLine("Capacity after clear: "_el, capacityAfterClear.toSizeT());
        el::io::printLine("Final capacity    : "_el, samples.capacity().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Gözlem            : Meşe yenilenmesi
    Reserved capacity : 32
    Grown samples     : 15 22 00 00 00
    Length / end index: 2 / 2
    Capacity after clear: 32
    Final capacity    : 2

.. erbsland-demo-end::

Accessing Individual Bytes
==========================

Byte access comes in tolerant and strict forms.
:cpp:func:`get() <erbsland::mem::ByteBuffer::get>` returns a caller-selected fallback for an invalid index, and
:cpp:func:`set() <erbsland::mem::ByteBuffer::set>` ignores an invalid index.
These forms work well when a field is genuinely optional.

When the binary layout promises that a byte exists, use
:cpp:func:`getOrThrow() <erbsland::mem::ByteBuffer::getOrThrow>` and
:cpp:func:`setOrThrow() <erbsland::mem::ByteBuffer::setOrThrow>`.
An invalid position then raises :cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>` instead of quietly hiding
a broken invariant.
:cpp:func:`xorAt() <erbsland::mem::ByteBuffer::xorAt>` and
:cpp:func:`xorAtOrThrow() <erbsland::mem::ByteBuffer::xorAtOrThrow>` offer the same choice for toggling selected bits.

For complete traversal, :cpp:func:`forEach() <erbsland::mem::ByteBuffer::forEach>` invokes a callback for every visible
byte and can optionally provide its :cpp:type:`ByteIndex <erbsland::unit::ByteIndex>`.
Its :cpp:enum:`LoopResult <erbsland::util::LoopResult>` also supports callbacks that stop early.

.. erbsland-demo::
    :source: mem/ByteBuffer/AccessingBytes.cpp
    :exec: mem/byte_buffer --demo AccessingBytes
    :source-sha256: dcf632001903ee67b9a7d83d8fdb53c0acda8a668dc3bb9491001fa0d37514b6

.. code-block:: cpp

    /// Read, update, and visit individual bytes in a working buffer.
    ///
    /// Tolerant access ignores invalid writes or returns a chosen fallback. The
    /// `OrThrow` variants are appropriate when the buffer layout guarantees that a
    /// position exists, and `forEach()` handles complete traversal.
    void accessingBytes() {
        auto treeHeights = el::ByteBuffer{el::Byte{12U}, el::Byte{19U}, el::Byte{27U}, el::Byte{35U}};

        // Treat one known position strictly and one optional position tolerantly.
        treeHeights.setOrThrow(el::ByteIndex{1U}, el::Byte{20U});
        treeHeights.set(el::ByteIndex{99U}, el::Byte{50U});
        treeHeights.xorAtOrThrow(el::ByteIndex{3U}, el::Byte{1U});
        const auto missingHeight = treeHeights.get(el::ByteIndex{99U}, el::Byte{255U});

        // Visit all trees to find the tallest recorded height.
        auto tallest = el::Byte{};
        const auto visitResult = treeHeights.forEach([&tallest](const el::Byte height) -> void {
            if (height > tallest) {
                tallest = height;
            }
        });

        el::io::printLine("Alan              : Yaşlı meşeler"_el);
        el::io::printLine(
            "Recorded heights  : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(treeHeights.span()));
        el::io::printLine("Missing fallback  : "_el, missingHeight.toUInt32());
        el::io::printLine("Tallest height    : "_el, tallest.toUInt32());
        el::io::printLine("Visited all bytes : "_el, el::BooleanFormat::yesNo(), visitResult == el::LoopResult::Success);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Alan              : Yaşlı meşeler
    Recorded heights  : 0c 14 1b 22
    Missing fallback  : 255
    Tallest height    : 34
    Visited all bytes : yes

.. erbsland-demo-end::

Reading and Writing Integers
============================

Binary records often contain fixed-width native integers.
:cpp:func:`setInteger() <erbsland::mem::ByteBuffer::setInteger>` stores one at a byte offset and reports whether its
complete range fitted.
It never grows the buffer and leaves the bytes unchanged on failure.
:cpp:func:`setIntegerOrThrow() <erbsland::mem::ByteBuffer::setIntegerOrThrow>` is the strict counterpart for a required
field.

:cpp:func:`getInteger() <erbsland::mem::ByteBuffer::getInteger>` returns a selected fallback when the range is invalid,
while :cpp:func:`getIntegerOrThrow() <erbsland::mem::ByteBuffer::getIntegerOrThrow>` enforces the expected layout.
:cpp:func:`getIntegerInto() <erbsland::mem::ByteBuffer::getIntegerInto>` is convenient when a ``bool`` result and an
unchanged destination on failure fit the surrounding control flow.

All these operations accept :cpp:enum:`Endianness <erbsland::mem::Endianness>`.
Choose the byte order required by the data format rather than the machine running the code.
For integer encodings whose width is not simply ``sizeof(T)``, see :doc:`byte_integer_formats` and use a
:cpp:class:`ByteReader <erbsland::mem::ByteReader>` or :cpp:class:`ByteWriter <erbsland::mem::ByteWriter>`.

.. erbsland-demo::
    :source: mem/ByteBuffer/ReadingAndWritingIntegers.cpp
    :exec: mem/byte_buffer --demo ReadingAndWritingIntegers
    :source-sha256: 850b918ecf67c467b8b2895b41664170613561a40109de0be8efd2d0e92a5cdc

.. code-block:: cpp

    /// Read and write fixed-width integers inside a byte buffer.
    ///
    /// Integer access uses an explicit byte offset and byte order. Tolerant methods
    /// report an invalid range without changing the buffer, while `OrThrow` methods
    /// make a required binary layout easy to enforce.
    void readingAndWritingIntegers() {
        auto observation = el::ByteBuffer{el::ByteLength{8U}};

        // Store a big-endian plot identifier and a little-endian tree count.
        observation.setIntegerOrThrow<uint32_t>(el::ByteIndex{0U}, 0x464f5245U, el::Endianness::Big);
        const auto countStored = observation.setInteger<uint16_t>(el::ByteIndex{4U}, 317U, el::Endianness::Little);
        const auto outsideStored = observation.setInteger<uint32_t>(el::ByteIndex{7U}, 1U);

        // Decode fields using the same byte order as their wire representation.
        const auto plotId = observation.getIntegerOrThrow<uint32_t>(el::ByteIndex{0U}, el::Endianness::Big);
        auto treeCount = uint16_t{};
        const auto countRead = observation.getIntegerInto(treeCount, el::ByteIndex{4U}, el::Endianness::Little);

        el::io::printLine("Kayıt             : Orman sayımı"_el);
        el::io::printLine(
            "Encoded record    : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(observation.span()));
        el::io::printLine("Plot identifier   : "_el, plotId);
        el::io::printLine("Tree count        : "_el, treeCount);
        el::io::printLine("Count stored/read : "_el, el::BooleanFormat::yesNo(), countStored && countRead);
        el::io::printLine("Outside write     : "_el, el::BooleanFormat::yesNo(), outsideStored);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Kayıt             : Orman sayımı
    Encoded record    : 46 4f 52 45 3d 01 00 00
    Plot identifier   : 1179603525
    Tree count        : 317
    Count stored/read : yes
    Outside write     : no

.. erbsland-demo-end::

Revising Byte Ranges
====================

Some changes preserve the current layout.
:cpp:func:`fill() <erbsland::mem::ByteBuffer::fill>` assigns one value to all bytes or a clamped range, and
:cpp:func:`overwrite() <erbsland::mem::ByteBuffer::overwrite>` copies as much of a source span as fits into an existing
range.
Neither operation extends the buffer.

:cpp:func:`xorWith() <erbsland::mem::ByteBuffer::xorWith>` combines bytes with a mask.
The whole-buffer form requires equal lengths and reports failure without modification when they differ;
:cpp:func:`xorWithOrThrow() <erbsland::mem::ByteBuffer::xorWithOrThrow>` enforces that requirement.
The ranged form is intentionally partial and clamps the target to the visible data.

Structural operations change the sequence itself.
:cpp:func:`append() <erbsland::mem::ByteBuffer::append>` adds bytes at the end, and
:cpp:func:`insert() <erbsland::mem::ByteBuffer::insert>` places them before an index, clamping an index beyond the end to
the end.
:cpp:func:`replace() <erbsland::mem::ByteBuffer::replace>` substitutes a clamped range and may change the length.
:cpp:func:`remove() <erbsland::mem::ByteBuffer::remove>` deletes a range, while
:cpp:func:`keep() <erbsland::mem::ByteBuffer::keep>` discards everything outside one selected range.
These clamped semantics are useful for deliberate partial edits; validate lengths separately when malformed input must
instead be rejected.

.. erbsland-demo::
    :source: mem/ByteBuffer/ManipulatingRanges.cpp
    :exec: mem/byte_buffer --demo ManipulatingRanges
    :source-sha256: 9e5cbaf5acf71d5e155ab821f96e0d256f13eb7af8213ff4fe508d53898630f4

.. code-block:: cpp

    /// Assemble and revise ranges in a reusable byte buffer.
    ///
    /// Non-structural operations modify bytes already present. `append()`,
    /// `insert()`, `replace()`, `remove()`, and `keep()` change the visible layout,
    /// making the buffer suitable for repeatedly building binary records.
    void manipulatingRanges() {
        auto habitat = el::ByteBuffer{el::ByteLength{6U}, el::Byte{10U}};

        // Modify existing survey fields without changing the length.
        habitat.fill(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{2U}}, el::Byte{20U});
        habitat.overwrite(el::ByteIndex{3U}, el::ByteArray{el::Byte{30U}, el::Byte{40U}}.span());
        habitat.xorWith(
            el::ByteRange{el::ByteIndex{0U}, el::ByteLength{3U}},
            el::ByteArray{el::Byte{1U}, el::Byte{2U}, el::Byte{4U}}.span());

        // Change the record structure, then retain only the useful payload.
        habitat.insert(el::ByteIndex{2U}, el::ByteArray{el::Byte{15U}}.span());
        habitat.replace(
            el::ByteRange{el::ByteIndex{4U}, el::ByteLength{2U}},
            el::ByteArray{el::Byte{33U}, el::Byte{44U}, el::Byte{55U}}.span());
        habitat.remove(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{1U}});
        habitat.append(el::Byte{99U});
        habitat.keep(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{6U}});

        el::io::printLine("Habitat           : Kayın korusu"_el);
        el::io::printLine("Final record      : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(habitat.span()));
        el::io::printLine("Record length     : "_el, habitat.length().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Habitat           : Kayın korusu
    Final record      : 0f 10 21 2c 37 0a
    Record length     : 6

.. erbsland-demo-end::

Crossing Standard-Library Boundaries
====================================

The :cpp:func:`fromSpan() <erbsland::mem::ByteBuffer::fromSpan>` overloads copy spans of ``std::byte``, ``uint8_t``, or
``char`` into owned Erbsland storage.
They make the copy visible at an interface where a native API or file format supplies standard contiguous bytes.

In the other direction, :cpp:func:`toUInt8Vector() <erbsland::mem::ByteBuffer::toUInt8Vector>` and
:cpp:func:`toCharVector() <erbsland::mem::ByteBuffer::toCharVector>` make independent vectors.
Within Erbsland APIs, prefer :cpp:func:`span() <erbsland::mem::ByteBuffer::span>` for temporary read-only access because
it avoids a copy.
Remember that a span borrows the buffer: any operation that reallocates the buffer can invalidate the view.

.. erbsland-demo::
    :source: mem/ByteBuffer/ConvertingBuffers.cpp
    :exec: mem/byte_buffer --demo ConvertingBuffers
    :source-sha256: a7b358a71576564515ce46ce2a6f0feaed622858ad6ea2bcb316ad3c9737ff62

.. code-block:: cpp

    /// Copy between a byte buffer and standard contiguous storage.
    ///
    /// The `fromSpan()` factories are explicit ownership boundaries for standard
    /// byte-like types. The matching vector conversions produce independent data
    /// suitable for APIs that do not use Erbsland byte containers.
    void convertingBuffers() {
        const auto sensorBytes = std::array<uint8_t, 4>{5U, 8U, 13U, 21U};
        const auto labelBytes = std::array{'O', 'R', 'M', 'A', 'N'};
        const auto flags = std::array{std::byte{0x01U}, std::byte{0x04U}};

        // Copy standard spans into owned buffers.
        const auto sensors = el::ByteBuffer::fromSpan(std::span{sensorBytes});
        const auto label = el::ByteBuffer::fromSpan(std::span{labelBytes});
        const auto featureFlags = el::ByteBuffer::fromSpan(std::span{flags});

        // Copy buffer contents back to the representation expected by another API.
        const auto sensorVector = sensors.toUInt8Vector();
        const auto labelVector = label.toCharVector();

        el::io::printLine("İstasyon          : Orman sınırı"_el);
        el::io::printLine("Sensor bytes      : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(sensors.span()));
        el::io::printLine(
            "Feature flags     : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(featureFlags.span()));
        el::io::printLine("Vector sizes      : "_el, sensorVector.size(), " / "_el, labelVector.size());
    }

.. erbsland-ansi::
    :escape-char: ␛

    İstasyon          : Orman sınırı
    Sensor bytes      : 05 08 0d 15
    Feature flags     : 01 04
    Vector sizes      : 4 / 5

.. erbsland-demo-end::

Once editing is complete, copying the visible span into a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` gives the
result the library's preferred read-only owning representation.
Keep the buffer itself when another build cycle will benefit from its retained allocation.
For data arriving continuously rather than being rebuilt as a value, continue with :doc:`using_byte_ring_buffers`.
