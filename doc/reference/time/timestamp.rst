.. index::
    single: Timestamp
    single: Time; UTC timestamps

**************
UTC Timestamps
**************

:cpp:class:`Timestamp <erbsland::time::Timestamp>` is a UTC instant with nanosecond precision in the calendar range
``0000-01-01..9999-12-31``, without display-zone metadata.
Default construction is invalid; invalid values compare equal and sort before valid values.
Fixed arithmetic saturates at the calendar bounds, with checked variants and overflow predicates.
Distance methods offer typed resolutions and checked invalid/overflow handling.
See :doc:`../../topics/time/working_with_timestamps` for construction, comparison, text and binary exchange, epoch
conversion, and arithmetic examples.

Serialization Contract
======================

``RawValue`` is ``std::pair<int32_t, int64_t>``: days since the Core epoch and nanoseconds since UTC midnight.
Valid fields lie in ``0..3652424`` and ``0..86399999999999``.
Any negative field decodes to the canonical invalid state ``(-1, 0)``; oversized nonnegative fields fail validation.
The binary encoding is exactly twelve big-endian bytes: signed 32-bit days followed by signed 64-bit nanoseconds.
It excludes object padding; the invalid encoding is ``ffffffff0000000000000000``.
Bytewise order is chronological for valid encodings, but the invalid encoding sorts after them, unlike object
comparison.

Canonical ISO output is ``YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ``.
Scalar epoch ticks support signed seconds, milliseconds, microseconds, and nanoseconds, truncating toward zero.
Split seconds/fractions use floor seconds and a nonnegative nanosecond fraction.
Every valid timestamp converts exactly to UTC ``DateTime``; converting back discards display metadata.

Interface
=========

.. doxygenclass:: erbsland::time::Timestamp
    :members:
