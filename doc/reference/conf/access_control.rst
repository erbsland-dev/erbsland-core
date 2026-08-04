.. index::
    single: Configuration Access Control

****************************
Configuration Access Control
****************************

Every source requested by the parser is passed to an
:cpp:class:`erbsland::conf::AccessCheck <erbsland::conf::AccessCheck>` before it is opened.
This includes the initial source.
The check receives both the requested source and its parent source, when the request originated from an include
directive.

The default :cpp:class:`erbsland::conf::FileAccessCheck <erbsland::conf::FileAccessCheck>` accepts ordinary ELCL files
in the directory of the initial file and, by default, its subdirectories.
This prevents an include from silently escaping the configuration tree.
Applications that load configurations from another trust boundary can install their own check with
:cpp:func:`erbsland::conf::Parser::setAccessCheck <erbsland::conf::Parser::setAccessCheck>`.

A custom check returns the appropriate access result for each request.
It may also throw a configuration ``erbsland::conf::ConfError`` when it needs to report a more specific reason.
Setting a null access check disables includes together with source resolution; it is not a way to bypass access checks.

Interface
=========

.. doxygenclass:: erbsland::conf::AccessCheck
    :members:

.. doxygentypedef:: erbsland::conf::AccessCheckPtr
.. doxygenenum:: erbsland::conf::AccessCheckResult
.. doxygenstruct:: erbsland::conf::AccessSources
    :members:
.. doxygenclass:: erbsland::conf::FileAccessCheck
    :members:

.. doxygentypedef:: erbsland::conf::FileAccessCheckPtr
