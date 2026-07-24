.. index::
    single: Duration and Time Amounts

*************************
Duration and Time Amounts
*************************

Introduction
============

Duration and time span types store signed time intervals at different resolutions.

:cpp:class:`Duration <erbsland::time::Duration>` stores a signed span with second resolution. Conversions to coarser
parts truncate toward zero.
Conversions to nanosecond precision, such as ``toTimeDelta()``, saturate if the represented nanoseconds exceed the
target type.
Use ``wouldConvertToTimeDeltaSaturate()`` or ``toTimeDeltaOrThrow()`` when saturation must be detected or rejected.

:cpp:class:`TimeDelta <erbsland::time::TimeDelta>` stores a signed span with nanosecond resolution. Conversion to
:cpp:class:`Duration <erbsland::time::Duration>` truncates sub-second nanoseconds toward zero.
``toSecondsWithFractions()`` and ``toDaysWithFractions()`` return approximate floating-point values and do not treat
rounding as an error.
The unit factories from ``nanoseconds()`` through ``weeks()`` saturate when conversion exceeds the stored nanosecond
range.
Use the corresponding ``...OrThrow()`` factory, including ``weeksOrThrow()``, when overflow must be rejected.

:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>` stores nanoseconds through years as independent signed
components.
It deliberately does not normalize its stored parts: one month remains one month, and mixed positive and negative
components remain visible through the typed accessors.
Conversion to ``TimeDelta`` is available only when the month and year components are zero and the exact fixed-unit sum
fits the nanosecond range.

Applying a ``CalendarDelta`` to a ``DateTime`` processes nanoseconds, microseconds, milliseconds, seconds, minutes,
hours, days, weeks, months, and years in that order.
Month and year steps use the same end-of-month clamping semantics as ``Date``.
Arithmetic is performed on the UTC representation; fixed display offsets are retained and named-zone metadata is
refreshed for the final instant.

:cpp:class:`TimeDeltaFormat <erbsland::time::TimeDeltaFormat>` controls short or long names, separators, the smallest
fixed unit, and fractional output.
``TimeDeltaFormat::elcl()`` selects the aliases and separators required for ELCL serialization.
Calendar-delta formatting always emits non-zero years and months independently and uses exact signed normalization for
the fixed units without first forcing the total into ``TimeDelta``.

Interface
=========

.. doxygenclass:: erbsland::time::CalendarDelta
    :members:
.. doxygenclass:: erbsland::time::Duration
    :members:
.. doxygenenum:: erbsland::time::DurationPart
.. doxygenclass:: erbsland::time::ElapsedTimer
    :members:
.. doxygenfunction:: erbsland::time::literals::operator""_ns(const unsigned long long value) -> Nanoseconds

.. doxygenfunction:: erbsland::time::literals::operator""_us(const unsigned long long value) -> Microseconds

.. doxygenfunction:: erbsland::time::literals::operator""_ms(const unsigned long long value) -> Milliseconds

.. doxygenfunction:: erbsland::time::literals::operator""_s(const unsigned long long value) -> Seconds

.. doxygenfunction:: erbsland::time::literals::operator""_m(const unsigned long long value) -> Minutes

.. doxygenfunction:: erbsland::time::literals::operator""_h(const unsigned long long value) -> Hours
.. doxygentypedef:: erbsland::time::Nanoseconds

.. doxygentypedef:: erbsland::time::Microseconds

.. doxygentypedef:: erbsland::time::Milliseconds

.. doxygentypedef:: erbsland::time::Seconds

.. doxygentypedef:: erbsland::time::Minutes

.. doxygentypedef:: erbsland::time::Hours

.. doxygentypedef:: erbsland::time::Days

.. doxygentypedef:: erbsland::time::Weeks

.. doxygentypedef:: erbsland::time::Months

.. doxygentypedef:: erbsland::time::Years
.. doxygenclass:: erbsland::time::TimeDelta
    :members:
.. doxygenclass:: erbsland::time::TimeDeltaFormat
    :members:
.. doxygenenum:: erbsland::time::TimeDeltaUnit
.. doxygenclass:: erbsland::time::TimePoint
    :members:
.. doxygenstruct:: erbsland::time::SecondsUnitTag
    :members:

.. doxygenstruct:: erbsland::time::MonthsUnitTag
    :members:

.. doxygenstruct:: erbsland::time::YearsUnitTag
    :members:
