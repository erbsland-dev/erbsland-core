..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Errors; Reference
    single: Diagnostics; Reference
    single: Error Types and Error Handling
    single: Diagnostics

**********************
Errors and Diagnostics
**********************

Introduction
============

The error domain separates exception handling from diagnostic presentation.
:cpp:class:`Exception <erbsland::err::Exception>` carries a reason and an optional cause;
:cpp:class:`Diagnostic <erbsland::err::Diagnostic>` describes one exception as structured information.
Diagnostic documents preserve that information for plain-text or terminal rendering.

For examples and handling strategies, see :doc:`/topics/err/handling_exceptions` and
:doc:`/topics/err/diagnostics`.

Exception Categories
====================

:cpp:class:`Exception <erbsland::err::Exception>` is the neutral base class for all Erbsland Core exceptions and derives
from ``std::exception``.
Its two main branches define how callers should handle a problem:

* :cpp:class:`RuntimeError <erbsland::err::RuntimeError>` represents recoverable data and operational errors.
  Catch this type, or a more specific subclass, where the application can recover.
  Runtime exception names end in ``Error``.
* :cpp:class:`LogicFailure <erbsland::err::LogicFailure>` represents programming mistakes, broken invariants, or
  exhausted system resources with no practical recovery.
  Its subclasses have names ending in ``Failure``.

Fatal failures remain exceptions: throwing one does not unconditionally terminate the process.
Application boundaries should preserve their diagnostics and propagate them instead of treating them as recoverable
input errors.
Production code must not catch failures to return fallbacks or wrap them in runtime errors.
The ``OrThrow`` suffix identifies a throwing operation; it does not determine whether an exception is recoverable.

Recoverable Errors
------------------

The following domain-neutral types derive from :cpp:class:`RuntimeError <erbsland::err::RuntimeError>`.

.. list-table::
    :header-rows: 1
    :widths: 30 70

    * - Type
      - Meaning
    * - :cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>`
      - A value exceeds numeric bounds or a configured processing limit.
    * - :cpp:class:`NotFoundError <erbsland::err::NotFoundError>`
      - A requested key or name is absent.
    * - :cpp:class:`OverflowError <erbsland::err::OverflowError>`
      - An operation results in an overflow.
    * - :cpp:class:`ParseError <erbsland::err::ParseError>`
      - Text cannot be parsed as the requested value.
        The error can carry a code-point index, byte index, or code location.

Data-conversion factories use ``OutOfRangeError`` for invalid numeric data, ``ParseError`` for malformed encodings, and
``NotFoundError`` for unknown names.
A factory can still report a fatal failure for an invalid algorithm selection or a missing required initialized object.

Fatal Failures
--------------

These types refine :cpp:class:`LogicFailure <erbsland::err::LogicFailure>` by identifying the violated precondition or
capacity constraint.

.. list-table::
    :header-rows: 1
    :widths: 30 70

    * - Type
      - Meaning
    * - :cpp:class:`ParameterFailure <erbsland::err::ParameterFailure>`
      - A developer-supplied parameter violates a precondition, such as an infinite or unrepresentable repeat count.
    * - :cpp:class:`OutOfRangeFailure <erbsland::err::OutOfRangeFailure>`
      - An internal bounds violation or an unsupported system or storage capacity.

Both types represent fatal failures.
The distinction identifies whether an invalid caller parameter directly causes the failure.

Related Text Errors
-------------------

The text domain provides more specific runtime errors:

* :cpp:class:`FormatError <erbsland::text::FormatError>` reports invalid format patterns, mismatched arguments,
  unsupported format options, and format output limit failures.
* :cpp:class:`ParseNumberError <erbsland::text::ParseNumberError>` extends
  :cpp:class:`ParseError <erbsland::err::ParseError>` with the reader status that caused numeric parsing to fail.

Integer parsing reports invalid digits, invalid or disallowed base prefixes, invalid separators, missing digits, and
strict trailing characters as parse errors.

Exception Details and Causes
============================

:cpp:func:`Exception::reason() <erbsland::err::Exception::reason>` returns the stored human-readable reason.
:cpp:func:`Exception::what() <erbsland::err::Exception::what>` exposes it at the standard exception boundary, while
:cpp:func:`Exception::toString() <erbsland::err::Exception::toString>` provides local display text for the exception.

An exception can preserve a separate failure layer as a ``std::exception_ptr``.
Use :cpp:func:`Exception::hasCause() <erbsland::err::Exception::hasCause>` and
:cpp:func:`Exception::cause() <erbsland::err::Exception::cause>` to inspect this optional cause.
Use :cpp:class:`DiagnosticHelper <erbsland::err::DiagnosticHelper>` to include the cause chain in a diagnostic document.

Diagnostics
===========

One Diagnostic or a Complete Document
-------------------------------------

:cpp:class:`Diagnostic <erbsland::err::Diagnostic>` describes one exception, with optional source name, source path,
and :cpp:class:`CodeLocation <erbsland::unit::CodeLocation>` metadata.
Its document representation is a renderer-neutral :cpp:class:`TextDocument <erbsland::text::TextDocument>` tree.

Choose the conversion according to the information you need:

* :cpp:func:`Exception::diagnostic() <erbsland::err::Exception::diagnostic>` returns the diagnostic for one exception.
* :cpp:func:`Diagnostic::toTextDocument() <erbsland::err::Diagnostic::toTextDocument>` converts one diagnostic to a
  document; :cpp:func:`Diagnostic::toString() <erbsland::err::Diagnostic::toString>` renders it as plain text.
* :cpp:func:`DiagnosticHelper::toDiagnostic() <erbsland::err::DiagnosticHelper::toDiagnostic>` adapts a caught
  ``std::exception`` to one diagnostic.
* :cpp:func:`DiagnosticHelper::toDocument() <erbsland::err::DiagnosticHelper::toDocument>` builds a document from the
  exception and its preserved causes.
* :cpp:func:`DiagnosticHelper::documentFromError() <erbsland::err::DiagnosticHelper::documentFromError>` builds the
  document when you already have a ``std::exception_ptr``.

Foreign standard exceptions become escaped fallback diagnostics.
Unknown exceptions receive a neutral message, and excessively deep cause chains are limited.

Building Diagnostic Documents
-----------------------------

:cpp:class:`ErrorDocumentBuilder <erbsland::err::ErrorDocumentBuilder>` supplies a consistent title, optional
description, section headings, and source fields.
Use its document root for domain-specific additions, then take the completed document with
:cpp:func:`ErrorDocumentBuilder::takeDocument() <erbsland::err::ErrorDocumentBuilder::takeDocument>`.
See :doc:`/topics/err/writing_custom_exceptions` for custom exceptions and diagnostics.

The shared document structure keeps related information together:

* Metadata uses ``FieldList`` nodes below a diagnostic section heading.
  Each list aligns its own labels and values.
* Source excerpts use :cpp:class:`CodeSnippetMarker <erbsland::text::CodeSnippetMarker>` and
  :cpp:enumerator:`TextNodeType::CodeSnippet <erbsland::text::TextNodeType::Value::CodeSnippet>` nodes.
* Each cause uses a “Caused By” heading followed by a ``Blockquote``, both with the ``diagnostic-cause`` style.
  The quote contains that cause's diagnostic and, if present, the next heading and quote pair.

Developer-authored contexts and display text are trusted.
Escape external values such as paths, command-line arguments, and native messages before rendering; use
:cpp:func:`TextNode::addEscapedText() <erbsland::text::TextNode::addEscapedText>` when inserting untrusted text into a
document.

Rendering Diagnostic Documents
------------------------------

Use :cpp:class:`PlainTextRenderer <erbsland::text::PlainTextRenderer>` or
:cpp:func:`TextDocument::toString() <erbsland::text::TextDocument::toString>` for logs and redirected output.
Plain rendering prefixes the root title with ``Error:``, adds colons to section headings, and indents ordinary section
content by two spaces, with another two spaces for each cause depth.
Code snippets keep compact indentation and use ``│`` as their gutter.

Use :cpp:class:`TerminalDocumentRenderer <erbsland::cterm::TerminalDocumentRenderer>` for styled terminal output.
The default system-output style gives each cause depth a visible border and reserves the primary error color for the
root diagnostic title.
Cause headings and frames use subdued bright black.
See :doc:`/reference/cterm/terminal_document_renderer` for rendering and style details.

Interface
=========

.. doxygenclass:: erbsland::err::Diagnostic
    :members:

.. doxygentypedef:: erbsland::err::DiagnosticPtr

.. doxygentypedef:: erbsland::err::DiagnosticConstPtr
.. doxygenclass:: erbsland::err::DiagnosticHelper
    :members:
.. doxygenclass:: erbsland::err::ErrorDocumentBuilder
    :members:
.. doxygenclass:: erbsland::err::Exception
    :members:
.. doxygenclass:: erbsland::err::LogicFailure
    :members:
.. doxygenclass:: erbsland::err::NotFoundError
    :members:
.. doxygenclass:: erbsland::err::OutOfRangeError
    :members:
.. doxygenclass:: erbsland::err::OutOfRangeFailure
    :members:
.. doxygenclass:: erbsland::err::OverflowError
    :members:
.. doxygenclass:: erbsland::err::ParameterFailure
    :members:
.. doxygenclass:: erbsland::err::ParseError
    :members:
.. doxygenclass:: erbsland::err::RuntimeError
    :members:
