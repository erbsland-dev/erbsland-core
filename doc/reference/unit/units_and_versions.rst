..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Units; Reference
    single: Versions; Reference
    single: Integer Indexes, Amounts, Offsets and Ranges
    single: Version

******************
Units and Versions
******************

Integer Indexes, Amounts, Offsets and Ranges
============================================

Representation and Construction
-------------------------------

Include each concrete unit type through its own public header, such as ``<erbsland/ArgumentIndex.hpp>`` or
``<erbsland/ArgumentCount.hpp>``.
``ArgumentUnit.hpp`` declares only the shared argument-unit tag.

Positional unit tags identify the coordinate system without configuring storage.
Indexes and amounts use unsigned 64-bit values; offsets use signed 64-bit values.
The largest unsigned value is reserved for ``noIndex()`` or ``infinite()``.
``maximum()`` is always the largest finite value.

Numeric constructors are ``consteval``: ``ByteIndex{12}`` and ``ByteIndex(12)`` both validate their input at compile
time.
Native integer types other than ``bool`` are accepted, but floating-point inputs, negative indexes or amounts, and
numeric sentinel values are rejected.
Negative offsets are valid, including the signed minimum.
Default construction produces zero; copying typed values remains available at runtime.

For runtime integers, ``fromInteger(value)`` saturates into the finite domain and ``fromIntegerOrThrow(value)`` throws
``OverflowError`` outside that domain.
Use ``noIndex()`` and ``infinite()`` explicitly to create special states.
``fromSizeT()`` and ``fromSizeTOrThrow()`` follow the same rules.
Extraction through ``toSizeTOrThrow()`` checks representability only: it does not reject a sentinel that fits.

Movement and Distances
----------------------

Move indexes by matching amounts with ``advance()`` and ``retreat()``, or by signed offsets with ``move()``.
Their copy variants are ``advanced()``, ``retreated()``, and ``moved()``.
Regular movement saturates and preserves an absent index; ``OrThrow`` variants require exact finite movement.
``wouldAdvanceSaturate()``, ``wouldRetreatSaturate()``, and ``wouldMoveSaturate()`` detect failure of that exact
operation.
``canAdvance(step, bounds)``, ``canRetreat(step, bounds)``, and ``canMove(offset, bounds)`` additionally require the
result to lie within the supplied length.
The starting index may be outside those bounds.

``distanceFromZero()`` and ``absoluteDistanceTo()`` return non-negative amounts, using infinity for absent indexes.
``offsetFromZero()`` and ``offsetTo()`` return signed offsets, saturating when necessary; their ``OrThrow`` variants
reject absent indexes and unrepresentable differences.
``mirroredWithin(length)`` returns ``length - index - 1`` only for positions inside a finite length, otherwise no-index.

An amount's ``remainingFrom(index)`` returns the length or count from an inclusive index to that amount's exclusive end.
Positions at or beyond the end return zero, as does ``noIndex()``.
An infinite amount returns infinity for every valid index and zero for ``noIndex()``.
For example, ``ByteCount{12}.remainingFrom(ByteIndex{5})`` returns ``ByteCount{7}``.
The method accepts only the matching index family.

``Index::last(length)`` returns the final included index of a finite, nonempty sequence measured from zero.
For example, ``ByteIndex::last(ByteCount{8})`` returns ``ByteIndex{7}``.
Zero and infinite lengths return ``noIndex()`` because they have no last element.
Unlike ``Index::end(length).decremented()``, the empty case does not saturate to index zero.

Ranges
------

A range stores an inclusive ``beginIndex()`` and a length; ``endIndex()`` is exclusive.
Construction accepts either ``(begin, length)`` or ``(begin, end)``, including compact slicing such as
``text.slice({begin, end})``.
``fromBeginEnd()`` provides the same endpoint semantics:

* Ordered finite endpoints produce their difference.
* Reversed finite endpoints produce an empty range at the beginning.
* An absent beginning produces ``noRange()``.
* A valid beginning and absent endpoint produce an open-ended range.

``fromBeginEndOrThrow()`` rejects reversed endpoints and an absent beginning with ``OutOfRangeError``; it accepts an
open endpoint.

``fromBegin(begin)`` stores the beginning directly and sets an infinite length.
An absent beginning produces an absent, invalid range that retains its infinite length.
The factory performs no checks or conversions.

``isNoRange()`` tests an absent beginning.
``isEmpty()`` tests zero length independently of validity.
``isValid()`` requires a valid beginning and, for finite lengths, an exclusive endpoint at most ``Index::maximum()``.
An empty range at the maximum is valid; a finite one-element range beginning there is invalid.
Open-ended ranges remain valid and include the maximum index.
``endIndex()`` returns no-index for invalid or open-ended ranges.
``lastIndex()`` returns the final included index, accounting for the range's beginning.
For example, ``ByteRange{ByteIndex{4}, ByteCount{3}}.lastIndex()`` returns ``ByteIndex{6}``.
Empty, infinite, and invalid ranges return ``noIndex()``.

Invalid ranges contain nothing, produce ``noRange()`` when clamped or translated, and return ``LoopResult::Error`` from
``forEach()``.
Open-ended iteration visits representable indexes through the maximum without wrapping.
Saturating movement preserves length and keeps a valid range valid by limiting its beginning to ``maximum() - length()``
for finite ranges.
Throwing movement variants reject invalid input or required saturation.

``toAbsolute(origin)`` adds an origin; ``relativeTo(origin)`` subtracts it.
Both preserve length exactly and return ``noRange()`` for invalid input, overflow, or subtraction below zero.
``intersection(other)`` returns the common range, an empty range at the later beginning if disjoint or touching, or
``noRange()`` if either operand is invalid.
Open endpoints are treated as unbounded.
``contains(otherRange)`` requires both ranges to be valid.
An empty range is contained at either boundary; a finite range never contains an open-ended range.

``remainingFrom(index)`` returns the suffix beginning at an absolute index in the range's coordinate system.
An index before or at the beginning returns the original range; one at or beyond a finite end returns an empty range at
that end.
Open-ended ranges retain their infinite length.
An absent index or invalid range returns ``noRange()``.
For example, ``ByteRange{ByteIndex{4}, ByteCount{8}}.remainingFrom(ByteIndex{7})`` represents ``[7, 12)``.

Scalar Arithmetic
-----------------

Integer unit lengths and offsets support scalar multiplication, division and modulo while preserving their unit type.
Lengths require unsigned scalar operands.
Offsets require signed scalar operands.
Scalar operands may be native integer types or :cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>`
values.

Unit values are not multiplied, divided or modulo-applied with other unit values.
Indexes also keep their stricter domain model and can only be moved with matching lengths and offsets.
Unchecked Operations and Iteration
----------------------------------

Use ``T{rawValue, T::Unchecked{}}`` when the native value is already proven representable.
The constructor stores the raw representation directly, including the reserved sentinel representation.
It does not validate narrowing or signed-to-unsigned conversion before construction.
Keep runtime factories at boundaries where negative values, overflow, or numeric sentinels remain possible.

Indexes provide ``uncheckedAdvance()``, ``uncheckedRetreat()``, ``uncheckedMove()``, ``uncheckedIncrement()``, and
``uncheckedDecrement()``, plus the copy variants ending in ``Advanced``, ``Retreated``, ``Moved``, ``Incremented``, and
``Decremented``.
Amounts provide ``uncheckedAdd()``, ``uncheckedSubtract()``, their copy variants ``uncheckedAdded()`` and
``uncheckedSubtracted()``, and ``uncheckedIncrement()`` and ``uncheckedDecrement()``.
These operations require finite operands and an exactly representable finite result.
Use them inside loops whose enclosing buffer or range checks already establish those bounds.

``amount.forEach(function)`` visits indexes from zero up to the exclusive amount bound.
An infinite amount returns ``LoopResult::Error`` without invoking the callback.
``range.forEach(function)`` visits the range's indexes after validating the range once.
Both capture their original bounds, use raw increments internally, and accept callbacks returning ``void`` or
``LoopStatus``.
``Continue`` proceeds, ``Stop`` returns ``LoopResult::Stopped``, and ``Error`` returns ``LoopResult::Error``.
An empty valid iteration returns ``LoopResult::Success`` without calling the function.
Exceptions from callbacks propagate to the caller.

Scalar Limits
-------------

Finite length multiplication saturates at the largest finite length.
Infinite lengths remain infinite for scalar multiplication, division and modulo.
Division and modulo by zero terminate, matching the saturating integer helpers.

Integer Amounts
===============

:cpp:class:`IntegerAmount <erbsland::unit::IntegerAmount>` stores a signed saturating count with a unit tag and a scale.
Construction accepts signed counts; unsigned construction is deleted.
Comparison, addition, and subtraction require the exact same amount type.
Signed scalar multiplication, division, and remainder preserve the amount type; division and remainder by zero
terminate.
Arithmetic saturates, including negation of the minimum value to the maximum.
Increment and decrement change the count by one and saturate at the bounds.

Compatible conversion requires a shared unit tag and a supported scale ratio.
``converted<T>()`` saturates on overflow and truncates division toward zero.
``wouldConvertSaturate<T>()`` and ``convertedOrThrow<T>()`` check overflow, without rejecting discarded remainders.
``toRawValue()`` returns the native count; ``toValue()`` returns its saturating integer wrapper.
The Interface section below gives the remaining helpers for extraction, clamping, absolute values, and factories.
See :doc:`../../topics/time/working_with_time_amounts` for practical construction, conversion, and arithmetic with the
time aliases.

Version
=======

Introduction
------------

:cpp:class:`Version <erbsland::unit::Version>` stores a four-part version as major, minor, revision, and build number.
The default value is ``0.0.0.0``.
You can construct versions from raw part values or from typed part values in any order:

.. code-block:: cpp

    auto api = el::Version{1, 2, 0};
    auto selected = el::Version{el::Major{13}, el::Revision{2}}

Parsing
-------

:cpp:func:`fromString() <erbsland::unit::Version::fromString>` accepts one to four decimal parts separated by dots.
Each part must contain digits only, have no leading zeros unless it is zero, and fit in an unsigned 16-bit value.
Whitespace, signs, and trailing characters are invalid.
The ``requiredPrecision`` argument sets the least significant part that must be present; omitted parts become zero.
Invalid text returns ``std::nullopt``.
Use
:cpp:func:`fromStringOrThrow() <erbsland::unit::Version::fromStringOrThrow>` to receive an
:cpp:class:`err::ParseError <erbsland::err::ParseError>` instead.

Precision
---------

:cpp:func:`compare() <erbsland::unit::Version::compare>` accepts a
:cpp:enum:`VersionPart <erbsland::unit::VersionPart>` as precision.
The selected part is the last compared part.
For example, minor precision compares major and minor, while revision and build are ignored.

Numeric Representation
----------------------

:cpp:func:`toNumber() <erbsland::unit::Version::toNumber>` packs the four 16-bit parts into a ``uint64_t`` as
``major << 48 | minor << 32 | revision << 16 | build``.
This keeps numeric ordering identical to full version ordering.

Version Part
============

:cpp:enum:`VersionPart <erbsland::unit::VersionPart>` identifies the part of a
:cpp:class:`Version <erbsland::unit::Version>`.
It is also used as comparison precision: the selected part is included, and less-significant parts are ignored.

Version Range
=============

:cpp:class:`VersionRange <erbsland::unit::VersionRange>` stores optional inclusive minimum and maximum version bounds.
Missing bounds are open-ended, so a default range contains every version.

Containment uses the same precision rules as :cpp:func:`Version::compare <erbsland::unit::Version::compare>`.
If both bounds are present and the minimum is greater than the maximum at the selected precision, the range is empty.

Common Ranges
-------------

Use :cpp:func:`all() <erbsland::unit::VersionRange::all>` for an open range,
:cpp:func:`atLeast() <erbsland::unit::VersionRange::atLeast>` for a lower-bounded range,
:cpp:func:`atMost() <erbsland::unit::VersionRange::atMost>` for an upper-bounded range,
:cpp:func:`between() <erbsland::unit::VersionRange::between>` for inclusive minimum and maximum bounds, and
:cpp:func:`exact() <erbsland::unit::VersionRange::exact>` for a single version.

Version Components
==================

``VersionComponent<VersionPart>`` represents an independent unsigned 16-bit version field.
Use the distinct aliases ``Major``, ``Minor``, ``Revision``, and ``BuildNumber``.
Each accepts the full range 0–65535 and has no index or infinity state.

Compile-time integer construction and runtime ``fromInteger()`` / ``fromIntegerOrThrow()`` follow the unit construction
rules, with 65535 as the finite maximum.
Components support comparison, hashing, formatting, saturating increment and decrement, and unsigned scalar addition and
subtraction.
``addedOrThrow()`` and ``subtractedOrThrow()`` reject results outside the component range; matching mutating variants
are also available.

The raw numeric ``Version`` constructor validates constants at compile time.
For runtime values, construct typed components with the appropriate runtime factories and pass those to ``Version``.
Parsing and packed 64-bit representation retain their existing format.

Interface
=========

.. doxygentypedef:: erbsland::unit::ArgumentCount
.. doxygentypedef:: erbsland::unit::ArgumentIndex
.. doxygenstruct:: erbsland::unit::ArgumentUnit
    :members:
.. doxygentypedef:: erbsland::unit::BuildNumber
.. doxygentypedef:: erbsland::unit::ByteCount
.. doxygentypedef:: erbsland::unit::ByteIndex
.. doxygentypedef:: erbsland::unit::ByteOffset
.. doxygentypedef:: erbsland::unit::ByteRange
.. doxygenstruct:: erbsland::unit::ByteUnit
    :members:
.. doxygenstruct:: erbsland::unit::CodeContinuousRange
    :members:
.. doxygenclass:: erbsland::unit::CodeLocation
    :members:
.. doxygentypedef:: erbsland::unit::ColumnCount
.. doxygentypedef:: erbsland::unit::ColumnIndex
.. doxygentypedef:: erbsland::unit::ColumnOffset
.. doxygentypedef:: erbsland::unit::ColumnRange
.. doxygenstruct:: erbsland::unit::ColumnUnit
    :members:
.. doxygentypedef:: erbsland::unit::CpCount
.. doxygentypedef:: erbsland::unit::CpIndex
.. doxygentypedef:: erbsland::unit::CpOffset
.. doxygentypedef:: erbsland::unit::CpRange
.. doxygenstruct:: erbsland::unit::CpUnit
    :members:
.. doxygenclass:: erbsland::unit::ExitCode
    :members:
.. doxygenclass:: erbsland::unit::IntegerAmount
    :members:
.. doxygenstruct:: erbsland::unit::IntegerUnit
    :members:
.. doxygenclass:: erbsland::unit::IntegerUnitCount
    :members:
.. doxygenclass:: erbsland::unit::IntegerUnitIndex
    :members:
.. doxygenclass:: erbsland::unit::IntegerUnitOffset
    :members:
.. doxygenclass:: erbsland::unit::IntegerUnitRange
    :members:
.. doxygentypedef:: erbsland::unit::ItemCount
.. doxygentypedef:: erbsland::unit::ItemIndex
.. doxygentypedef:: erbsland::unit::ItemOffset
.. doxygentypedef:: erbsland::unit::ItemRange
.. doxygenstruct:: erbsland::unit::ItemUnit
    :members:
.. doxygentypedef:: erbsland::unit::LineCount
.. doxygentypedef:: erbsland::unit::LineIndex
.. doxygentypedef:: erbsland::unit::LineOffset
.. doxygentypedef:: erbsland::unit::LineRange
.. doxygenstruct:: erbsland::unit::LineUnit
    :members:
.. doxygentypedef:: erbsland::unit::Major
.. doxygentypedef:: erbsland::unit::Minor
.. doxygentypedef:: erbsland::unit::Revision
.. doxygentypedef:: erbsland::unit::U16Count
.. doxygentypedef:: erbsland::unit::U16Index
.. doxygentypedef:: erbsland::unit::U16Offset
.. doxygentypedef:: erbsland::unit::U16Range
.. doxygenstruct:: erbsland::unit::U16Unit
    :members:
.. doxygenclass:: erbsland::unit::Version
    :members:
.. doxygenclass:: erbsland::unit::VersionComponent
    :members:
.. doxygenenum:: erbsland::unit::VersionPart
.. doxygenclass:: erbsland::unit::VersionRange
    :members:
