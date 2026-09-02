..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Mathematics; Reference
    single: Integer Math
    single: Saturating Math
    single: Integer Tools
    single: Big Integer
    single: Arbitrary-Precision Integer
    single: Math
    single: Integer Rotation
    single: Byte Order
    single: Endianness

*********************************
Mathematical Types and Operations
*********************************

Integer Math
============

Introduction
------------

Integer Conversion
~~~~~~~~~~~~~~~~~~

The ``integer_conversion`` module provides utility functions for converting between integer operand types and their
native integer representations.

.. _integer_conversion_to_native_integer:

toNativeInteger
^^^^^^^^^^^^^^^

``toNativeInteger`` converts an integer operand to its native integer value.
For :cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>` types, it unwraps the value via ``toRawValue()``.
Native integers pass through unchanged.

.. _integer_conversion_to_saturating_integer:

toSaturatingInteger
^^^^^^^^^^^^^^^^^^^

:cpp:func:`toSaturatingInteger <erbsland::math::toSaturatingInteger>` converts an integer operand to a
:cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>`.
:cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>` values pass through unchanged, while native integers
are wrapped.

Unsigned Absolute Values
~~~~~~~~~~~~~~~~~~~~~~~~

:cpp:func:`toUnsignedAbsolute <erbsland::math::toUnsignedAbsolute>` converts a native integer into an unsigned value of
the same width.
It exists because ``std::abs()`` cannot represent the absolute value of the minimum signed integer in the same signed
type.
An unsigned integer of the same width can represent that magnitude, and unsigned input values are returned unchanged.

Integer Range
~~~~~~~~~~~~~

:cpp:class:`IntegerRange <erbsland::math::IntegerRange>` is a compact inclusive range for native integer values.
It safely compares mixed signed and unsigned integer operands for containment checks and can clamp incoming values
without relying on unsafe casts.

Signed Magnitude
~~~~~~~~~~~~~~~~

:cpp:class:`SignedMagnitude <erbsland::math::SignedMagnitude>` represents a same-width integer value as a sign and an
unsigned magnitude.
This is useful when you write constexpr integer algorithms that must handle values such as the signed minimum value
without ever evaluating an overflowing signed expression.

The type is deliberately small.
It is not meant to replace normal integers in user-facing data models.
Use it when an algorithm temporarily needs a wider mathematical view of a native integer domain, especially when signed
and unsigned inputs meet.

Creating Values
^^^^^^^^^^^^^^^

Use :cpp:func:`SignedMagnitude::fromValue <erbsland::math::SignedMagnitude::fromValue>` when you start from a native
integer or from a :cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>`.
The source operand's native integer must have the same byte width as the template argument.

.. code-block:: cpp

    using Magnitude = el::SignedMagnitude<std::int32_t>;

    constexpr auto negative = Magnitude::fromValue(std::int32_t{-5});
    constexpr auto unsignedValue = Magnitude::fromValue(std::uint32_t{0x8000'0000U});
    constexpr auto saturatedValue = Magnitude::fromValue(el::SatInt32{-5});

Use :cpp:func:`SignedMagnitude::fromSignAndMagnitude <erbsland::math::SignedMagnitude::fromSignAndMagnitude>` when the
sign and absolute magnitude are already available.
Zero is always normalized to a positive value, so there is only one representation for zero.

Bounded Conversion
^^^^^^^^^^^^^^^^^^

Use :cpp:func:`SignedMagnitude::toSaturatingValue <erbsland::math::SignedMagnitude::toSaturatingValue>` to convert back
to the native result type.
The explicit bounds are applied before the native conversion, which keeps signed-minimum magnitudes and unsigned values
above a signed maximum safe in constant expressions.

.. code-block:: cpp

    using Magnitude = el::SignedMagnitude<std::int32_t>;

    constexpr auto value = Magnitude::fromValue(std::uint32_t{0x8000'0000U});
    constexpr auto clamped = value.toSaturatingValue(
        std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max()); // 2147483647

Use :cpp:func:`SignedMagnitude::wouldSaturate <erbsland::math::SignedMagnitude::wouldSaturate>` when you need to know if
the conversion would clamp instead of returning an exact native result.

Bounded Addition
^^^^^^^^^^^^^^^^

Use :cpp:func:`SignedMagnitude::saturatingAddBounded <erbsland::math::SignedMagnitude::saturatingAddBounded>` for
addition in the sign/magnitude domain.
The method is the shared implementation behind the constexpr bounded addition and subtraction helpers.

.. code-block:: cpp

    using Magnitude = el::SignedMagnitude<std::uint8_t>;

    constexpr auto index = Magnitude::fromValue(std::uint8_t{5});
    constexpr auto movement = Magnitude::fromValue(std::int8_t{-8});
    constexpr auto result = index.saturatingAddBounded(movement, std::uint8_t{0}, std::uint8_t{254}); // 0

Use :cpp:func:`SignedMagnitude::wouldAddBoundedSaturate <erbsland::math::SignedMagnitude::wouldAddBoundedSaturate>` to
distinguish exact sums from clamped sums.

Bounded Multiplication, Division and Modulo
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use :cpp:func:`SignedMagnitude::saturatingMultiplyBounded <erbsland::math::SignedMagnitude::saturatingMultiplyBounded>`,
``SignedMagnitude::saturatingDivideBounded``, and ``SignedMagnitude::saturatingModuloBounded`` for the remaining bounded
arithmetic operations in the sign/magnitude domain.
These methods are the shared implementation behind the constexpr bounded multiplication, division, and modulo helpers.

.. code-block:: cpp

    using Magnitude = el::SignedMagnitude<std::int32_t>;

    constexpr auto minimum = std::numeric_limits<std::int32_t>::min();
    constexpr auto maximum = std::numeric_limits<std::int32_t>::max();
    constexpr auto value = Magnitude::fromValue(minimum);
    constexpr auto divided = value.saturatingDivideBounded(Magnitude::fromValue(std::int32_t{-1}), minimum, maximum);
    // divided == maximum

Use the matching ``would...Saturate`` methods to distinguish exact results from clamped results.
Division and modulo by zero terminate.

Saturating Math
===============

Introduction
------------

Saturating Integer
~~~~~~~~~~~~~~~~~~

:cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>` is a small integer wrapper for calculations where
overflow must never wrap around silently.
When an operation exceeds the native integer range, the value is clamped to the nearest representable limit.
For example, adding ``20`` to ``SatInt8{120}`` produces ``127`` instead of wrapping to a negative number.

This is useful for counters, sizes, terminal layout calculations, progress values, and other places where a clipped
result is safer and easier to reason about than undefined behavior or modulo-style overflow.

Available Types
^^^^^^^^^^^^^^^

The library provides signed and unsigned aliases for the fixed-width integer sizes:

-   :cpp:type:`SatInt8 <erbsland::math::SatInt8>`, :cpp:type:`SatInt16 <erbsland::math::SatInt16>`,
    :cpp:type:`SatInt32 <erbsland::math::SatInt32>`, and :cpp:type:`SatInt64 <erbsland::math::SatInt64>`.
-   :cpp:type:`SatUInt8 <erbsland::math::SatUInt8>`, :cpp:type:`SatUInt16 <erbsland::math::SatUInt16>`,
    :cpp:type:`SatUInt32 <erbsland::math::SatUInt32>`, and :cpp:type:`SatUInt64 <erbsland::math::SatUInt64>`.

Basic Usage
^^^^^^^^^^^

Use operators when both operands have compatible signedness and you want the result type to grow to the larger operand
size.

.. code-block:: cpp

    #include <erbsland/math/SaturatingInteger.hpp>

    auto small = el::SatInt8{100};
    auto larger = el::SatInt16{40};
    auto result = small + larger; // el::SatInt16{140}

Use the named arithmetic methods when the result shall keep the current type, or when mixed signedness is intentional.
The value-returning methods are :cpp:func:`added <erbsland::math::SaturatingInteger::added>`,
:cpp:func:`subtracted <erbsland::math::SaturatingInteger::subtracted>`,
:cpp:func:`absoluteDifference <erbsland::math::SaturatingInteger::absoluteDifference>`,
:cpp:func:`multiplied <erbsland::math::SaturatingInteger::multiplied>`,
:cpp:func:`divided <erbsland::math::SaturatingInteger::divided>`, and
:cpp:func:`modulo <erbsland::math::SaturatingInteger::modulo>`.

.. code-block:: cpp

    auto value = el::SatInt8{120};
    auto clipped = value.added(20); // el::SatInt8{127}

    auto mixed = math::SatUInt8{10}.subtracted(el::SatInt8{20}); // el::SatUInt8{0}

Use the mutating methods ``add``,
:cpp:func:`subtract <erbsland::math::SaturatingInteger::subtract>`,
:cpp:func:`multiply <erbsland::math::SaturatingInteger::multiply>`,
:cpp:func:`divide <erbsland::math::SaturatingInteger::divide>`, and
:cpp:func:`applyModulo <erbsland::math::SaturatingInteger::applyModulo>` when the operation should update the existing
object.

.. code-block:: cpp

    auto column = el::SatUInt16{250};
    column.add(20);         // 270
    column.applyModulo(80); // 30

Conversions and Helpers
^^^^^^^^^^^^^^^^^^^^^^^

:cpp:func:`toRawValue <erbsland::math::SaturatingInteger::toRawValue>` returns the wrapped integer.
:cpp:func:`cast <erbsland::math::SaturatingInteger::cast>` converts to another saturating integer type, and
:cpp:func:`castOrThrow <erbsland::math::SaturatingInteger::castOrThrow>` converts without clipping or throws
:cpp:class:`OverflowError <erbsland::err::OverflowError>`.
:cpp:func:`toSizeT <erbsland::math::SaturatingInteger::toSizeT>` converts to ``std::size_t`` with saturation.
:cpp:func:`toAbsolute <erbsland::math::SaturatingInteger::toAbsolute>` keeps the current type, while
:cpp:func:`toUnsignedAbsolute <erbsland::math::SaturatingInteger::toUnsignedAbsolute>` returns the matching unsigned
type so the minimum signed value can be represented exactly.

.. code-block:: cpp

    auto negative = el::SatInt8{-128};

    auto sameType = negative.toAbsolute();         // el::SatInt8{127}
    auto unsignedType = negative.toUnsignedAbsolute(); // el::SatUInt8{128}
    auto asSize = negative.toSizeT();              // std::size_t{0}

Use :cpp:func:`range <erbsland::math::SaturatingInteger::range>`,
:cpp:func:`clamp <erbsland::math::SaturatingInteger::clamp>`, and
:cpp:func:`clamped <erbsland::math::SaturatingInteger::clamped>` for inclusive range handling.
:cpp:func:`wrap <erbsland::math::SaturatingInteger::wrap>` and
:cpp:func:`wrapped <erbsland::math::SaturatingInteger::wrapped>` apply modulo-style wrapping into a range.
The count-returning variants :cpp:func:`wrapAndCount <erbsland::math::SaturatingInteger::wrapAndCount>` and
:cpp:func:`wrappedAndCount <erbsland::math::SaturatingInteger::wrappedAndCount>` additionally report how often the
value crossed the range boundary.
Values below the range return a negative count, values above the range return a positive count, and values already in
range return zero.
Reversed direct bounds and ranges outside the native value range produce zero.

.. code-block:: cpp

    auto minute = el::SatInt16{75}.wrapped(0, 59); // el::SatInt16{15}

    auto seconds = el::SatInt64{-1};
    auto days = seconds.wrapAndCount(0, 86'399); // seconds == 86'399, days == -1

The predicates :cpp:func:`isZero <erbsland::math::SaturatingInteger::isZero>`,
:cpp:func:`isOne <erbsland::math::SaturatingInteger::isOne>`,
:cpp:func:`isNegative <erbsland::math::SaturatingInteger::isNegative>`,
:cpp:func:`isMinimum <erbsland::math::SaturatingInteger::isMinimum>`, and
:cpp:func:`isMaximum <erbsland::math::SaturatingInteger::isMaximum>` make boundary checks readable. Use
:cpp:func:`wouldAddSaturate <erbsland::math::SaturatingInteger::wouldAddSaturate>`,
:cpp:func:`wouldSubtractSaturate <erbsland::math::SaturatingInteger::wouldSubtractSaturate>`,
:cpp:func:`wouldMultiplySaturate <erbsland::math::SaturatingInteger::wouldMultiplySaturate>`,
:cpp:func:`wouldDivideSaturate <erbsland::math::SaturatingInteger::wouldDivideSaturate>`, and
:cpp:func:`wouldModuloSaturate <erbsland::math::SaturatingInteger::wouldModuloSaturate>` when you need to know if an
operation would clip before you apply it.

.. code-block:: cpp

    auto width = el::SatUInt8{250};

    if (width.wouldAddSaturate(20)) {
        width = el::SatUInt8::maximum();
    }

Static Construction
^^^^^^^^^^^^^^^^^^^

The static :cpp:func:`fromAddition <erbsland::math::SaturatingInteger::fromAddition>`,
:cpp:func:`fromSubtraction <erbsland::math::SaturatingInteger::fromSubtraction>`,
:cpp:func:`fromMultiplication <erbsland::math::SaturatingInteger::fromMultiplication>`,
:cpp:func:`fromDivision <erbsland::math::SaturatingInteger::fromDivision>`, and
:cpp:func:`fromModulo <erbsland::math::SaturatingInteger::fromModulo>` methods create a value directly from an
operation.
They are useful when the target type should be explicit at the call site.

.. code-block:: cpp

    auto sum = el::SatInt8::fromAddition(120, 20);       // el::SatInt8{127}
    auto product = el::SatUInt8::fromMultiplication(20U, 20U); // el::SatUInt8{255}

For division algorithms,
:cpp:func:`fromDivisionWithRemainder <erbsland::math::SaturatingInteger::fromDivisionWithRemainder>` returns quotient
first and remainder second.
The matching mutating helpers are
:cpp:func:`divideGetRemainder <erbsland::math::SaturatingInteger::divideGetRemainder>` and
:cpp:func:`divideKeepRemainder <erbsland::math::SaturatingInteger::divideKeepRemainder>`.

.. code-block:: cpp

    auto [quotient, remainder] = el::SatInt8::fromDivisionWithRemainder(127, 10);
    // quotient == 12, remainder == 7

Saturating Integer Types
~~~~~~~~~~~~~~~~~~~~~~~~

The ``SaturatingIntegerTypes`` header provides type traits and concepts for identifying
:cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>` types.

Type Trait
^^^^^^^^^^

``IsSaturatingInteger<T>``
    A type trait that is ``std::true_type`` when ``T`` is a
    ``SaturatingInteger<NativeInteger>``, and ``std::false_type`` otherwise.

Concept
^^^^^^^

``SaturatingIntegerType<T>``
    A concept that is satisfied when ``T`` is a :cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>` type.
    This is a convenience wrapper around :cpp:struct:`IsSaturatingInteger <erbsland::math::IsSaturatingInteger>`.

Saturating Math Functions
~~~~~~~~~~~~~~~~~~~~~~~~~

The saturating math functions are low-level integer helpers for calculations where overflow must be clipped instead of
wrapped.
They operate on native integer types and return native integer values.
If you want a value type with operators and named member functions, use
:cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>` instead.

For arithmetic operations, saturation means that a result beyond the target type range is clamped to
``std::numeric_limits<T>::min()`` or ``std::numeric_limits<T>::max()``.
For casts, values below the target range become the target minimum, and values above the target range become the target
maximum.

Arithmetic
^^^^^^^^^^

Use :cpp:func:`saturatingAdd <erbsland::math::saturatingAdd>`,
:cpp:func:`saturatingSubtract <erbsland::math::saturatingSubtract>`,
:cpp:func:`saturatingMultiply <erbsland::math::saturatingMultiply>`,
:cpp:func:`saturatingDivide <erbsland::math::saturatingDivide>`, and
:cpp:func:`saturatingModulo <erbsland::math::saturatingModulo>` for arithmetic that keeps the result in the range of
the first operand type.

.. code-block:: cpp

    #include <erbsland/math/SaturatingMath.hpp>

    auto a = std::int8_t{120};
    auto b = std::int8_t{20};
    auto sum = el::saturatingAdd(a, b); // std::int8_t{127}

    auto count = std::uint8_t{5};
    auto remaining = el::saturatingSubtract(count, 10); // std::uint8_t{0}

.. _saturating-math-mixed-types:

Mixed Types
^^^^^^^^^^^

The mixed-type overloads allow the second operand to use another compatible integer type.
They still keep the first operand type as the result type.
This makes the target range explicit at the call site.

.. code-block:: cpp

    auto width = std::uint8_t{250};
    auto next = el::saturatingAdd(width, std::uint16_t{20}); // std::uint8_t{255}

    auto offset = std::int8_t{-100};
    auto scaled = el::saturatingMultiply(offset, std::int16_t{3}); // std::int8_t{-128}

Division and Modulo
^^^^^^^^^^^^^^^^^^^

:cpp:func:`saturatingDivide <erbsland::math::saturatingDivide>` clips the signed minimum divided by ``-1`` to the
signed maximum, because the mathematical result cannot be represented in the same signed type.
:cpp:func:`saturatingModulo <erbsland::math::saturatingModulo>` returns ``0`` for the matching ``minimum % -1`` edge
case.

.. code-block:: cpp

    auto quotient = el::saturatingDivide(std::int8_t{-128}, std::int8_t{-1}); // std::int8_t{127}
    auto remainder = el::saturatingModulo(std::int8_t{-128}, std::int8_t{-1}); // std::int8_t{0}

Division and modulo by zero are programming errors.
These functions are ``noexcept`` and call ``std::terminate()`` for a zero divisor.

.. _saturating-math-casts:

Saturating Casts
^^^^^^^^^^^^^^^^

Use :cpp:func:`saturatingCast <erbsland::math::saturatingCast>` to convert between integer types without wrapping.
Use :cpp:func:`willCastOverflow <erbsland::math::willCastOverflow>` when you need to know whether the conversion would
change the value by clipping it.
For example, converting the unsigned 16-bit value ``0x2000`` into an unsigned 8-bit value produces ``0xff``.
Converting a negative signed value such as ``-10`` into an unsigned type produces the unsigned target minimum, which is
zero.

.. code-block:: cpp

    auto small = el::saturatingCast<std::uint8_t>(500); // std::uint8_t{255}
    auto none = el::saturatingCast<std::uint8_t>(-5);   // std::uint8_t{0}

    if (el::willCastOverflow<std::uint8_t>(300)) {
        // The cast would be clipped to 255.
    }

Overflow Prediction
^^^^^^^^^^^^^^^^^^^

Use :cpp:func:`willAddOverflow <erbsland::math::willAddOverflow>`,
:cpp:func:`willSubtractOverflow <erbsland::math::willSubtractOverflow>`,
:cpp:func:`willMultiplyOverflow <erbsland::math::willMultiplyOverflow>`,
:cpp:func:`willDivideOverflow <erbsland::math::willDivideOverflow>`, and
:cpp:func:`willModuloOverflow <erbsland::math::willModuloOverflow>` to test whether an operation would need
saturation before you perform it.

.. code-block:: cpp

    auto value = std::int8_t{120};

    if (el::willAddOverflow(value, 20)) {
        value = std::numeric_limits<std::int8_t>::max();
    } else {
        value += 20;
    }

Increment and Decrement
^^^^^^^^^^^^^^^^^^^^^^^

:cpp:func:`saturatingIncrement <erbsland::math::saturatingIncrement>` and
:cpp:func:`saturatingDecrement <erbsland::math::saturatingDecrement>` update an integer in place and stop at the type
limits.

.. code-block:: cpp

    auto cursor = std::uint8_t{255};
    math::saturatingIncrement(cursor); // still 255

    auto signedValue = std::int8_t{-128};
    math::saturatingDecrement(signedValue); // still -128

Constexpr Saturating Math
~~~~~~~~~~~~~~~~~~~~~~~~~

The constexpr saturating math helpers are small native-integer tools for code that must work in constant expressions.
Use them when you need saturating arithmetic inside value types, templates, or other APIs that promise ``constexpr``
behavior.

Unlike :cpp:func:`Saturating Math <erbsland::math::saturatingAdd>`, these helpers do not use compiler overflow
intrinsics.
They are intentionally limited to same-width operands and explicit result bounds.
This makes the edge cases easy to reason about while still supporting signed and unsigned combinations.

Internally, these helpers use :cpp:class:`SignedMagnitude <erbsland::math::SignedMagnitude>` to avoid signed overflow in
constant expressions.
Use that type directly only when you are building similar low-level integer algorithms.

Bounded Arithmetic
^^^^^^^^^^^^^^^^^^

Use :cpp:func:`saturatingAddBounded <erbsland::math::saturatingAddBounded>` and
:cpp:func:`saturatingSubtractBounded <erbsland::math::saturatingSubtractBounded>` when the result must be clamped to an
explicit domain range instead of the natural type range.

.. code-block:: cpp

    #include <erbsland/math/ConstexprSaturatingMath.hpp>

    constexpr auto index = std::uint8_t{250};
    constexpr auto moved = el::saturatingAddBounded(
        index, std::int8_t{10}, std::uint8_t{0}, std::uint8_t{254}); // 254

    constexpr auto offset = el::saturatingSubtractBounded(
        std::uint32_t{0},
        std::uint32_t{0x8000'0000U},
        std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max()); // -2147483648

Use :cpp:func:`willAddBoundedSaturate <erbsland::math::willAddBoundedSaturate>` and
:cpp:func:`willSubtractBoundedSaturate <erbsland::math::willSubtractBoundedSaturate>` when you need to distinguish an
exact result from a clamped one.

Negation and Steps
^^^^^^^^^^^^^^^^^^

Use :cpp:func:`saturatingNegateBounded <erbsland::math::saturatingNegateBounded>` for signed negation that must also
work for the minimum signed value and for unsigned input values.

Use :cpp:func:`saturatingIncrementBounded <erbsland::math::saturatingIncrementBounded>` and
:cpp:func:`saturatingDecrementBounded <erbsland::math::saturatingDecrementBounded>` for one-step movement inside custom
bounds.

Multiplication, Division and Modulo
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use ``saturatingMultiplyBounded``, ``saturatingDivideBounded``, and ``saturatingModuloBounded`` for same-width scalar
arithmetic inside custom bounds.
Division and modulo by zero terminate, matching the non-constexpr saturating math helpers.

Use the matching ``will...Saturate`` helpers when you need to distinguish exact results from clamped results.

Arbitrary-Precision Integers
============================

Introduction
------------

:cpp:class:`BigUnsignedInteger <erbsland::math::BigUnsignedInteger>` and
:cpp:class:`BigInteger <erbsland::math::BigInteger>` represent integers whose size is limited only by available memory.
They are useful when calculations can exceed native integer limits and saturating arithmetic would lose information.

The API deliberately covers the common integer operations only.
It has no bitwise operations, mixed native/big-integer operators, powers, or number-theory helpers.
Construct native operands explicitly before combining them with a big integer.

Construction and Text
---------------------

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
----------

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
-----------------

``cast<T>()`` converts to a native integer and clamps values outside the target range.
``castOrThrow<T>()`` instead requires an exact representation and throws
:cpp:class:`OverflowError <erbsland::err::OverflowError>` when the value is outside the target range.
Negative values clamp to zero when casting to an unsigned native type.

Integer Bit and Byte-Order Operations
=====================================

The integer bit operations provide compiler-safe rotations and byte-order conversion for unsigned native integers.
All operations are ``constexpr`` and independent of host byte order, alignment, and aliasing.
They use fixed-extent byte spans so the required number of bytes is part of the function signature.

Interface
=========

.. doxygenclass:: erbsland::math::BigInteger
    :members:
.. doxygenclass:: erbsland::math::BigUnsignedInteger
    :members:
.. doxygenclass:: erbsland::math::BoundedInteger
    :members:
.. doxygenfunction:: erbsland::math::saturatingAddBounded(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult

.. doxygenfunction:: erbsland::math::willAddBoundedSaturate(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool

.. doxygenfunction:: erbsland::math::saturatingSubtractBounded(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult

.. doxygenfunction:: erbsland::math::willSubtractBoundedSaturate(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool

.. doxygenfunction:: erbsland::math::saturatingNegateBounded(tValue value, tResult minimum, tResult maximum) noexcept -> tResult

.. doxygenfunction:: erbsland::math::willNegateBoundedSaturate(tValue value, tResult minimum, tResult maximum) noexcept -> bool

.. doxygenfunction:: erbsland::math::saturatingIncrementBounded(tValue value, tResult minimum, tResult maximum) noexcept -> tResult

.. doxygenfunction:: erbsland::math::saturatingDecrementBounded(tValue value, tResult minimum, tResult maximum) noexcept -> tResult

.. doxygenfunction:: erbsland::math::saturatingMultiplyBounded(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult

.. doxygenfunction:: erbsland::math::willMultiplyBoundedSaturate(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool

.. doxygenfunction:: erbsland::math::saturatingDivideBounded(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult

.. doxygenfunction:: erbsland::math::willDivideBoundedSaturate(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool

.. doxygenfunction:: erbsland::math::saturatingModuloBounded(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult

.. doxygenfunction:: erbsland::math::willModuloBoundedSaturate(tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool
.. doxygenconcept:: erbsland::math::UnsignedNativeInteger

.. doxygenfunction:: erbsland::math::rotateLeft(const T value, const int amount) noexcept -> T

.. doxygenfunction:: erbsland::math::rotateRight(const T value, const int amount) noexcept -> T

.. doxygenfunction:: erbsland::math::loadBigEndian(const std::span<const std::byte, sizeof(T)> bytes) noexcept -> T

.. doxygenfunction:: erbsland::math::loadLittleEndian(const std::span<const std::byte, sizeof(T)> bytes) noexcept -> T

.. doxygenfunction:: erbsland::math::storeBigEndian(const T value, const std::span<std::byte, sizeof(T)> bytes) noexcept

.. doxygenfunction:: erbsland::math::storeLittleEndian(const T value, const std::span<std::byte, sizeof(T)> bytes) noexcept
.. doxygenfunction:: erbsland::math::toNativeInteger(T value) noexcept -> NativeIntegerOfT<T>

.. doxygenfunction:: erbsland::math::toSaturatingInteger(T value) noexcept -> SaturatingInteger<NativeIntegerOfT<T>>
.. doxygenfunction:: erbsland::math::isNegativeValue(T value) noexcept -> bool

.. doxygenfunction:: erbsland::math::toUnsignedAbsolute(T value) noexcept -> std::make_unsigned_t<T>

.. doxygenfunction:: erbsland::math::mixedIntegerCompare(tFirst first, tSecond second) noexcept -> std::strong_ordering

.. doxygenfunction:: erbsland::math::integerAbsoluteDifference(tFirst first, tSecond second) noexcept -> std::make_unsigned_t<CompatibleNativeIntegerT<tFirst, tSecond>>

.. doxygenfunction:: erbsland::math::toIntegerNormal(T value) noexcept -> T

.. doxygenfunction:: erbsland::math::orderMinimumMaximum(T &minimum, T &maximum) noexcept(noexcept(std::swap(minimum, maximum)))
.. doxygenclass:: erbsland::math::IntegerRange
    :members:
.. doxygenclass:: erbsland::math::SaturatingInteger
    :members:

.. doxygentypedef:: erbsland::math::SatInt8

.. doxygentypedef:: erbsland::math::SatInt16

.. doxygentypedef:: erbsland::math::SatInt32

.. doxygentypedef:: erbsland::math::SatInt64

.. doxygentypedef:: erbsland::math::SatUInt8

.. doxygentypedef:: erbsland::math::SatUInt16

.. doxygentypedef:: erbsland::math::SatUInt32

.. doxygentypedef:: erbsland::math::SatUInt64
.. doxygenstruct:: erbsland::math::IsSaturatingInteger
    :members:
.. doxygenfunction:: erbsland::math::saturatingAdd(T first, T second) noexcept -> T

.. doxygenfunction:: erbsland::math::saturatingSubtract(T first, T second) noexcept -> T

.. doxygenfunction:: erbsland::math::saturatingMultiply(T first, T second) noexcept -> T

.. doxygenfunction:: erbsland::math::saturatingDivide(T first, T second) noexcept -> T

.. doxygenfunction:: erbsland::math::saturatingModulo(T first, T second) noexcept -> T

.. doxygenfunction:: erbsland::math::willAddOverflow(T first, T second) noexcept -> bool

.. doxygenfunction:: erbsland::math::willSubtractOverflow(T first, T second) noexcept -> bool

.. doxygenfunction:: erbsland::math::willMultiplyOverflow(T first, T second) noexcept -> bool

.. doxygenfunction:: erbsland::math::willDivideOverflow(T first, T second) noexcept -> bool

.. doxygenfunction:: erbsland::math::willModuloOverflow(T first, T second) noexcept -> bool

.. doxygenfunction:: erbsland::math::saturatingCast(tSourceType value) noexcept -> tTargetType

.. doxygenfunction:: erbsland::math::willCastOverflow(tSourceType value) noexcept -> bool

.. doxygenfunction:: erbsland::math::saturatingIncrement(T &value) noexcept

.. doxygenfunction:: erbsland::math::saturatingDecrement(T &value) noexcept

.. doxygenfunction:: erbsland::math::saturatingAdd(tFirst first, tSecond second) noexcept -> tFirst

.. doxygenfunction:: erbsland::math::saturatingSubtract(tFirst first, tSecond second) noexcept -> tFirst

.. doxygenfunction:: erbsland::math::saturatingMultiply(tFirst first, tSecond second) noexcept -> tFirst

.. doxygenfunction:: erbsland::math::saturatingDivide(tFirst first, tSecond second) noexcept -> tFirst

.. doxygenfunction:: erbsland::math::saturatingModulo(tFirst first, tSecond second) noexcept -> tFirst

.. doxygenfunction:: erbsland::math::willAddOverflow(tFirst first, tSecond second) noexcept -> bool

.. doxygenfunction:: erbsland::math::willSubtractOverflow(tFirst first, tSecond second) noexcept -> bool

.. doxygenfunction:: erbsland::math::willMultiplyOverflow(tFirst first, tSecond second) noexcept -> bool

.. doxygenfunction:: erbsland::math::willDivideOverflow(tFirst first, tSecond second) noexcept -> bool

.. doxygenfunction:: erbsland::math::willModuloOverflow(tFirst first, tSecond second) noexcept -> bool
.. doxygenclass:: erbsland::math::SignedMagnitude
    :members:
