..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: XML Documents

*************
XML Documents
*************

Introduction
============

:cpp:class:`XmlDocument <erbsland::data::xml::XmlDocument>` owns ordered top-level nodes and one root element.
:cpp:class:`XmlNode <erbsland::data::xml::XmlNode>` represents elements, mixed content, and preserved markup;
:cpp:class:`XmlParseOptions <erbsland::data::xml::XmlParseOptions>` bounds incoming text.
For parsing, construction, navigation, serialization, and reference behavior, see
:doc:`/topics/data/reading_and_writing_xml`.

Interface
=========

.. doxygenclass:: erbsland::data::xml::XmlAttribute
    :members:
.. doxygenclass:: erbsland::data::xml::XmlDocument
    :members:
.. doxygenclass:: erbsland::data::xml::XmlFormatOptions
    :members:
.. doxygenclass:: erbsland::data::xml::XmlNode
    :members:
.. doxygenenum:: erbsland::data::xml::XmlNodeType
.. doxygenclass:: erbsland::data::xml::XmlParseOptions
    :members:
