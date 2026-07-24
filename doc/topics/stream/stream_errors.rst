..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Errors
    single: Streams; Diagnostics
    single: Streams; Error Context

*****************************
Stream Errors and Diagnostics
*****************************

.. erbsland-draft::

The stream API separates bounded flow-control results from exceptional I/O and encoding failures.
This page shows how to branch on normal results, inspect ``StreamError``, render diagnostics, retain source context, and
handle malformed text separately.

Separate Results from Failures
==============================

``Timeout`` and ``Finished`` are result states.
Timeout gives control back to the caller without changing the stream into ``Failed``; finished marks normal input
completion.
Native read, write, flush, close, or positioning failures throw
:cpp:class:`StreamError <erbsland::stream::StreamError>`.

.. erbsland-demo::
    :source: stream/StreamErrors/DistinguishResultStates.cpp
    :exec: stream/stream_errors --demo DistinguishResultStates
    :source-sha256: c18ccdff9644da86faa15a84fb6b7d102075b80d36704f3cd3e4ba477d71427b

.. code-block:: cpp

    /// Keep timeout, end-of-stream, and failure on separate control-flow paths.
    /// Timeout is a bounded result, `Finished` is a successful end state, and a stream failure throws `StreamError`.
    void distinguishResultStates() {
        auto input = ScriptedByteInputStream{{7U}, 1U, 1U};
        el::io::printLine("Tiempo agotado: "_el, input.readByte().isTimeout());
        el::io::printLine("Dato marino: "_el, input.readByte().hasData());
        el::io::printLine("Fin normal: "_el, input.readByte().isFinished());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Tiempo agotado: true
    Dato marino: true
    Fin normal: true

.. erbsland-demo-end::

Inspect Structured Context
==========================

``StreamError`` contains :cpp:class:`StreamErrorContext <erbsland::stream::StreamErrorContext>`.
Its title states what failed; its description explains why.
Optional help suggests recovery, path identifies a file-backed source, and platform context carries native details
without nesting a second cause.

.. erbsland-demo::
    :source: stream/StreamErrors/InspectStreamError.cpp
    :exec: stream/stream_errors --demo InspectStreamError
    :source-sha256: 45d452501f99c721d3aae13a7ef379efa860c45e911ca8538a06c32b92dcf1b9

.. code-block:: cpp

    /// `StreamError` carries a structured, user-facing context.
    /// Handlers can inspect its title, description, recovery help, path, and optional native platform context.
    void inspectStreamError() {
        auto input = ScriptedByteInputStream{{1U}};
        input.setFailure(true);
        try {
            static_cast<void>(input.readByte());
        } catch (const el::StreamError &error) {
            el::io::printLine("Title: "_el, error.title());
            el::io::printLine("Description: "_el, error.description());
            el::io::printLine("Estado Failed: "_el, input.state() == el::StreamState::Failed);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Title: Failed to read the scripted stream.
    Description: The simulated sensor source failed.
    Estado Failed: true

.. erbsland-demo-end::

Report the Common Diagnostic
============================

Use ``diagnostic()`` or :cpp:class:`DiagnosticHelper <erbsland::err::DiagnosticHelper>` to transform the exception into
the same renderer-neutral document used by application errors.
This keeps terminal output, logs, and graphical reporting consistent.

.. erbsland-demo::
    :source: stream/StreamErrors/ReportDiagnostic.cpp
    :exec: stream/stream_errors --demo ReportDiagnostic
    :source-sha256: e664271fed5d898656b56edee876a3317bb4fd314475c9bae0401a59b1cfe98d

.. code-block:: cpp

    /// Convert a stream exception into the common diagnostic document used by logs and user interfaces.
    /// This avoids constructing a second message and preserves the original stream-domain context.
    void reportDiagnostic() {
        auto output = ScriptedByteOutputStream{};
        output.setFailure(true);
        try {
            output.flush();
        } catch (const el::StreamError &error) {
            const auto document = el::DiagnosticHelper{error}.toDocument();
            el::io::printLine(document.toString());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Error: Failed to flush the scripted stream.
      The simulated recorder failed.

.. erbsland-demo-end::

Preserve Context Through Wrappers
=================================

Text encoders, temporary streams, and standard proxies delegate context creation to the backing stream's
``StreamErrorSource`` before adding their own context data.
The final exception therefore retains a useful file path and native details even when failure was detected at a
decorated text operation.

.. erbsland-demo::
    :source: stream/StreamErrors/PreserveWrappedContext.cpp
    :exec: stream/stream_errors --demo PreserveWrappedContext
    :source-sha256: 31a03d75d8e7c16d804f9fe66de27f601870e5cea12e087c65181f1025779084

.. code-block:: cpp

    /// Encoded and temporary stream wrappers forward locally created errors to their backing stream.
    /// As a result, a text-level operation can still report the path associated with its file-backed byte source.
    void preserveWrappedContext() {
        const auto directory = createStreamDemoDirectory("océano"_el);
        const auto path = directory->path() / "arrecife.txt"_el;
        const auto output = path.content().openTextOutputStream();
        output->write("coral"_el);
        output->close();
        try {
            static_cast<void>(output->write("pez"_el));
        } catch (const el::StreamError &error) {
            el::io::printLine("Ruta conservada: "_el, error.path().endsWith("arrecife.txt"_el));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Ruta conservada: true

.. erbsland-demo-end::

Handle Encoding Errors Separately
=================================

Strict decoding throws :cpp:class:`EncodingError <erbsland::text::EncodingError>` because malformed text is not a native
stream failure.
Catch it separately when the application can explain invalid input differently from an unavailable file or device.

.. erbsland-demo::
    :source: stream/StreamErrors/HandleEncodingError.cpp
    :exec: stream/stream_errors --demo HandleEncodingError
    :source-sha256: 328fab55d9e00a416c96db46ca1a0f8e34af67a3b77fd672c278eb9d15275d27

.. code-block:: cpp

    /// Encoding failures are text-domain errors rather than stream failures.
    /// Configure strict decoding when malformed marine-observation text must be rejected and handle `EncodingError`
    /// separately from native I/O failures.
    void handleEncodingError() {
        const auto directory = createStreamDemoDirectory("codificación"_el);
        const auto path = directory->path() / "muestra.txt"_el;
        path.content().writeDataOrThrow(el::ByteBlock({0xf0U, 0x28U, 0x8cU, 0x28U}));
        auto options = el::PathReadTextOptions{el::StringEncoding::Utf8};
        options.setEncodingMode(el::EncodingMode::Strict);

        try {
            static_cast<void>(path.content().openTextInputStream(options)->readAll());
        } catch (const el::EncodingError &) {
            el::io::printLine("The malformed UTF-8 sequence was rejected."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    The malformed UTF-8 sequence was rejected.

.. erbsland-demo-end::

After a stream enters ``Failed``, report its original error and replace the stream only at an application-level recovery
boundary.
Do not retry arbitrary requests on the failed instance.
