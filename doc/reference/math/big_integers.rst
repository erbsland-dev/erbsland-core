.. index::
    single: Big Integer
    single: Arbitrary-Precision Integer

****************************
Arbitrary-Precision Integers
****************************

Introduction
============

:cpp:class:`BigUnsignedInteger <erbsland::math::BigUnsignedInteger>` and
:cpp:class:`BigInteger <erbsland::math::BigInteger>` represent integers whose size is limited only by available memory.
They are useful when calculations can exceed native integer limits and saturating arithmetic would lose information.

The API deliberately covers the common integer operations only.
It has no bitwise operations, mixed native/big-integer operators, powers, or number-theory helpers.
Construct native operands explicitly before combining them with a big integer.

Construction and Text
=====================

Both types default to zero and accept native integers through explicit constructors.
Constructing a
:cpp:class:`BigUnsignedInteger <erbsland::math::BigUnsignedInteger>` from a negative native value throws an
:cpp:class:`OverflowError <erbsland::err::OverflowError>`.

``toString()`` creates canonical decimal text.
``fromString()`` parses a complete decimal value and returns an empty optional for invalid input, while
``fromStringOrThrow()`` reports invalid text with
:cpp:class:`ParseError <erbsland::err::ParseError>`. Parsing accepts an optional leading plus sign;
:cpp:class:`BigInteger <erbsland::math::BigInteger>` also accepts a minus sign. Whitespace, digit separators, and base
prefixes are not accepted.

Arithmetic
==========

Both types support comparison, addition, subtraction, multiplication, division, modulo, and the corresponding compound
assignments.
Operands must have the same big-integer type.
Arithmetic is exact, except that subtracting a larger value from
:cpp:class:`BigUnsignedInteger <erbsland::math::BigUnsignedInteger>` throws
:cpp:class:`OverflowError <erbsland::err::OverflowError>`.

Division truncates toward zero.
A :cpp:class:`BigInteger <erbsland::math::BigInteger>` remainder has the dividend's sign.
``divideGetRemainder()`` performs quotient and remainder calculation together: it stores the quotient in the object and
returns the remainder.
As with native and saturating integer arithmetic, division or modulo by zero terminates the process.

Native Conversion
=================

``cast<T>()`` converts to a native integer and clamps values outside the target range.
``castOrThrow<T>()`` instead requires an exact representation and throws
:cpp:class:`OverflowError <erbsland::err::OverflowError>` when the value is outside the target range.
Negative values clamp to zero when casting to an unsigned native type.

Interface
=========

.. doxygenclass:: erbsland::math::BigInteger
    :members:
.. doxygenclass:: erbsland::math::BigUnsignedInteger
    :members:
