************************************
Unit and Value Domain API Guidelines
************************************

These guidelines extend the Common API Guidelines for public APIs in the ``unit`` namespace.
This namespace provides unit based types for indexes, lengths, ranges, counts and offsets.
If you introduce new vocabulary or types, update this page.

Core Semantics
==============

Vocabulary
----------

Index
    A zero-based position that points to one element in a sequence.
    An index may have a special "no index" state.

Length
    A non-negative span that counts consecutive elements in a sequence. An amount has no direction.
    An amount may have a special "infinite" state.

Integer Amount
    A signed amount in a ratio-scaled unit. Use this for values such as seconds, milliseconds, meters, or kilometers
    where explicit conversion between compatible ratios is required.

Offset
    A signed movement relative to an index.
    Positive offsets move forward, negative offsets move backward.

Range
    A start index plus an amount.
    Ranges are half-open: they contain index and exclude the end-index.

Raw Value
    The underlying native integer.

Raw Value Access
----------------

Unit types share the following uniform pattern to access the underlying raw value.vocabulary

.. code-block:: text

    o.toRawValue() -> R
    
This name is intentionally explicit: it marks the point where code leaves the protected domain model and talks to
low-level APIs or storage formats.
We do not add implicit conversions to native integers.

Operators
---------

Operators are added when their meaning is safe and obvious at the call site.
Operators never throw, special named ``...OrThrow()`` variants exists of a caller needs these exceptions.

.. code-block:: text

    T +/- T -> T  // add/subtract the same type
    T +/-= T -> T&  // add/subtract in-place
    ++/--T -> T&  // increment/decrement
    T++/-- -> T  // increment/decrement

    -offset -> offset  // negation only for signed offsets
    index +/- length -> index  // adding length to index moves
    index +/-= length -> index&  // adding length to index moves
    index +/- offset -> index  // adding offset to index moves
    index +/-= offset -> index&  // adding offset to index moves
    range +/- offset -> range  // adding offset to range moves
    range +/-= offset -> range&  // adding offset to range moves
    length * unsigned-integer -> length  // scale a length with saturation
    length / unsigned-integer -> length  // divide a length
    length % unsigned-integer -> length  // length remainder
    length *= unsigned-integer -> length&  // in-place length scaling
    length /= unsigned-integer -> length&  // in-place length division
    length %= unsigned-integer -> length&  // in-place length remainder
    offset * signed-integer -> offset  // scale an offset with saturation
    offset / signed-integer -> offset  // divide an offset
    offset % signed-integer -> offset  // offset remainder
    offset *= signed-integer -> offset&  // in-place offset scaling
    offset /= signed-integer -> offset&  // in-place offset division
    offset %= signed-integer -> offset&  // in-place offset remainder

Integer scalar operands include native integers and saturating integers.
Length operands require unsigned scalars because a length is non-negative.
Offset operands require signed scalars because an offset has direction.
Unit values are never multiplied, divided or modulo-applied with other unit values.

Ranges
------

.. code-block:: text

    [begin, length]  // [1, 3] => 3 elements, first element at zero-based index 1
    [begin, end]  // [1, 3] => 2 elements, first element at zero-based index 1

Primary Types
=============

.. code-block:: text

    // for UTF-8 stings and byte blocks and streams
    ByteIndex  // byte index
    ByteLength  // byte amount
    ByteOffset  // byte offset
    ByteRange  // byte range
    ByteUnit  // the unit definition for the byte types

    // for UTF-16 strings
    U16DataIndex  // a char16_t index
    U16DataLength  // a char16_t length
    U16DataOffset  // a char16_t offset
    U16DataRange  // a char16_t range
    U16DataUnit  // the unit definition for the char16_t types

    // for all sting types:
    CpIndex  // code-point index
    CpLength  // code-point length
    CpOffset  // code-point offset
    CpRange  // code-point length
    CpUnit  // the unit definition for the code-point types

    // for containers:
    ElementIndex  // element index
    ElementCount  // element count
    ElementOffset  // element offset
    ElementRange  // element range
    ElementUnit  // the unit definition for the element types

    // for command line and format arguments:
    ArgumentIndex  // an argument index
    ArgumentCount  // an argument count
    ArgumentUnit  // the unit definition for the argument types

    // specialized types:
    ExitCode  // an exit code returned by processes
    Version  // a version consisting of a major, minor, revision and build part
    VersionPart  // enum to address a part of a version
    VersionRange  // a range of versions
    Major, Minor, Revision, BuildNumber // individual version parts
    VersionUnit  // generic base unit for all version parts
    MajorUnit, MinorUnit, RevisionUnit, BuildNumberUnit // part specific units

Secondary Types
===============

.. code-block:: text

    // generic templates to create new unit types:
    IntegerAmount<UnitTag, Ratio, Value>  // signed ratio-scaled amount with saturating value
    IntegerUnit  // base class defining raw times for a unit based integer
    IntegerUnitIndex<Tag, Value>  // zero based index type with optional "no index" state
    IntegerUnitAmount<Tag, Value>  // amount for lengths or counts with optional "infinite" state
    IntegerUnitOffset<Tag, Value>  // signed offset type for moving an index or index differences
    IntegerUnitRange<Tag, Value>  // a composition of an index with an amount (length)
    ElementUnit  // integer unit for generic element containers
    ElementIndex  // zero-based position in a list-like container
    ElementCount  // non-negative number of elements. for list sizes, capacities, and counts
    ElementRange  // half-open range of elements
    ElementOffset  // signed movement in element units

    // Version
    Version  // represents a version with major, minor, revision and build number
    Major, Minor, Revision, BuildNumber  // strong types version part
    MajorUnit, MinorUnit, RevisionUnit, BuildNumberUnit  // units for version parts derive from `VersionUnit`

Common Factory Patterns
=======================

.. code-block:: text

    T::zero() -> T  // the zero value
    T::one() -> T  // the value one when it is meaningful
    T::minimum() -> T  // the smallest valid value, excluding special states
    T::maximum() -> T  // the largest valid value, excluding special states

Index Patterns
==============

.. code-block:: text

    o.isZero(), o.isOne(), o.isMinimum(), o.isMaximum()  // value predicates
    o.isNoIndex()  // the special no-index state
    o.isValid()  // "not no-index"
    o.isWithin(length)  // testing if the index is a valid position within a length
    o.advance(length) -> T&  // moving forward by a non-negative length
    o.advanced(length) -> T  // moving forward by a non-negative length
    o.retreat(length) -> T&  // moving backward by a non-negative length
    o.retreated(length) -> T  // moving backward by a non-negative length
    o.move(offset) -> T&  // moving by a signed offset
    o.moved(offset) -> T  // moving by a signed offset
    o.increment(), o.incremented(), o.decrement(), o.decremented()  // single-step movement
    o.distanceFromZero() -> Length  // the distance from zero
    o.absoluteDistanceTo(index) -> Length  // non-directional distance between two indexes
    o.offsetFromZero() -> Offset  // signed offset from zero, saturated if necessary
    o.offsetTo(index) -> Offset  // signed offset to another index, saturated if necessary
    o.wouldOffsetToSaturate(index)  // testing if offsetTo(index) would return a saturated value
    o.toSizeT()  // conversion to std::size_t
    T::noIndex() -> T  // the special no-index state
    T::end(length) -> T  // converting a length measured from zero into the matching end index

Length Patterns
===============

.. code-block:: text

    o.isZero(), o.isOne(), o.isMinimum(), o.isMaximum()  // value predicates
    o.isInfinite(), o.isFinite()  // the special infinite state
    o.add(length) -> T&  // combining spans
    o.added(length) -> T  // combining spans
    o.subtract(length) -> T&  // reducing spans
    o.subtracted(length) -> T  // reducing spans
    o.multiply(unsigned-integer), o.multiplied(unsigned-integer)  // scaling a length
    o.divide(unsigned-integer), o.divided(unsigned-integer)  // dividing a length
    o.applyModulo(unsigned-integer), o.modulo(unsigned-integer)  // remainder in the same length unit
    o.wouldAddSaturate(length)  // testing if addition would clamp or become infinite
    o.wouldSubtractSaturate(length)  // testing if subtraction would clamp to zero
    o.toSizeT()  // conversion to std::size_t
    T::infinite() -> T  // the special infinite length

Offset Patterns
===============

.. code-block:: text

    o.isZero(), o.isOne(), o.isMinusOne(), o.isMinimum(), o.isMaximum()  // value predicates
    o.isNegative(), o.isPositive()  // sign predicates
    o.add(offset) -> T&  // combining movement
    o.added(offset) -> T  // combining movement
    o.subtract(offset) -> T&  // subtracting movement
    o.subtracted(offset) -> T  // subtracting movement
    o.multiply(signed-integer), o.multiplied(signed-integer)  // scaling an offset
    o.divide(signed-integer), o.divided(signed-integer)  // dividing an offset
    o.applyModulo(signed-integer), o.modulo(signed-integer)  // remainder in the same offset unit
    o.wouldAddSaturate(offset), o.wouldSubtractSaturate(offset)  // testing if arithmetic would clamp
    o.absoluteLength() -> Length  // converting magnitude into the matching length
    o.negated() -> T  // sign inversion
    T::minusOne() -> T  // the value negative one

Range Patterns
==============

.. code-block:: text

    o.index() -> Index  // the start index
    o.length() -> Length  // the number of elements
    o.endIndex() -> Index  // the first index after the range
    o.contains(index)  // membership
    o.isWithin(length)  // testing if the whole range fits into bounds
    o.isEmpty(), o.isInfinite(), o.isValid()  // state predicates
    o.advance(length), o.retreat(length), o.move(offset)  // moving the start index
    T::empty(), T::emptyAt(index), T::all(), T::noRange()  // common factories

Version Patterns
================

.. code-block:: text

    o.major(), o.minor(), o.revision(), o.build()  // part access
    o.compare(version, precision)  // precision-limited comparison
    o.inRange(range, precision)  // range membership
    o.toNumber()  // packed numeric representation
    T::fromNumber(number)  // unpacking numeric representation

Version Patterns
================

.. code-block:: text

    T::all()  // all versions
    T::atLeast/Most(version)  // minimum/maximum version constraint
    T::between(minimum, maximum)  // bounded version range
    T::exact(version)  // single version match
