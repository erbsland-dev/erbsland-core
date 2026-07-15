..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Coroutines
    single: Streams; Async Generator
    single: Streams; Coroutine Ownership

*****************
Coroutine Streams
*****************

.. erbsland-draft::

Coroutine stream methods move one bounded synchronous operation onto the coroutine worker service.
This page covers eager tasks, asynchronous block and line generators, timeout preservation, shared ownership,
cancellation, and continuation execution.

Await Bounded Input
===================

``coRead...`` methods return an eager :cpp:class:`CoTask <erbsland::util::CoTask>`.
Work can begin before the caller awaits it.
Awaiting consumes the single result and resumes when the matching bounded read completes.

The result is unchanged from synchronous input: callers still handle ``Data``, ``Finished``, and ``Timeout``.
Exceptions raised on the worker are rethrown when the task result is observed.

.. erbsland-demo::
    :source: stream/CoroutineStreams/AwaitByteRead.cpp
    :exec: stream/coroutine_streams --demo AwaitByteRead
    :source-sha256: 2004f86b2243c0c2068fb455c2edd2283fc6cd2bac5cd7e410a368c71d96c67b

.. code-block:: cpp

    /// Stream `CoTask` operations start eagerly and run the matching bounded synchronous call on the worker service.
    /// Awaiting preserves the normal result status and rethrows stream errors at the observation point.
    void awaitByteRead() {
        const auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{37U});
        auto task = readRiverGauge(input);
        waitForTask(task);
        el::io::printLine("Vandstand: "_el, task.result(), " cm"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Vandstand: 37 cm

.. erbsland-demo-end::

Await Owned Atomic Output
=========================

Asynchronous output takes an owned ``ByteBlock`` or ``String`` so no borrowed view outlives the caller's frame.
It preserves atomic acceptance: success means the complete request was queued, while timeout means none was accepted.
Use it when waiting for output back pressure must not stall a parser or event-loop coroutine.

.. erbsland-demo::
    :source: stream/CoroutineStreams/AwaitTextWrite.cpp
    :exec: stream/coroutine_streams --demo AwaitTextWrite
    :source-sha256: aaaf1ed980464f71ea51e9cd1e38acb6557c8291ac814f2ef77fb3dd2591548c

.. code-block:: cpp

    /// Asynchronous text output owns its string until the complete atomic request is accepted.
    /// This is useful when a producer coroutine must not wait for output back pressure on its current thread.
    void awaitTextWrite() {
        const auto output = el::StringBuilderStream::create();
        auto task = output->coWriteLine(el::String{"Flodprofil: rolig strøm ved østbredden"_el});
        waitForTask(task);

        el::io::printLine("Skrivning accepteret: "_el, task.result().isSuccess());
        el::io::print(output->takeString());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skrivning accepteret: true
    Flodprofil: rolig strøm ved østbredden

.. erbsland-demo-end::

Generate Byte or Text Blocks
============================

``coReadBlocks()`` returns a lazy :cpp:class:`CoAsyncGenerator <erbsland::util::CoAsyncGenerator>`.
Each ``next()`` operation is awaited and yields an owned read result.
Timeout results are visible to the consumer; ``Finished`` completes the generator without yielding another value.
Only one ``next()`` operation may be outstanding.

.. erbsland-demo::
    :source: stream/CoroutineStreams/ProcessBlockGenerator.cpp
    :exec: stream/coroutine_streams --demo ProcessBlockGenerator
    :source-sha256: 8b0bb1164b9eb8a3ca549351196b3b941cc4a58c7d59ac38a1fe62ce61f85141

.. code-block:: cpp

    /// `coReadBlocks()` is a lazy, single-pass sequence of owned blocks.
    /// Advancing it is asynchronous; data and timeout results are yielded, while end-of-stream completes the generator.
    void processBlockGenerator() {
        const auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{1U, 1U, 2U, 3U, 5U}, 2U);
        auto task = countRiverBlocks(input);
        waitForTask(task);
        el::io::printLine("Data blocks from the stream: "_el, task.result());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Data blocks from the stream: 3

.. erbsland-demo-end::

Generate Complete Lines
=======================

``coReadLines()`` applies the same lazy sequence to decoded text.
Line endings, maximum-length fragments, final unterminated lines, and code-point boundaries match synchronous
``readLine()`` behavior.

.. erbsland-demo::
    :source: stream/CoroutineStreams/ProcessLineGenerator.cpp
    :exec: stream/coroutine_streams --demo ProcessLineGenerator
    :source-sha256: 1b188bf5aa5287981eea0b0e50f8156db9365727e7daccf314aa5ba54d6555a6

.. code-block:: cpp

    /// `coReadLines()` combines decoded, code-point-safe line input with asynchronous iteration.
    /// It yields complete lines or bounded fragments and completes normally after the final unterminated line.
    void processLineGenerator() {
        const auto directory = createStreamDemoDirectory("flod"_el);
        const auto path = directory->path() / "målinger.txt"_el;
        path.content().writeTextOrThrow("Nord: 12 cm\nMidte: 37 cm\nSyd: 19 cm"_el);
        const auto input = path.content().openTextInputStream();
        auto task = countRiverLines(input);
        waitForTask(task);
        el::io::printLine("Measurement lines: "_el, task.result());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Measurement lines: 3

.. erbsland-demo-end::

Keep Timeout as Flow Control
============================

Moving work to another thread does not create an unbounded operation.
The configured stream timeout remains active, and the task or generator yields the same timeout result.
The coroutine can then observe cancellation, update progress, or retry.

.. erbsland-demo::
    :source: stream/CoroutineStreams/HandleCoroutineTimeout.cpp
    :exec: stream/coroutine_streams --demo HandleCoroutineTimeout
    :source-sha256: f56fe6dad83e9ebca056478240a64c9ce6c92ad72891f256a309e5032ec8a262

.. code-block:: cpp

    /// Coroutine operations preserve timeout as a normal stream result.
    /// Awaiting moves bounded work away from the caller thread; it does not convert flow control into an exception.
    void handleCoroutineTimeout() {
        const auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{9U}, 1U, 1U);
        auto first = input->coRead(el::ByteLength{1U});
        waitForTask(first);
        el::io::printLine("First coroutine timeout: "_el, first.result().isTimeout());

        auto second = input->coRead(el::ByteLength{1U});
        waitForTask(second);
        el::io::printLine("Anden coroutine fik data: "_el, second.result().hasData());
    }

.. erbsland-ansi::
    :escape-char: ␛

    First coroutine timeout: true
    Anden coroutine fik data: true

.. erbsland-demo-end::

Retain Shared Stream Ownership
==============================

Each inherited coroutine operation obtains a shared pointer to its stream before scheduling work.
This keeps the stream and any backing decorator chain alive even if the initiating scope releases its pointer.
Calling one of these methods on a stack-backed subclass throws ``LogicError``.

.. erbsland-demo::
    :source: stream/CoroutineStreams/RetainStreamOwnership.cpp
    :exec: stream/coroutine_streams --demo RetainStreamOwnership
    :source-sha256: 52dfe613a77352571553095228dab774990d4ccdcb8b1f58512924e370d62c9e

.. code-block:: cpp

    /// An inherited coroutine operation retains shared ownership until its bounded call finishes.
    /// Library streams are therefore factory-created; invoking the same method on a stack-backed custom stream throws
    /// `LogicError` before unsafe suspension can occur.
    void retainStreamOwnership() {
        auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{55U});
        const auto weak = std::weak_ptr<ScriptedByteInputStream>{input};
        auto task = input->coRead(el::ByteLength{1U});
        input.reset();
        waitForTask(task);

        el::io::printLine("Data overlevede ejerens scope: "_el, task.result().hasData());
        el::io::printLine("Stream frigivet efter arbejdet: "_el, weak.expired());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Data overlevede ejerens scope: true
    Stream frigivet efter arbejdet: true

.. erbsland-demo-end::

Cancel by Releasing the Task
============================

Destroying or cancelling an incomplete task requests cancellation.
Already-running bounded native work may finish, but the cancelled user continuation is not resumed afterward.
Destroying a generator ends iteration and destroys its producer frame.

.. erbsland-demo::
    :source: stream/CoroutineStreams/CancelPendingTask.cpp
    :exec: stream/coroutine_streams --demo CancelPendingTask
    :source-sha256: 5ffd6f05095dab380d40fb95184c78f1e72b879a9535a693327eb256a353e70b

.. code-block:: cpp

    /// Destroying or cancelling an incomplete `CoTask` suppresses its continuation.
    /// Already-running bounded stream work may finish, and its retained stream ownership is released afterward.
    void cancelPendingTask() {
        auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{8U}, 1U, 0U, true, true);
        auto weak = std::weak_ptr<ScriptedByteInputStream>{input};
        auto task = input->coRead(el::ByteLength{1U});
        while (input->readCount() == 0U) {
            std::this_thread::yield();
        }
        task.cancel();
        input->release();
        input.reset();

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (!weak.expired() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        el::io::printLine("Annulleret opgave frigav streamen: "_el, weak.expired());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Annulleret opgave frigav streamen: true

.. erbsland-demo-end::

Continuations run on the thread that completes the awaited operation.
There is no automatic return to the initiating thread, so marshal UI or thread-affine work explicitly.
