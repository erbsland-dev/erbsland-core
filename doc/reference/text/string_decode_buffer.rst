.. index::
    single: String Decode Buffer

********************
String Decode Buffer
********************

Introduction
============

``StringDecodeBuffer`` incrementally decodes byte chunks into Erbsland strings.
It is useful for file, terminal, and network input where byte chunks can split an encoded code point.

The buffer keeps incomplete trailing code points pending until more bytes are written or ``finish()`` is called.
Use ``peek...()`` methods to inspect decoded text without consuming bytes, and ``take...()`` methods to consume the
bytes that produced the returned complete characters.
Use ``readChar()`` to consume a single decoded character without constructing a string, and ``takeStringLine()`` to
consume decoded UTF-8 text up to and including the next LF character.

``TextRingBuffer`` provides the output-side counterpart.
It preflights the complete encoded length and atomically encodes text into a bounded byte ring.
The default replacement mode writes incrementally without materializing a second byte block for the complete encoded
string.

.. code-block:: cpp

    auto buffer = el::text::StringDecodeBuffer{el::ByteLength{4096}, el::StringEncoding::Utf8};
    buffer.write(bytes);
    auto text = buffer.takeString();

Unsafe Write Access
===================

``text::impl::UnsafeDecodeBufferAccess`` exposes the first contiguous writable byte span of a ``StringDecodeBuffer``.
It is intended for native read APIs that write directly into caller-provided memory.
After a successful native read, call ``commitWritten()`` with the number of bytes actually written.

Interface
=========

.. doxygenclass:: erbsland::text::StringDecodeBuffer
    :members:
.. doxygenclass:: erbsland::text::TextRingBuffer
    :members:
