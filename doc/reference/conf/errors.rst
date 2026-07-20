.. index::
    single: Configuration Errors

********************
Configuration Errors
********************

Configuration diagnostics use :cpp:type:`erbsland::text::String <erbsland::text::String>` for messages and
:cpp:class:`erbsland::path::Path <erbsland::path::Path>` for affected files. A
``erbsland::conf::ConfErrorContext`` combines a short title, a detailed description, a configuration error category, and
any available code location, name path, file path, and source excerpt.
``erbsland::conf::ConfError`` carries this context through the Core exception and diagnostic interfaces.

The exception reason and ``what()`` text are the short title.
Use the diagnostic interface when presenting an error to a user; its structured text document contains the description
and every available context field.
Path, stream, and decoding failures remain attached as causes and are rendered by
:cpp:class:`erbsland::err::DiagnosticHelper <erbsland::err::DiagnosticHelper>`.
The diagnostic's ``toString()`` method renders the same text document through
:cpp:class:`erbsland::text::PlainTextRenderer <erbsland::text::PlainTextRenderer>`. Structured and styled consumers
can render that document without maintaining a separate diagnostic representation.

The throwing parser entry points propagate this structured error.
The non-throwing entry points return a null document and retain the same error in
:cpp:func:`erbsland::conf::Parser::lastError <erbsland::conf::Parser::lastError>`.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    erbsland::conf::Parser parser;
    auto document = parser.parseText("value: ?"_el);
    if (document == nullptr) {
        const auto context = parser.lastError();
        auto error = erbsland::conf::ConfError{context};
        // Present error.diagnostic() to the user.
    }

Error categories distinguish syntax, encoding, I/O, limits, signatures, validation, and internal failures.
Code should normally preserve the original category when adding context around an error.

Interface
=========

.. doxygenclass:: erbsland::conf::ConfError
    :members:
.. doxygenclass:: erbsland::conf::ConfErrorCategory
    :members:
.. doxygenclass:: erbsland::conf::ConfErrorContext
    :members:
