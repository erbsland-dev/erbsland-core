.. index::
    single: Configuration Parsing

*********************
Configuration Parsing
*********************

:cpp:class:`erbsland::conf::Parser <erbsland::conf::Parser>` reads one closed source and returns a typed
:cpp:class:`erbsland::conf::Document <erbsland::conf::Document>`. Each parser instance is reentrant and can be used
independently in one thread.
Use a separate instance in each concurrently executing thread.

Parsing Files and Text
======================

The convenience methods cover the common file and in-memory cases:

.. code-block:: cpp

    using namespace el::text::literals;

    el::conf::Parser parser;
    auto fromFile = parser.parseFileOrThrow(el::path::Path{"settings.elcl"_el});
    auto fromText = parser.parseTextOrThrow("[server]\nport = 8443\n"_el);

The equivalent source-level API is useful when an application implements its own source:

.. code-block:: cpp

    auto source = el::conf::Source::fromString("enabled = yes\n"_el);
    auto document = parser.parseOrThrow(source);

``parseOrThrow`` reports every failure as ``erbsland::conf::ConfError``.
``parse`` returns a null document instead and stores its
:cpp:class:`erbsland::conf::ConfErrorContext <erbsland::conf::ConfErrorContext>` for
:cpp:func:`erbsland::conf::Parser::lastError <erbsland::conf::Parser::lastError>`.
Regular-expression literals are retained without compilation, so invalid pattern syntax is reported later as
:cpp:class:`re::RegExError <erbsland::re::RegExError>` when the expression is compiled or first used.

Customization
=============

Includes are resolved by a :cpp:class:`erbsland::conf::SourceResolver <erbsland::conf::SourceResolver>` and approved by
an :cpp:class:`erbsland::conf::AccessCheck <erbsland::conf::AccessCheck>`.
The defaults implement restricted file-based includes.
A :cpp:class:`erbsland::conf::SignatureValidator <erbsland::conf::SignatureValidator>` can be installed for signed
documents.
Set these collaborators before starting a parse; null resolver or access-check values disable include handling.

Interface
=========

.. doxygenclass:: erbsland::conf::Parser
    :members:
.. doxygenclass:: erbsland::conf::TestFormat
    :members:
