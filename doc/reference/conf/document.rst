.. index::
    single: Configuration Documents

***********************
Configuration Documents
***********************

A :cpp:class:`erbsland::conf::Document <erbsland::conf::Document>` is the root of a parsed configuration value tree.
It provides the complete :cpp:class:`erbsland::conf::Value <erbsland::conf::Value>` interface and can also produce a
flat map from absolute name paths to values.
Values retain their source location, allowing errors and diagnostics to point back to the input after parsing has
finished.

Programmatic Construction
=========================

:cpp:class:`erbsland::conf::DocumentBuilder <erbsland::conf::DocumentBuilder>` creates the same tree structure
without parsing text.
Sections must be introduced before values are added to them; intermediate section maps are created where required.
The builder rejects name collisions and invalid document structures with a syntax error.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    erbsland::conf::DocumentBuilder builder;
    builder.addSectionMap("server"_el);
    builder.addText("name"_el, "gateway"_el);
    builder.addInteger("port"_el, 8443);
    auto document = builder.getDocumentAndReset();

Single names are relative to the most recently added section.
A multi-component name path selects its section explicitly.
Index and text-index components cannot be used by this builder.
Scalar overloads accept the same Core types returned by ``Value``.
Regular expressions must be compiled before they are added, and a null ``re::RegExPtr`` is rejected.

Interface
=========

.. doxygenclass:: erbsland::conf::Document
    :members:
.. doxygenclass:: erbsland::conf::DocumentBuilder
    :members:
