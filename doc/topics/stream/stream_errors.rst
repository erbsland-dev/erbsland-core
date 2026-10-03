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
    :sha256: 3307262d8dd1f6f0a55492ab58a58571c44d85d6de515b5065f1c77cbd556d6f

.. code-block:: cpp

    /// Keep timeout, end-of-stream, and failure on separate control-flow paths.
    /// Timeout is a bounded result, `Finished` is a successful end state, and a stream failure throws `StreamError`.
    void distinguishResultStates() {
        auto input = ScriptedByteInputStream{{7U}, 1U, 1U};
        el::io::printLine("Tiempo agotado: "_el, input.readByte().isTimeout());
        el::io::printLine("Dato marino: "_el, input.readByte().hasData());
        el::io::printLine("Fin normal: "_el, input.readByte().isFinished());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mTiempo agotado: true
        Dato marino: true
        Fin normal: true␛[0m

.. erbsland-demo-end::

Inspecting Stream Error Context
===============================

``StreamError`` contains :cpp:class:`StreamErrorContext <erbsland::stream::StreamErrorContext>`.
Its title states what failed; its description explains why.
Optional help suggests recovery, path identifies a file-backed source, and platform context carries native details
without nesting a second cause.

.. erbsland-demo::
    :source: stream/StreamErrors/InspectStreamError.cpp
    :exec: stream/stream_errors --demo InspectStreamError
    :sha256: a8be3ff42819dcb3ca0c6bfa102ea5e4cc45ea7f81c2bc40dc2eaeea34eb3df8

.. code-block:: cpp

    /// `StreamError` carries a structured, user-facing context.
    /// Handlers can inspect its title, description, recovery help, path, and optional native platform context.
    void inspectStreamError() {
        auto input = ScriptedByteInputStream{{1U}};
        input.setFailure(true);
        try {
            [[maybe_unused]] const auto byte = input.readByte();
            // ... process the byte ...
        } catch (const el::StreamError &error) {
            el::io::printLine("Title: "_el, error.title());
            el::io::printLine("Description: "_el, error.description());
            el::io::printLine("Estado Failed: "_el, input.state() == el::StreamState::Failed);
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mTitle: Failed to read the scripted stream.
        Description: The simulated sensor source failed.
        Estado Failed: true␛[0m

.. erbsland-demo-end::

Rendering Stream Error Diagnostics
==================================

Use ``diagnostic()`` or :cpp:class:`DiagnosticHelper <erbsland::err::DiagnosticHelper>` to transform the exception into
the same renderer-neutral document used by application errors.
This keeps terminal output, logs, and graphical reporting consistent.

.. erbsland-demo::
    :source: stream/StreamErrors/ReportDiagnostic.cpp
    :exec: stream/stream_errors --demo ReportDiagnostic
    :sha256: 4f8ec0419668e5d50dd3dd00982583d1ec514a0913aecb078983192533ae0187

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mError: Failed to flush the scripted stream.
          The simulated recorder failed.␛[0m

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
    :sha256: 9aca15ef09c956f80b2bbbc0e8dc1a952d155be0272bd325837ab546bc843431

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
            [[maybe_unused]] const auto status = output->write("pez"_el);
            // note: real code must handle the returned status.
        } catch (const el::StreamError &error) {
            el::io::printLine("Ruta conservada: "_el, error.path().endsWith("arrecife.txt"_el));
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRuta conservada: true␛[0m

.. erbsland-demo-end::

Handle Encoding Errors Separately
=================================

Strict decoding throws :cpp:class:`EncodingError <erbsland::text::EncodingError>` because malformed text is not a native
stream failure.
Catch it separately when the application can explain invalid input differently from an unavailable file or device.
Enable ``Strict`` only when encoding validity must be enforced at this boundary and the validation requirement takes
priority over decoding time.
Strict conversion and whole-file decoding can be much slower than tolerant processing; see
:doc:`text_input_and_output` for the performance tradeoff and how it applies to incremental streams.

.. erbsland-demo::
    :source: stream/StreamErrors/HandleEncodingError.cpp
    :exec: stream/stream_errors --demo HandleEncodingError
    :sha256: 191547d042a8c4cd1c9ddbb201b7a129d63708ee37d62a9ceefcc0d44c5b1e49

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
            [[maybe_unused]] const auto input = path.content().openTextInputStream(options)->readAll();
            // ... do something with the input ...
        } catch (const el::EncodingError &) {
            el::io::printLine("The malformed UTF-8 sequence was rejected."_el);
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mThe malformed UTF-8 sequence was rejected.␛[0m

.. erbsland-demo-end::

After a stream enters ``Failed``, report its original error and replace the stream only at an application-level recovery
boundary.
Do not retry arbitrary requests on the failed instance.
