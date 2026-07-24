************************************
Block Geometry Domain API Guidelines
************************************

Core Semantics
==============

Coordinate Model
----------------

.. code-block:: text

    coordinate = signed terminal-cell-space scalar with saturating arithmetic
    position/vector = signed x/y coordinates
    size = non-negative width/height extents
    rectangle = top-left position plus size

Ranges and Bounds
-----------------

.. code-block:: text

    rectangle bounds = inclusive top-left and exclusive bottom-right
    empty rectangle = zero width or height
    containment = half-open for positions and complete for rectangles
    clamping = nearest coordinate in the represented bounds

Primary Types
=============

.. code-block:: text

    BlockCoordinate // signed saturating terminal-cell coordinate
    BlockPosition // two-dimensional coordinate or vector
    BlockSize // non-negative width and height
    BlockRectangle // axis-aligned half-open rectangle

Secondary Types
===============

.. code-block:: text

    BlockMargins // top, right, bottom and left offsets
    BlockDirection // compass direction on a block grid
    BlockAnchor, Alignment // normalized anchor and content alignment
    Orientation, Symmetry // axis selection and rectangle-preserving transformation
    BlockAxisMapper // mapping between main/cross axes and physical axes
    BlockAlignedSource // target/source rectangle pair after alignment and cropping
    BlockAnchorFlag, BlockAnchorFlags // low-level anchor flag bits
    AlignmentFlag, AlignmentFlags // low-level alignment flag bits
    BlockPositionList, BlockRectangleList // common position and rectangle containers

Pattern Definitions
===================

.. code-block:: text

    G = BlockPosition/BlockSize // position or size selected by a shared accessor pattern
    Pa = BlockPositionArray❮size❯ // fixed-size array of block positions
    V = ❮Value❯ // value selected for a physical axis

Value Object Patterns
=====================

.. code-block:: text

    o.x/y() -> BlockCoordinate // get position coordinates
    o.setX/setY(value) -> void // set one position coordinate
    o.width/height() -> BlockCoordinate // get size dimensions
    o.setWidth/setHeight(value) -> void // set one size dimension, clamping to non-negative
    o.pos/size() -> G // get rectangle origin and extent
    o.setPos/setSize(value) -> void // set rectangle origin or extent
    o.top/right/bottom/left() -> BlockCoordinate // get margins by side
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
    o.expandedBy/insetBy(margins) -> BlockRectangle // apply margins to rectangle bounds
    o.clampTo(minimum, maximum) -> BlockSize // component-wise size clamp
    o.area() -> math::SatInt32 // compute width times height
    T::minimum()/maximum()/zero() -> T // common boundary factories

Direction and Ring Patterns
===========================

.. code-block:: text

    o.contains(direction) -> bool // lexical containment of compass components
    o.toDelta() -> BlockPosition // convert direction to unit position delta
    T::fromDelta(delta) -> BlockDirection // convert delta signs to direction
    o.toString() -> text::String // normalized lowercase direction name
    T::fromString(text) -> T // parse text, returning the neutral state on failure
    T::isValidString(text) -> bool // test if text can be parsed
    o.cardinalFour() -> Pa // four neighbor positions
    T::cardinalFourDeltas() -> Pa // four unit deltas
    o.ringEight() -> Pa // eight surrounding positions
    T::ringEightDeltas() -> Pa // eight unit deltas

Layout Patterns
===============

.. code-block:: text

    o.anchor(anchor) -> BlockPosition // resolve anchor to a position inside size or rectangle
    o.center() -> BlockPosition // shorthand for the center anchor
    o.alignmentOffset(size, alignment) -> BlockPosition // place content inside available space
    o.subRectangle(anchor, size, margins) -> BlockRectangle // create an anchored child rectangle
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
    o.rotateCCW/mirror/transform(position, operation) -> BlockPosition // transform a position inside the rectangle
    o.gridCells(rows, columns[, spacing]) -> vector<BlockRectangle> // divide a rectangle into grid cells
    T::bounds(positions) -> BlockRectangle // create bounds for a position list
