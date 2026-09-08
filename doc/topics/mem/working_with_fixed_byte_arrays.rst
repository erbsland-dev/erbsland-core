.. index::
    single: Memory; Fixed byte arrays
    single: Byte array; Low-level operations
    single: Binary data; Fixed-size storage

******************************
Working with Fixed Byte Arrays
******************************

Some binary values have a size that is part of their meaning: a cryptographic state, a protocol identifier, or a small
group of samples processed by a low-level algorithm.
:cpp:class:`ByteArray <erbsland::mem::ByteArray>` gives these values fixed-size mutable storage and provides safe
operations for individual bytes, ranges, and complete bit strings.
This page explains when that low-level representation is useful, how its tolerant and strict operations differ, and how
to cross safely into borrowed spans or growing buffers.

When a Fixed Array Fits the Problem
===================================

The extent ``N`` in the :cpp:class:`ByteArray <erbsland::mem::ByteArray>` template is known at compile time and is part
of the type.
That makes the array a natural match for byte blocks whose size is an invariant rather than a value discovered while the
program is running.
It is especially useful for fixed cryptographic inputs and state, compact fields in binary algorithms, and temporary
buffers that stay inside one function.

The fixed extent also draws a useful boundary around low-level work.
An array never grows, and operations that receive a range cannot extend it beyond those ``N`` bytes.
For application data with an independent or variable lifetime, prefer an owning block or specialized container.
Those higher-level types make sharing, editing, and growth explicit instead of forcing a fixed array into a role it was
not designed to fill.

Inspecting Individual Bytes
===========================

:cpp:func:`length() <erbsland::mem::ByteArray::length>`,
:cpp:func:`endIndex() <erbsland::mem::ByteArray::endIndex>`, and
:cpp:func:`isEmpty() <erbsland::mem::ByteArray::isEmpty>` describe the extent using the library's byte units.
They are static properties of the array type, although calling them through a value often reads naturally.
Even its zero-length specialization, ``ByteArray<0>``, is valid and reports itself as empty.

Choose the access style according to what an invalid index means in your algorithm.
:cpp:func:`get() <erbsland::mem::ByteArray::get>` returns a chosen fallback and never throws, which is convenient when
absence is an ordinary condition.
:cpp:func:`getOrThrow() <erbsland::mem::ByteArray::getOrThrow>` raises
:cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>` when the index is invalid, making it the clearer choice
when the layout guarantees that the byte must exist.

:cpp:func:`forEach() <erbsland::mem::ByteArray::forEach>` visits the complete array without exposing raw storage.
Its callback may receive just the byte or both the byte and its :cpp:type:`ByteIndex <erbsland::unit::ByteIndex>`.
The following example uses the indexed form to find the crest of a compact sampled wave.

.. erbsland-demo::
    :source: mem/ByteArray/AccessingBytes.cpp
    :exec: mem/byte_array --demo AccessingBytes
    :source-sha256: 7f83ce52cabdaf3cb8f88fb762d9d8fe822dce91319a7a94df41d9eb08f8838e

.. code-block:: cpp

    /// Inspect individual bytes in a fixed-size `ByteArray`.
    ///
    /// The array reports its compile-time extent through byte-aware length and index
    /// values. Tolerant access returns a fallback for an invalid index, strict access
    /// throws, and `forEach()` can provide both each byte and its index.
    void accessingBytes() {
        const auto waveSamples = el::ByteArray{el::Byte{24U}, el::Byte{63U}, el::Byte{112U}, el::Byte{63U}, el::Byte{24U}};

        // Inspect the fixed extent and read known sample positions.
        const auto firstSample = waveSamples.get(el::ByteIndex{0U});
        const auto crestSample = waveSamples.getOrThrow(el::ByteIndex{2U});

        // Choose an explicit fallback when an index may come from outside the array.
        const auto unavailableSample = waveSamples.get(el::ByteIndex{99U}, el::Byte{255U});

        // Visit every sample together with its index to locate the wave crest.
        auto peak = el::Byte{};
        auto peakIndex = el::ByteIndex::zero();
        const auto iterationResult = waveSamples.forEach([&](const el::Byte sample, const el::ByteIndex index) -> void {
            if (sample > peak) {
                peak = sample;
                peakIndex = index;
            }
        });

        el::io::printLine("Wave               : Havsvåg"_el);
        el::io::printLine("Sample count       : "_el, waveSamples.length().toSizeT());
        el::io::printLine("End index          : "_el, waveSamples.endIndex().toSizeT());
        el::io::printLine("Array is empty     : "_el, el::BooleanFormat::yesNo(), waveSamples.isEmpty());
        el::io::printLine("First sample       : "_el, firstSample.toUInt32());
        el::io::printLine("Crest sample       : "_el, crestSample.toUInt32());
        el::io::printLine("Missing fallback   : "_el, unavailableSample.toUInt32());
        el::io::printLine("Peak index         : "_el, peakIndex.toSizeT());
        el::io::printLine(
            "Visited all samples : "_el, el::BooleanFormat::yesNo(), iterationResult == el::LoopResult::Success);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Wave               : Havsvåg
    Sample count       : 5
    End index          : 5
    Array is empty     : no
    First sample       : 24
    Crest sample       : 112
    Missing fallback   : 255
    Peak index         : 2
    Visited all samples : yes

.. erbsland-demo-end::

Changing Selected Bytes
=======================

The same distinction between expected absence and a broken invariant applies to mutation.
:cpp:func:`set() <erbsland::mem::ByteArray::set>` and :cpp:func:`xorAt() <erbsland::mem::ByteArray::xorAt>` ignore an
invalid index and leave the array unchanged.
This is useful when a calculated position may legitimately fall outside the current fixed window.

:cpp:func:`setOrThrow() <erbsland::mem::ByteArray::setOrThrow>` and
:cpp:func:`xorAtOrThrow() <erbsland::mem::ByteArray::xorAtOrThrow>` instead raise
:cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>`.
The XOR forms are particularly handy for toggling flags or applying a small mask because the read and write stay one
clear operation.

.. erbsland-demo::
    :source: mem/ByteArray/ChangingBytes.cpp
    :exec: mem/byte_array --demo ChangingBytes
    :source-sha256: 53e6049675071e0525780c3cc072d3e39705e549ebc71422bd96f4722644f39c

.. code-block:: cpp

    /// Change selected bytes in a fixed-size `ByteArray`.
    ///
    /// `set()` and `xorAt()` quietly ignore invalid indexes when absence is an
    /// expected possibility. Their `OrThrow` counterparts make a valid index an
    /// invariant and report a violation with an exception.
    void changingBytes() {
        auto waveSamples = el::ByteArray{el::Byte{32U}, el::Byte{64U}, el::Byte{96U}, el::Byte{64U}, el::Byte{32U}};

        // Replace two samples whose positions are guaranteed by the waveform layout.
        waveSamples.set(el::ByteIndex{1U}, el::Byte{72U});
        waveSamples.setOrThrow(el::ByteIndex{3U}, el::Byte{72U});

        // Toggle flag bits directly at selected positions without a separate read.
        waveSamples.xorAt(el::ByteIndex{0U}, el::Byte{0x08U});
        waveSamples.xorAtOrThrow(el::ByteIndex{4U}, el::Byte{0x08U});

        // Tolerant writes leave the array unchanged when an index is unavailable.
        const auto beforeIgnoredWrites = waveSamples;
        waveSamples.set(el::ByteIndex{99U}, el::Byte{255U});
        waveSamples.xorAt(el::ByteIndex::noIndex(), el::Byte{255U});

        el::io::printLine("Wave               : Havsvåg"_el);
        el::io::printLine("Adjusted samples   : "_el, el::ByteFormat::separated(), el::ByteBlock{waveSamples});
        el::io::printLine("Invalid writes ignored: "_el, el::BooleanFormat::yesNo(), waveSamples == beforeIgnoredWrites);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Wave               : Havsvåg
    Adjusted samples   : 28 48 60 48 28
    Invalid writes ignored: yes

.. erbsland-demo-end::

Changing Byte Ranges
====================

Bulk operations let an algorithm describe a complete transformation rather than repeat index handling.
:cpp:func:`fill() <erbsland::mem::ByteArray::fill>` can replace the entire array or a selected
:cpp:type:`ByteRange <erbsland::unit::ByteRange>`.
:cpp:func:`overwrite() <erbsland::mem::ByteArray::overwrite>` can begin at index zero, at a chosen index, or within a
chosen target range.
The ranged forms clamp their target to existing bytes and copy only as much of the source as fits, so a request that
reaches beyond the end cannot enlarge or overrun the array.

:cpp:func:`xorWith() <erbsland::mem::ByteArray::xorWith>` has two deliberately different roles.
The whole-array form accepts only an equally sized source and returns ``false`` without changing anything when the
length differs.
:cpp:func:`xorWithOrThrow() <erbsland::mem::ByteArray::xorWithOrThrow>` expresses the same equal-length requirement as
an invariant and raises :cpp:class:`ParameterError <erbsland::err::ParameterError>` on a mismatch.
The range form is intentionally more permissive: it combines as many bytes as fit in the clamped target.

.. erbsland-demo::
    :source: mem/ByteArray/ChangingRanges.cpp
    :exec: mem/byte_array --demo ChangingRanges
    :source-sha256: b1d88d65d01b90baa788a71d02aa6b6c34615d7e0cb3434a14e01ea87e0cbd6f

.. code-block:: cpp

    /// Fill, overwrite, and combine ranges in a fixed-size `ByteArray`.
    ///
    /// Range operations stay within the array and use as much of their source as
    /// fits. Whole-array XOR validates equal lengths before changing anything, while
    /// ranged XOR is a clamped operation for deliberately partial data.
    void changingRanges() {
        auto waveSamples = el::ByteArray<8>{};

        // Establish a quiet baseline, then raise a three-sample pulse in the middle.
        waveSamples.fill(el::Byte{16U});
        waveSamples.fill(el::ByteRange{el::ByteIndex{2U}, el::ByteLength{3U}}, el::Byte{64U});

        // Copy complete or partial source waves into selected positions.
        waveSamples.overwrite(el::ByteArray{el::Byte{20U}, el::Byte{30U}}.span());
        waveSamples.overwrite(
            el::ByteIndex{5U}, el::ByteArray{el::Byte{80U}, el::Byte{96U}, el::Byte{112U}, el::Byte{128U}}.span());
        waveSamples.overwrite(
            el::ByteRange{el::ByteIndex{3U}, el::ByteLength{2U}},
            el::ByteArray{el::Byte{70U}, el::Byte{78U}, el::Byte{86U}}.span());

        // Apply a partial mask, then require an exact-size mask for the complete array.
        waveSamples.xorWith(
            el::ByteRange{el::ByteIndex{2U}, el::ByteLength{3U}},
            el::ByteArray{el::Byte{1U}, el::Byte{2U}, el::Byte{4U}}.span());
        const auto shortMaskAccepted = waveSamples.xorWith(el::ByteArray{el::Byte{255U}}.span());
        waveSamples.xorWithOrThrow(
            el::ByteArray{
                el::Byte{0U},
                el::Byte{0U},
                el::Byte{0U},
                el::Byte{0U},
                el::Byte{0U},
                el::Byte{0U},
                el::Byte{0U},
                el::Byte{0x0fU}}
                .span());

        el::io::printLine("Wave               : Havsvåg"_el);
        el::io::printLine("Combined samples   : "_el, el::ByteFormat::separated(), el::ByteBlock{waveSamples});
        el::io::printLine("Short mask accepted: "_el, el::BooleanFormat::yesNo(), shortMaskAccepted);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Wave               : Havsvåg
    Combined samples   : 14 1e 41 44 4a 50 60 7f
    Short mask accepted: no

.. erbsland-demo-end::

Moving a Complete Bit String or Separate Bytes
==============================================

A :cpp:class:`ByteArray <erbsland::mem::ByteArray>` can represent one packed bit string or several independent byte
values, and the distinction matters at byte boundaries.
:cpp:func:`shiftedLeft() <erbsland::mem::ByteArray::shiftedLeft>` and
:cpp:func:`shiftedRight() <erbsland::mem::ByteArray::shiftedRight>` treat index zero as the most-significant byte and
move bits across the complete array.
Vacated positions become zero, bits shifted beyond the array are discarded, and a shift by the total bit count or more
produces an all-zero array.
Their :cpp:func:`shiftLeft() <erbsland::mem::ByteArray::shiftLeft>` and
:cpp:func:`shiftRight() <erbsland::mem::ByteArray::shiftRight>` counterparts change the original value.

Rotation preserves every bit by wrapping it around the opposite end.
:cpp:func:`rotatedLeft() <erbsland::mem::ByteArray::rotatedLeft>` and
:cpp:func:`rotatedRight() <erbsland::mem::ByteArray::rotatedRight>` return a new array, while
:cpp:func:`rotateLeft() <erbsland::mem::ByteArray::rotateLeft>` and
:cpp:func:`rotateRight() <erbsland::mem::ByteArray::rotateRight>` work in place.
Amounts wrap to the total bit count, and a negative amount reverses the direction.

When each byte is an independent sample or flag group, use the ``eachByteShifted...()`` and ``eachByteRotated...()``
families instead.
Their returning forms are :cpp:func:`eachByteShiftedLeft() <erbsland::mem::ByteArray::eachByteShiftedLeft>`,
:cpp:func:`eachByteShiftedRight() <erbsland::mem::ByteArray::eachByteShiftedRight>`,
:cpp:func:`eachByteRotatedLeft() <erbsland::mem::ByteArray::eachByteRotatedLeft>`, and
:cpp:func:`eachByteRotatedRight() <erbsland::mem::ByteArray::eachByteRotatedRight>`.
:cpp:func:`shiftEachByteLeft() <erbsland::mem::ByteArray::shiftEachByteLeft>`,
:cpp:func:`shiftEachByteRight() <erbsland::mem::ByteArray::shiftEachByteRight>`,
:cpp:func:`rotateEachByteLeft() <erbsland::mem::ByteArray::rotateEachByteLeft>`, and
:cpp:func:`rotateEachByteRight() <erbsland::mem::ByteArray::rotateEachByteRight>` make the same transformations in
place.
No bit crosses from one byte into its neighbor.

.. erbsland-demo::
    :source: mem/ByteArray/MovingBits.cpp
    :exec: mem/byte_array --demo MovingBits
    :source-sha256: c356588dd63b150ea68bc9a3f485d4b856c17b769b344af88786d5f9f4618bab

.. code-block:: cpp

    /// Shift or rotate a complete `ByteArray` and each byte independently.
    ///
    /// Whole-array operations treat index zero as the most-significant byte and let
    /// bits cross byte boundaries. The `eachByte...()` family keeps every byte as a
    /// separate value. Both families offer result-returning and in-place forms.
    void movingBits() {
        const auto shiftWave = el::ByteArray{el::Byte{0x81U}, el::Byte{0x80U}};
        const auto rotationWave = el::ByteArray{el::Byte{0x81U}, el::Byte{0x40U}};

        // Shift one packed 16-bit pattern and two independent eight-bit samples.
        const auto wholeShiftedLeft = shiftWave.shiftedLeft(1U);
        const auto wholeShiftedRight = shiftWave.shiftedRight(1U);
        const auto bytesShiftedLeft = shiftWave.eachByteShiftedLeft(1U);
        const auto bytesShiftedRight = shiftWave.eachByteShiftedRight(1U);

        // Rotate with and without crossing the boundary between the two bytes.
        const auto wholeRotatedLeft = rotationWave.rotatedLeft(1);
        const auto wholeRotatedRight = rotationWave.rotatedRight(1);
        const auto bytesRotatedLeft = rotationWave.eachByteRotatedLeft(1);
        const auto bytesRotatedRight = rotationWave.eachByteRotatedRight(1);

        // The mutating forms produce the same values in their original arrays.
        auto changedWholeLeft = shiftWave;
        auto changedWholeRight = shiftWave;
        auto changedBytesLeft = shiftWave;
        auto changedBytesRight = shiftWave;
        auto changedWholeRotateLeft = rotationWave;
        auto changedWholeRotateRight = rotationWave;
        auto changedBytesRotateLeft = rotationWave;
        auto changedBytesRotateRight = rotationWave;
        changedWholeLeft.shiftLeft(1U);
        changedWholeRight.shiftRight(1U);
        changedBytesLeft.shiftEachByteLeft(1U);
        changedBytesRight.shiftEachByteRight(1U);
        changedWholeRotateLeft.rotateLeft(1);
        changedWholeRotateRight.rotateRight(1);
        changedBytesRotateLeft.rotateEachByteLeft(1);
        changedBytesRotateRight.rotateEachByteRight(1);

        const auto inPlaceFormsAgree = changedWholeLeft == wholeShiftedLeft && changedWholeRight == wholeShiftedRight &&
            changedBytesLeft == bytesShiftedLeft && changedBytesRight == bytesShiftedRight &&
            changedWholeRotateLeft == wholeRotatedLeft && changedWholeRotateRight == wholeRotatedRight &&
            changedBytesRotateLeft == bytesRotatedLeft && changedBytesRotateRight == bytesRotatedRight;

        el::io::printLine("Wave               : Havsvåg"_el);
        el::io::printLine("Whole shift left   : "_el, el::ByteFormat::separated(), el::ByteBlock{wholeShiftedLeft});
        el::io::printLine("Each byte left     : "_el, el::ByteFormat::separated(), el::ByteBlock{bytesShiftedLeft});
        el::io::printLine("Whole shift right  : "_el, el::ByteFormat::separated(), el::ByteBlock{wholeShiftedRight});
        el::io::printLine("Each byte right    : "_el, el::ByteFormat::separated(), el::ByteBlock{bytesShiftedRight});
        el::io::printLine("Whole rotate left  : "_el, el::ByteFormat::separated(), el::ByteBlock{wholeRotatedLeft});
        el::io::printLine("Each byte left rot.: "_el, el::ByteFormat::separated(), el::ByteBlock{bytesRotatedLeft});
        el::io::printLine("Whole rotate right : "_el, el::ByteFormat::separated(), el::ByteBlock{wholeRotatedRight});
        el::io::printLine("Each byte right rot: "_el, el::ByteFormat::separated(), el::ByteBlock{bytesRotatedRight});
        el::io::printLine("In-place forms agree: "_el, el::BooleanFormat::yesNo(), inPlaceFormsAgree);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Wave               : Havsvåg
    Whole shift left   : 03 00
    Each byte left     : 02 00
    Whole shift right  : 40 c0
    Each byte right    : 40 40
    Whole rotate left  : 02 81
    Each byte left rot.: 03 80
    Whole rotate right : 40 a0
    Each byte right rot: c0 20
    In-place forms agree: yes

.. erbsland-demo-end::

Borrowing an Existing Range
===========================

:cpp:func:`span() <erbsland::mem::ByteArray::span>` exposes the complete array as a read-only
:cpp:type:`FixedConstByteSpan <erbsland::mem::FixedConstByteSpan>` whose extent remains part of the type.
The range overloads return a dynamic :cpp:type:`ConstByteSpan <erbsland::mem::ConstByteSpan>` containing only existing
bytes.
An oversized length is clamped at the end, while an invalid starting index produces an empty span.
This makes range calculation safe without turning the borrowed view into owning storage.

That last distinction is essential: a span does not extend the array's lifetime.
It sees later in-place changes because it still refers to the same bytes, but it becomes invalid when its array is
destroyed or when an owner relocates the array's containing object.
Keep these views local and short-lived, and use an owning container when the bytes must escape the operation.

.. erbsland-demo::
    :source: mem/ByteArray/BorrowingRanges.cpp
    :exec: mem/byte_array --demo BorrowingRanges
    :source-sha256: ade805c083e856d11e5b3eca5873a38aee3646bc9a76e79361768229cc200c65

.. code-block:: cpp

    /// Borrow complete and clamped read-only spans from a `ByteArray`.
    ///
    /// The complete span retains the array extent in its type. Ranged spans contain
    /// only existing bytes, so an oversized length is clamped and an invalid start
    /// produces an empty view. Every returned span remains tied to the array lifetime.
    void borrowingRanges() {
        auto waveSamples =
            el::ByteArray{el::Byte{18U}, el::Byte{42U}, el::Byte{81U}, el::Byte{126U}, el::Byte{81U}, el::Byte{42U}};

        // Borrow the whole fixed extent and two ranges selected at runtime.
        const auto completeWave = waveSamples.span();
        const auto crest = waveSamples.span(el::ByteRange{el::ByteIndex{2U}, el::ByteLength{3U}});
        const auto clampedTail = waveSamples.span(el::ByteIndex{4U}, el::ByteLength{99U});
        const auto outside = waveSamples.span(el::ByteIndex{99U}, el::ByteLength{2U});
        static_assert(decltype(completeWave)::extent == 6U);

        // A span observes later in-place changes because it still views the array.
        waveSamples.setOrThrow(el::ByteIndex{2U}, el::Byte{88U});

        auto crestSum = uint32_t{};
        for (const auto sample : crest) {
            crestSum += sample.toUInt32();
        }

        el::io::printLine("Wave               : Havsvåg"_el);
        el::io::printLine("Complete length    : "_el, completeWave.size());
        el::io::printLine("Crest length       : "_el, crest.size());
        el::io::printLine("Clamped tail length: "_el, clampedTail.size());
        el::io::printLine("Outside is empty   : "_el, el::BooleanFormat::yesNo(), outside.empty());
        el::io::printLine("Updated crest sum  : "_el, crestSum);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Wave               : Havsvåg
    Complete length    : 6
    Crest length       : 3
    Clamped tail length: 2
    Outside is empty   : yes
    Updated crest sum  : 295

.. erbsland-demo-end::

Copying an Exact Span into an Array
===================================

The opposite boundary turns borrowed bytes into an independent fixed-size value.
:cpp:func:`fromSpan() <erbsland::mem::ByteArray::fromSpan>` copies only when the source length is exactly ``N`` and
otherwise returns an empty ``std::optional``.
This form suits parsed or external input whose length is part of normal validation.

:cpp:func:`fromSpanOrThrow() <erbsland::mem::ByteArray::fromSpanOrThrow>` performs the same exact-size copy but raises
:cpp:class:`ParameterError <erbsland::err::ParameterError>` on a mismatch.
Use it when an earlier layer or a fixed format has already guaranteed the extent.
Neither factory keeps a reference to the source, so later changes to the source do not affect the array.

.. erbsland-demo::
    :source: mem/ByteArray/CopyingFromSpan.cpp
    :exec: mem/byte_array --demo CopyingFromSpan
    :source-sha256: 55ffd361f7df28b2a067a2f569da93d08beb85a481a6c76d0f9f65a2a6803171

.. code-block:: cpp

    /// Create an owning fixed-size `ByteArray` from a borrowed span.
    ///
    /// `fromSpan()` reports a length mismatch with an empty optional, while
    /// `fromSpanOrThrow()` treats the mismatch as an error. Both factories accept
    /// only an exact extent and copy the bytes into an independent array.
    void copyingFromSpan() {
        auto receivedSamples = el::ByteBuffer{el::Byte{31U}, el::Byte{74U}, el::Byte{121U}, el::Byte{74U}};

        // Use the optional factory when an incoming sample count is expected to vary.
        const auto copiedWave = el::ByteArray<4>::fromSpan(receivedSamples.span());
        const auto wrongExtent = el::ByteArray<5>::fromSpan(receivedSamples.span());
        if (!copiedWave.has_value()) {
            el::io::printLine("The four-sample wave was unavailable."_el);
            return;
        }

        // Use the throwing factory when the format guarantees the exact sample count.
        const auto requiredWave = el::ByteArray<4>::fromSpanOrThrow(receivedSamples.span());

        // Changing the source after the copy does not affect either array.
        receivedSamples.setOrThrow(el::ByteIndex{0U}, el::Byte{255U});

        el::io::printLine("Wave               : Havsvåg"_el);
        el::io::printLine("Copied samples     : "_el, el::ByteFormat::separated(), el::ByteBlock{*copiedWave});
        el::io::printLine("Wrong extent copied: "_el, el::BooleanFormat::yesNo(), wrongExtent.has_value());
        el::io::printLine("Strict copy agrees : "_el, el::BooleanFormat::yesNo(), requiredWave == *copiedWave);
        el::io::printLine("Source first sample: "_el, receivedSamples.get(el::ByteIndex{0U}).toUInt32());
        el::io::printLine("Copy first sample  : "_el, copiedWave->get(el::ByteIndex{0U}).toUInt32());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Wave               : Havsvåg
    Copied samples     : 1f 4a 79 4a
    Wrong extent copied: no
    Strict copy agrees : yes
    Source first sample: 255
    Copy first sample  : 31

.. erbsland-demo-end::

Growing Beyond the Fixed Array
==============================

Occasionally an algorithm begins with a fixed block but later discovers or produces more bytes.
:cpp:func:`toByteBuffer() <erbsland::mem::ByteArray::toByteBuffer>` copies the current array into a
:cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` that can grow from that initial value.
The copy keeps the boundary explicit: appending to the buffer does not change the original array.

.. erbsland-demo::
    :source: mem/ByteArray/GrowingIntoBuffer.cpp
    :exec: mem/byte_array --demo GrowingIntoBuffer
    :source-sha256: 9fca0fd1488e4b9b74aa46a56fb605c853e28f37237cf6828a46da212c7b8851

.. code-block:: cpp

    /// Start a growing `ByteBuffer` with the contents of a `ByteArray`.
    ///
    /// `toByteBuffer()` makes an independent dynamic copy. The resulting buffer can
    /// grow as more bytes arrive, while the original fixed-size array remains a
    /// compact, unchanged value.
    void growingIntoBuffer() {
        const auto initialWave = el::ByteArray{el::Byte{22U}, el::Byte{58U}, el::Byte{104U}, el::Byte{58U}};

        // Begin with a fixed observation, then append a longer measured tail.
        auto recordedWave = initialWave.toByteBuffer();
        recordedWave.append(el::Byte{22U});
        recordedWave.append(el::ByteArray{el::Byte{10U}, el::Byte{4U}}.span());

        el::io::printLine("Wave               : Havsvåg"_el);
        el::io::printLine("Initial samples    : "_el, el::ByteFormat::separated(), el::ByteBlock{initialWave});
        el::io::printLine(
            "Growing buffer     : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(recordedWave.span()));
        el::io::printLine("Initial length     : "_el, initialWave.length().toSizeT());
        el::io::printLine("Buffer length      : "_el, recordedWave.length().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Wave               : Havsvåg
    Initial samples    : 16 3a 68 3a
    Growing buffer     : 16 3a 68 3a 16 0a 04
    Initial length     : 4
    Buffer length      : 7

.. erbsland-demo-end::

From Arrays to Application Data
===============================

Fixed byte arrays are best kept close to the low-level operation that needs their exact extent.
For an arbitrary immutable value that must be stored, shared, or passed through application code, use
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
Choose :cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` for copy-on-write editing, or
:cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` for a uniquely owned sequence that changes size frequently.
Specialized containers such as :cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` and
:cpp:class:`RingBuffer <erbsland::mem::RingBuffer>` add the behavior of a queue rather than merely storing a byte block.
