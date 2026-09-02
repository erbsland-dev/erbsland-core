************************************
Unit and Value Domain API Guidelines
************************************

Core Semantics
==============

.. code-block:: text

    index = zero-based position with an optional no-index state
    length or count = non-negative span with an optional infinite state
    offset = signed movement relative to an index
    range = half-open start index plus length
    integer amount = signed ratio-scaled value with explicit compatible-ratio conversion

Primary Types
=============

.. code-block:: text

    IntegerUnitIndex❮Unit❯ // zero-based index with a no-index state
    IntegerUnitAmount❮Unit❯ // non-negative length or count with an infinite state
    IntegerUnitOffset❮Unit❯ // signed movement or index difference
    IntegerUnitRange❮Unit❯ // half-open index and length composition
    IntegerAmount❮UnitRatio❯ // signed ratio-scaled amount

Concrete Unit Types
===================

.. code-block:: text

    ByteIndex, ByteLength, ByteOffset, ByteRange // UTF-8, memory, and stream byte units
    U16DataIndex, U16DataLength, U16DataOffset, U16DataRange // UTF-16 storage units
    CpIndex, CpLength, CpOffset, CpRange // Unicode code-point units
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
    ByteUnit, U16DataUnit, CpUnit, ItemUnit, ArgumentUnit // storage and collection unit tags
    LineUnit, ColumnUnit // diagnostic source-position unit tags
    VersionPart // selected version comparison precision
    Major, Minor, Revision, BuildNumber // strongly typed version parts
    VersionUnit, MajorUnit, MinorUnit, RevisionUnit, BuildNumberUnit // version unit tags

Pattern Definitions
===================

.. code-block:: text

    I = ❮Unit❯Index // index in a unit family
    L = ❮Unit❯Length/❮Unit❯Count // non-negative span in a unit family
    O = ❮Unit❯Offset // signed movement in a unit family
    R = ❮NativeInteger❯ // underlying native integer

Common Value Patterns
=====================

.. code-block:: text

    T(value) // explicitly create from a raw value
    o.toRawValue() -> R // cross the explicit native-integer boundary
    o.isZero/isOne/isMinimum/isMaximum() -> bool // test common value states
    T::zero/one/minimum/maximum() -> T // create common value states
    o.toSizeT() -> std::size_t // convert non-negative values to a native size

Index Patterns
==============

.. code-block:: text

    o.isNoIndex()/isValid()/isWithin(length) -> bool // test index states and membership
    o.advance/retreat(length) -> T& // move by a non-negative length
    o.advanced/retreated(length) -> T // return an index moved by a non-negative length
    o.move(offset) -> T& // move in place by a signed offset
    o.moved(offset) -> T // return a value moved by a signed offset
    o.increment/decrement() -> T& // move one position
    o.distanceFromZero/absoluteDistanceTo(index) -> L // calculate non-directional distance
    o.offsetFromZero/offsetTo(index) -> O // calculate signed distance
    T::noIndex()/end(length) -> T // create the absent state or first excluded index

Length and Offset Patterns
==========================

.. code-block:: text

    o.isInfinite()/isFinite() -> bool // test amount special states
    o.add/subtract(value) -> T& // combine values in place with saturation
    o.added/subtracted(value) -> T // return a combined value
    o.multiply/divide/applyModulo(integer) -> T& // scale or reduce in place
    o.multiplied/divided/modulo(integer) -> T // return a scaled or reduced value
    o.wouldAddSaturate/wouldSubtractSaturate(value) -> bool // test arithmetic saturation
    o.absoluteLength() -> L // convert an offset magnitude to a matching length
    o.negated() -> O // reverse an offset direction
    T::infinite()/minusOne() -> T // create family-specific special values

Range Patterns
==============

.. code-block:: text

    T(index, length) // create a half-open range
    T::fromBeginEnd(begin, end) -> T // create from included and excluded boundaries
    o.index()/length()/endIndex() -> T // inspect range components
    o.contains(index)/isWithin(length) -> bool // test membership or complete fit
    o.isEmpty()/isInfinite()/isValid() -> bool // test range states
    o.forEach(function) -> util::LoopResult // visit each represented index
    o.advance/retreat(length) -> T& // move by a non-negative length
    o.move(offset) -> T& // move a range in place
    o.moved(offset) -> T // return a moved range
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
    o.toNumber() -> uint64_t // pack all version parts
    T::fromNumber(number) -> T // unpack all version parts
    T::all/atLeast/atMost/between/exact([versions]) -> VersionRange // create a version constraint
    o.contains(version[, precision]) -> bool // test a version constraint
