***********************
Geometry API Guidelines
***********************

Primary Types
=============

.. code-block:: text

    Alignment // normalized horizontal and vertical content alignment
    Anchor // normalized relative position in a two-dimensional area
    Axis // physical x, y, or z axis
    SignedAxis // physical axis with an optional direction reversal
    AxisMapping // exact unsigned permutation of one, two, or three axes
    SignedAxisMapping // exact permutation with optional direction reversals
    AxisMapper // reconstruct axis-mappable values from a mapping
    Orientation // horizontal or vertical physical axis selection
    Symmetry // rectangle-preserving position transformation

Secondary Types
===============

.. code-block:: text

    AlignmentFlag, AlignmentFlags // low-level alignment flag bits
    AnchorFlag, AnchorFlags // low-level anchor flag bits
    Dimensionality // one, two, or three dimensions
    AxisMappable❮T❯ // value exposing typed components and matching reconstruction

Value Object Patterns
=====================

.. code-block:: text

    o.horizontal()/vertical() -> Alignment // keep one normalized alignment component
    o.horizontal()/vertical() -> Anchor // keep one normalized anchor component
    o.source(axis) -> Axis // unsigned source used for a destination axis
    o.source(axis) -> SignedAxis // signed source used for a destination axis
    o.map(value) -> T // permute and optionally reverse all components of a value
    o.crossed() -> Orientation // switch between horizontal and vertical orientation
    o.toRawValue() -> AlignmentFlags // expose normalized low-level flags
    o.is❮Side❯() -> bool // test an alignment side, e.g. isLeft(), isBottom()
    o.isHorizontalCenter/isVerticalCenter() -> bool // test center component
