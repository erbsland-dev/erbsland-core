***********************************
Configuration Domain API Guidelines
***********************************

These guidelines extend the Common, Memory, Regular Expression, Text, and Time API Guidelines for public APIs in the
``conf`` namespace.

Scalar Type Mapping
===================

Configuration APIs use Core scalar types directly.
Do not add configuration-specific wrappers or compatibility aliases for these values.

.. code-block:: text

    text -> text::String
    bytes -> mem::ByteBlock
    date -> time::Date
    time without suffix -> time::Time
    time with Z or numeric offset -> time::TimeWithZone
    date-time -> time::DateTime
    time delta -> time::CalendarDelta
    regular expression -> re::RegExPtr

The ``ValueType`` and validation ``RuleType`` names describe ELCL value categories and remain configuration-domain
enumerators.

Access Patterns
===============

.. code-block:: text

    o.as❮Type❯() -> CoreType // scalar value or the Core type's documented fallback
    o.as❮Type❯OrThrow() -> CoreType // scalar value or conf::ConfError(TypeMismatch)
    o.get❮Type❯(path[, default]) -> CoreType // child value or caller-provided/default fallback
    o.get❮Type❯OrThrow(path) -> CoreType // child value or conf::ConfError
    builder.add❮Type❯(path, value) -> void // add a Core scalar value

The regular-expression fallback is ``nullptr`` and builders reject null pointers where a real expression is required.
Compiled expressions are immutable and remain shared when configuration values are copied.

ELCL Time Semantics
===================

Standalone times without a suffix are floating ``time::Time`` values.
Times with ``Z`` or a numeric offset use ``time::TimeWithZone``.
Both representations have the logical ``ValueType::Time`` type.
Accessing either representation as ``Time`` removes its zone; accessing it as ``TimeWithZone`` adds
``TimeZone::local()`` when needed.
Date-times without a suffix resolve ``TimeZone::local()`` for the complete civil date and time.
All date-times are stored as UTC-backed ``time::DateTime`` values.
Each time-delta literal stores its signed amount as one independent ``time::CalendarDelta`` component from nanoseconds
through years.
Configuration lists are not implicitly combined into calendar deltas.
