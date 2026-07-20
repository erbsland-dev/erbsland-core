.. index::
    single: Configuration Scalar Data

*************************
Configuration Scalar Data
*************************

Configuration scalar values use the corresponding Core types directly:

*   Text uses :cpp:type:`erbsland::text::String <erbsland::text::String>`.
*   Bytes use :cpp:class:`erbsland::mem::ByteBlock <erbsland::mem::ByteBlock>`.
*   Dates, times, date-times, and deltas use :cpp:class:`erbsland::time::Date <erbsland::time::Date>`,
    :cpp:class:`erbsland::time::Time <erbsland::time::Time>` or
    ``erbsland::time::TimeWithZone``,
    :cpp:class:`erbsland::time::DateTime <erbsland::time::DateTime>`, and
    :cpp:class:`erbsland::time::CalendarDelta <erbsland::time::CalendarDelta>`.
*   Regular expressions use the immutable shared pointer
    :cpp:type:`erbsland::re::RegExPtr <erbsland::re::RegExPtr>`.

Regular-expression literals are stored with lazy compilation.
Parsing retains their pattern, flags, and settings without invoking the regular-expression compiler.
The first matching operation compiles the expression and may throw
:cpp:class:`re::RegExError <erbsland::re::RegExError>` for an invalid pattern.
Call :cpp:func:`re::RegEx::compileNow() <erbsland::re::RegEx::compileNow>` when an application needs explicit validation
before using configuration values.

Standalone ELCL times without a suffix are stored as floating :cpp:class:`Time <erbsland::time::Time>` values.
Values with ``Z`` or a numeric offset are stored as ``TimeWithZone``.
Both use :cpp:enumerator:`ValueType::Time <erbsland::conf::ValueType::Time>`.
The ``asTime()`` family removes a zone, while the ``asTimeWithZone()`` family attaches ``TimeZone::local()`` to a
floating value.

Date-times without a suffix are interpreted in the system-local zone at the complete civil date and time.
This preserves historical shifts, daylight-saving state, gaps, and folds in the resulting UTC-backed ``DateTime``.
Every ELCL delta literal is stored as one independent ``CalendarDelta`` component, including month and year values.
Ordinary ELCL value lists remain lists and are not implicitly combined into one delta.

Integers and floating-point values use the :cpp:type:`erbsland::conf::Integer <erbsland::conf::Integer>` and
:cpp:type:`erbsland::conf::Float <erbsland::conf::Float>` aliases.
Validation rules use ``erbsland::text::CaseSensitivity`` to define their text matching mode.

Lists and matrices are structural value types rather than scalar classes.
Access them through the typed operations of :cpp:class:`erbsland::conf::Value <erbsland::conf::Value>`.

Interface
=========

.. doxygentypedef:: erbsland::conf::Float
.. doxygentypedef:: erbsland::conf::Integer
.. doxygenclass:: erbsland::conf::Matrix
    :members:
