************************************
Block Geometry Domain API Guidelines
************************************

These guidelines extend the Common API Guidelines for public APIs that model integer block-grid geometry.

The purpose of this document is to define a base naming vocabulary for block geometry APIs.
It is intentionally plain, technical and list based to get a quick overview of method names and their usage patterns.
If you introduce new vocabulary, update this page to provide a good reference for future extensions.

Core Semantics
==============

Coordinate Model
----------------

``Block`` is the prefix for values whose numeric data is tied to block coordinates.

.. code-block:: text

    BlockCoordinate // signed saturating coordinate value, alias for math::SatInt32
    BlockPosition // x/y location or vector in a block grid
    BlockSize // non-negative width/height extent
    BlockRect // top-left position plus size, with x2/y2 as exclusive end coordinates
    BlockMargins // top/right/bottom/left offsets around a rectangle

Ranges and Bounds
-----------------

.. code-block:: text

    BlockRect(topLeft, bottomRight) // bottomRight is outside the rectangle
    o.x1()/o.y1() // inclusive top-left coordinates
    o.x2()/o.y2() // exclusive bottom-right coordinates
    o.contains(pos) // half-open rectangle or size containment
    o.clamp(pos) // clamp into the represented bounds

Primary Types
=============

.. code-block:: text

    BlockCoordinate // signed saturating coordinate value
    BlockPosition // two-dimensional coordinate or vector
    BlockSize // non-negative width and height
    BlockRect // axis-aligned rectangle with top-left position and size
    BlockMargins // top, right, bottom and left offsets
    BlockDirection // compass direction on a block grid
    BlockAnchor // normalized anchor position inside a block rectangle or size
    Alignment // normalized content alignment inside available space
    Orientation // horizontal or vertical axis selection
    BlockAxisMapper // maps main/cross-axis values to x/y and width/height values

Secondary Types
===============

.. code-block:: text

    BlockAlignedSource // target/source rectangle pair after alignment and cropping
    BlockAnchorFlag, BlockAnchorFlags // low-level anchor flag bits
    AlignmentFlag, AlignmentFlags // low-level alignment flag bits
    BlockPositionList, BlockRectList // common position and rectangle containers

Value Object Patterns
=====================

.. code-block:: text

    o.x()/o.y() -> BlockCoordinate // get position coordinates
    o.setX/setY(value) -> void // set one position coordinate
    o.width()/o.height() -> BlockCoordinate // get size dimensions
    o.setWidth/setHeight(value) -> void // set one size dimension, clamping to non-negative
    o.pos()/o.size() -> BlockPosition/BlockSize // get rectangle origin and extent
    o.setPos/setSize(value) -> void // set rectangle origin or extent
    o.top()/o.right()/o.bottom()/o.left() -> BlockCoordinate // get margins by side
    o.setTop/setRight/setBottom/setLeft(value) -> void // set margins by side
    o.coordinate(orientation) -> BlockCoordinate // select x/y or width/height by orientation
    o.at(side) -> BlockCoordinate // select a margin by side
    o.toRawValue() -> Flags // expose normalized low-level flags
    o.hash() -> std::size_t // hash a value object

State and Membership Patterns
=============================

.. code-block:: text

    o.isZero() -> bool // test whether a size has no area
    o.isInRange(minimum, maximum) -> bool // component-wise range test
    o.fitsInto(size) -> bool // component-wise size containment
    o.contains(position/rect) -> bool // geometry membership
    o.overlaps(rect) -> bool // rectangle intersection test
    o.isFrame(position) -> bool // test whether a position lies on the rectangle frame
    o.is❮Side❯() -> bool // test anchor or alignment side, e.g. isLeft(), isBottom()
    o.isHorizontalCenter/isVerticalCenter() -> bool // test center component

Geometry Arithmetic Patterns
============================

.. code-block:: text

    o.distanceTo(position) -> BlockCoordinate // Manhattan distance
    o.componentMin/componentMax(position) -> BlockPosition // component-wise position extrema
    o.add/subtract(value[, orientation]) -> T& // saturating in-place arithmetic
    o.expandedWith/limitedWith(value[, axis-or-side]) -> T // return constrained copy
    o.expandTo/limitTo(value[, axis-or-side]) -> T& // apply component-wise constraints
    o.expandedBy/insetBy(margins) -> BlockRect // apply margins to rectangle bounds
    o.clampTo(minimum, maximum) -> BlockSize // component-wise size clamp
    o.area() -> math::SatInt32 // compute width times height
    T::minimum()/maximum()/zero() -> T // common boundary factories

Direction and Ring Patterns
===========================

.. code-block:: text

    o.contains(direction) -> bool // lexical containment of compass components
    o.toDelta() -> BlockPosition // convert direction to unit position delta
    T::fromDelta(delta) -> BlockDirection // convert delta signs to direction
    o.toString() -> text::StringView // normalized lowercase direction name
    T::fromString(text) -> T // parse text, returning the neutral state on failure
    T::isValidString(text) -> bool // test if text can be parsed
    o.cardinalFour() -> array<BlockPosition, 4> // four neighbor positions
    T::cardinalFourDeltas() -> array<BlockPosition, 4> // four unit deltas
    o.ringEight() -> array<BlockPosition, 8> // eight surrounding positions
    T::ringEightDeltas() -> array<BlockPosition, 8> // eight unit deltas

Layout Patterns
===============

.. code-block:: text

    o.anchor(anchor) -> BlockPosition // resolve anchor to a position inside size or rectangle
    o.center() -> BlockPosition // shorthand for the center anchor
    o.alignmentOffset(size, alignment) -> BlockPosition // place content inside available space
    o.subRectangle(anchor, size, margins) -> BlockRect // create an anchored child rectangle
    o.alignedSource(rect, alignment) -> BlockAlignedSource // align and crop a source rectangle
    o.horizontal()/vertical() -> T // keep only one normalized alignment or anchor component
    o.horizontalOffset/verticalOffset(available, content) -> BlockCoordinate // offset for one axis
    o.crossed() -> Orientation // switch between horizontal and vertical orientation
    o.size(main, cross) -> BlockSize // map main/cross extents to width/height
    o.position(main[, cross]) -> BlockPosition // map main/cross coordinates to x/y
    o.horizontalValue/verticalValue(main, cross) -> V // select value for the physical axis

Iteration and Division Patterns
===============================

.. code-block:: text

    o.forEach(function) -> void // visit all contained positions in row-major order
    o.forEachInFrame(function) -> void // visit frame positions clockwise
    o.frameIndex(position) -> int64_t // clockwise frame index or -1
    o.frameDirection(position) -> BlockDirection // frame direction or None
    o.gridCells(rows, columns[, spacing]) -> vector<BlockRect> // divide a rectangle into grid cells
    T::bounds(positions) -> BlockRect // create bounds for a position list
