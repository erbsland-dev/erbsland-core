.. index::
    single: String Decode Buffer

********************
String Decode Buffer
********************

Introduction
============

``StringDecodeBuffer`` incrementally decodes byte chunks into Erbsland Core strings.
It is useful for file, terminal, and network input where byte chunks can split an encoded code point.
It is backed by :cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` and can operate in ordinary or sensitive mode.

The buffer keeps incomplete trailing code points pending until more bytes are written or ``finish()`` is called.
Use ``peek...()`` methods to inspect decoded text without consuming bytes, and ``take...()`` methods to consume the
bytes that produced the returned complete characters.
Use ``readChar()`` to consume a single decoded character without constructing a string, and ``takeStringLine()`` to
consume decoded UTF-8 text up to and including the next LF character.

For the output direction, use ``StringEncoder::encodedLength()`` and ``StringEncoder::encodeTo()`` with a
:cpp:class:`RingBuffer <erbsland::mem::RingBuffer>`.
The encoder preflights the complete encoded length and atomically writes text into a bounded byte ring without
materializing a second byte block.

.. code-block:: cpp

    auto buffer = el::text::StringDecodeBuffer{el::ByteLength{4096}, el::StringEncoding::Utf8};
    buffer.write(bytes);
    auto text = buffer.takeString();

Unsafe Write Access
===================

``text::impl::UnsafeDecodeBufferAccess`` exposes the first contiguous writable byte span of a ``StringDecodeBuffer``.
It is intended for native read APIs that write directly into caller-provided memory.
After a successful native read, call ``commitWritten()`` with the number of bytes actually written.

Sensitive Decoding
==================

Enable sensitive decoding with ``setSensitive(true)`` before writing input bytes.
This erases consumed prefixes, malformed sequences, byte-order marks, resets, and the final byte allocation.
UTF-8 results are marked sensitive; UTF-16 and UTF-32 results are ordinary because sensitivity is intentionally limited
to UTF-8 strings.
Disabling sensitive mode securely erases buffered input and resets the decoder.

Interface
=========

.. doxygenclass:: erbsland::text::StringDecodeBuffer
    :members:
