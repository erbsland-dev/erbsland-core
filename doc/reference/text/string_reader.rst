.. index::
    single: String Parser and Reader

************************
String Parser and Reader
************************

Introduction
============

String Char Reader
------------------

:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` is a sequential reader for decoded Unicode code points.
It accepts :cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>` /:cpp:class:`U8String
<erbsland::text::U8String>`,
:cpp:class:`U16StringEditor <erbsland::text::U16StringEditor>` /:cpp:class:`U16String <erbsland::text::U16String>`, and
:cpp:class:`U32StringEditor <erbsland::text::U32StringEditor>` /:cpp:class:`U32String <erbsland::text::U32String>` and
exposes the same read API for all encodings.

The reader keeps the source storage alive through the read-only string object stored in its backend.
Copying a reader shares the immutable source data, but the cursor state is copied, so moving one reader forward does not
move another copy.

Positions and States
~~~~~~~~~~~~~~~~~~~~

:cpp:func:`position() <erbsland::text::StringCharReader::position>` returns the current decoded code-point index as
``unit::CpIndex``.
This is the value user code should use for diagnostics and parse errors.

Reader Operations
~~~~~~~~~~~~~~~~~

:cpp:func:`read() <erbsland::text::StringCharReader::read>` consumes the next decoded character, while
:cpp:func:`peek() <erbsland::text::StringCharReader::peek>` only inspects it.
:cpp:func:`advance() <erbsland::text::StringCharReader::advance>` skips decoded characters when the value is not
needed.

Use :cpp:func:`readIf() <erbsland::text::StringCharReader::readIf>` and
:cpp:func:`advanceIf() <erbsland::text::StringCharReader::advanceIf>` for optional grammar characters.
They leave the cursor unchanged when the next character does not match.
The string overload of ``advanceIf()`` matches a complete UTF-8 token by decoded character and restores the starting
position after a partial match or insufficient input.
Its optional character-comparison function supports cases such as ASCII-insensitive grammar tokens without converting
the source text.
The ``OrThrow`` variants reject end-of-data and malformed encoding before matching.

``readWhile()`` and ``readUntil()`` invoke a callback and return ``LoopResult`` because the callback can stop or report
an error.
``advanceWhile()`` and ``advanceUntil()`` have no callback and instead return the ``unit::CpLength`` actually skipped.
They leave the boundary character unread and stop at the configured maximum or end-of-data.
The count can be ignored when only the skip operation matters, for example when discarding optional whitespace.

``parseInteger()`` parses a low-level integer token with
:cpp:class:`IntegerParseOptions <erbsland::text::IntegerParseOptions>`.
It restores the original reader position on failure and returns the parsed magnitude, sign state, resolved base, digit
count, and status.
Use ``readIntegerOrThrow<T>()`` when a reader should consume an integer token, convert it into a native or saturating
integer type, and report failures as ``ParseNumberError``.

Capture and Buffer
~~~~~~~~~~~~~~~~~~

The capture API marks a source range and returns it as an inexpensive owning read-only string slice.
Use
:cpp:func:`startCapture() <erbsland::text::StringCharReader::startCapture>` and
:cpp:func:`takeCapture() <erbsland::text::StringCharReader::takeCapture>` when the parsed token can be represented as
an unchanged slice of the input.

The reader buffer is owned parser text in the same encoding as the reader backend.
Use it for tokens that are assembled, normalized, escaped, or mixed from source text and manually appended characters.
``readToBuffer()`` and the ``readToBufferIf()`` variants consume and append one character.
``readToBufferWhile()`` and ``readToBufferUntil()`` append only accepted characters; stop, mismatch, end-of-data, and
limit characters are left unread and are not appended.

Saved reader states restore only the cursor position.
Capture and buffer state intentionally remain unchanged.

Interface
=========

.. doxygenclass:: erbsland::text::ParseNumberError
    :members:
.. doxygenstruct:: erbsland::text::ReadIntegerResult
    :members:
.. doxygenenum:: erbsland::text::ReadNumberStatus
.. doxygenclass:: erbsland::text::StringCharReader
    :members:
.. doxygenclass:: erbsland::text::StringCharReaderState
    :members:
