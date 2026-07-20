.. index::
    single: Configuration Names

*******************
Configuration Names
*******************

ELCL addresses values with typed names.
A regular name selects a named child, an index selects an element by its position, and text and text-index names
represent the corresponding language forms.
:cpp:class:`erbsland::conf::Name <erbsland::conf::Name>` stores one such component without losing its type.

A :cpp:class:`erbsland::conf::NamePath <erbsland::conf::NamePath>` is an ordered sequence of components.
It is used for document lookup, validation rules, errors, and flat document maps.
Builder APIs accept
:cpp:type:`erbsland::conf::NamePathLike <erbsland::conf::NamePathLike>`, allowing a single Core string, a
:cpp:class:`erbsland::conf::Name <erbsland::conf::Name>`, or a complete path where appropriate. Use
:cpp:func:`erbsland::conf::toNamePath <erbsland::conf::toNamePath>` when an owning path is needed explicitly.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    const auto section = erbsland::conf::toNamePath("server"_el);
    const auto value = document->value(section);

Interface
=========

.. doxygenclass:: erbsland::conf::Name
    :members:
.. doxygenclass:: erbsland::conf::NamePath
    :members:

.. doxygentypedef:: erbsland::conf::NamePathLike

.. doxygentypedef:: erbsland::conf::NamePathList

.. doxygenfunction:: erbsland::conf::toNamePath(const NamePathLike &namePathLike) -> NamePath
.. doxygenenum:: erbsland::conf::NameType
