.. index::
    single: Memory; Ring buffers
    single: Ring buffer; FIFO byte streams
    single: Binary data; Bounded queues

***********************
Using Byte Ring Buffers
***********************

:cpp:class:`RingBuffer <erbsland::mem::RingBuffer>` holds a bounded FIFO stream of bytes between a producer and a
consumer.
Its storage can be fixed or allowed to grow to a hard limit, and reads consume data in exactly the order it was written.
:cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` adds endian-aware integer operations for structured binary
streams.
This page explains how to choose between them, reason about growth and capacity, and perform partial or atomic stream
operations safely.

Choosing a Queue for Byte Streams
=================================

A ring buffer is appropriate when data arrives and departs continuously: a protocol receiver, incremental decoder, or
producer-consumer boundary rarely needs to preserve already-consumed bytes.
The buffer reuses that space while presenting one logical FIFO sequence, even when the physical storage wraps around.

Use :cpp:class:`RingBuffer <erbsland::mem::RingBuffer>` when the stream consists of raw chunks and the caller decides
their structure.
Use :cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` when fixed-width integers are written and read directly
from the queue.
Both types have the same capacity and byte-transfer behavior.

A :cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` is a better fit for one mutable value that is repeatedly rebuilt
without consuming reads.
For stable data passed between application components, prefer
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.

Creating Fixed and Growable Rings
=================================

Constructing a ring with one capacity creates a fixed-size queue.
Its current :cpp:func:`capacity() <erbsland::mem::RingBuffer::capacity>` and
:cpp:func:`maximumCapacity() <erbsland::mem::RingBuffer::maximumCapacity>` are equal, so writes can never allocate beyond
that budget.

The two-capacity constructor creates a growable queue.
It starts with the initial allocation and may grow as required, but it never exceeds the maximum.
Both capacities must be nonzero and the maximum cannot be smaller than the initial value.

:cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` uses the same constructors.
Its :cpp:func:`endianness() <erbsland::mem::ByteRingBuffer::endianness>` initially reports little-endian byte order and
can be changed with :cpp:func:`setEndianness() <erbsland::mem::ByteRingBuffer::setEndianness>` before transferring
integer fields.

.. erbsland-demo::
    :source: mem/RingBuffer/CreatingBuffers.cpp
    :exec: mem/ring_buffer --demo CreatingBuffers
    :source-sha256: b1c81e93004ba8bf28e58a22773998629a38a6fe8c137d0290c9a95bf059ba9d

.. code-block:: cpp

    /// Create fixed and growable FIFO byte buffers.
    ///
    /// `RingBuffer` queues raw byte streams, while `ByteRingBuffer` adds atomic
    /// integer operations and a configurable byte order. A single capacity creates
    /// a fixed ring; two capacities allow growth up to a hard limit.
    void creatingBuffers() {
        // Use a fixed ring when the queue must stay within a strict memory budget.
        const auto fixedSamples = el::RingBuffer{el::ByteLength{8U}};

        // Give a growable ring a modest initial allocation and an explicit ceiling.
        const auto growingSamples = el::RingBuffer{el::ByteLength{4U}, el::ByteLength{32U}};

        // Choose ByteRingBuffer when the stream contains structured integer fields.
        auto structuredSamples = el::ByteRingBuffer{el::ByteLength{8U}, el::ByteLength{32U}};
        structuredSamples.setEndianness(el::Endianness::Big);

        el::io::printLine("Barriera          : Secca azzurra"_el);
        el::io::printLine("Fixed capacity    : "_el, fixedSamples.capacity().toSizeT());
        el::io::printLine("Fixed maximum     : "_el, fixedSamples.maximumCapacity().toSizeT());
        el::io::printLine("Growing capacity  : "_el, growingSamples.capacity().toSizeT());
        el::io::printLine("Growing maximum   : "_el, growingSamples.maximumCapacity().toSizeT());
        el::io::printLine(
            "Structured big-endian: "_el,
            el::BooleanFormat::yesNo(),
            structuredSamples.endianness() == el::Endianness::Big);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Barriera          : Secca azzurra
    Fixed capacity    : 8
    Fixed maximum     : 8
    Growing capacity  : 4
    Growing maximum   : 32
    Structured big-endian: yes

.. erbsland-demo-end::

FIFO Order, Wrap-Around, and Growth
===================================

Every read removes bytes from the front, and every write adds bytes at the back.
Once reads free storage near the beginning of the allocation, later writes can wrap around and reuse it.
That physical split is invisible to safe callers: a read still returns one correctly ordered sequence.

A growable ring allocates more storage when an accepted write needs it.
Queued bytes retain their order across this growth.
Capacity grows only as far as :cpp:func:`maximumCapacity() <erbsland::mem::RingBuffer::maximumCapacity>`; reaching the
current capacity is therefore not necessarily the same as reaching the hard limit.

A fixed ring cannot grow.
When it is full, a partial write transfers no bytes and an atomic write fails without modification.
Reading creates space for subsequent writes, which is the normal steady-state cycle of a ring buffer.

.. erbsland-demo::
    :source: mem/RingBuffer/UnderstandingGrowth.cpp
    :exec: mem/ring_buffer --demo UnderstandingGrowth
    :source-sha256: 55611f5aea4e5ea54437318d3926a8ae5e6896859a6210b804095f04e55dafe0

.. code-block:: cpp

    /// Follow FIFO order through wrap-around and automatic growth.
    ///
    /// Reads consume the oldest bytes. Later writes may wrap around the allocation;
    /// a growable ring can reorganize that queued data into a larger allocation
    /// without changing the order observed by readers.
    void understandingGrowth() {
        auto samples = el::RingBuffer{el::ByteLength{4U}, el::ByteLength{8U}};

        // Fill most of the initial storage and consume the two oldest samples.
        const auto firstWrite = samples.write(el::ByteArray{el::Byte{1U}, el::Byte{2U}, el::Byte{3U}}.span());
        const auto firstRead = samples.read(el::ByteLength{2U});

        // This write wraps and then grows the ring, while preserving FIFO order.
        const auto secondWrite =
            samples.write(el::ByteArray{el::Byte{4U}, el::Byte{5U}, el::Byte{6U}, el::Byte{7U}}.span());
        const auto capacityAfterGrowth = samples.capacity();
        const auto remaining = samples.read(el::ByteLength::infinite());

        el::io::printLine("Comunità          : Coralli e castagnole"_el);
        el::io::printLine("First write/read  : "_el, firstWrite.toSizeT(), " / "_el, firstRead.length().toSizeT());
        el::io::printLine("Second write      : "_el, secondWrite.toSizeT());
        el::io::printLine("Grown capacity    : "_el, capacityAfterGrowth.toSizeT());
        el::io::printLine("FIFO result       : "_el, el::ByteFormat::separated(), remaining);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Comunità          : Coralli e castagnole
    First write/read  : 3 / 2
    Second write      : 4
    Grown capacity    : 8
    FIFO result       : 03 04 05 06 07

.. erbsland-demo-end::

Understanding Size and Capacity
===============================

:cpp:func:`length() <erbsland::mem::RingBuffer::length>` reports readable bytes, while
:cpp:func:`available() <erbsland::mem::RingBuffer::available>` reports unused space in the current allocation.
:cpp:func:`isEmpty() <erbsland::mem::RingBuffer::isEmpty>` describes the read side and
:cpp:func:`isFull() <erbsland::mem::RingBuffer::isFull>` describes the current allocation.
A full growable ring may still accept a write by growing.

:cpp:func:`canWrite() <erbsland::mem::RingBuffer::canWrite>` answers the more useful question for an atomic operation:
whether that many additional bytes fit below the hard maximum.
:cpp:func:`reserveAdditional() <erbsland::mem::RingBuffer::reserveAdditional>` then prepares current capacity for the
additional bytes without making them readable.
It returns :cpp:class:`Result <erbsland::util::Result>` so exceeding the limit can remain an expected control-flow
event.

:cpp:func:`clear() <erbsland::mem::RingBuffer::clear>` discards queued bytes and keeps the current allocation ready for
reuse.
After the ring is empty, :cpp:func:`shrinkToInitial() <erbsland::mem::RingBuffer::shrinkToInitial>` releases growth and
returns to the initial capacity.
Calling it while data remains queued leaves the ring unchanged, avoiding an implicit data move or discard.

.. erbsland-demo::
    :source: mem/RingBuffer/ManagingCapacity.cpp
    :exec: mem/ring_buffer --demo ManagingCapacity
    :source-sha256: cd8087bd85a9e1341271a8a0423f523ecc4614f00e527e6696443a7559597118

.. code-block:: cpp

    /// Inspect, reserve, clear, and shrink a bounded ring buffer.
    ///
    /// State queries distinguish readable data, currently available storage, and
    /// the hard limit for atomic writes. Reserving prepares capacity without adding
    /// data; clearing makes data unreadable but keeps the allocation for reuse.
    void managingCapacity() {
        auto observations = el::RingBuffer{el::ByteLength{4U}, el::ByteLength{12U}};

        // Reserve enough current capacity for an upcoming atomic observation.
        const auto reserved = observations.reserveAdditional(el::ByteLength{7U});
        const auto reservedCapacity = observations.capacity();
        const auto canWriteTwelve = observations.canWrite(el::ByteLength{12U});
        const auto canWriteThirteen = observations.canWrite(el::ByteLength{13U});

        // Queue data, inspect state, then reuse and finally release excess capacity.
        const auto queued = observations.write(el::ByteArray{el::Byte{2U}, el::Byte{3U}, el::Byte{5U}}.span());
        const auto readable = observations.length();
        const auto available = observations.available();
        observations.clear();
        const auto emptyAfterClear = observations.isEmpty();
        observations.shrinkToInitial();

        el::io::printLine("Monitoraggio      : Laguna corallina"_el);
        el::io::printLine("Reservation       : "_el, el::BooleanFormat::yesNo(), reserved.isSuccessful());
        el::io::printLine("Reserved capacity : "_el, reservedCapacity.toSizeT());
        el::io::printLine("Queued bytes      : "_el, queued.toSizeT());
        el::io::printLine("Readable/available: "_el, readable.toSizeT(), " / "_el, available.toSizeT());
        el::io::printLine(
            "Can write 12 / 13 : "_el,
            el::BooleanFormat::yesNo(),
            canWriteTwelve,
            " / "_el,
            el::BooleanFormat::yesNo(),
            canWriteThirteen);
        el::io::printLine("Empty after clear : "_el, el::BooleanFormat::yesNo(), emptyAfterClear);
        el::io::printLine("Shrunk capacity   : "_el, observations.capacity().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Monitoraggio      : Laguna corallina
    Reservation       : yes
    Reserved capacity : 12
    Readable/available: 3 / 9
    Can write 12 / 13 : yes / no
    Empty after clear : yes
    Shrunk capacity   : 4

.. erbsland-demo-end::

Writing and Consuming Bytes
===========================

:cpp:func:`write() <erbsland::mem::RingBuffer::write>` copies as many source bytes as the hard limit permits and returns
the transferred length.
This partial behavior suits streams where the producer can retain and retry a remainder.
:cpp:func:`writeExact() <erbsland::mem::RingBuffer::writeExact>` is transactional at the buffer boundary: it queues the
complete span or returns failure without changing the stream.
Use it when splitting a record would complicate the surrounding protocol state.

Reading into a :cpp:type:`ByteSpan <erbsland::mem::ByteSpan>` copies as many queued bytes as fit in the destination and
returns the transferred length.
The other :cpp:func:`read() <erbsland::mem::RingBuffer::read>` overload consumes up to a chosen maximum into a new
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
The ``infinite()`` value of :cpp:type:`ByteLength <erbsland::unit::ByteLength>` is useful when you deliberately want all
currently buffered data.

Neither read waits for more input.
A short result simply means that fewer bytes were available at that moment, so the caller decides whether to continue,
retry later, or treat the short record as an error.

.. erbsland-demo::
    :source: mem/RingBuffer/ReadingAndWritingBytes.cpp
    :exec: mem/ring_buffer --demo ReadingAndWritingBytes
    :source-sha256: 7ddfb65ba97aba00bd7cd6f95c2cc9d277542c63d8b16c740b3b8350a8ef3766

.. code-block:: cpp

    /// Write and consume byte sequences with partial or atomic semantics.
    ///
    /// `write()` accepts as many bytes as fit below the hard limit. `writeExact()`
    /// either queues a complete sequence or leaves the ring unchanged. Reads likewise
    /// consume either into caller-owned storage or into a new `ByteBlock`.
    void readingAndWritingBytes() {
        auto current = el::RingBuffer{el::ByteLength{4U}, el::ByteLength{6U}};

        // Queue a complete observation and reject an atomic record that cannot fit.
        const auto first = el::ByteArray{el::Byte{10U}, el::Byte{20U}, el::Byte{30U}, el::Byte{40U}};
        const auto firstWrite = current.write(first.span());
        const auto rejected = current.writeExact(el::ByteArray{el::Byte{50U}, el::Byte{60U}, el::Byte{70U}}.span());

        // Consume two bytes into caller storage, then append a partial sequence.
        auto prefixStorage = std::array<el::Byte, 2>{};
        const auto prefixLength = current.read(el::ByteSpan{prefixStorage});
        const auto partialWrite =
            current.write(el::ByteArray{el::Byte{50U}, el::Byte{60U}, el::Byte{70U}, el::Byte{80U}}.span());
        const auto remaining = current.read(el::ByteLength::infinite());

        el::io::printLine("Corrente          : Canale della barriera"_el);
        el::io::printLine("First write       : "_el, firstWrite.toSizeT());
        el::io::printLine("Atomic write      : "_el, el::BooleanFormat::yesNo(), rejected.isSuccessful());
        el::io::printLine("Consumed prefix   : "_el, prefixLength.toSizeT(), " bytes"_el);
        el::io::printLine("Partial write     : "_el, partialWrite.toSizeT());
        el::io::printLine("Remaining FIFO    : "_el, el::ByteFormat::separated(), remaining);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Corrente          : Canale della barriera
    First write       : 4
    Atomic write      : no
    Consumed prefix   : 2 bytes
    Partial write     : 4
    Remaining FIFO    : 1e 28 32 3c 46 50

.. erbsland-demo-end::

Transferring Structured Integers
================================

:cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` adds
:cpp:func:`writeInteger() <erbsland::mem::ByteRingBuffer::writeInteger>` for native fixed-width integer types.
The complete integer is written atomically or the operation fails without adding bytes.
This prevents half of a field from becoming visible to the consumer.

:cpp:func:`readInteger() <erbsland::mem::ByteRingBuffer::readInteger>` follows the same rule from the read side.
If too few bytes are queued for ``sizeof(T)``, it returns an empty ``std::optional`` and consumes nothing.
After enough bytes arrive, reading the value consumes the complete field.

Both operations use the byte order selected with
:cpp:func:`setEndianness() <erbsland::mem::ByteRingBuffer::setEndianness>`.
Producer and consumer must also agree on the sequence and C++ width of each field; a ring buffer deliberately carries no
type tags or record boundaries of its own.

.. erbsland-demo::
    :source: mem/RingBuffer/ReadingAndWritingIntegers.cpp
    :exec: mem/ring_buffer --demo ReadingAndWritingIntegers
    :source-sha256: 8159f1bf19104bdce940f0d200c996eb9e63afd60bfbe4c18d110951bd30440e

.. code-block:: cpp

    /// Queue complete integer fields in a byte ring buffer.
    ///
    /// `ByteRingBuffer` applies one configured byte order to fixed-width integers.
    /// Writes are atomic at the hard capacity limit, and reads return an empty
    /// optional without consuming data when a complete value is unavailable.
    void readingAndWritingIntegers() {
        auto measurements = el::ByteRingBuffer{el::ByteLength{4U}, el::ByteLength{8U}};
        measurements.setEndianness(el::Endianness::Big);

        // Queue two complete fields in their stream order.
        const auto temperatureWritten = measurements.writeInteger<int16_t>(245);
        const auto colonyWritten = measurements.writeInteger<uint32_t>(120000U);

        // Read the same field types in FIFO order.
        const auto temperature = measurements.readInteger<int16_t>();
        const auto colonySize = measurements.readInteger<uint32_t>();
        const auto missingValue = measurements.readInteger<uint16_t>();

        el::io::printLine("Rilevamento       : Giardino di corallo"_el);
        el::io::printLine(
            "Fields written    : "_el,
            el::BooleanFormat::yesNo(),
            temperatureWritten.isSuccessful() && colonyWritten.isSuccessful());
        el::io::printLine("Temperature       : "_el, temperature.value_or(0));
        el::io::printLine("Colony size       : "_el, colonySize.value_or(0U));
        el::io::printLine("Extra field       : "_el, el::BooleanFormat::yesNo(), missingValue.has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Rilevamento       : Giardino di corallo
    Fields written    : yes
    Temperature       : 245
    Colony size       : 120000
    Extra field       : no

.. erbsland-demo-end::

The :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` returned by ``read(maximum)`` is the natural value to pass onward
after bytes leave the queue.
For more elaborate structured parsing, feed such a block to a
:cpp:class:`ByteReader <erbsland::mem::ByteReader>` so parsing state and format decisions remain separate from buffering.
