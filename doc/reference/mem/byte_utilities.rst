.. index::
    single: Byte Reader, Writer, Types and Utilities

****************************************
Byte Reader, Writer, Types and Utilities
****************************************

Introduction
============

Byte Block
----------

:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` stores arbitrary bytes in a copy-on-write block.
Use it when you need a safe, shareable byte container with explicit :cpp:class:`Byte <erbsland::mem::Byte>` values and
byte-unit indexes.

Basic Usage
~~~~~~~~~~~

Create a block from raw byte values, read and write with :cpp:type:`ByteIndex <erbsland::unit::ByteIndex>`, and use
:cpp:class:`ByteBlockView <erbsland::mem::ByteBlockView>` for inexpensive slices.

.. code-block:: cpp

    auto data = el::ByteBlock{std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04}};
    auto header = data.slice(el::ByteIndex{0}, el::ByteLength{2});

    data.set(el::ByteIndex{1}, el::Byte{0xff});
    auto bytes = data.toUInt8Vector();

Modification
~~~~~~~~~~~~

Ranges are clamped to the visible data.
Invalid ranges are treated as empty for removals and replacements; :cpp:func:`keep() <erbsland::mem::ByteBlock::keep>`
with an invalid range clears the block.

Byte Block View
---------------

:cpp:class:`ByteBlockView <erbsland::mem::ByteBlockView>` is a read-only, shared view into byte block data.
Slices are cheap because they keep the same storage and only adjust an internal byte range.

Byte Reader
-----------

:cpp:class:`ByteReader <erbsland::mem::ByteReader>` reads bytes and integer values sequentially from a
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` or :cpp:class:`ByteBlockView <erbsland::mem::ByteBlockView>`.
Default-returning reads leave the position unchanged when there are not enough bytes.

Byte Writer
-----------

:cpp:class:`ByteWriter <erbsland::mem::ByteWriter>` writes bytes and integer values sequentially into a
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
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
submit its byte block with one atomic byte-stream write. This avoids checking each individual field write.

Interface
=========

.. doxygenclass:: erbsland::mem::Byte
    :members:
.. doxygenclass:: erbsland::mem::ByteBlock
    :members:
.. doxygenclass:: erbsland::mem::ByteBlockView
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
