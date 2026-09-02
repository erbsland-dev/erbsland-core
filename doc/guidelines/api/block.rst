************************************
Block Geometry Domain API Guidelines
************************************

Core Semantics
==============

Coordinate Model
----------------

.. code-block:: text

    coordinate = signed terminal-cell-space scalar
    coordinate span = half-open origin plus non-negative extent on one axis
    position/vector = signed x/y coordinates
    size = non-negative width/height extents
    rectangle = top-left position plus size
    margin pair = leading and trailing offsets on one axis
    empty rectangle = zero width or height

Primary Types
=============

.. code-block:: text

    Coordinate // signed saturating terminal-cell coordinate
    CoordinateSpan // one-dimensional half-open coordinate range
    MarginPair // leading and trailing margins on one axis
    Position // two-dimensional coordinate or vector
    Size // non-negative width and height
    Rectangle // axis-aligned half-open rectangle

Secondary Types
===============

.. code-block:: text

    Margins // horizontal and vertical margin pairs; also top, right, bottom and left offsets
    Direction // compass direction on a block grid
    AlignedSource // target/source rectangle pair after alignment and cropping
    PositionList, RectangleList // common position and rectangle containers

Pattern Definitions
===================

.. code-block:: text

    G = Position/Size // position or size selected by a shared accessor pattern
    Pa = PositionArray❮size❯ // fixed-size array of block positions
    V = ❮Value❯ // value selected for a physical axis

Value Object Patterns
=====================

.. code-block:: text

    o.x/y() -> Coordinate // get position coordinates
    o.setX/setY(value) -> void // set one position coordinate
    o.width/height() -> Coordinate // get size dimensions
    o.setWidth/setHeight(value) -> void // set one size dimension, clamping to non-negative
    o.pos/size() -> G // get rectangle origin and extent
    o.setPos/setSize(value) -> void // set rectangle origin or extent
    o.top/right/bottom/left() -> Coordinate // get margins by side
    o.setTop/setRight/setBottom/setLeft(value) -> void // set margins by side
    o.horizontal/vertical() -> MarginPair // get physical-axis margins
    o.component(axis/orientation) -> AxisComponent // select one physical-axis component
    o.leading/trailing() -> Coordinate // get a margin-pair side
    o.origin/extent/end() -> Coordinate // inspect a coordinate span
    o.extent/delta/spacing() -> Coordinate // calculate one-axis margin effects
    o.at(side) -> Coordinate // select a margin by side
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

Geometry Arithmetic Patterns
============================

.. code-block:: text

    o.distanceTo(position) -> Coordinate // Manhattan distance
    o.componentMin/componentMax(position) -> Position // component-wise position extrema
    o.add/subtract(value[, orientation]) -> T& // saturating in-place arithmetic
    o.expandedWith/limitedWith(value[, axis-or-side]) -> T // return constrained copy
    o.expandTo/limitTo(value[, axis-or-side]) -> T& // apply component-wise constraints
    o.expandedBy/insetBy(margins) -> Rectangle // apply margins to rectangle bounds
    o.clampTo(minimum, maximum) -> Size // component-wise size clamp
    o.area() -> math::SatInt32 // compute width times height
    T::minimum()/maximum()/zero() -> T // common boundary factories

Direction and Ring Patterns
===========================

.. code-block:: text

    o.contains(direction) -> bool // lexical containment of compass components
    o.toDelta() -> Position // convert direction to unit position delta
    T::fromDelta(delta) -> Direction // convert delta signs to direction
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

    o.anchor(anchor) -> Position // resolve anchor to a position inside size or rectangle
    o.center() -> Position // shorthand for the center anchor
    o.alignmentOffset(size, geometry::Alignment) -> Position // place content inside available space
    o.subRectangle(anchor, size, margins) -> Rectangle // create an anchored child rectangle
    o.alignedSource(rect, geometry::Alignment) -> AlignedSource // align and crop a source rectangle

Iteration and Division Patterns
===============================

.. code-block:: text

    o.forEach(function) -> void // visit all contained positions in row-major order
    o.forEachInFrame(function) -> void // visit frame positions clockwise
    o.frameIndex(position) -> int64_t // clockwise frame index or -1
    o.frameDirection(position) -> Direction // frame direction or None
    o.rotateCCW/mirror/transform(position, geometry::Orientation/Symmetry) -> Position // transform positions
    o.gridCells(rows, columns[, spacing]) -> vector<Rectangle> // divide a rectangle into grid cells
    T::bounds(positions) -> Rectangle // create bounds for a position list
