.. index::
    single: Match
    single: Match16
    single: Match32
    single: capture group

*******************
The Match Interface
*******************

Match Families
==============

A successful match is represented by one encoding-specific type:

*   :cpp:class:`Match <erbsland::re::Match>` returns :cpp:class:`text::StringView
    <erbsland::text::U8StringView>` content.
*   :cpp:class:`Match16 <erbsland::re::Match16>` returns :cpp:class:`text::U16StringView
    <erbsland::text::U16StringView>` content.
*   :cpp:class:`Match32 <erbsland::re::Match32>` returns :cpp:class:`text::U32StringView
    <erbsland::text::U32StringView>` content.

There is no separate view-result family.
Core string views are read-only owning values, so matches retain their subject storage and return copy-free slices from
it.

Lifetime and Ownership
======================

Match objects are shared pointers and are immutable after creation.
The match keeps the complete subject view alive; every value returned by ``content()`` also owns the referenced storage.
It is safe to retain a match or captured view after the source variable, temporary subject or generator has been
destroyed.

Groups and Positions
====================

:cpp:class:`MatchBase <erbsland::re::MatchBase>` provides ``begin``, ``end``, ``range`` and ``group`` access. Overloads
without a selector address capture group zero, which represents the whole match.
Other groups are selected with
:cpp:type:`CaptureGroupIndex <erbsland::re::CaptureGroupIndex>` or a :cpp:class:`text::StringView
<erbsland::text::U8StringView>` name.

Positions are coordinates in the original encoding:

*   UTF-8 positions count bytes.
*   UTF-16 positions count ``char16_t`` code units, including both units of a surrogate pair.
*   UTF-32 positions count ``char32_t`` code units.

``end()`` identifies the first unit after the captured range.
``content()`` converts the stored coordinates into a native Core string slice without copying.

Interface
=========

.. doxygenclass:: erbsland::re::CaptureGroup
    :members:

.. doxygentypedef:: erbsland::re::CaptureGroupList
.. doxygentypedef:: erbsland::re::CaptureGroupIndex
.. doxygenclass:: erbsland::re::CaptureRange
    :members:
.. doxygentypedef:: erbsland::re::MatchPtr

.. doxygenclass:: erbsland::re::Match
    :members:

.. doxygentypedef:: erbsland::re::MatchGenerator

.. doxygentypedef:: erbsland::re::MatchList
.. doxygentypedef:: erbsland::re::Match16Ptr

.. doxygenclass:: erbsland::re::Match16
    :members:

.. doxygentypedef:: erbsland::re::Match16Generator

.. doxygentypedef:: erbsland::re::Match16List
.. doxygentypedef:: erbsland::re::Match32Ptr

.. doxygenclass:: erbsland::re::Match32
    :members:

.. doxygentypedef:: erbsland::re::Match32Generator

.. doxygentypedef:: erbsland::re::Match32List
.. doxygentypedef:: erbsland::re::MatchBasePtr

.. doxygenclass:: erbsland::re::MatchBase
    :members:
