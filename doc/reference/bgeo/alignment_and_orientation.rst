.. index::
    single: Alignment and Orientation

*************************
Alignment and Orientation
*************************

Introduction
============

Alignment
---------

:cpp:class:`Alignment <erbsland::bgeo::Alignment>` is the safe value object for text and graphics alignment.
Use it in public APIs when callers should choose at most one horizontal and at most one vertical alignment.

The default value is ``Alignment::TopLeft``.
You can also pass one of the predefined constants, such as ``Alignment::Center`` or ``Alignment::BottomRight``.

Orientation
-----------

:cpp:class:`Orientation <erbsland::bgeo::Orientation>` represents a rotation in 90-degree increments.
It is useful for describing the layout direction of text or graphics content.

Basic Usage
===========

Construct an alignment from a predefined constant whenever possible:

.. code-block:: cpp

    auto titleAlignment = el::bgeo::Alignment::Center;
    auto iconAlignment = el::bgeo::Alignment::BottomRight;

When a lower-level flag set is used, :cpp:class:`Alignment <erbsland::bgeo::Alignment>` normalizes it.
If more than one horizontal flag is set, the first one in the order ``Left``, ``HCenter``, ``Right`` is kept.
If more than one vertical flag is set, the first one in the order ``Top``, ``VCenter``, ``Bottom`` is kept.

Optional Axes
=============

Horizontal and vertical components are optional.
This is useful when you want to pass only one axis to a lower-level operation:

.. code-block:: cpp

    auto verticalOnly = el::Alignment{el::AlignmentFlag::VCenter};

Use ``horizontal()`` and ``vertical()`` to extract the individual components.
Use ``toRawValue()`` when you need the underlying :cpp:type:`AlignmentFlags <erbsland::bgeo::AlignmentFlags>` value.

Alignment Flags
===============

:cpp:enum:`AlignmentFlag <erbsland::bgeo::AlignmentFlag>` contains the low-level bits used to describe horizontal and
vertical alignment.
Most user-facing APIs should prefer :cpp:class:`Alignment <erbsland::bgeo::Alignment>`, because it normalizes
conflicting flags into a valid value object.

Flag Groups
-----------

The horizontal flags are ``Left``, ``HCenter`` and ``Right``.
The vertical flags are ``Top``, ``VCenter`` and ``Bottom``.
The combined enum entries, such as ``TopLeft`` and ``BottomRight``, are convenience values made from one horizontal and
one vertical flag.

Use :cpp:type:`AlignmentFlags <erbsland::bgeo::AlignmentFlags>` when an implementation needs to inspect or combine raw
alignment bits before creating an :cpp:class:`Alignment <erbsland::bgeo::Alignment>`.

Interface
=========

.. doxygenclass:: erbsland::bgeo::Alignment
    :members:
.. doxygenenum:: erbsland::bgeo::AlignmentFlag

.. doxygentypedef:: erbsland::bgeo::AlignmentFlags
.. doxygenclass:: erbsland::bgeo::Orientation
    :members:
.. doxygenenum:: erbsland::bgeo::Symmetry
