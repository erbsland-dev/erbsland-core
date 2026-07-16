.. index::
    single: RegExError
    single: Error Handling

*************************
Regular Expression Errors
*************************

Regular-expression parsing, diagnostics and execution failures are reported with
:cpp:class:`RegExError <erbsland::re::RegExError>`. The exception derives from
:cpp:class:`err::RuntimeError <erbsland::err::RuntimeError>` and carries a
:cpp:class:`RegExErrorContext <erbsland::re::RegExErrorContext>`.

.. code-block:: cpp

    try {
        const auto expression = re::RegEx::compile(pattern);
    } catch (const re::RegExError &error) {
        log(error.diagnostic()->toString());
    }

Error Context
=============

The context separates information that applications commonly need to inspect:

*   ``title`` is a concise statement of what failed. The inherited ``reason()`` and ``what()`` contain only this title.
*   ``description`` optionally explains why the operation failed, including limits or relevant values.
*   ``category`` identifies the subsystem or failure kind without duplicating it in the title.
*   ``location`` stores optional, zero-based typed line, column and code-point indices.

The corresponding convenience accessors are available directly on ``RegExError``.
Missing location components return the respective ``noIndex`` value.
``withLineNumber()`` returns a copy with a replaced zero-based line index and preserves every other context field.

Every RE subsystem follows the same message split.
The title states which operation failed, such as ``Failed to parse regular expression`` or
``Failed to assemble regular expression``.
The description states the concrete cause, including relevant values and limits.
Category and location are never duplicated in either text field.

Diagnostics
===========

``diagnostic()`` returns a structured Core diagnostic whose ``toString()`` method renders the complete plain-text
diagnostic, including the title, optional description, category and every available location component.
Human-readable locations are rendered one-based, while the inspection API remains zero-based.
``RegExError::toString()`` provides a compact single-line summary in the form ``<category>: <title>. <description>`` and
deliberately omits structured location details.

Core Strings and Encoding
=========================

Every RE API that receives a Core string uses tolerant decoding.
Malformed UTF-8, UTF-16 or UTF-32 units become U+FFFD and participate normally in pattern parsing, assembler parsing,
replacement parsing and matching.
These string overloads therefore do not throw encoding errors for malformed units.

Custom :doc:`input` implementations remain exception-transparent.
An encoding error or another runtime error thrown by ``read()``, ``peek()``, ``skip()`` or ``createMatch()`` propagates
unchanged and is not converted into ``RegExError``.

Categories
==========

``Parser`` and ``Format`` identify invalid pattern and replacement syntax.
``Assembler`` identifies invalid diagnostic program text.
``Limit`` and ``Timeout`` identify configured resource boundaries.
``Engine`` and ``Internal`` identify execution or invariant failures.

Interface
=========

.. doxygenenum:: erbsland::re::ErrorCategory

.. doxygenfunction:: erbsland::re::toString(const ErrorCategory category) noexcept -> text::StringView
.. doxygenclass:: erbsland::re::RegExError
    :members:
.. doxygenclass:: erbsland::re::RegExErrorContext
    :members:
