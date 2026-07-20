.. index::
    single: Configuration Locations

***********************
Configuration Locations
***********************

A :cpp:class:`erbsland::unit::CodeLocation <erbsland::unit::CodeLocation>` stores zero-based line, column, and absolute
code-point indices.
Its textual representation uses customary one-based numbers for display.
A
:cpp:class:`erbsland::conf::Location <erbsland::conf::Location>` combines a code location with a
:cpp:class:`erbsland::conf::SourceIdentifier <erbsland::conf::SourceIdentifier>`, so locations remain meaningful
after the source has been closed.

Source identifiers contain a source name and path.
The name describes the source kind or protocol, while the path identifies the concrete input.
Their textual form is suitable for diagnostics, but callers should compare the structured identifier when identity
matters.

Interface
=========

.. doxygenclass:: erbsland::conf::Location
    :members:
