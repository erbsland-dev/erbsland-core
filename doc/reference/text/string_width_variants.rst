.. index::
    single: String Width Variants

*********************
String Width Variants
*********************

.. _u8-string-storage-management:
.. _u8-string-byte-based-reading:
.. _u8-string-advance-retreat:
.. _u8-string-character-indexed-reading:
.. _u8-string-indexed-sequential-read:
.. _u16-string-storage-management:
.. _u16-string-code-unit-based-reading:
.. _u16-string-advance-retreat:
.. _u16-string-character-indexed-reading:
.. _u16-string-indexed-sequential-read:
.. _u32-string-storage-management:
.. _u32-string-code-unit-based-reading:
.. _u32-string-advance-retreat:
.. _u32-string-character-indexed-reading:
.. _u32-string-indexed-sequential-read:

Indexed Character Access
========================

The UTF-8 and UTF-16 read-only and editor types support direct character access by native data index and by
``unit::CpIndex``.
Native data-index access reads from the given byte or UTF-16 data position.
``unit::CpIndex`` access is a convenience for small offsets and may be slow for large strings, because the
implementation must iterate from the start to find the requested character position.

All indexed character access follows the ``charAt`` signal behavior: the exact end position returns
``Char::endOfData()``, invalid or past-end positions return ``Char::noCodePoint()``, and malformed encoded data is
decoded as ``Char::replacement()``.

Indexed Sequential Reads
========================

``readCharAndAdvance(index)`` reads the character at a native data index and advances the index to the position after
the decoded character.
At the exact end position, it returns ``Char::endOfData()`` and leaves the index unchanged.
For ``noIndex`` or a past-end index, it returns ``Char::noCodePoint()`` and leaves the index unchanged.

Malformed encoding is returned as ``Char::replacement()`` and advances according to the tolerant decoding rules.
Call ``isValidUtf8()`` before the read loop when a UTF-8-only parser must reject malformed internal text.

``readCharAndRetreat(index)`` treats the index as the position after the character to read.
It reads the previous character and retreats the index to that character's start.
This works with an index initialized from ``indexAt(StringSide::Back)`` to read backwards from the end of a string.
At zero, it returns ``Char::endOfData()`` and leaves the index unchanged.
For ``noIndex`` or a past-end index, it returns ``Char::noCodePoint()`` and leaves the index unchanged.
Unlike ``retreat(index)``, this method does not clamp a past-end index to the end before reading.

Character-Indexed Slices
========================

The UTF-8, UTF-16 and UTF-32 read-only and editor types support direct slicing by ``unit::CpRange`` and by
``StringSide`` with ``unit::CpLength``.
For UTF-8 and UTF-16, character-indexed slices return ranges aligned to decoded code-point boundaries, while the native
``ByteRange`` and ``U16DataRange`` overloads remain available for raw data-unit slices.
Trailing character slices are found from the back of the native data, so requesting the last few code points does not
require counting the entire string first.
Zero-length, invalid, or out-of-bounds ranges return empty strings.
Side-based slices with zero length return an empty string, while infinite length returns the entire string.

Display Width
=============

``displayWidth()`` returns the approximate display width of a string by summing the decoded
:cpp:class:`Char <erbsland::text::Char>` display widths.
Unicode control characters, including line breaks, contribute ``0``.

This is intentionally a simple per-code-point measurement.
It does not perform line layout, grapheme-cluster shaping, bidirectional reordering, emoji ZWJ sequence handling, or
terminal/font-specific corrections.
For text containing line breaks, the result is usually not the width of any rendered line.

Sensitive UTF-8 Storage
=======================

``U8String`` and ``U8StringEditor`` can mark their shared allocation with ``markAsSensitive()``.
The mark is one-way and is visible to every alias of the same allocation.
Copies, slices, trims, and same-string modified results preserve it, while inserting marked text into an ordinary
destination does not change that destination.
Conversions to another string width, encoded data, standard-library strings, escaped text, formatted text, and
diagnostics produce ordinary unmarked results.

Marking a non-empty literal first materializes shared storage.
Storage-less empty strings remain unmarked.
Marked allocations are securely erased when replaced or finally released.
This facility is best-effort storage hygiene rather than a high-security container or information-flow policy.

Searching
=========

All string ``find...`` overloads that accept a start or end position treat a no-index position as invalid input and
return the matching ``noIndex()`` value immediately.

Boolean Conversion
==================

Every read-only and editor string width provides ``toBoolean(defaultValue)`` and ``toBooleanOrThrow()``.
Both recognize the complete ASCII-case-insensitive ELCL literals ``true``, ``on``, ``yes``, ``enabled``, ``false``,
``off``, ``no``, and ``disabled``.
Empty input, surrounding whitespace, partial matches, and all other text are invalid.
``toBoolean()`` returns its supplied default for invalid text, while ``toBooleanOrThrow()`` raises
:cpp:class:`ParseError <erbsland::err::ParseError>`.

Interface
=========

.. doxygenclass:: erbsland::text::U16String
    :members:
.. doxygenclass:: erbsland::text::U16StringEditor
    :members:
.. doxygenclass:: erbsland::text::U32String
    :members:
.. doxygenclass:: erbsland::text::U32StringEditor
    :members:
.. doxygenclass:: erbsland::text::U8String
    :members:
.. doxygenclass:: erbsland::text::U8StringEditor
    :members:
