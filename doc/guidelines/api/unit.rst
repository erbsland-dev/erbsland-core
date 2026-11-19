************************************
Unit and Value Domain API Guidelines
************************************

Core Semantics
==============

.. code-block:: text

    index = zero-based position with an no-index state
    length or count = non-negative span with an infinite state
    offset = signed movement relative to an index
    range = half-open start index plus length
    integer amount = signed ratio-scaled value with explicit compatible-ratio conversion

Primary Types
=============

.. code-block:: text

    IntegerUnitIndex❮Unit❯ // zero-based index with a no-index state
    IntegerUnitCount❮Unit❯ // non-negative length or count with an infinite state
    IntegerUnitOffset❮Unit❯ // signed movement or index difference
    IntegerUnitRange❮Unit❯ // half-open index and length composition
    IntegerAmount❮UnitRatio❯ // signed ratio-scaled amount

Concrete Unit Types
===================

.. code-block:: text

    ByteIndex, ByteCount, ByteOffset, ByteRange // UTF-8, memory, and stream byte units
    U16Index, U16Count, U16Offset, U16Range // unsigned 16-bit value units
    CpIndex, CpCount, CpOffset, CpRange // Unicode code-point units
    ItemIndex, ItemCount, ItemOffset, ItemRange // generic container units
    ArgumentIndex, ArgumentCount // command-line and format argument units
    LineIndex, LineCount, LineOffset, LineRange // diagnostic source-line units
    ColumnIndex, ColumnCount, ColumnOffset, ColumnRange // diagnostic source-column units

Value Types
===========

.. code-block:: text

    CodeLocation, CodeContinuousRange // source location and continuous source range
    Version, VersionRange // version value and constraint
    ExitCode // process exit status

Supporting Types
================

.. code-block:: text

    IntegerUnit // base tag for index, amount, offset, and range families
    ByteUnit, U16Unit, CpUnit, ItemUnit, ArgumentUnit // storage and collection unit tags
    LineUnit, ColumnUnit // diagnostic source-position unit tags
    VersionPart // selected version comparison precision
    Major, Minor, Revision, BuildNumber // strongly typed version parts
    VersionComponent❮Part❯ // independent unsigned 16-bit version component

Pattern Definitions
===================

.. code-block:: text

    I = ❮Unit❯Index // index in a unit family
    L = ❮Unit❯Count // non-negative span in a unit family
    O = ❮Unit❯Offset // signed movement in a unit family
    R = ❮NativeInteger❯ // underlying native integer

Common Value Patterns
=====================

.. code-block:: text

    T(constant) // checked compile-time integer construction; no numeric sentinels
    T(value, T::Unchecked{}) // direct raw construction after proving representability
    T::fromInteger(value)/fromIntegerOrThrow(value) -> T // saturating or exact runtime construction
    o.toRawValue() -> R // cross the explicit native-integer boundary
    o.isZero/isOne/isMinimum/isMaximum() -> bool // test common value states
    T::zero/one/minimum/maximum() -> T // create common value states
    o.toSizeT() -> std::size_t // convert non-negative values to a native size

Index, Count, Offset, and Amount Step Patterns
==============================================

.. code-block:: text

    o.increment/decrement() -> T& // step in place with saturation, also ++ and --
    o.incremented/decremented() -> T // return a value stepped by one with saturation

Index Patterns
==============

.. code-block:: text

    o.isNoIndex()/isValid()/isWithin(length) -> bool // test index states and membership
    o.advance/retreat(length) -> T& // move by a non-negative length
    o.advanced/retreated(length) -> T // return an index moved by a non-negative length
    o.move(offset) -> T& // move in place by a signed offset
    o.moved(offset) -> T // return a value moved by a signed offset
    o.uncheckedAdvance/uncheckedRetreat(length) -> T& // exact movement with caller-proven bounds
    o.uncheckedMove(offset) -> T& // exact movement with caller-proven bounds
    o.uncheckedIncrement/uncheckedDecrement() -> T& // exact step with caller-proven bounds
    o.uncheckedAdvanced/uncheckedRetreated/uncheckedMoved(step) -> T // matching copy variants
    o.uncheckedIncremented/uncheckedDecremented() -> T // matching copy variants
    o.distanceFromZero/absoluteDistanceTo(index) -> L // calculate non-directional distance
    o.offsetFromZero/offsetTo(index) -> O // calculate signed distance
    o.wouldAdvanceSaturate/wouldRetreatSaturate(length) -> bool // exact movement failure
    o.wouldMoveSaturate(offset) -> bool // exact movement failure
    o.canAdvance/canRetreat(step, bounds) -> bool // exact movement ending inside bounds
    o.canMove(offset, bounds) -> bool // exact movement ending inside bounds
    o.mirroredWithin(length) -> I // mirror a valid position within a finite length
    T::noIndex()/end(length) -> T // create the absent state or first excluded index
    T::last(length) -> T // final included index; noIndex for a zero or infinite length

Length and Offset Patterns
==========================

.. code-block:: text

    o.isInfinite()/isFinite() -> bool // test count finiteness
    o.remainingFrom(index) -> L // amount from an index to this bound; zero for absent or out-of-bounds indexes
    o.add/subtract(value) -> T& // combine values in place with saturation
    o.added/subtracted(value) -> T // return a combined value
    o.multiply/divide/applyModulo(integer) -> T& // scale or reduce in place
    o.multiplied/divided/modulo(integer) -> T // return a scaled or reduced value
    o.uncheckedAdd/uncheckedSubtract(length) -> T& // exact arithmetic with caller-proven finite bounds
    o.uncheckedAdded/uncheckedSubtracted(length) -> T // matching copy variants
    o.uncheckedIncrement/uncheckedDecrement() -> T& // exact step with caller-proven finite bounds
    o.uncheckedIncremented/uncheckedDecremented() -> T // matching copy variants
    o.forEach(function) -> util::LoopResult // visit indexes below a finite length
    o.absoluteLength() -> L // convert an offset magnitude to a matching length
    o.negate() -> void // reverse an offset direction in place
    o.negated() -> O // reverse an offset direction
    T::infinite()/minusOne() -> T // create family-specific special values

Range Patterns
==============

.. code-block:: text

    T(index, length) // create a half-open range
    T(begin, end) // compact slicing syntax; reversed finite endpoints produce emptyAt(begin)
    T::fromBegin(begin) -> T // preserve the beginning directly and set infinite length
    T::fromBeginEnd(begin, end)/fromBeginEndOrThrow(begin, end) -> T // permissive or validated endpoints
    o.beginIndex()/length()/endIndex() -> T // inspect range components
    o.lastIndex() -> I // final absolute index; noIndex for an empty, infinite, or invalid range
    o.contains(index)/isWithin(length) -> bool // test membership or complete fit
    o.isEmpty()/isInfinite()/isFinite()/isNoRange()/isValid() -> bool // test range states
    o.toAbsolute(origin)/relativeTo(origin) -> T // exact coordinate conversion or noRange
    o.intersection(range) -> T // common range, or empty at the later beginning
    o.remainingFrom(index) -> T // suffix clamped to this range; noRange for invalid input
    o.contains(range) -> bool // complete containment, including empty endpoints
    o.forEach(function) -> util::LoopResult // visit each represented index
    o.advance/retreat(length) -> T& // translate in place by a length, also += and -=
    o.advanced/retreated(length) -> T // return a range translated by a length, also + and -
    o.move(offset) -> T& // translate in place by an offset, also += and -=
    o.moved(offset) -> T // return a range translated by an offset, also + and -
    o.wouldAdvanceSaturate/wouldRetreatSaturate(length) -> bool // exact range translation failure
    o.wouldMoveSaturate(offset) -> bool // exact range translation failure
    o.clampTo(bounds) -> T& // clip the range to a sequence in place
    o.clampedTo(bounds) -> T // return the clipped range
    T::empty/all/noRange/emptyAt(index) -> T // create common range states

Ratio Amount Patterns
=====================

.. code-block:: text

    T(value) // create an amount at its declared ratio
    T(otherAmount) // explicitly convert a compatible unit and ratio
    o.to❮Ratio❯()/to❮Ratio❯OrThrow() -> T // convert with saturation or exact failure
    o.add/subtract(amount) -> T& // combine compatible amounts in place
    o.added/subtracted(amount) -> T // return a compatible amount result

Version Patterns
================

.. code-block:: text

    T(major[, minor, revision, build]) // create a version from strongly typed parts
    o.major()/minor()/revision()/build() -> T // access version parts
    o.compare(version[, precision]) -> std::strong_ordering // compare through a selected part
    T::fromString(text[, requiredPrecision]) -> std::optional❮T❯ // parse a complete dotted decimal version
    T::fromStringOrThrow(text[, requiredPrecision]) -> T // parse or throw ParseError
    o.toString([precision]) -> String // format the selected version parts
    o.toNumber() -> uint64_t // pack all version parts
    T::fromNumber(number) -> T // unpack all version parts
    T::all/atLeast/atMost/between/exact([versions]) -> VersionRange // create a version constraint
    o.contains(version[, precision]) -> bool // test a version constraint
