..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Loop Drivers

*******************************************
Integrating Native Events with Loop Drivers
*******************************************

A loop can have an empty callback queue while work is waiting in the operating system.
A driver determines what can wake the sleeping loop.
A backend determines what the loop should do with that readiness.
Keeping those responsibilities separate lets the same dispatch rules apply to native input, deadlines, and work posted
from another thread.

Use the default driver with Core's native event sources.
Inject a driver when you own a waiting facility that must participate directly in the loop's sleep.
The public :cpp:class:`EventLoopDriver <erbsland::event::EventLoopDriver>` interface consists of two wait methods and a
thread-safe wake.
A custom driver and backend can agree on additional operations for their own input.
Core's built-in native sources can depend on platform-specific registration interfaces.

One Wait for All Reasons to Run
===============================

Consider a pipe producer that sends half a line and pauses.
A blocking line read inside a callback would prevent the loop from dispatching a timer that expires during the pause.
Periodically checking the pipe from a timer adds polling delay and wakes an idle process.
A driver can instead wait for input and loop requests together, using the loop's next deadline to bound the wait.

.. erbsland-in-practice-begin:: Reading a pipeline on Linux and macOS

The :doc:`log summarizer </demo_apps/event/pipe_log_summary>` counts complete log lines and publishes a running total
once per second.
Its POSIX driver watches two descriptors in one ``poll()`` call.
One carries the log; the other carries wake requests.
Core's scheduler supplies the timer deadline.

.. erbsland-demo::
    :source: event/apps/PipeLogSummary/src/PipeLogSummaryApp.cpp
    :show: f:PipeLogSummaryApp::main
    :sha256: 455177fc8e45fe2a6b49ec0e06162bebee8994a1f256762a419dee5471700a21

.. code-block:: cpp

    auto PipeLogSummaryApp::main() -> el::ExitCode {
        auto counts = LogCounts{};
        const auto driver = std::make_shared<PosixPipeDriver>(_file);
        const auto loop =
            el::EventLoop::create(driver, el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
        const auto timer = loop->createTimer([&counts]() -> void { counts.report("Running"_el); });
        loop->registerBackend(std::make_unique<PipeBackend>(eventRegistry(), [&](const el::ByteBlock &bytes) -> void {
            counts.consume(bytes);
            if (bytes.isEmpty()) {
                timer->stop();
                counts.report("Final"_el);
                loop->quit();
            }
        }));
        timer->startFixedRate(el::Seconds{1});
        loop->run();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
        return el::ExitCode::success();
    }

.. erbsland-demo-end::

The application runs this standalone loop on its calling thread.
The backend is registered before dispatch.
Its consumer updates the counters on that same thread, so the timer can safely read them between input events.
The periodic timer stops when EOF arrives.

.. erbsland-in-practice-end::

Translate Readiness Into One Delivery
=====================================

When the operating-system wait returns, control goes back to the loop.
The loop polls its backends again and dispatches ready work.
Readiness can mean that a read will return bytes, report EOF, or fail.
Keep each read bounded and preserve any incomplete record for the next delivery.

.. erbsland-in-practice-begin:: Following a partial line

.. erbsland-demo::
    :source: event/apps/PipeLogSummary/src/PipeBackend.cpp
    :show: f:PipeBackend::poll, f:PipeBackend::handleEvent
    :sha256: 26fe39dbc7fcd4c20a957106b8be8750df5845a72c798f8d3a62af2663d116b5

.. code-block:: cpp

    void PipeBackend::poll(el::TimePoint) {
        if (_pending) {
            return;
        }
        _pending = _driver->readReadyChunk();
        if (_pending) {
            const auto target = _target.lock();
            if (!target || !target->postFromBackend(el::Event{_inputEvent})) {
                throw el::ApplicationError{"Failed to queue log input."_el};
            }
        }
    }

    auto PipeBackend::handleEvent(const el::Event &event) -> bool {
        if (event.identifier() != _inputEvent) {
            return false;
        }
        if (!_pending) {
            throw el::LogicFailure{"Log event without pending input."_el};
        }
        _consume(*_pending);
        _pending.reset();
        return true;
    }

.. erbsland-demo-end::

The driver remembers input readiness.
The backend consumes it once and reads at most 4 KiB.
It retains that block until its event reaches the front of the queue.
There is only one outstanding block, so another poll cannot overwrite bytes awaiting dispatch.
A refused event ends the run with a diagnostic because the bytes have already left the pipe.

If the block ends with ``ER``, the consumer retains the prefix.
A timer can dispatch and report the previous totals before ``ROR timeout`` and its newline arrive.

.. erbsland-in-practice-end::

Make a Wake Survive the Gap Before Waiting
==========================================

Posting work and entering a native wait occur on different threads.
A wake can arrive after the owner finds its queue empty and before it calls the operating system.
The wake therefore needs state that survives that gap.
Multiple wakes may share a single signal when the loop will recheck the queue on return.

.. erbsland-in-practice-begin:: A readable wake pipe

.. erbsland-demo::
    :source: event/apps/PipeLogSummary/src/PosixPipeDriver.cpp
    :show: f:PosixPipeDriver::wake, f:PosixPipeDriver::pollOnce
    :sha256: d9b6f51e3b1e7f8dfec65d460260af85d3fd19c08e112e233ced21956c531714

.. code-block:: cpp

    void PosixPipeDriver::wake() {
        const char byte = 1;
        while (::write(_wakePipe[1], &byte, 1) < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return; // A full pipe is already readable.
            }
            if (errno != EINTR) {
                throw el::system::PlatformFailure{
                    el::StringFormat{"Cannot wake the log reader (errno {})."_el}.build(errno)};
            }
        }
    }

    void PosixPipeDriver::pollOnce(const int timeout) {
        pollfd descriptors[] = {{_wakePipe[0], POLLIN, 0}, {_input, POLLIN, 0}};
        if (::poll(descriptors, 2, timeout) < 0) {
            if (errno == EINTR) {
                return; // Let the loop recompute the remaining deadline.
            }
            throw el::PlatformError{el::StringFormat{"Polling log input failed (errno {})."_el}.build(errno)};
        }
        if (descriptors[0].revents & POLLIN) {
            std::array<char, 256> bytes;
            if (::read(_wakePipe[0], bytes.data(), bytes.size()) < 0 && errno != EINTR) {
                throw el::PlatformError{"Cannot consume a wake notification."_el};
            }
        }
        if (descriptors[1].revents & (POLLERR | POLLNVAL)) {
            throw el::PlatformError{"The log input descriptor failed."_el};
        }
        if (descriptors[1].revents & (POLLIN | POLLHUP)) {
            _inputReady = true; // A hangup can still have buffered data.
        }
    }

.. erbsland-demo-end::

A byte written before ``poll()`` makes the read end ready immediately.
Both ends are nonblocking.
A full pipe already records a wake, so another producer can return.
The owner consumes a bounded batch of wake bytes and returns to the loop.
Only ``wake()`` touches the write end; input readiness belongs to the waiting thread.

.. erbsland-in-practice-end::

Implement indefinite waiting for an idle loop, bounded waiting for a deadline, and immediate polling for a caller with
zero time to wait.
POSIX uses a negative timeout for indefinite waits and zero for an immediate poll.
Round a positive fractional millisecond up to the next representable timeout.
If a signal interrupts a wait, return to the loop so it recalculates the remaining time against its monotonic deadline.
Restarting the original timeout on every interruption can postpone a due timer indefinitely.

Deliver the Last Bytes Before Closing
=====================================

.. erbsland-in-practice-begin:: A producer exits after its last write

.. erbsland-demo::
    :source: event/apps/PipeLogSummary/src/PosixPipeDriver.cpp
    :show: f:PosixPipeDriver::readReadyChunk
    :sha256: 84451642ce94c1997f7ddefd6a7f6e9512c63564255fa02f4da1e99c13d53f8b

.. code-block:: cpp

    auto PosixPipeDriver::readReadyChunk() -> std::optional<el::ByteBlock> {
        if (!std::exchange(_inputReady, false)) {
            return {};
        }
        std::array<char, 4096> bytes;
        const auto count = ::read(_input, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                return {};
            }
            throw el::PlatformError{el::StringFormat{"Reading log input failed (errno {})."_el}.build(errno)};
        }
        if (count == 0) {
            _input = -1; // poll ignores this slot from now on.
        }
        return el::ByteBlock::fromSpan(std::span{bytes}.first(static_cast<std::size_t>(count)));
    }

.. erbsland-demo-end::

A hangup and readable data may be reported together.
The driver continues reading on each readiness notification until a read returns zero.
It then disables the input descriptor and the backend queues one empty block for EOF.
During dispatch the consumer counts a final unterminated line, prints its report, and requests quit.
The input file and wake pipe remain valid until the run and all uses of the driver finish.
Standard input is borrowed for exclusive reading and stays open.

.. erbsland-in-practice-end::

See :doc:`writing_event_backends` for the backend polling and admission contract and :doc:`running_event_loops_directly`
for the behavior of individual dispatch calls.
