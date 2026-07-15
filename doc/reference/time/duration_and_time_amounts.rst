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

Interface
=========

.. doxygenclass:: erbsland::time::Duration
    :members:
.. doxygenenum:: erbsland::time::DurationPart
.. doxygenclass:: erbsland::time::ElapsedTimer
    :members:
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

.. doxygenfunction:: erbsland::time::literals::operator_ns(const int64_t value) -> Nanoseconds

.. doxygenfunction:: erbsland::time::literals::operator_us(const int64_t value) -> Microseconds

.. doxygenfunction:: erbsland::time::literals::operator_ms(const int64_t value) -> Milliseconds

.. doxygenfunction:: erbsland::time::literals::operator_s(const int64_t value) -> Seconds

.. doxygenfunction:: erbsland::time::literals::operator_m(const int64_t value) -> Minutes

.. doxygenfunction:: erbsland::time::literals::operator_h(const int64_t value) -> Hours
.. doxygenclass:: erbsland::time::TimeDelta
    :members:
.. doxygenclass:: erbsland::time::TimePoint
    :members:
.. doxygenstruct:: erbsland::time::SecondsUnitTag
    :members:

.. doxygenstruct:: erbsland::time::MonthsUnitTag
    :members:

.. doxygenstruct:: erbsland::time::YearsUnitTag
    :members:
