**************************
Math Domain API Guidelines
**************************

Core Semantics
==============

Integer Arithmetic
------------------

.. code-block:: text

    saturating = clamp an out-of-range result to the represented domain
    bounded = use explicit minimum and maximum instead of the native type limits
    overflow = mathematical result outside the native result type
    inclusive range = minimum and maximum are both represented values
    absolute magnitude = unsigned non-negative magnitude safe for the signed minimum
    compatible result = native integer selected without unsafe signed or width promotion

Primary Types
=============

.. code-block:: text

    SaturatingInteger❮Native❯ // integer with saturating arithmetic and conversion
    BoundedInteger❮Native❯ // integer constrained to compile-time bounds

Secondary Types
===============

.. code-block:: text

    SatInt8, SatInt16, SatInt32, SatInt64 // signed SaturatingInteger aliases
    SatUInt8, SatUInt16, SatUInt32, SatUInt64 // unsigned SaturatingInteger aliases
    SignedMagnitude❮Native❯ // sign and safe unsigned magnitude of a signed value
    IntegerRange❮Native❯ // inclusive native-integer range
    AnyIntegerType, AnyIntegerPair // concepts for native and saturating integer operands
    NativeInteger❮type❯, NativeIntegerPair❮types❯ // supported native integer concepts
    CompatibleNativeInteger❮types❯, WiderNativeInteger❮types❯ // safe native result selections
    NativeIntegerOf❮type❯ // native representation of an integer operand
    UnsignedNativeInteger❮type❯ // unsigned native integer excluding bool
    SameSignednessNativeIntegers❮types❯ // native operands with matching signedness
    SignCompatibleIntegerOperand❮types❯ // native result and operand with matching signedness
    BigInteger // arbitrary-precision signed integer
    BigUnsignedInteger // arbitrary-precision unsigned integer

Pattern Definitions
===================

.. code-block:: text

    N = ❮NativeInteger❯ // native integer participating in an operation
    S = SaturatingInteger❮Native❯ // saturating integer result

Saturating Arithmetic Patterns
==============================

.. code-block:: text

    saturating❮Operation❯(a[, b]) -> N // apply an operation and clamp to native limits
    saturating❮Operation❯Bounded(a[, b], minimum, maximum) -> N // clamp to explicit bounds
    will❮Operation❯Overflow(a[, b]) -> bool // test overflow against native limits
    will❮Operation❯Saturate(a[, b], minimum, maximum) -> bool // test saturation against explicit bounds
    saturatingCast❮Target❯(value) -> N // convert and clamp to the target domain
    saturatingIncrement/saturatingDecrement(value) // mutate with saturation

Integer Utility Patterns
========================

.. code-block:: text

    toNativeInteger/toSaturatingInteger(value) -> T // convert between supported representations
    mixedIntegerCompare(a, b) -> std::strong_ordering // compare without unsafe promotion
    integerAbsoluteDifference(a, b) -> N // compute an unsigned absolute difference
    toUnsignedAbsolute(value) -> N // compute a safe unsigned magnitude
    toIntegerNormal(value) -> int // normalize to -1, 0, or 1
    orderMinimumMaximum(minimum, maximum) // order two range bounds in place
    rotateLeft/rotateRight(value, amount) -> N // perform modulo-width rotation
    load❮Endian❯❮N❯(bytes) -> N // load independently of alignment and host byte order
    store❮Endian❯(value, bytes) // store independently of alignment and host byte order

Big Integer Patterns
====================

.. code-block:: text

    T(N) // explicitly construct from a native integer
    o.divideGetRemainder(divisor) -> T // store quotient and return remainder
    o.cast❮N❯() -> N // convert to a native integer with clamping
    o.castOrThrow❮N❯() -> N // convert exactly or throw on overflow
    o.toString() -> String // create decimal text
    T::fromString(text) -> std::optional❮T❯ // parse complete decimal text
    T::fromStringOrThrow(text) -> T // parse decimal text or throw

Saturating Value Patterns
=========================

.. code-block:: text

    T(value) // construct with saturation from a supported integer
    T::zero/minimum/maximum() -> S // create a common boundary value
    T::from❮Operation❯(a[, b]) -> S // create from a saturating operation
    o.isZero/isMinimum/isMaximum() -> bool // test a common boundary state
    o.would❮Operation❯Saturate(value[, bounds]) -> bool // test saturation before mutation
    o.toRawValue() -> N // access the native representation
    o.cast/castOrThrow❮Target❯() -> S // convert with saturation or exact failure reporting
    o.wrap(range) // wrap in place
    o.wrapped(range) -> S // return a wrapped copy
    o.wrapAndCount/wrappedAndCount(range) -> T // wrap and report signed boundary crossings
    o.toAbsolute()/toUnsignedAbsolute() -> T // create a safe magnitude

Range and Bounded Value Patterns
================================

.. code-block:: text

    T(minimum, maximum) // create an inclusive range with ordered bounds
    o.minimum()/maximum() -> N // inspect range boundaries
    o.contains(value) -> bool // test inclusive membership
    o.clamp(value) -> N // clamp a value into the range
    o.toRawValue() -> N // access a bounded integer's native representation
