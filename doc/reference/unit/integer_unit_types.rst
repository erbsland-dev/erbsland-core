.. index::
    single: Integer Indexes, Amounts, Offsets and Ranges

********************************************
Integer Indexes, Amounts, Offsets and Ranges
********************************************

Scalar Arithmetic
=================

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
.. doxygentypedef:: erbsland::unit::ElementCount
.. doxygentypedef:: erbsland::unit::ElementIndex
.. doxygentypedef:: erbsland::unit::ElementOffset
.. doxygentypedef:: erbsland::unit::ElementRange
.. doxygenstruct:: erbsland::unit::ElementUnit
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
