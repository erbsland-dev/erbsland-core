.. index::
    single: Version

*******
Version
*******

Introduction
============

:cpp:class:`Version <erbsland::unit::Version>` stores a four-part version as major, minor, revision, and build number.
The default value is ``0.0.0.0``.
You can construct versions from raw part values or from typed part values in any order:

.. code-block:: cpp

    auto api = el::Version{1, 2, 0};
    auto selected = el::Version{el::Major{13}, el::Revision{2}}

Precision
~~~~~~~~~

:cpp:func:`compare() <erbsland::unit::Version::compare>` accepts a
:cpp:enum:`VersionPart <erbsland::unit::VersionPart>` as precision.
The selected part is the last compared part.
For example, minor precision compares major and minor, while revision and build are ignored.

Numeric Representation
~~~~~~~~~~~~~~~~~~~~~~

:cpp:func:`toNumber() <erbsland::unit::Version::toNumber>` packs the four 16-bit parts into a ``uint64_t`` as
``major << 48 | minor << 32 | revision << 16 | build``.
This keeps numeric ordering identical to full version ordering.

Version Part
------------

:cpp:enum:`VersionPart <erbsland::unit::VersionPart>` identifies the part of a
:cpp:class:`Version <erbsland::unit::Version>`.
It is also used as comparison precision: the selected part is included, and less-significant parts are ignored.

Version Range
-------------

:cpp:class:`VersionRange <erbsland::unit::VersionRange>` stores optional inclusive minimum and maximum version bounds.
Missing bounds are open-ended, so a default range contains every version.

Containment uses the same precision rules as :cpp:func:`Version::compare <erbsland::unit::Version::compare>`.
If both bounds are present and the minimum is greater than the maximum at the selected precision, the range is empty.

Common Ranges
~~~~~~~~~~~~~

Use :cpp:func:`all() <erbsland::unit::VersionRange::all>` for an open range,
:cpp:func:`atLeast() <erbsland::unit::VersionRange::atLeast>` for a lower-bounded range,
:cpp:func:`atMost() <erbsland::unit::VersionRange::atMost>` for an upper-bounded range,
:cpp:func:`between() <erbsland::unit::VersionRange::between>` for inclusive minimum and maximum bounds, and
:cpp:func:`exact() <erbsland::unit::VersionRange::exact>` for a single version.

Version Unit
------------

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
