.. index::
    single: Configuration Source Resolution

*******************************
Configuration Source Resolution
*******************************

Include directives describe a source to load relative to the source containing the directive.
A
:cpp:class:`erbsland::conf::SourceResolver <erbsland::conf::SourceResolver>` receives this context and returns one or
more closed source objects in deterministic order.
It resolves names only; each result is still passed through the configured access check before the parser opens it.

The default :cpp:class:`erbsland::conf::FileSourceResolver <erbsland::conf::FileSourceResolver>` implements file
includes, including supported patterns and recursive requests.
Relative paths are based on the including file.
Its results work with :cpp:class:`erbsland::conf::FileAccessCheck <erbsland::conf::FileAccessCheck>`, which applies the
security boundary independently.

Applications can install another resolver with
:cpp:func:`erbsland::conf::Parser::setSourceResolver <erbsland::conf::Parser::setSourceResolver>`, for example to
address embedded resources or a database.
A custom resolver should create lightweight sources and preserve a stable source identifier; it should not open or parse
them itself.

Interface
=========

.. doxygenclass:: erbsland::conf::FileSourceResolver
    :members:
.. doxygenclass:: erbsland::conf::SourceResolver
    :members:
.. doxygenstruct:: erbsland::conf::SourceResolverContext
    :members:
