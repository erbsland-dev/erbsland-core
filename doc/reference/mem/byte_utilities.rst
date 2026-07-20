.. index::
    single: Byte Reader, Writer, Types and Utilities

****************************************
Byte Reader, Writer, Types and Utilities
****************************************

Introduction
============

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

    const auto data = el::ByteBlock{std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04}};
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

:cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` adds atomic endian-aware integer reads and writes.

For structured stream output, assemble a complete record with :cpp:class:`ByteWriter <erbsland::mem::ByteWriter>` and
submit its byte block with one atomic byte-stream write.
This avoids checking each individual field write.

Interface
=========

.. doxygenclass:: erbsland::mem::Byte
    :members:
.. doxygenclass:: erbsland::mem::ByteBlock
    :members:
.. doxygenclass:: erbsland::mem::ByteBlockEditor
    :members:
.. doxygenclass:: erbsland::mem::ByteReader
    :members:
.. doxygenclass:: erbsland::mem::ByteRingBuffer
    :members:
.. doxygenclass:: erbsland::mem::ByteWriter
    :members:
.. doxygenenum:: erbsland::mem::Endianness
.. doxygenclass:: erbsland::mem::RingBuffer
    :members:
