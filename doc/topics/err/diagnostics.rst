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
    :sha256: c6032813a36ce6a8b82b568c1d9e737fd919fd52c75ed67f8dbb8ca2ca9b0d54

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
        el::io::printLine("Location: "_el, location.line().toSizeT() + 1, ":"_el, location.column().toSizeT() + 1);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSource: 夜の演奏
        Path: scores/夜の演奏.music
        Location: 4:6␛[0m

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
    :sha256: 1ef3b008a6106ac893bc2125c0074c31fc510d1f395d64e155f95e081e11863c

.. code-block:: cpp

    /// `DiagnosticHelper` converts an exception into one diagnostic or a complete document containing every cause.
    /// Use `toDocument()` at a reporting boundary so valuable information from translated failures is not lost.
    void buildDiagnosticDocument() {
        try {
            try {
                throw std::runtime_error{"MIDI input stopped responding"};
            } catch (const std::runtime_error &) {
                throw el::RuntimeError{"The digital-piano input could not be read."_el, std::current_exception()};
            }
        } catch (const el::RuntimeError &error) {
            auto document = el::DiagnosticHelper{error}.toDocument();
            el::io::printLine(document.toString());
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mError: The digital-piano input could not be read.
        Caused By:
          MIDI input stopped responding␛[0m

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
    :sha256: a1e61732b9f706b713427c33f7c4332aee483648a75324a38bf9d0b5a99bd0a3

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m--- Plain text ---
        Error: The score could not be read.
          The tempo-marking value is out of range.
        Error Source:
          Path:   scores/朝の合奏.music
          Line:   7
          Column: 15
        --- Terminal document ---

          ␛[91;49m␛[1mThe␛[22m ␛[1mscore␛[22m ␛[1mcould␛[22m ␛[1mnot␛[22m ␛[1mbe␛[22m ␛[1mread.

          ␛[39;49m␛[22mThe tempo-marking value is out of range.

        ␛[1mError␛[22m ␛[1mSource:
          ␛[90;49m␛[22mPath:␛[39;49m   scores/朝の合奏.music
          ␛[90;49mLine:␛[39;49m   7
          ␛[90;49mColumn:␛[39;49m 15␛[0m

.. erbsland-demo-end::

:cpp:func:`Diagnostic::toString() <erbsland::err::Diagnostic::toString>` is a convenient plain-text rendering of one
diagnostic.
It is not a substitute for ``DiagnosticHelper::toDocument()`` when an exception may have causes.

Display Text and Localization
=============================

Diagnostic conversion uses the application's :cpp:class:`DisplayText <erbsland::i18n::DisplayText>` registry for
structural wording such as section labels and cause headings.
Register a compiled catalog to customize this wording, or install a translator to translate the resolved texts.
The compiled English Core catalog supplies the built-in defaults.

Domain-authored titles and descriptions are kept separate from this structural wording.
Translate application text in the application, and use the registry to adapt reusable diagnostic labels.
This keeps domain context intact while allowing the same semantic document to be rendered for different audiences.

Read :doc:`writing_custom_exceptions` to create diagnostics for your own domain without coupling the exception itself to
a particular renderer.
