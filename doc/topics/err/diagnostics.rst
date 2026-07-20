..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Diagnostics; Error Handling
    single: Diagnostic Document
    single: Error Rendering

****************************
Working with Diagnostic Data
****************************

Diagnostics turn exceptions into structured, user-facing information.
This page explains how to inspect a diagnostic, build a complete document from an exception and its causes, and render
that document for logs or an interactive terminal.

One Exception, One Diagnostic
=============================

:cpp:class:`Diagnostic <erbsland::err::Diagnostic>` describes one error. It can expose a source name, source path, and
:cpp:class:`CodeLocation <erbsland::unit::CodeLocation>`, and it converts its domain-specific details into a neutral
:cpp:class:`TextDocument <erbsland::text::TextDocument>`.

Call :cpp:func:`Exception::diagnostic() <erbsland::err::Exception::diagnostic>` when you need the diagnostic for the
current exception only.
The base implementation creates a compact diagnostic from ``toString()``; richer domain errors override it to preserve
structured fields.

.. erbsland-demo::
    :source: err/Diagnostic/InspectDiagnostic.cpp
    :exec: err/diagnostic --demo InspectDiagnostic
    :source-sha256: a73edfd105a9cffa938870f764118b5123d61b9a1d66b4e4112720914ade7aff

.. code-block:: cpp

    /// `diagnostic()` returns the structured description for one exception.
    /// Its source accessors let handlers inspect optional metadata without knowing the concrete exception type.
    void inspectDiagnostic() {
        auto context = el::ApplicationErrorContext{"The score could not be read."_el};
        context.setSourceName("夜の演奏"_el)
            .setSourcePath("scores/夜の演奏.music"_el)
            .setCodeLocation(el::CodeLocation{el::LineIndex{3}, el::ColumnIndex{5}});
        const auto diagnostic = el::ApplicationError{std::move(context), {}}.diagnostic();
        const auto location = diagnostic->location();

        el::io::printLine("Source: "_el, diagnostic->sourceName());
        el::io::printLine("Path: "_el, diagnostic->sourcePath());
        el::io::printLine(
            "Location: "_el, location.line().toSizeT() + 1, ":"_el, location.column().toSizeT() + 1);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Source: 夜の演奏
    Path: scores/夜の演奏.music
    Location: 4:6

.. erbsland-demo-end::

All source metadata is optional.
Check the returned values before displaying them in custom interfaces, and remember that line and column indices are
zero-based values even though the standard diagnostic renderers display them as one-based positions.

Build a Complete Diagnostic Document
====================================

Most reporting boundaries need more than one diagnostic.
:cpp:class:`DiagnosticHelper <erbsland::err::DiagnosticHelper>` normalizes a ``std::exception`` and provides two
conversions:

* :cpp:func:`DiagnosticHelper::toDiagnostic() <erbsland::err::DiagnosticHelper::toDiagnostic>` returns the diagnostic
  for the supplied exception only.
* :cpp:func:`DiagnosticHelper::toDocument() <erbsland::err::DiagnosticHelper::toDocument>` builds the root diagnostic
  followed by every preserved cause.

.. erbsland-demo::
    :source: err/Diagnostic/BuildDiagnosticDocument.cpp
    :exec: err/diagnostic --demo BuildDiagnosticDocument
    :source-sha256: 0935438425ed9f80ed7ea3eb16c0f75ab676586ef2819da9cca66b3cfff31aa9

.. code-block:: cpp

    /// `DiagnosticHelper` converts an exception into one diagnostic or a complete document containing every cause.
    /// Use `toDocument()` at a reporting boundary so valuable information from translated failures is not lost.
    void buildDiagnosticDocument() {
        try {
            try {
                throw std::runtime_error{"MIDI input stopped responding"};
            } catch (...) {
                throw el::RuntimeError{"The digital-piano input could not be read."_el, std::current_exception()};
            }
        } catch (const el::Exception &error) {
            auto document = el::DiagnosticHelper{error}.toDocument();
            el::io::printLine(document.toString());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Error: The digital-piano input could not be read.
    Caused By:
      MIDI input stopped responding

.. erbsland-demo-end::

Use the static
:cpp:func:`DiagnosticHelper::documentFromError() <erbsland::err::DiagnosticHelper::documentFromError>` when you already
have a ``std::exception_ptr`` instead of a caught reference.
Foreign standard exceptions become escaped fallback diagnostics, and unknown exceptions receive a neutral message.

Render the Document for Its Destination
=======================================

A diagnostic document contains semantic nodes rather than terminal control sequences or preformatted log text.
Choose the renderer at the final destination:

When you use :cpp:func:`Application::run() <erbsland::core::Application::run>`, this reporting pipeline is already in
place: ``Application`` builds the complete diagnostic document, including causes, and renders it through its system
output.
Render a document manually only when you need a custom reporting boundary, another destination, a preview, or code that
does not use ``Application``.

* :cpp:func:`TextDocument::toString() <erbsland::text::TextDocument::toString>` or
  :cpp:class:`PlainTextRenderer <erbsland::text::PlainTextRenderer>` produces portable text for logs, files, tests, and
  redirected output.
* :cpp:class:`TerminalDocumentRenderer <erbsland::cterm::TerminalDocumentRenderer>` applies the application's terminal
  style and retains visual hierarchy for interactive output.

.. erbsland-demo::
    :source: err/Diagnostic/RenderDiagnosticDocument.cpp
    :exec: err/diagnostic --demo RenderDiagnosticDocument
    :source-sha256: 431f4c7291e69db5d0c65c1b7de6b9176169f5085192f230dd59768df7090eab

.. code-block:: cpp

    /// Diagnostic documents are renderer-neutral `TextDocument` trees.
    /// The same document can become plain text for logs or styled terminal output for an interactive application.
    void renderDiagnosticDocument() {
        auto context = el::ApplicationErrorContext{
            "The score could not be read."_el,
            "The tempo-marking value is out of range."_el,
        };
        context.setSourcePath("scores/朝の合奏.music"_el)
            .setCodeLocation(el::CodeLocation{el::LineIndex{6}, el::ColumnIndex{14}});
        const auto error = el::ApplicationError{std::move(context), {}};
        const auto document = el::DiagnosticHelper{error}.toDocument();

        el::io::printLine("--- Plain text ---"_el);
        el::io::printLine(document.toString());
        el::io::printLine("--- Terminal document ---"_el);
        const auto renderer = el::cterm::TerminalDocumentRenderer{el::application().systemOutputStyle()};
        renderer.renderTo(*el::application().terminal(), document);
    }

.. erbsland-ansi::
    :escape-char: ␛

    --- Plain text ---
    Error: The score could not be read.
      The tempo-marking value is out of range.
    Error Source:
      Path:   scores/朝の合奏.music
      Line:   7
      Column: 15
    --- Terminal document ---

      ␛[1;91mThe␛[22m ␛[1mscore␛[22m ␛[1mcould␛[22m ␛[1mnot␛[22m ␛[1mbe␛[22m ␛[1mread.

      ␛[22;39mThe tempo-marking value is out of range.

    ␛[1mError␛[22m ␛[1mSource:
      ␛[22;90mPath:␛[39m   scores/朝の合奏.music
      ␛[90mLine:␛[39m   7
      ␛[90mColumn:␛[39m 15

.. erbsland-demo-end::

:cpp:func:`Diagnostic::toString() <erbsland::err::Diagnostic::toString>` is a convenient plain-text rendering of one
diagnostic.
It is not a substitute for ``DiagnosticHelper::toDocument()`` when an exception may have causes.

Display Text and Localization
=============================

Diagnostic conversion accepts an optional :cpp:class:`DisplayTextMap <erbsland::i18n::DisplayTextMap>`.
It supplies structural wording such as section labels and cause headings.
A null pointer selects the library's English defaults; ``Application`` passes its configured display-text map
automatically.

Domain-authored titles and descriptions are kept separate from this structural wording.
Translate application text in the application, and use a display-text map to adapt reusable diagnostic labels.
This keeps domain context intact while allowing the same semantic document to be rendered for different audiences.

Read :doc:`writing_custom_exceptions` to create diagnostics for your own domain without coupling the exception itself to
a particular renderer.
