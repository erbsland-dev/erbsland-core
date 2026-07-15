.. index::
    single: Block Geometry

**************
Block Geometry
**************

Introduction
============

Block Anchor
------------

:cpp:class:`BlockAnchor <erbsland::bgeo::BlockAnchor>` is the safe value object for choosing a position inside a block
size or block rectangle.
Use it when callers should select at most one horizontal anchor and at most one vertical anchor.

The low-level :cpp:enum:`BlockAnchorFlag <erbsland::bgeo::BlockAnchorFlag>` and
:cpp:type:`BlockAnchorFlags <erbsland::bgeo::BlockAnchorFlags>` types are available for implementation code that needs
to combine raw flags before constructing a normalized :cpp:class:`BlockAnchor <erbsland::bgeo::BlockAnchor>` value.

Block Axis Mapper
-----------------

:cpp:class:`BlockAxisMapper <erbsland::bgeo::BlockAxisMapper>` maps main/cross-axis values to block positions and sizes
for horizontal or vertical layouts.

Block Coordinate
----------------

:cpp:type:`BlockCoordinate <erbsland::bgeo::BlockCoordinate>` is the signed saturating coordinate type used by block
geometry.
Arithmetic saturates at the coordinate limits, while sizes still clamp their dimensions to non-negative values.

Block Direction
---------------

:cpp:class:`BlockDirection <erbsland::bgeo::BlockDirection>` represents the eight compass directions on a block grid,
plus ``None``.
Direction strings use ``text::StringView`` for parsing and ``text::StringLiteral`` backed values for canonical output.

Block Margins
-------------

:cpp:class:`BlockMargins <erbsland::bgeo::BlockMargins>` stores top, right, bottom and left offsets for block
rectangles.
Positive margins expand outward; negative margins inset or reduce geometry where the consuming API supports it.

Block Position
--------------

:cpp:class:`BlockPosition <erbsland::bgeo::BlockPosition>` stores an ``x`` and ``y`` coordinate for block grids.
It is useful both as an absolute position and as a small vector for block-based arithmetic.

Block Size
----------

:cpp:class:`BlockSize <erbsland::bgeo::BlockSize>` stores a non-negative width and height.
Negative inputs are clamped to zero, even though the underlying coordinate type itself supports negative values.

Block Rect
----------

:cpp:class:`BlockRect <erbsland::bgeo::BlockRect>` stores an axis-aligned rectangle as a top-left position and a
:cpp:class:`BlockSize <erbsland::bgeo::BlockSize>`.
Position transforms use the size transform in local rectangle coordinates and translate the result back to global
coordinates.
Horizontal mirroring exchanges left and right, vertical mirroring exchanges top and bottom.

Interface
=========

.. doxygenstruct:: erbsland::bgeo::BlockAlignedSource
    :members:
.. doxygenenum:: erbsland::bgeo::BlockAnchorFlag

.. doxygentypedef:: erbsland::bgeo::BlockAnchorFlags

.. doxygenclass:: erbsland::bgeo::BlockAnchor
    :members:
.. doxygenclass:: erbsland::bgeo::BlockAxisMapper
    :members:
.. doxygentypedef:: erbsland::bgeo::BlockCoordinate
.. doxygenclass:: erbsland::bgeo::BlockDirection
    :members:
.. doxygenclass:: erbsland::bgeo::BlockMargins
    :members:
.. doxygenclass:: erbsland::bgeo::BlockPosition
    :members:
.. doxygenclass:: erbsland::bgeo::BlockRectangle
    :members:
.. doxygenclass:: erbsland::bgeo::BlockSize
    :members:
