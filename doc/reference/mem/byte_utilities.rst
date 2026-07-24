.. index::
    single: Byte Reader, Writer, Types and Utilities

****************************************
Byte Reader, Writer, Types and Utilities
****************************************

Introduction
============

Byte Values, Arrays, and Views
------------------------------

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
----------

:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` is the owning read-only type for arbitrary byte sequences.
It shares copy-on-write storage, and its slices retain the same storage while exposing a smaller byte range.
It is also the scalar byte-storage type used by configuration values, hashes, signatures, and other Core APIs, so these
subsystems can exchange immutable byte data without adapter wrappers.

Basic Usage
~~~~~~~~~~~

Create a block from raw byte values and read it with :cpp:type:`ByteIndex <erbsland::unit::ByteIndex>`.
Use :cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` when the bytes must be changed.

.. code-block:: cpp

    const auto data = el::ByteBlock({0x01, 0x02, 0x03, 0x04});
    auto header = data.slice(el::ByteIndex{0}, el::ByteLength{2});

    auto editor = el::ByteBlockEditor{data};
    editor.set(el::ByteIndex{1}, el::Byte{0xff});
    const auto modified = el::ByteBlock{editor};

Modification
~~~~~~~~~~~~

Ranges are clamped to the visible data.
Invalid ranges are treated as empty for removals and replacements;
:cpp:func:`keep() <erbsland::mem::ByteBlockEditor::keep>` with an invalid range clears the editor.

Byte Block Editor
-----------------

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
The ``xorWith*()`` methods combine equal-length blocks without exposing writable storage.

Sensitive Storage
~~~~~~~~~~~~~~~~~

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
~~~~~~~~~~~~~~~~~~~~~~~~

``ByteBuffer`` replaces raw dynamic byte vectors at owning Erbsland API boundaries.
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

Internally, byte blocks expose their storage through ``mem::impl::ByteDataView``.
Shared read, comparison, and runtime-aware modification tools implement the algorithms once, so editors do not create
temporary read-only blocks or byte vectors to reuse behavior.
``mem::impl::UnsafeByteBlockBuffer`` supports growing uncommitted storage while preserving a written prefix and then
transfers the completed allocation into a byte block without a final copy.
Its storage is uninitialized and can be created in sensitive mode; a low-level producer must write every byte included
in ``take()``.

Byte Reader
-----------

:cpp:class:`ByteReader <erbsland::mem::ByteReader>` reads bytes and integer values sequentially from a
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` or
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>`.
Default-returning reads leave the position unchanged when there are not enough bytes.

Byte Writer
-----------

:cpp:class:`ByteWriter <erbsland::mem::ByteWriter>` writes bytes and integer values sequentially into an internal editor
and returns the completed data as a read-only :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
Writing overwrites at the current position or appends when the position is at the end.

Ring Buffers
------------

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
--------------

:cpp:func:`secureErase() <erbsland::mem::secureErase>` overwrites a caller-owned writable byte span using the
platform's optimizer-resistant erasure primitive.
``ByteArray``, ``ByteBuffer``, ``ByteBlock``, and ``ByteBlockEditor`` provide a member ``secureErase()`` for owned
storage.

Block erasure preserves the logical length.
A uniquely owned block erases its complete capacity.
Erasing a shared block installs independent zero-filled storage, so aliases and slices remain unchanged.
Editors also preserve their observable capacity.
If allocating replacement storage fails, the invoking shared block is unchanged.

Interface
=========

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
.. doxygenfunction:: erbsland::mem::getInteger(const ConstByteSpan bytes, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little, const T defaultOnError = T()) noexcept -> T

.. doxygenfunction:: erbsland::mem::getIntegerOrThrow(const ConstByteSpan bytes, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) -> T

.. doxygenfunction:: erbsland::mem::getIntegerInto(const ConstByteSpan bytes, T &value, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) noexcept -> bool

.. doxygenfunction:: erbsland::mem::setInteger(const ByteSpan bytes, const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little) noexcept -> bool

.. doxygenfunction:: erbsland::mem::setIntegerOrThrow(const ByteSpan bytes, const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little)
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
.. doxygenclass:: erbsland::mem::ByteWriter
    :members:
.. doxygenenum:: erbsland::mem::Endianness
.. doxygenclass:: erbsland::mem::RingBuffer
    :members:
.. doxygenfunction:: erbsland::mem::secureErase(ByteSpan span) noexcept
