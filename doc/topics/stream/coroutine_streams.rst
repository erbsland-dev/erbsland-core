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
    :sha256: 695e4bfb7f573394016a878dd0c4d474f636c895670f56b28bd3132ee5eaad6f

.. code-block:: cpp

    /// Stream `CoTask` operations start eagerly and run the matching bounded synchronous call on the worker service.
    /// Awaiting preserves the normal result status and rethrows stream errors at the observation point.
    void awaitByteRead() {
        const auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{37U});
        auto task = readRiverGauge(input);
        waitForTask(task);
        el::io::printLine("Vandstand: "_el, task.result(), " cm"_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mVandstand: 37 cm␛[0m

.. erbsland-demo-end::

Await Owned Atomic Output
=========================

Asynchronous output takes an owned ``ByteBlock`` or ``String`` so no borrowed view outlives the caller's frame.
It preserves atomic acceptance: success means the complete request was queued, while timeout means none was accepted.
Use it when waiting for output back pressure must not stall a parser or event-loop coroutine.

.. erbsland-demo::
    :source: stream/CoroutineStreams/AwaitTextWrite.cpp
    :exec: stream/coroutine_streams --demo AwaitTextWrite
    :sha256: d29585190650ba4230445bbe0e19568b626a3f50f35387ab87a2c5255c01828d

.. code-block:: cpp

    /// Asynchronous text output owns its string until the complete atomic request is accepted.
    /// This is useful when a producer coroutine must not wait for output back pressure on its current thread.
    void awaitTextWrite() {
        const auto output = el::AnyStringBuilderStream::create();
        auto task = output->coWriteLine(el::String{"Flodprofil: rolig strøm ved østbredden"_el});
        waitForTask(task);

        el::io::printLine("Skrivning accepteret: "_el, task.result().isSuccess());
        el::io::print(output->takeString());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSkrivning accepteret: true
        Flodprofil: rolig strøm ved østbredden␛[0m

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
    :sha256: 338545a00f375a222441fcfa24a596d190e812102453812b5c55cdbd3c1b9c06

.. code-block:: cpp

    /// `coReadBlocks()` is a lazy, single-pass sequence of owned blocks.
    /// Advancing it is asynchronous; data and timeout results are yielded, while end-of-stream completes the generator.
    void processBlockGenerator() {
        const auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{1U, 1U, 2U, 3U, 5U}, 2U);
        auto task = countRiverBlocks(input);
        waitForTask(task);
        el::io::printLine("Data blocks from the stream: "_el, task.result());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mData blocks from the stream: 3␛[0m

.. erbsland-demo-end::

Generate Complete Lines
=======================

``coReadLines()`` applies the same lazy sequence to decoded text.
Line endings, maximum-length fragments, final unterminated lines, and code-point boundaries match synchronous
``readLine()`` behavior.

.. erbsland-demo::
    :source: stream/CoroutineStreams/ProcessLineGenerator.cpp
    :exec: stream/coroutine_streams --demo ProcessLineGenerator
    :sha256: 97e57ddf40f9ff3e1a90c7eba99410f3b09987e091b8bbe439e48b7cdea99f38

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMeasurement lines: 3␛[0m

.. erbsland-demo-end::

Keep Timeout as Flow Control
============================

Moving work to another thread does not create an unbounded operation.
The configured stream timeout remains active, and the task or generator yields the same timeout result.
The coroutine can then observe cancellation, update progress, or retry.

.. erbsland-demo::
    :source: stream/CoroutineStreams/HandleCoroutineTimeout.cpp
    :exec: stream/coroutine_streams --demo HandleCoroutineTimeout
    :sha256: c064693dff5caccf7441d7e02da6816df399efe78cde8e3eefed4baf3ecda013

.. code-block:: cpp

    /// Coroutine operations preserve timeout as a normal stream result.
    /// Awaiting moves bounded work away from the caller thread; it does not convert flow control into an exception.
    void handleCoroutineTimeout() {
        const auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{9U}, 1U, 1U);
        auto first = input->coRead(el::ByteCount{1U});
        waitForTask(first);
        el::io::printLine("First coroutine timeout: "_el, first.result().isTimeout());

        auto second = input->coRead(el::ByteCount{1U});
        waitForTask(second);
        el::io::printLine("Anden coroutine fik data: "_el, second.result().hasData());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFirst coroutine timeout: true
        Anden coroutine fik data: true␛[0m

.. erbsland-demo-end::

Retain Shared Stream Ownership
==============================

Each inherited coroutine operation obtains a shared pointer to its stream before scheduling work.
This keeps the stream and any backing decorator chain alive even if the initiating scope releases its pointer.
Calling one of these methods on a stack-backed subclass throws ``LogicFailure``.

.. erbsland-demo::
    :source: stream/CoroutineStreams/RetainStreamOwnership.cpp
    :exec: stream/coroutine_streams --demo RetainStreamOwnership
    :sha256: 1c85e28e39205c6ece8175201d1ff43dc7da8d8671c8ac14f76d0812f70ab8b0

.. code-block:: cpp

    /// An inherited coroutine operation retains shared ownership until its bounded call finishes.
    /// Library streams are therefore factory-created; invoking the same method on a stack-backed custom stream throws
    /// `LogicFailure` before unsafe suspension can occur.
    void retainStreamOwnership() {
        auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{55U});
        const auto weak = std::weak_ptr<ScriptedByteInputStream>{input};
        auto task = input->coRead(el::ByteCount{1U});
        input.reset();
        waitForTask(task);

        el::io::printLine("Data overlevede ejerens scope: "_el, task.result().hasData());
        el::io::printLine("Stream frigivet efter arbejdet: "_el, weak.expired());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mData overlevede ejerens scope: true
        Stream frigivet efter arbejdet: true␛[0m

.. erbsland-demo-end::

Cancel by Releasing the Task
============================

Destroying or cancelling an incomplete task requests cancellation.
Already-running bounded native work may finish, but the cancelled user continuation is not resumed afterward.
Destroying a generator ends iteration and destroys its producer frame.

.. erbsland-demo::
    :source: stream/CoroutineStreams/CancelPendingTask.cpp
    :exec: stream/coroutine_streams --demo CancelPendingTask
    :sha256: 2a8f8cfb0fba779b29d76525323eb29da3c08197aaf2ca261a5c6b61dccf69ac

.. code-block:: cpp

    /// Destroying or cancelling an incomplete `CoTask` suppresses its continuation.
    /// Already-running bounded stream work may finish, and its retained stream ownership is released afterward.
    void cancelPendingTask() {
        auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{8U}, 1U, 0U, true, true);
        auto weak = std::weak_ptr<ScriptedByteInputStream>{input};
        auto task = input->coRead(el::ByteCount{1U});
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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAnnulleret opgave frigav streamen: true␛[0m

.. erbsland-demo-end::

Continuations run on the thread that completes the awaited operation.
There is no automatic return to the initiating thread, so marshal UI or thread-affine work explicitly.
