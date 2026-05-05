**************************
Math Domain API Guidelines
**************************

These guidelines extend the Common API Guidelines for public APIs that model mathematical values, native integer
helpers, or arithmetic operations.

The purpose of this document is to define a base naming vocabulary for math APIs.
It is intentionally plain, technical and list based to get a quick overview of method names and its usage patterns.
If you introduce new vocabulary, update this page to provide a good reference for future extensions.

Core Semantics
==============

Main Vocabulary
---------------

"saturating" / "saturate"
    Use when an out-of-range result is intentionally clamped to the represented type range or to an explicit
    domain range.

"bounded"
    Use when an operation uses explicit minimum and maximum parameters instead of the natural limits of the
    represented type.

"overflow"
    Use for native integer operations when the mathematical result cannot be represented by the native result type.

"minimum" / "maximum"
    Use for the smallest/largest possible or valid value of a type or explicit domain.

"absolute"
    Use for the mathematical non-negative magnitude of a value. When the value may not fit the same signed type,
    use an unsigned-result pattern.

"remainder"
    Use for the remainder part of division, especially when quotient and remainder are returned together.

Operation Vocabulary
--------------------

Use only these operation stems for the common arithmetic operations.

addition
    Add, add, added, Addition

subtraction
    Subtract, subtract, subtracted, Subtraction

multiplication
    Multiply, multiply, multiplied, Multiplication

division
    Divide, divide, divided, Division

modulo
    Modulo, applyModulo, modulo, Modulo

negation
    Negate, negate, negated, Negation

increment
    Increment, increment, incremented

decrement
    Decrement, decrement, decremented

power
    RaiseTo, raiseTo, raisedTo

negate
    Negate, negated, negated

Primary Types
=============

.. code-block:: text

    BoundedInteger // a template to create integers that are safely bound to a given integer range
    IntegerRange // inclusive range for native integer values
    SaturatingInteger // integer with all operations safely saturating.
    SatInt8, SatInt16, SatInt32, SatInt64 // Signed saturated integer (aliases of SaturatingInteger)
    SatUInt8, SatUInt16, SatUInt32, SatUInt64 // Unsigned saturated integers (aliases of SaturatingInteger)
    SignedMagnitude // represents a safe sign and a unsigned magnitude from any signed value.

Helper Methods and Concepts
===========================

.. code-block:: text

    // AnyIntegerTypes.hpp:
    AnyIntegerType // concept to test for any supported integer type, including saturated ones
    AnyIntegerPair // convenience to test for a pair of supported integers

    // ConstexprSaturatingMath.hpp:
    saturating❮Stem❯Bounded(a, b, min, max) -> bool // constexpr bounded operation
    will❮Stem❯Saturate(a, b, min, max) -> bool // test if the operation will saturate

    // IntegerConversion.hpp:
    toNativeInteger(v) -> U // converts any integer type into a native integer type
    toSaturatingInteger(v) -> U // converts any integer into a saturating integer type

    // IntegerMath.hpp:
    mixedIntegerCompare(a, b) -> strong_ordering // safely compare two native integer values
    integerAbsoluteDifference(a, b) -> U // safely get the absolute difference `abs(a - b)` as unsigned
    toUnsignedAbsolute(a) -> U // safely get the unsigned absolute value of `a`
    toIntegerNormal(a) -> v // get -1, 0 or 1 from 'a'
    orderMinimumMaximum(minimum, maximum) // order two range bounds in place

    // IntegerTraits.hpp:
    SameSignednessNativeIntegers<U, V> // Check if both native integer types have the same signedness
    WiderNativeInteger[T]<U, V> // Get the wider native integer of both
    SecondHasGreaterPositiveRange<U, V> // Check if the second type can represent a greater range of positive values
    CompatibleNativeInteger[T]<U, V> // Get a compatible native integer for an operation between two integer types
    NativeIntegerOf[T]<U> // Get the native integer type from an integer operand (removes saturating type)
    SignCompatibleIntegerOperand<V, T> // Check if a native integer and an operand have matching signedness
    SignCompatibleIntegerOperandPair<V, A, B> // Check if a native integer and two operands all have matching signedness

    // IntegerTypes.hpp:
    NativeInteger<T> // A native integer type supported by the math helpers
    NativeIntegerPair<U, V> // Check if both types are supported native integer types

    // SaturatingMath.hpp:
    saturating❮Stem❯(a, b) -> T // A safe, efficient saturating operation
    will❮Stem❯Overflow(a, b) -> bool // Test if the given operation will overflow.
    saturatingCast<T>(V) -> T // Safe cast from V to T, saturating the result.
    saturatingIncrement/Decrement(V&) // Safe saturating increment, saturating at the values min/max

Object API Patterns
===================

``SaturatingInteger``
---------------------

and also: SatInt8, SatInt16, SatInt32, SatInt64, SatUInt8, SatUInt16, SatUInt32, SatUInt64

.. code-block:: text

    states: Zero, One, MinusOne, Minimum, Maximum

    static auto T::❮state❯() -> T    // create that value (e.g. T::zero(), T::minimum())
    static auto T::fromValue(v) -> T  // convert from a native value
    static auto T::from❮Stem❯(a, b) -> T // store the result from a saturating operation
    static auto T::from❮Domain❯(...) -> T // store the result from a saturating operation
    o.is❮State❯() const -> bool   // test if the value is in that state (e.g `o.isZero()`/`o.isMinimum()`)
    o.would❮Stem❯Saturate(v) const -> bool // test if an operation would saturate
    o.would❮Stem❯BoundedSaturate(v, minimum, maximum) const -> bool // test if it would saturate in bounds.
    auto o.toRawValue() const -> R // native unprotected value.
    auto o.toSizeT() const -> std::size_t // saturating conversion to std::size_t
    auto o.cast<U>() const -> SaturatingInteger<U> // safe saturating cast to another saturating integer.
    auto o.castOrThrow<U>() const -> SaturatingInteger<U> // non-clipping cast, throws OverflowError if impossible.
    static auto T::range() -> IntegerRange<NativeValue> // range of all representable values.
    o.wrap(range/minimum, maximum) -> void // wrap into an inclusive range.
    o.wrapped(range/minimum, maximum) const -> T // return a wrapped copy.
    o.wrapAndCount(range/minimum, maximum) -> SatInt64 // wrap and return signed boundary crossings.
    o.wrappedAndCount(range/minimum, maximum) const -> tuple<T, SatInt64> // wrapped copy and boundary crossings.
    auto o.toSaturatingValue(minimum, maximum) const -> R // convert
    auto o.toAbsolute() const -> T  // same width
    auto o.toUnsignedAbsolute() const -> U // same width
