*********************
Stream API Guidelines
*********************

These guidelines extend the Common API Guidelines for public APIs in the ``stream`` namespace.
This namespace provides explicit byte and text input/output interfaces.
If you introduce new vocabulary or stream types, update this page.

Core Semantics
==============

Streams are explicit read/write objects.
They do not use ``operator<<`` or ``operator>>``.

.. code-block:: text

    stream.write(value)
    stream.writeLine(text)
    stream.print(value, otherValue)
    stream.printLine(value, otherValue)
    auto value = stream.read(...)
    auto text = stream.readLine()

Input and output are separate interfaces.
Byte and text streams are separate interfaces.
Concrete implementations may combine interfaces through multiple inheritance only when the object naturally supports all
of them.

End-of-stream is not an error.
Byte span reads report end-of-stream with a zero byte count when the requested span is not empty.
Single-value and line reads use ``std::optional`` so empty data remains distinguishable from end-of-stream.

Write calls accept the complete input or throw ``err::StreamError``.
User code should not have to recover from silent partial writes.

Primary Types
=============

.. code-block:: text

    InputStream // common base class for readable streams
    OutputStream // common base class for writable streams
    ByteInputStream // reads raw bytes and endian integer values
    ByteOutputStream // writes raw bytes and endian integer values
    TextInputStream // reads decoded Unicode text
    TextOutputStream // writes decoded Unicode text

Option and Enumeration Types
============================

.. code-block:: text

    StringEncoding // configured or effective byte encoding for text streams
    StringBomMode // byte order mark handling for encoded text streams
    EncodingErrorMode // decoding error behavior for text input streams

Byte Stream Patterns
====================

.. code-block:: text

    o.isOpen() -> bool // test if operations are allowed
    o.close() // close the stream wrapper
    o.flush() // output streams only: flush pending output
    o.endianness() -> mem::Endianness // byte order for integer helpers
    o.setEndianness(endianness) // set byte order for integer helpers
    o.read(span) -> ByteLength // read up to span.size() bytes
    o.read(length) -> ByteBlock // read up to length bytes once
    o.readExact(length) -> optional<ByteBlock> // read length bytes or return nullopt at early EOF
    o.readExactOrThrow(length) -> ByteBlock // read length bytes or throw at early EOF
    o.readByte() -> optional<Byte> // read one byte
    o.readByteOrThrow() -> Byte // read one byte or throw at EOF
    o.readAll() -> ByteBlock // read until EOF
    o.read❮IntType❯() -> optional<T> // read an endian integer or nullopt at early EOF
    o.read❮IntType❯OrThrow() -> T // read an endian integer or throw at early EOF
    o.write(byte/span/view) // write all bytes or throw
    o.write❮IntType❯(value) // write an endian integer

Text Stream Patterns
====================

.. code-block:: text

    o.encoding() -> StringEncoding // configured encoding
    o.effectiveEncoding() -> StringEncoding // concrete encoding after BOM resolution
    o.read() -> optional<String> // read up to TextInputStream::cDefaultTextReadMaximum
    o.read(maximum) -> optional<String> // read up to maximum decoded code points
    o.readLine() -> optional<String> // read one line with the default maximum
    o.readLine(maximum) -> optional<String> // read one line with its line ending
    o.readAll() -> String // read up to the default maximum
    o.readAll(maximum) -> String // read up to maximum decoded code points
    o.write(character/text) // write text without added line ending
    o.writeLine() // write a line-feed character
    o.writeLine(text) // write text followed by a line-feed character
    o.print(args...) // convenience output; builds a temporary string first
    o.printLine(args...) // convenience output plus one line-feed character
