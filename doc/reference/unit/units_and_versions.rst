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

Scalar Arithmetic
-----------------

Integer unit lengths and offsets support scalar multiplication, division and modulo while preserving their unit type.
Lengths require unsigned scalar operands.
Offsets require signed scalar operands.
Scalar operands may be native integer types or :cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>`
values.

Unit values are not multiplied, divided or modulo-applied with other unit values.
Indexes also keep their stricter domain model and can only be moved with matching lengths and offsets.
The ``uncheckedAdvance()`` and ``uncheckedIncrement()`` operations are reserved for measured low-level loops whose
finite inputs and valid result bounds are already proven.
Regular code uses the checked, saturating index operations.

Finite length multiplication saturates at the largest finite length.
Infinite lengths remain infinite for scalar multiplication, division and modulo.
Division and modulo by zero terminate, matching the saturating integer helpers.

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

Precision
^^^^^^^^^

:cpp:func:`compare() <erbsland::unit::Version::compare>` accepts a
:cpp:enum:`VersionPart <erbsland::unit::VersionPart>` as precision.
The selected part is the last compared part.
For example, minor precision compares major and minor, while revision and build are ignored.

Numeric Representation
^^^^^^^^^^^^^^^^^^^^^^

:cpp:func:`toNumber() <erbsland::unit::Version::toNumber>` packs the four 16-bit parts into a ``uint64_t`` as
``major << 48 | minor << 32 | revision << 16 | build``.
This keeps numeric ordering identical to full version ordering.

Version Part
~~~~~~~~~~~~

:cpp:enum:`VersionPart <erbsland::unit::VersionPart>` identifies the part of a
:cpp:class:`Version <erbsland::unit::Version>`.
It is also used as comparison precision: the selected part is included, and less-significant parts are ignored.

Version Range
~~~~~~~~~~~~~

:cpp:class:`VersionRange <erbsland::unit::VersionRange>` stores optional inclusive minimum and maximum version bounds.
Missing bounds are open-ended, so a default range contains every version.

Containment uses the same precision rules as :cpp:func:`Version::compare <erbsland::unit::Version::compare>`.
If both bounds are present and the minimum is greater than the maximum at the selected precision, the range is empty.

Common Ranges
^^^^^^^^^^^^^

Use :cpp:func:`all() <erbsland::unit::VersionRange::all>` for an open range,
:cpp:func:`atLeast() <erbsland::unit::VersionRange::atLeast>` for a lower-bounded range,
:cpp:func:`atMost() <erbsland::unit::VersionRange::atMost>` for an upper-bounded range,
:cpp:func:`between() <erbsland::unit::VersionRange::between>` for inclusive minimum and maximum bounds, and
:cpp:func:`exact() <erbsland::unit::VersionRange::exact>` for a single version.

Version Unit
~~~~~~~~~~~~

:cpp:struct:`VersionUnit <erbsland::unit::VersionUnit>` is the shared base unit for unsigned 16-bit version components.
It disables the ``noIndex()`` sentinel so the full 16-bit range is available to
:cpp:class:`IntegerUnitIndex <erbsland::unit::IntegerUnitIndex>`.
Use the public aliases :cpp:type:`Major <erbsland::unit::Major>`, :cpp:type:`Minor <erbsland::unit::Minor>`,
:cpp:type:`Revision <erbsland::unit::Revision>`, and :cpp:type:`BuildNumber <erbsland::unit::BuildNumber>` in user code.

The concrete units :cpp:struct:`MajorUnit <erbsland::unit::MajorUnit>`,
:cpp:struct:`MinorUnit <erbsland::unit::MinorUnit>`, :cpp:struct:`RevisionUnit <erbsland::unit::RevisionUnit>`, and
:cpp:struct:`BuildNumberUnit <erbsland::unit::BuildNumberUnit>` keep the represented
:cpp:enum:`VersionPart <erbsland::unit::VersionPart>` in the type.

Interface
=========

.. doxygenstruct:: erbsland::unit::ArgumentUnit
    :members:

.. doxygentypedef:: erbsland::unit::ArgumentIndex

.. doxygentypedef:: erbsland::unit::ArgumentCount
.. doxygentypedef:: erbsland::unit::ByteIndex
.. doxygentypedef:: erbsland::unit::ByteLength
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
.. doxygentypedef:: erbsland::unit::CpIndex
.. doxygentypedef:: erbsland::unit::CpLength
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
.. doxygenclass:: erbsland::unit::IntegerUnitAmount
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
.. doxygentypedef:: erbsland::unit::U16DataIndex
.. doxygentypedef:: erbsland::unit::U16DataLength
.. doxygentypedef:: erbsland::unit::U16DataOffset
.. doxygentypedef:: erbsland::unit::U16DataRange
.. doxygenstruct:: erbsland::unit::U16DataUnit
    :members:
.. doxygenclass:: erbsland::unit::Version
    :members:

.. doxygenstruct:: std::hash
    :members:
.. doxygenenum:: erbsland::unit::VersionPart
.. doxygenclass:: erbsland::unit::VersionRange
    :members:
.. doxygenstruct:: erbsland::unit::VersionUnit
    :members:
.. doxygenstruct:: erbsland::unit::MajorUnit
    :members:

.. doxygenstruct:: erbsland::unit::MinorUnit
    :members:

.. doxygenstruct:: erbsland::unit::RevisionUnit
    :members:

.. doxygenstruct:: erbsland::unit::BuildNumberUnit
    :members:

.. doxygentypedef:: erbsland::unit::Major

.. doxygentypedef:: erbsland::unit::Minor

.. doxygentypedef:: erbsland::unit::Revision

.. doxygentypedef:: erbsland::unit::BuildNumber
