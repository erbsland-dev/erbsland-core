.. index::
    single: String Width Variants

*********************
String Width Variants
*********************

.. _u8-string-storage-management:
.. _u8-string-view-byte-based-reading:
.. _u8-string-view-advance-retreat:
.. _u8-string-view-character-indexed-reading:
.. _u8-string-char-view-character-based-reading:
.. _u16-string-storage-management:
.. _u16-string-view-code-unit-based-reading:
.. _u16-string-view-advance-retreat:
.. _u16-string-view-character-indexed-reading:
.. _u16-string-char-view-character-based-reading:
.. _u32-string-storage-management:
.. _u32-string-view-code-unit-based-reading:
.. _u32-string-view-advance-retreat:
.. _u32-string-view-character-indexed-reading:

Indexed Character Access
========================

The UTF-8 and UTF-16 string and view types support direct character access by native data index and by
``unit::CpIndex``.
Native data-index access reads from the given byte or UTF-16 data position.
``unit::CpIndex`` access is a convenience for small offsets and may be slow for large strings, because the
implementation must iterate from the start to find the requested character position.

All indexed character access follows the ``charAt`` signal behavior: the exact end position returns
``Char::endOfData()``, invalid or past-end positions return ``Char::noCodePoint()``, and malformed encoded data is
decoded as ``Char::replacement()``.

Character-Indexed Slices
========================

The UTF-8, UTF-16 and UTF-32 string and view types support direct slicing by ``unit::CpRange`` and by ``StringSide``
with ``unit::CpLength``.
For UTF-8 and UTF-16, character-indexed slices return ranges aligned to decoded code-point boundaries, while the native
``ByteRange`` and ``U16DataRange`` overloads remain available for raw data-unit slices.
Trailing character slices are found from the back of the native data, so requesting the last few code points does not
require counting the entire string first.

Interface
=========

.. doxygenclass:: erbsland::text::U16String
    :members:
.. doxygenclass:: erbsland::text::U16StringCharView
    :members:
.. doxygenclass:: erbsland::text::U16StringView
    :members:
.. doxygenclass:: erbsland::text::U32String
    :members:
.. doxygenclass:: erbsland::text::U32StringView
    :members:
.. doxygenclass:: erbsland::text::U8String
    :members:
.. doxygenclass:: erbsland::text::U8StringCharView
    :members:
.. doxygenclass:: erbsland::text::U8StringView
    :members:
