.. index::
    single: Configuration Sources

*********************
Configuration Sources
*********************

A :cpp:class:`erbsland::conf::Source <erbsland::conf::Source>` supplies UTF-8 configuration text line by line.
A source object is deliberately lightweight and closed when created.
The parser opens it once, reads until the end, then closes it and releases any external resources.
Returned lines are owning Core strings and remain valid after subsequent reads.
Sources also provide best-effort excerpts for diagnostics.
In-memory sources scan only the requested area, while stream sources retain the five most recently read lines and may
read up to two immediately following context lines when an error excerpt is requested.

Built-in Sources
================

Use :cpp:func:`erbsland::conf::Source::fromFile <erbsland::conf::Source::fromFile>` for a lazy file source and
:cpp:func:`erbsland::conf::Source::fromString <erbsland::conf::Source::fromString>` for in-memory text:

.. code-block:: cpp

    using namespace el::text::literals;

    auto fileSource = el::conf::Source::fromFile(erbsland::path::Path{"settings.elcl"_el});
    auto textSource = el::conf::Source::fromString("answer = 42\n"_el);

    el::conf::Parser parser;
    auto document = parser.parseOrThrow(textSource);

Custom sources should defer I/O and heavy allocation to ``open()``, report I/O failures as configuration errors, include
newline sequences in returned lines, and make an empty line value signal the end of input.
Each source has a stable :cpp:class:`erbsland::conf::SourceIdentifier <erbsland::conf::SourceIdentifier>` for locations
and access decisions.

Interface
=========

.. doxygenclass:: erbsland::conf::Source
    :members:
.. doxygenclass:: erbsland::conf::SourceIdentifier
    :members:
