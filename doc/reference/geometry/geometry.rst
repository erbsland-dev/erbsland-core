.. index::
    single: Geometry Types

**************
Geometry Types
**************

Introduction
============

Alignment
---------

:cpp:class:`Alignment <erbsland::geometry::Alignment>` is the safe value object for text and graphics alignment.
Use it in public APIs when callers should choose at most one horizontal and at most one vertical alignment.

The default value is ``Alignment::TopLeft``.
You can also pass one of the predefined constants, such as ``Alignment::Center`` or ``Alignment::BottomRight``.

Anchor
------

:cpp:class:`Anchor <erbsland::geometry::Anchor>` selects a corner, edge, or center of any two-dimensional area.
It is appropriate for resolving a relative position in rectangles, sizes, grids, and other geometry types.
Unlike :cpp:class:`Alignment <erbsland::geometry::Alignment>`, it describes the position to select rather than how
content should be arranged.

Axis Mapper
-----------

:cpp:class:`AxisMapper <erbsland::geometry::AxisMapper>` applies an exact one-, two-, or three-dimensional axis mapping
to a compatible geometry value.
An unsigned mapping only permutes axes; a signed mapping can also reverse each selected component.

Orientation
-----------

:cpp:class:`Orientation <erbsland::geometry::Orientation>` selects a horizontal or vertical main axis.
It is useful for describing the layout direction of text or graphics content without implying a direction reversal.

Basic Usage
===========

Construct an alignment from a predefined constant whenever possible:

.. code-block:: cpp

    auto titleAlignment = el::geometry::Alignment::Center;
    auto iconAlignment = el::geometry::Alignment::BottomRight;

When a lower-level flag set is used, :cpp:class:`Alignment <erbsland::geometry::Alignment>` normalizes it.
If more than one horizontal flag is set, the first one in the order ``Left``, ``HCenter``, ``Right`` is kept.
If more than one vertical flag is set, the first one in the order ``Top``, ``VCenter``, ``Bottom`` is kept.

Optional Axes
=============

Horizontal and vertical components are optional.
This is useful when you want to pass only one axis to a lower-level operation:

.. code-block:: cpp

    auto verticalOnly = el::Alignment{el::AlignmentFlag::VCenter};

Use ``horizontal()`` and ``vertical()`` to extract the individual components.
Use ``toRawValue()`` when you need the underlying :cpp:type:`AlignmentFlags <erbsland::geometry::AlignmentFlags>` value.

Anchor Positions
================

Construct an :cpp:class:`Anchor <erbsland::geometry::Anchor>` from a predefined constant when a position on a
two-dimensional area is needed:

.. code-block:: cpp

    auto anchor = el::geometry::Anchor::BottomRight;

The :cpp:enum:`AnchorFlag <erbsland::geometry::AnchorFlag>` and
:cpp:type:`AnchorFlags <erbsland::geometry::AnchorFlags>` types support lower-level flag composition before an
:cpp:class:`Anchor <erbsland::geometry::Anchor>` normalizes conflicting selections.
Horizontal and vertical components are optional, and ``horizontal()`` and ``vertical()`` return either component alone.

Axis Mapping
============

:cpp:enum:`Axis <erbsland::geometry::Axis>` names the physical X, Y, and Z axes.
:cpp:class:`AxisMapping <erbsland::geometry::AxisMapping>` specifies the source axis used for each destination axis.
Its constructor arity declares the dimensionality and its axes must be an exact permutation for that dimensionality.

Use :cpp:class:`AxisMapper <erbsland::geometry::AxisMapper>` to apply the mapping to any
:cpp:concept:`AxisMappable <erbsland::geometry::AxisMappable>` value:

.. code-block:: cpp

    const auto mapper = el::geometry::AxisMapper{
        el::geometry::AxisMapping{el::geometry::Axis::Y, el::geometry::Axis::X}};
    const auto mapped = mapper.map(el::block::Position{10, 20}); // (20, 10)

The orientation constructor is a two-dimensional convenience mapping.
Horizontal orientation preserves X/Y order and vertical orientation exchanges X and Y.

Signed Axis Mapping
-------------------

:cpp:class:`SignedAxisMapping <erbsland::geometry::SignedAxisMapping>` additionally describes direction reversal with
:cpp:class:`SignedAxis <erbsland::geometry::SignedAxis>`.
The mapped value defines reversal semantics for its component type: positions negate coordinates, sizes preserve their
non-negative extents, rectangles reflect their half-open coordinate spans, and margins exchange leading and trailing
sides.

Mappings reject duplicate, missing, or out-of-range axes.
``AxisMapper::map()`` also rejects a value whose declared dimensionality differs from the mapping instead of filling or
discarding components.

Alignment Flags
===============

:cpp:enum:`AlignmentFlag <erbsland::geometry::AlignmentFlag>` contains the low-level bits used to describe horizontal and
vertical alignment.
Most user-facing APIs should prefer :cpp:class:`Alignment <erbsland::geometry::Alignment>`, because it normalizes
conflicting flags into a valid value object.

Flag Groups
-----------

The horizontal flags are ``Left``, ``HCenter`` and ``Right``.
The vertical flags are ``Top``, ``VCenter`` and ``Bottom``.
The combined enum entries, such as ``TopLeft`` and ``BottomRight``, are convenience values made from one horizontal and
one vertical flag.

Use :cpp:type:`AlignmentFlags <erbsland::geometry::AlignmentFlags>` when an implementation needs to inspect or combine
raw alignment bits before creating an :cpp:class:`Alignment <erbsland::geometry::Alignment>`.

Interface
=========

.. doxygenclass:: erbsland::geometry::Alignment
    :members:
.. doxygenenum:: erbsland::geometry::AlignmentFlag

.. doxygentypedef:: erbsland::geometry::AlignmentFlags
.. doxygenclass:: erbsland::geometry::Anchor
    :members:
.. doxygenenum:: erbsland::geometry::AnchorFlag

.. doxygentypedef:: erbsland::geometry::AnchorFlags
.. doxygenenum:: erbsland::geometry::Axis
.. doxygenconcept:: erbsland::geometry::AxisMappable
.. doxygenclass:: erbsland::geometry::AxisMapper
    :members:
.. doxygenclass:: erbsland::geometry::AxisMapping
    :members:
.. doxygenenum:: erbsland::geometry::Dimensionality
.. doxygenclass:: erbsland::geometry::Orientation
    :members:
.. doxygenclass:: erbsland::geometry::SignedAxis
    :members:
.. doxygenclass:: erbsland::geometry::SignedAxisMapping
    :members:
.. doxygenenum:: erbsland::geometry::Symmetry
