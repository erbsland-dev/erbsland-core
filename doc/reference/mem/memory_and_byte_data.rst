..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Memory; Reference
    single: Byte Data; Reference
    single: Byte Reader, Writer, Types and Utilities
    single: Byte Compression
    single: LZ4
    single: Cow Storage
    single: Unsafe Pointers

********************
Memory and Byte Data
********************

Byte Reader, Writer, Types and Utilities
========================================

Introduction
------------

Byte Values, Arrays, and Views
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:cpp:class:`Byte <erbsland::mem::Byte>` represents one explicit byte and provides conversions, masking, bitwise
operators, shifts, and rotations.
:cpp:class:`ByteArray <erbsland::mem::ByteArray>` provides fixed-size mutable byte storage with safe indexed and bulk
operations, whole-array and per-byte bit operations, and endian-aware integer access.
:cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` provides equivalent operations for compact, uniquely owned
dynamic byte storage without copy-on-write.
:cpp:type:`ByteIndex <erbsland::unit::ByteIndex>`,
:cpp:type:`ByteLength <erbsland::unit::ByteLength>`, and
:cpp:type:`ByteRange <erbsland::unit::ByteRange>` describe all owning-container access.
Range spans and mutations clamp to the owner; invalid ranges produce an empty view or a no-op, and infinite lengths
extend to the end.
The ``xorAt*()`` methods modify one indexed byte without a separate read, while ``xorWith*()`` combines byte ranges.
``ByteArray::secureErase()`` clears the complete fixed storage.
``ByteBuffer::secureErase()`` clears the complete allocated capacity while preserving its visible length and capacity.

:cpp:type:`ByteSpan <erbsland::mem::ByteSpan>` and
:cpp:type:`ConstByteSpan <erbsland::mem::ConstByteSpan>` are dynamic-extent writable and read-only views.
:cpp:type:`FixedByteSpan <erbsland::mem::FixedByteSpan>` and
:cpp:type:`FixedConstByteSpan <erbsland::mem::FixedConstByteSpan>` retain an extent in the type.
Spans are views rather than owning values and therefore remain bound to their source object.
The ``toByteSpan()`` and ``toConstByteSpan()`` compatibility functions reinterpret compatible ``std::byte``,
``uint8_t``, and ``char`` spans without copying; the returned view has the same lifetime and invalidation rules as the
source span.
The free ``getInteger*`` and ``setInteger*`` helpers provide the same checked endian-aware access for any byte span.

Byte Block
~~~~~~~~~~

:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` is the owning read-only type for arbitrary byte sequences.
It shares copy-on-write storage, and its slices retain the same storage while exposing a smaller byte range.
It is also the scalar byte-storage type used by configuration values, hashes, signatures, and other Core APIs, so these
subsystems can exchange immutable byte data without adapter wrappers.

Basic Usage
^^^^^^^^^^^

Create a block from raw byte values and read it with :cpp:type:`ByteIndex <erbsland::unit::ByteIndex>`.
Use :cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` when the bytes must be changed.

.. code-block:: cpp

    const auto data = el::ByteBlock({0x01, 0x02, 0x03, 0x04});
    auto header = data.slice(el::ByteIndex{0}, el::ByteLength{2});

    auto editor = el::ByteBlockEditor{data};
    editor.set(el::ByteIndex{1}, el::Byte{0xff});
    const auto modified = el::ByteBlock{editor};

Modification
^^^^^^^^^^^^

Ranges are clamped to the visible data.
Invalid ranges are treated as empty for removals and replacements;
:cpp:func:`keep() <erbsland::mem::ByteBlockEditor::keep>` with an invalid range clears the editor.

Byte Block Editor
~~~~~~~~~~~~~~~~~

:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` provides mutation and capacity management.
Converting an editor to ``ByteBlock`` shares the complete data without copying; later editor mutations detach and leave
the read-only value unchanged.
Constructing an editor from a ``ByteBlock`` copies only the visible bytes, including when the source is a slice.
The editor provides the same checked little- and big-endian integer helpers as ``ByteArray``.
Failed tolerant writes validate before detaching, so both bytes and sharing remain unchanged.
``appendInteger()`` encodes and appends one integer.
``overwrite()`` is a non-resizing operation that copies the largest possible prefix of the source into a clamped target
range.
Invalid, outside, or empty targets and empty sources are no-ops.
It validates no-op cases before detaching and differs intentionally from the clamping, resizing ``replace()`` operation.
The whole-sequence ``xorWith()`` methods return ``false`` without changing the destination when lengths differ.
Use ``xorWithOrThrow()`` when equal lengths are an invariant and a mismatch must raise ``ParameterError``.
Range overloads remain clamped operations that combine as many source bytes as fit.

Sensitive Storage
^^^^^^^^^^^^^^^^^

``ByteBlock`` and ``ByteBlockEditor`` can mark their shared allocation with ``markAsSensitive()``.
The mark is visible to every alias and cannot be cleared.
Owning-block mutations and deep copies propagate it, while raw spans carry no sensitivity metadata.
Marked allocations are erased in full when reallocated or finally destroyed; ordinary formatting and conversions remain
available and produce unprotected copies.

``ByteBuffer`` instead has a reversible object mode controlled by ``setSensitive()``.
Copies duplicate the mode and storage independently, while moves transfer both.
In sensitive mode, truncated and replaced bytes and old allocations are erased before release.
Disabling the mode erases the complete allocation, discards visible bytes, and retains capacity.
``clear()``, ``reset()``, and ``shrinkToFit()`` preserve the selected mode.
See :doc:`/topics/security/about_sensitive_strings_and_byte_blocks` for the complete contract.

Compatibility Boundaries
^^^^^^^^^^^^^^^^^^^^^^^^

``ByteBuffer`` replaces raw dynamic byte vectors at owning Erbsland Core API boundaries.
Short byte sequences can be initialized directly with ``ByteBlock({0x01, 0x02})``.
The ``fromSpan()`` and raw ``uint8_t`` /``char`` ``fromVector()`` factories make compatibility copies explicit, while
``toByteBuffer()``, ``toUInt8Vector()``, and ``toCharVector()`` explicitly copy ordinary block data out.
Searches and editor modifiers accept ``ConstByteSpan`` and ``FixedConstByteSpan`` for call-scoped borrowing without
creating an intermediate block.
If such an input overlaps the destination storage, the editor preserves only the necessary source bytes before detaching
or reallocating.
Raw vector parameters remain confined to the explicit compatibility factories.

Every contiguous owning byte type exposes borrowed read-only full and ranged spans.
An array span remains bound through in-place mutation.
A buffer span remains bound through non-resizing writes, but allocation-changing edits, reset, move, or destruction
invalidate it.
Treat any editor mutation as invalidating its spans because copy-on-write may detach storage; block erasure, reset,
move, or destruction likewise invalidates a block span.
A block slice keeps its own owning reference when selected bytes must outlive the call.
Owning types expose no writable span, raw pointer, size, or iterator API.
Low-level integrations keep :cpp:type:`ByteSpan <erbsland::mem::ByteSpan>` as an explicit native boundary.
Internal native adapters use ``mem::impl::UnsafeByteArrayAccess`` or ``mem::impl::UnsafeByteBufferAccess`` when owning
scratch storage must be passed to a writable byte-span boundary.

Internally, every contiguous byte owner exposes its storage through a private ``mem::impl::ByteDataView``.
Stateful read and comparison tools bind to this raw view, which lets arrays, buffers, blocks, editors, and readers share
algorithms without temporary owners or repeated shared-storage traversal.
Writable owners expose a private writable span for fixed-size mutation tools.
The owner still validates the operation and handles copy-on-write, allocation, sensitivity, and aliased sources before
requesting writable storage.
``mem::impl::UnsafeByteBlockAccess`` marks native read-only integration boundaries and exposes the same data-view type;
it does not provide editor or writable access.
``mem::impl::UnsafeByteBlockBuffer`` supports growing uncommitted storage while preserving a written prefix and then
transfers the completed allocation into a byte block without a final copy.
Its storage is uninitialized and can be created in sensitive mode; a low-level producer must write every byte included
in ``take()``.

Byte Reader
~~~~~~~~~~~

:cpp:class:`ByteReader <erbsland::mem::ByteReader>` reads bytes and integer values sequentially from a
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` or
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>`.
It can return exact byte ranges and decode structured text frames.
``ByteIntegerFormat`` fixes the signedness and wire width of a formatted integer independently from its C++ destination
type.
Alongside fixed widths and the compact count formats, it supports canonical most-significant-group-first unsigned
base-128 integers for ASN.1 identifier and object-identifier components.
``ByteTextOptions`` defines UTF encoding, count prefix, optional validated end mark, and dynamic or padded-field
framing.
Structured reads parse with a local cursor over the raw data and commit the reader position only after the complete
value has been validated and decoded.
This includes readers positioned in the middle of a block.
Strict reads leave the position unchanged on failure; optional reads report an incomplete or invalid frame without
advancing.

Bit Reader
~~~~~~~~~~

``BitReader`` reads individual bits from a borrowed
:cpp:type:`ConstByteSpan <erbsland::mem::ConstByteSpan>` without copying or owning its input.
It processes the most-significant bit first in each byte and exposes the total bit count, current position, remaining
count, bounded position changes, and end checks.
``readBool()`` returns the next bit as a boolean, while ``readInteger<T>()`` returns the same bit as zero or one of a
selected native integer type.
Reads at the end return ``false`` or zero without advancing.

Byte Writer
~~~~~~~~~~~

:cpp:class:`ByteWriter <erbsland::mem::ByteWriter>` writes bytes and integer values sequentially into an internal editor
and returns the completed data as a read-only :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
Writing overwrites at the current position or appends when the position is at the end.
Text and formatted-integer writes prepare the complete encoded value before modifying the writer, so encoding or
validation failures leave the written data and position unchanged.
Text writes use ``ByteTextOptions``.
The default is dynamic UTF-8 with an unsigned 32-bit unit count, while ``ByteTextOptions::compact()`` uses an unsigned
variable-length count.
An explicit end mark is always validated by the reader, including in padded fields.

Ring Buffers
~~~~~~~~~~~~

:cpp:class:`RingBuffer <erbsland::mem::RingBuffer>` stores bytes in fixed or bounded-growing contiguous storage.
Safe methods copy data into and out of the ring.
Atomic reserve and exact-write operations return :cpp:class:`Result <erbsland::util::Result>`; failure means the hard
limit was exceeded and the ring was not modified.
Low-level platform adapters use an internal exclusive access lease to pass the ring's one or two contiguous sections
directly to native APIs.
Sensitive mode is enabled with ``setSensitive(true)``.
In this mode the ring securely erases consumed bytes and old allocations after capacity changes, and erases its storage
on destruction.
``secureErase()`` wipes the complete capacity and empties the ring while preserving the mode.
Disabling sensitive mode also wipes the complete capacity and discards unread bytes.
Owned reads from a sensitive ring return a marked ``ByteBlock``.

:cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` adds atomic endian-aware integer reads and writes.

For structured stream output, assemble a complete record with :cpp:class:`ByteWriter <erbsland::mem::ByteWriter>` and
submit its byte block with one atomic byte-stream write.
This avoids checking each individual field write.

Secure Erasure
~~~~~~~~~~~~~~

:cpp:func:`secureErase() <erbsland::mem::secureErase>` overwrites a caller-owned writable byte span using the
platform's optimizer-resistant erasure primitive.
``ByteArray``, ``ByteBuffer``, ``ByteBlock``, and ``ByteBlockEditor`` provide a member ``secureErase()`` for owned
storage.

Block erasure preserves the logical length.
A uniquely owned block erases its complete capacity.
Erasing a shared block installs independent zero-filled storage, so aliases and slices remain unchanged.
Editors also preserve their observable capacity.
If allocating replacement storage fails, the invoking shared block is unchanged.

Byte Compression
================

The byte-compression API provides raw LZ4 blocks and a framed representation that records the algorithm and original
length.
Use raw blocks when another format already carries this metadata, and use the envelope for standalone stored or
transmitted values.

Raw Compression
---------------

:cpp:class:`ByteCompressor <erbsland::mem::ByteCompressor>` always returns a valid raw block, even when the block is
larger than its input.
Raw :cpp:class:`ByteDecompressor <erbsland::mem::ByteDecompressor>` calls require the exact original size and reject
output-length mismatches.

Compression Envelopes
---------------------

``compressWithEnvelope()`` frames the raw payload with the stable ``ELBC`` header, algorithm identifier, original
length, and payload length.
:cpp:func:`ByteDecompressor::decompressWithEnvelope()
<erbsland::mem::ByteDecompressor::decompressWithEnvelope>` validates the complete frame and automatically selects the
encoded algorithm.
The optional maximum-output limit is checked before allocating output storage.
The envelope provides framing only; it is not an integrity or authentication mechanism.

Buffered Operation
------------------

Both codec classes accept input through ``update()`` and emit one result during finalization.
The first successful finalizer selects raw or envelope output until ``reset()`` is called, and repeated calls to the
same finalizer return the cached result.
One-shot methods do not alter this buffered state.

Cow Storage
===========

Introduction
------------

Cow Storage
~~~~~~~~~~~

:cpp:class:`CowStorage <erbsland::mem::CowStorage>` is a small copy-on-write helper for ordinary C++ data types.
It stores the data in a ``std::shared_ptr`` and always keeps a valid data object, so user code can work with references
instead of raw pointers.

Copying the storage shares the data object.
Mutable access through ``data()`` automatically detaches when the object is shared.
Calling ``detach()`` before a group of write operations is optional, but can make the copy-on-write point explicit.
Operations that create, replace, or detach data may throw allocation errors or exceptions from the stored type.

Separate storage objects may be copied, destroyed, and detached from different threads.
Concurrent access to the same storage object, or concurrent mutation of the same detached data object, still requires
external synchronization.

Example
^^^^^^^

.. code-block:: cpp

    using Storage = erbsland::mem::CowStorage<std::set<erbsland::text::Char>>;

    auto storage = Storage::from(std::set<erbsland::text::Char>{ch});
    auto copy = storage;

    storage.detach();
    storage.data().insert(otherChar);

Cow Manual Storage
~~~~~~~~~~~~~~~~~~

:cpp:class:`CowManualStorage <erbsland::mem::CowManualStorage>` is a copy-on-write helper for code that wants
explicit writable access.
It stores the data in a ``std::shared_ptr`` and always keeps a valid data object.

Reading uses ``data()``.
Writing uses ``detachedData()``, which detaches first when the data object is shared.
This makes mutable call sites easy to find while avoiding nullable or raw pointer handling in user code.
Operations that create, replace, or detach data may throw allocation errors or exceptions from the stored type.

Use ``sharedDefault()`` when many storage instances can start with the same default-constructed data object.
The method keeps one shared default object for each data type and returns another owner for every call.
The retained canonical owner contributes to ``useCount()`` and ensures that writable access always detaches from the
default object.
Ordinary default construction remains unique and constructs a separate data object for each storage instance.

Shared Array Data
~~~~~~~~~~~~~~~~~

:cpp:class:`SharedArrayData <erbsland::mem::SharedArrayData>` stores a small shared-data header and a trailing array in
one allocation.
The actual elements are placed in aligned storage immediately after the header, which keeps compact string-like storage
types cache-friendly while still using the intrusive reference-counting model.

Use this type when you build library internals that need copy-on-write array storage.
The allocation must be created, cloned, and destroyed through
:cpp:class:`SharedArrayData <erbsland::mem::SharedArrayData>` itself, because the header and trailing elements are one
memory block.

:cpp:enum:`SharedArrayDataCleanupMethod <erbsland::mem::SharedArrayDataCleanupMethod>` selects ordinary cleanup or
secure erasure for raw, trivially copyable arrays.
Secure allocations are zero-initialized across their complete element capacity and securely erased through an
optimizer-resistant platform backend before deallocation.
The final erase covers the one-block allocation in full: reference-count metadata, size and capacity fields, alignment
padding, used elements, and unused capacity.
Every copy-on-write allocation is erased independently when its final owner releases it.
If construction or cloning fails, already constructed elements are destroyed and the failed allocation follows the same
cleanup path before the exception is rethrown.

Headers that only store or pass a :cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>` to shared array data
can include ``SharedArrayData_fwd.hpp``.
Constructors, destructors, copies, detach operations, and direct data access must be implemented in a source file that
includes ``SharedArrayData.hpp``.
This keeps the full allocation template out of dependent headers without adding a PImpl allocation or changing the
storage layout.

Shared Data
~~~~~~~~~~~

:cpp:class:`SharedData <erbsland::mem::SharedData>` is the base class for custom data blocks that participate in
Erbsland Core's intrusive shared-data model.
It places the :cpp:class:`ReferenceCounter <erbsland::mem::ReferenceCounter>` at the start of derived data and marks the
type as compatible with the shared-data traits.

Use this class only when you extend the library with a new shared storage type.
For ordinary application data, prefer the public value types that already manage their storage for you.

Shared Data Pointer
~~~~~~~~~~~~~~~~~~~

:cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>` is an intrusive copy-on-write pointer for supported
shared data objects.
It owns a pointer to data with an embedded :cpp:class:`ReferenceCounter <erbsland::mem::ReferenceCounter>`, increments
and decrements that counter as pointers are copied or destroyed, and destroys the allocation when the last reference is
released.

Copying the pointer shares the same data.
Mutable access automatically detaches shared data by cloning it, unless ``tManualDetach`` is enabled.
Manual detach mode is useful for code that wants to control exactly when copy-on-write materialization happens.

Separate :cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>` instances may be copied, destroyed, reset,
and detached from different threads.
Concurrent access to the same pointer object, or concurrent mutation of the same already-detached data object, still
requires external synchronization.

Unsafe Pointers
===============

Interface
=========

.. doxygenclass:: erbsland::mem::BitReader
    :members:
.. doxygenclass:: erbsland::mem::Byte
    :members:
.. doxygenclass:: erbsland::mem::ByteArray
    :members:
.. doxygenclass:: erbsland::mem::ByteBlock
    :members:
.. doxygenclass:: erbsland::mem::ByteBlockEditor
    :members:
.. doxygenclass:: erbsland::mem::ByteBuffer
    :members:
.. doxygenclass:: erbsland::mem::ByteCompressionAlgorithm
    :members:
.. doxygenclass:: erbsland::mem::ByteCompressionError
    :members:
.. doxygenenum:: erbsland::mem::ByteCompressionErrorReason
.. doxygenclass:: erbsland::mem::ByteCompressor
    :members:
.. doxygenclass:: erbsland::mem::ByteDecompressor
    :members:
.. doxygenfunction:: erbsland::mem::getInteger(const ConstByteSpan bytes, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little, const T defaultOnError = T()) noexcept -> T

.. doxygenfunction:: erbsland::mem::getIntegerOrThrow(const ConstByteSpan bytes, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) -> T

.. doxygenfunction:: erbsland::mem::getIntegerInto(const ConstByteSpan bytes, T &value, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) noexcept -> bool

.. doxygenfunction:: erbsland::mem::setInteger(const ByteSpan bytes, const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little) noexcept -> bool

.. doxygenfunction:: erbsland::mem::setIntegerOrThrow(const ByteSpan bytes, const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little)
.. doxygenclass:: erbsland::mem::ByteIntegerFormat
    :members:
.. doxygenclass:: erbsland::mem::ByteReader
    :members:
.. doxygenclass:: erbsland::mem::ByteRingBuffer
    :members:
.. doxygentypedef:: erbsland::mem::ByteSpan

.. doxygentypedef:: erbsland::mem::ConstByteSpan

.. doxygentypedef:: erbsland::mem::FixedByteSpan

.. doxygentypedef:: erbsland::mem::FixedConstByteSpan

.. doxygenfunction:: erbsland::mem::toByteSpan(std::span<std::byte> span) noexcept -> ByteSpan

.. doxygenfunction:: erbsland::mem::toByteSpan(std::span<uint8_t> span) noexcept -> ByteSpan

.. doxygenfunction:: erbsland::mem::toByteSpan(std::span<char> span) noexcept -> ByteSpan

.. doxygenfunction:: erbsland::mem::toConstByteSpan(std::span<const std::byte> span) noexcept -> ConstByteSpan

.. doxygenfunction:: erbsland::mem::toConstByteSpan(std::span<const uint8_t> span) noexcept -> ConstByteSpan

.. doxygenfunction:: erbsland::mem::toConstByteSpan(std::span<const char> span) noexcept -> ConstByteSpan
.. doxygenenum:: erbsland::mem::ByteTextFormat
.. doxygenclass:: erbsland::mem::ByteTextOptions
    :members:
.. doxygenclass:: erbsland::mem::ByteWriter
    :members:
.. doxygenclass:: erbsland::mem::CowManualStorage
    :members:
.. doxygenclass:: erbsland::mem::CowStorage
    :members:
.. doxygenenum:: erbsland::mem::Endianness
.. doxygenclass:: erbsland::mem::ReferenceCounter
    :members:
.. doxygenclass:: erbsland::mem::RingBuffer
    :members:
.. doxygenfunction:: erbsland::mem::secureErase(ByteSpan span) noexcept

.. doxygenfunction:: erbsland::mem::secureErase(const std::span<T, Extent> span) noexcept
.. doxygenclass:: erbsland::mem::SharedArrayData
    :members:
.. doxygenenum:: erbsland::mem::SharedArrayDataCleanupMethod
.. doxygenenum:: erbsland::mem::SharedArrayDataConstructMethod
.. doxygenclass:: erbsland::mem::SharedData
    :members:
.. doxygenclass:: erbsland::mem::SharedDataPointer
    :members:
.. doxygenclass:: erbsland::mem::SharedVirtualData
    :members:
.. doxygenclass:: erbsland::mem::StorageIdentifier
    :members:
.. doxygentypedef:: erbsland::mem::UnsafeConstCharPtr

.. doxygentypedef:: erbsland::mem::UnsafeCharPtr

.. doxygentypedef:: erbsland::mem::UnsafeConstChar8Ptr

.. doxygentypedef:: erbsland::mem::UnsafeChar8Ptr
.. doxygentypedef:: erbsland::mem::UnsafeConstMemoryPtr

.. doxygentypedef:: erbsland::mem::UnsafeMemoryPtr
