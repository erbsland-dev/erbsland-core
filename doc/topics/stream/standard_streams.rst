..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Standard Input
    single: Streams; Standard Output
    single: Streams; Standard Error
    single: Streams; Redirection

****************
Standard Streams
****************

Every command-line process starts with byte-oriented channels for input, regular output, and diagnostics.
Application code, however, normally wants to read and write Unicode text through the same bounded, error-aware
interfaces it uses for files, pipes, and in-memory streams.
The standard text streams provide this bridge.
They keep native handles and platform details out of application code while preserving the text-stream rules for
encoding, atomic writes, timeouts, and failures.

Standard streams also form an integration boundary.
A command can write ordinary results to a terminal or a pipeline without knowing which one is attached, tests can
replace input with deterministic data, and callers can capture output without changing the component that produces it.
Stable process-wide proxies make these substitutions transparent to code that already holds a stream pointer.

This page explains that proxy model, shows when to use the concise ``io`` helpers or the stream objects directly, and
develops safe patterns for input replacement, output capture, and nested redirection.
It also explains where operation timeouts, stream failures, process-wide state, and output delivery require an explicit
application policy.

Overview: Follow the Stable Proxy Model
=======================================

:cpp:func:`stdIn() <erbsland::stream::stdIn>` returns a shared
:cpp:class:`TextInputStream <erbsland::stream::TextInputStream>` proxy.
:cpp:func:`stdOut() <erbsland::stream::stdOut>` and :cpp:func:`stdErr() <erbsland::stream::stdErr>` return separate,
shared :cpp:class:`TextOutputStream <erbsland::stream::TextOutputStream>` proxies.
Repeated calls return the same proxy objects.
The proxies do not permanently wrap one native destination; each operation resolves the target that is active at that
moment.

.. mermaid::

    classDiagram
        class TextInputStream {
            +read(maximum) StreamReadResult
            +readLine(maximum) StreamReadResult
            +readAll(maximum) StreamReadResult
        }
        class TextOutputStream {
            +write(text) StreamWriteStatus
            +writeLine(text) StreamWriteStatus
            +print(values) StreamWriteStatus
            +printLine(values) StreamWriteStatus
            +flush() StreamWriteStatus
        }
        class StandardStreams {
            +stdIn() TextInputStreamPtr
            +stdOut() TextOutputStreamPtr
            +stdErr() TextOutputStreamPtr
            +redirectStdIn(input) StandardStreamRedirect
            +redirectStdOut(output) StandardStreamRedirect
            +redirectStdErr(error) StandardStreamRedirect
            +redirectStandardStreams(output, error) StandardStreamRedirect
        }
        class io {
            +write(text) StreamWriteStatus
            +writeLine(text) StreamWriteStatus
            +print(values) StreamWriteStatus
            +printLine(values) StreamWriteStatus
            +printError(values) StreamWriteStatus
            +printErrorLine(values) StreamWriteStatus
        }
        class StandardStreamRedirect {
            +isActive() bool
            +reset()
        }
        StandardStreams ..> TextInputStream : returns stable proxy
        StandardStreams ..> TextOutputStream : returns stable proxies
        StandardStreams ..> StandardStreamRedirect : creates guard
        io ..> StandardStreams : resolves output proxy per call

The indirection is deliberate.
If a parser caches ``stdIn()`` during construction, a later
:cpp:func:`redirectStdIn() <erbsland::stream::redirectStdIn>` still affects that parser.
The same rule applies to cached output and error pointers.
Library components therefore do not need a special injection mechanism merely to participate in a scoped integration
test or capture.

Each redirect function returns a move-only
:cpp:class:`StandardStreamRedirect <erbsland::stream::StandardStreamRedirect>`.
The guard keeps the replacement alive and restores the previous target when it is destroyed or when ``reset()`` is
called.
Passing an empty replacement or trying to use a standard proxy as its own replacement throws
:cpp:class:`StreamError <erbsland::stream::StreamError>` because either case would leave the proxy without a valid
forwarding target.

Choose Concise Helpers for Everyday Output
==========================================

The functions in ``stream::io`` are the shortest way to write to the currently active standard output or error target.
Use :cpp:func:`io::write() <erbsland::stream::io::write>` and
:cpp:func:`io::writeLine() <erbsland::stream::io::writeLine>` for text that is already built.
Use :cpp:func:`io::print() <erbsland::stream::io::print>` and
:cpp:func:`io::printLine() <erbsland::stream::io::printLine>` to combine text, numbers, characters, and formatting
objects.
The ``printError...`` variants provide the same formatting interface for standard error.

These helpers do not bypass the stream contract.
They resolve ``stdOut()`` or ``stdErr()`` on every call and return the underlying
:cpp:class:`StreamWriteStatus <erbsland::stream::StreamWriteStatus>`.
``Success`` means that the entire atomic call was accepted; ``Timeout`` means none of that call was accepted before the
target's deadline.
A failed or unavailable native destination throws ``StreamError``.

For a timeout, choose an application policy rather than ignoring the result.
An interactive display may retry the unchanged call within a finite overall budget, while a command-line report often
does better to stop and return a failure because silently omitting a record would make its output unreliable.
No public stream operation itself waits without a bound, but an unlimited retry loop would recreate that problem at the
application level.

The following demo uses ``printLine()`` for a line assembled from text and a number, and ``writeLine()`` for text that
is already complete.
It checks both atomic calls and stops on the first timeout, which avoids an unbounded wait and avoids retrying an
earlier line that was already accepted.
It then checks ``flush()`` because this short-lived command must deliver its queued output before returning.
A ``StreamError`` is translated where the rehearsal context can be added without discarding the original exception.

.. erbsland-demo::
    :source: stream/StandardStreams/EasyOutput.cpp
    :exec: stream/standard_streams --demo WriteStandardOutput
    :source-sha256: 74e7f2de13ef06340474cd42565191276b0c0f45913ed460594ed315b118c090

.. code-block:: cpp

    /// Write concise standard output with a bounded failure policy.
    /// The `io` helpers resolve the active `stdOut()` target for every call. Each call is atomic, so timeout means that
    /// none of that call was accepted. Flush after the final write when delivery must complete before continuing.
    void announceRehearsalPlan() {
        try {
            if (el::io::printLine("Prova d'orchestra — tempo iniziale: "_el, 88, " bpm"_el).isTimeout() ||
                el::io::writeLine("Accelerando dalla battuta 17."_el).isTimeout()) {
                throw el::RuntimeError{"The rehearsal announcement timed out."_el};
            }
            if (el::stdOut()->flush().isTimeout()) {
                throw el::RuntimeError{"Sending the rehearsal program timed out."_el};
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The rehearsal program could not be written."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Prova d'orchestra — tempo iniziale: 88 bpm
    Accelerando dalla battuta 17.

.. erbsland-demo-end::

Keep Results and Diagnostics on Separate Channels
=================================================

Standard output and standard error have different roles even when both currently appear in the same terminal.
Write the command's primary result to ``stdOut()`` so a caller can redirect or pipe it as data.
Write warnings, progress explanations, and failure details to ``stdErr()`` so those messages remain visible without
contaminating the result stream.
This distinction is particularly important for output consumed by another program.

Use the proxy objects directly when code needs the full ``TextOutputStream`` interface, wants to pass a destination to
another component, or benefits from making the selected channel explicit.
Use the ``io`` helpers when a short call at the current site communicates the same intent more clearly.
Both forms use exactly the same active targets and result semantics.

The two channels are independent.
Success on standard output says nothing about standard error, and there is no cross-stream atomicity or ordering
guarantee.
Check the status of each operation that matters.
If a protocol requires one indivisible record, keep that record on one stream and issue it as one ``print()`` or
``write()`` call rather than splitting it across the two channels.

The demo emits a machine-readable result on standard output and a human-readable warning on standard error.
Each write has its own timeout path because either destination may be redirected or stalled independently.
It flushes both channels before returning because acceptance into one stream's queue is not delivery to its native
destination.
The surrounding ``try`` block distinguishes those normal bounded timeouts from a broken stream, which is reported with
the context of publishing the rehearsal result.

.. erbsland-demo::
    :source: stream/StandardStreams/StandardOutputAndError.cpp
    :exec: stream/standard_streams --demo WriteStandardError
    :source-sha256: f53905a668a907dd63431f8c3160bf9e7691f6ce795602e94ba47a617e4fdbf4

.. code-block:: cpp

    /// Keep regular results and human-readable diagnostics on separate standard streams.
    /// `stdOut()` and `stdErr()` return stable process-wide proxies. Check each channel independently.
    /// A bounded timeout is ordinary flow control, while a failed destination throws `StreamError`.
    /// Flush both channels before the command exits.
    void reportRehearsalResult() {
        try {
            if (el::stdOut()->printLine("tempo_bpm=88"_el).isTimeout()) {
                throw el::RuntimeError{"Writing the result timed out."_el};
            }
            if (el::stdErr()->printLine("Warning: measure 24 has no dynamic marking."_el).isTimeout()) {
                throw el::RuntimeError{"Writing the warning timed out."_el};
            }
            if (el::stdOut()->flush().isTimeout() || el::stdErr()->flush().isTimeout()) {
                throw el::RuntimeError{"Sending the rehearsal result timed out."_el};
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The rehearsal result could not be published."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    tempo_bpm=88
    Warning: measure 24 has no dynamic marking.

.. erbsland-demo-end::

Make Standard Input Deterministic with Scoped Redirection
=========================================================

Direct terminal input depends on a person, while redirected process input may come from a file or another command.
Application parsers should not need to distinguish these origins.
They can consume the ``TextInputStream`` returned by ``stdIn()`` and handle ``Data``, ``Finished``, ``Timeout``, and
``StreamError`` in the same way as for any other text source.

``redirectStdIn()`` is useful when an integration test, embedded command runner, or higher-level workflow already has a
text stream that should act as process input.
Create the replacement stream first, install it only for the operation that needs it, and let the guard restore the
previous target automatically.
This gives the tested code realistic access through ``stdIn()`` without changing production signatures merely to feed
fixture data.

Redirection does not remove the need for input bounds.
Pass a maximum to ``readLine()`` or ``readAll()`` so malformed input cannot force unbounded allocation, and apply a
finite attempt or elapsed-time budget around repeated timeouts.
Treat ``Finished`` as normal end of input, not as a stalled source.
When a read fails, catch ``StreamError`` where you know which logical input was being parsed and can add that context.

The hidden part of the demo creates a small text file and opens it as the deterministic replacement.
The documented function obtains the standard-input proxy *before* installing the redirect, proving that an existing
pointer follows the active target.
It reads a line of at most 40 code points, retries at most three bounded operations, reports an empty input separately,
and relies on the guard destructor to restore the original standard input on every return or exception path.

.. erbsland-demo::
    :source: stream/StandardStreams/IntegerFormatting.cpp
    :exec: stream/standard_streams --demo ReadRedirectedInput
    :source-sha256: 853ffb88bac81693dbe3ef932cf679c5147829a03cb0b455b20e29f18cea8dff

.. code-block:: cpp

    /// Redirect standard input to a supplied text stream for deterministic parsing or tests.
    /// A stable proxy follows the replacement even when obtained before the redirect.
    /// A finite line length and attempt count bound both memory use and time spent on a source that repeatedly stalls.
    auto readTempoFrom(el::TextInputStreamPtr input) -> el::String {
        constexpr auto cMaximumLineLength = el::CpLength{40U};
        constexpr auto cMaximumAttempts = 3U;
        const auto standardInput = el::stdIn();

        try {
            auto redirect = el::redirectStdIn(std::move(input));
            for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
                const auto result = standardInput->readLine(cMaximumLineLength);
                if (result.hasData()) {
                    return result.data();
                }
                if (result.isFinished()) {
                    throw el::RuntimeError{"Standard input does not contain a tempo marking."_el};
                }
            }
            throw el::RuntimeError{"Too many timeouts occurred while reading the tempo."_el};
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The tempo could not be read from standard input."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Read tempo: Andante

.. erbsland-demo-end::

Protect Native Standard-Input Buffers
=====================================

Use :cpp:class:`SensitiveInputScope <erbsland::stream::io::SensitiveInputScope>` while reading secret text from the
process-native standard input pipeline.
Requests are process-wide, thread-safe, and counted, so nested components can independently request protection.
The final scope erases decoder, retained text, and byte buffers and may discard unread input.
The native target stays stable behind ``stdIn()`` while its unified decoder changes storage policy in place.
An epoch-checked protected transfer prevents a native read that crossed the final transition from publishing its bytes.

.. code-block:: cpp

    auto outer = el::stream::io::SensitiveInputScope{};
    {
        auto inner = el::stream::io::SensitiveInputScope{};
        const auto result = el::stdIn()->readLine(el::CpLength{1024U});
        consumeSecret(result.data());
    }
    outer.reset();

Manual lifetimes can stop out of nesting order:

.. code-block:: cpp

    auto first = el::stream::io::startSensitiveInput();
    auto second = el::stream::io::startSensitiveInput();
    el::stream::io::stopSensitiveInput(std::move(first));
    el::stream::io::stopSensitiveInput(std::move(second));

The token identifier and source location are diagnostic aids.
Invalid, moved-from, or already stopped tokens raise ``LogicError``.
These scopes affect only the process-native pipeline; a third-party stream installed with ``redirectStdIn()`` retains
its own buffering policy.

Capture Output Without Changing the Producer
============================================

Scoped output redirection is useful when existing code writes through ``stdOut()`` but a caller needs its text in
memory.
Typical examples include integration tests, command embedding, report previews, and tools that post-process another
component's console output.
The producer remains unaware of the capture and continues to use the standard-output API.

Create a :cpp:class:`AnyStringBuilderStream <erbsland::stream::AnyStringBuilderStream>`, redirect standard output to it,
and keep the guard in the narrowest scope that covers the producer call.
Once the guard is gone, standard output again points to the previous target and the captured string can safely be
printed, compared, or returned.
Printing the capture while the redirect is still active would append it to itself instead of displaying it on the
original output target.

An in-memory builder performs no external I/O and is always ready while open, so retrying timeout is unnecessary.
The demo checks for timeout anyway as an invariant: if a future change made the known in-memory target reject a write,
silently returning an incomplete capture would hide a programming error.
Automatic guard destruction also restores standard output when producing the rehearsal notes throws.

.. erbsland-demo::
    :source: stream/StandardStreams/FloatFormatting.cpp
    :exec: stream/standard_streams --demo CaptureOutput
    :source-sha256: 20e188ad71c6eebef1c370d169cbf15478a3ef8b68d3b2954d16d06a11e862a3

.. code-block:: cpp

    /// Capture output from code that writes to the standard-output proxy.
    /// Keep the redirect guard in a narrow scope so automatic restoration also covers early returns and exceptions.
    /// A `AnyStringBuilderStream` performs no external I/O, therefore timeout would violate an in-memory stream invariant.
    auto captureRehearsalNotes() -> el::String {
        const auto capture = el::AnyStringBuilderStream::create();
        {
            auto redirect = el::redirectStdOut(capture);
            if (el::io::printLine("Moderato, 96 bpm"_el).isTimeout() ||
                el::io::printLine("Rallentando nelle ultime quattro battute"_el).isTimeout()) {
                throw el::LogicError{"An in-memory standard-output capture unexpectedly timed out."};
            }
        }
        return capture->takeString();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Captured output:
    Moderato, 96 bpm
    Rallentando nelle ultime quattro battute

.. erbsland-demo-end::

Compose Redirections in Strictly Nested Scopes
==============================================

Redirections form a stack only when their lifetimes are strictly nested.
An inner guard records the outer target as the value to restore, so destroying the inner guard resumes the outer
capture; destroying the outer guard then resumes the original process target.
This makes it possible to isolate one specialized part of a larger captured operation.

The reverse destruction order is part of the usage contract.
Do not call ``reset()`` on an outer guard while an inner redirect is still active, and do not let guards owned by
unrelated components overlap unpredictably.
Restoring an older target out of order breaks the logical stack and can send later output to the wrong destination.
Lexical scopes make the intended order visible and let C++ enforce it during normal and exceptional exits.

The demo writes a main musical theme to an outer destination, temporarily diverts one variation to an inner destination,
and then continues the main theme after the inner guard is destroyed.
Every write still checks its bounded result, and ``StreamError`` receives capture-specific context.
The hidden wrapper supplies two in-memory destinations and displays their contents only after both redirects have ended.

.. erbsland-demo::
    :source: stream/StandardStreams/LowLevelWrite.cpp
    :exec: stream/standard_streams --demo UseNestedRedirects
    :source-sha256: 26e6214dd05d026ff8189fc47d9909ffc17aea8b8c020aa962ecde2b48b66dc6

.. code-block:: cpp

    /// Compose standard-output redirections in strictly nested scopes.
    /// Each guard restores the target that was active when it was created.
    /// Destroy the guards in reverse order so the outer destination resumes after the inner capture finishes.
    void captureMusicalVariation(const el::TextOutputStreamPtr &outer, const el::TextOutputStreamPtr &inner) {
        try {
            auto outerRedirect = el::redirectStdOut(outer);
            if (el::io::writeLine("Tema principale"_el).isTimeout()) {
                throw el::RuntimeError{"Capturing the theme timed out."_el};
            }
            {
                auto innerRedirect = el::redirectStdOut(inner);
                if (el::io::writeLine("Variazione veloce"_el).isTimeout()) {
                    throw el::RuntimeError{"Capturing the variation timed out."_el};
                }
            }
            if (el::io::writeLine("Ripresa del tema"_el).isTimeout()) {
                throw el::RuntimeError{"Capturing the reprise timed out."_el};
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The musical sections could not be captured."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Outer:
    Tema principale
    Ripresa del tema
    Inner:
    Variazione veloce

.. erbsland-demo-end::

Treat Redirects as Shared Process State
=======================================

The standard proxies and their current targets belong to the complete process, not to one thread.
A redirect therefore affects every component and thread that uses the matching proxy while the guard is active.
Use redirects during controlled startup, tests with serialized access, or narrowly scoped operations for which other
standard-stream users are paused or coordinated.
For concurrent application work, passing an explicit ``TextInputStream`` or ``TextOutputStream`` to the component is
usually a clearer isolation boundary.

Use :cpp:func:`redirectStandardStreams() <erbsland::stream::redirectStandardStreams>` when standard output and standard
error must change under one guard.
This prevents a gap in which one channel has been replaced and the other still points to its old target.
It does not merge the channels or make their writes jointly atomic; each replacement remains an independent text stream.

The proxies are non-owning process infrastructure.
Calling ``close()`` or ``abort()`` on ``stdIn()``, ``stdOut()``, or ``stdErr()`` does not close the native process
channel and does not deactivate the proxy.
Code that owns an ordinary replacement stream remains responsible for that stream's lifecycle after the redirect ends.
Do not take ownership of or replace native handles behind the proxy; use the redirect API so cached pointers continue to
behave consistently.

A successful output write means the complete request was accepted by the stream, but buffered work may still be pending.
Call :cpp:func:`flush() <erbsland::stream::OutputStream::flush>` when the application must hand queued output to the
native destination before continuing or exiting, and check its ``Success`` or ``Timeout`` result.
Flush is also bounded and may throw ``StreamError`` if delivery fails.
