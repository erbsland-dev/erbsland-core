.. index::
    single: Block Geometry

**************
Block Geometry
**************

Introduction
============

Block Coordinate
----------------

:cpp:type:`Coordinate <erbsland::block::Coordinate>` is the signed saturating coordinate type used by block
geometry.
Arithmetic saturates at the coordinate limits, while sizes still clamp their dimensions to non-negative values.

Coordinate Span
---------------

:cpp:class:`CoordinateSpan <erbsland::block::CoordinateSpan>` represents a half-open range on one physical axis as an
origin and a non-negative extent.
Reversing a non-empty span reflects its discrete cells about coordinate zero and preserves their membership.

Block Direction
---------------

:cpp:class:`Direction <erbsland::block::Direction>` represents the eight compass directions on a block grid,
plus ``None``.
Direction strings use ``text::String`` for parsing and ``text::StringLiteral`` backed values for canonical output.

Block Margins
-------------

:cpp:class:`Margins <erbsland::block::Margins>` stores top, right, bottom and left offsets for block
rectangles.
Positive margins expand outward; negative margins inset or reduce geometry where the consuming API supports it.
Its ``horizontal()``, ``vertical()``, and ``component()`` accessors return a
:cpp:class:`MarginPair <erbsland::block::MarginPair>`.

Margin Pair
-----------

:cpp:class:`MarginPair <erbsland::block::MarginPair>` stores leading and trailing offsets along one axis.
Use ``extent()`` for the total positive space consumed, ``delta()`` for the signed sum, and ``spacing()`` for the
largest positive side.
Direction reversal exchanges the leading and trailing values.

Block Position
--------------

:cpp:class:`Position <erbsland::block::Position>` stores an ``x`` and ``y`` coordinate for block grids.
It is useful both as an absolute position and as a small vector for block-based arithmetic.

Block Size
----------

:cpp:class:`Size <erbsland::block::Size>` stores a non-negative width and height.
Negative inputs are clamped to zero, even though the underlying coordinate type itself supports negative values.

Block Rectangle
---------------

:cpp:class:`Rectangle <erbsland::block::Rectangle>` stores an axis-aligned rectangle as a top-left position and a
:cpp:class:`Size <erbsland::block::Size>`.
Position transforms use the size transform in local rectangle coordinates and translate the result back to global
coordinates.
Horizontal mirroring exchanges left and right, vertical mirroring exchanges top and bottom.

Interface
=========

.. doxygenstruct:: erbsland::block::AlignedSource
    :members:
.. doxygentypedef:: erbsland::block::Coordinate
.. doxygenclass:: erbsland::block::CoordinateSpan
    :members:
.. doxygenclass:: erbsland::block::Direction
    :members:
.. doxygenclass:: erbsland::block::MarginPair
    :members:
.. doxygenclass:: erbsland::block::Margins
    :members:
.. doxygenclass:: erbsland::block::Position
    :members:
.. doxygenclass:: erbsland::block::Rectangle
    :members:
.. doxygenclass:: erbsland::block::Size
    :members:
