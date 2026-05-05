.. index::
    single: Integer Math

************
Integer Math
************

Introduction
============

Integer Conversion
------------------

The ``integer_conversion`` module provides utility functions for converting between integer operand types and their
native integer representations.

.. _integer_conversion_to_native_integer:

toNativeInteger
~~~~~~~~~~~~~~~

``toNativeInteger`` converts an integer operand to its native integer value.
For :cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>` types, it unwraps the value via ``toRawValue()``.
Native integers pass through unchanged.

.. _integer_conversion_to_saturating_integer:

toSaturatingInteger
~~~~~~~~~~~~~~~~~~~

:cpp:func:`toSaturatingInteger <erbsland::math::toSaturatingInteger>` converts an integer operand to a
:cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>`.
:cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>` values pass through unchanged, while native integers
are wrapped.

Unsigned Absolute Values
------------------------

:cpp:func:`toUnsignedAbsolute <erbsland::math::toUnsignedAbsolute>` converts a native integer into an unsigned value of
the same width.
It exists because ``std::abs()`` cannot represent the absolute value of the minimum signed integer in the same signed
type.
An unsigned integer of the same width can represent that magnitude, and unsigned input values are returned unchanged.

Integer Range
-------------

:cpp:class:`IntegerRange <erbsland::math::IntegerRange>` is a compact inclusive range for native integer values.
It safely compares mixed signed and unsigned integer operands for containment checks and can clamp incoming values
without relying on unsafe casts.

Signed Magnitude
----------------

:cpp:class:`SignedMagnitude <erbsland::math::SignedMagnitude>` represents a same-width integer value as a sign and an
unsigned magnitude.
This is useful when you write constexpr integer algorithms that must handle values such as the signed minimum value
without ever evaluating an overflowing signed expression.

The type is deliberately small.
It is not meant to replace normal integers in user-facing data models.
Use it when an algorithm temporarily needs a wider mathematical view of a native integer domain, especially when signed
and unsigned inputs meet.

Creating Values
~~~~~~~~~~~~~~~

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
~~~~~~~~~~~~~~~~~~

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
~~~~~~~~~~~~~~~~

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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

Interface
=========

.. doxygenclass:: erbsland::math::BoundedInteger
    :members:
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
.. doxygenclass:: erbsland::math::SignedMagnitude
    :members:
