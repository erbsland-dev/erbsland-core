..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Demo Applications; PipeLogSummary

************************************
PipeLogSummary and a Live Log Stream
************************************

PipeLogSummary reads a Linux or macOS pipeline and reports cumulative severity counts once a second.
A service can pause without closing its pipe.
The report must still run during that pause, even if the last write stopped in the middle of a label.

Build the ``pipe-log-summary`` target.
From the repository root, try a producer that pauses with only the first two bytes of an error label written:

.. code-block:: sh

    { printf 'INFO listening\nER'; sleep 2; printf 'ROR timeout\n'; } | \
        cmake-build-debug/demo-apps/event/pipe-log-summary

A running report during the pause includes one information line.
The final report includes both lines.
For repeatable totals, pass a finished log file:

.. erbsland-demo::
    :source: event/apps/PipeLogSummary/src/LogCounts.cpp
    :show: f:LogCounts::report
    :files: ../../../demos/event/apps/PipeLogSummary/example-files/service.txt
    :exec: event/pipe-log-summary service.txt
    :show-cmd-line:
    :sha256: b8c59c7ec8facd035673ad1a7e5e7d5525d6d33eeb9714d36ff36827f46cecce

.. code-block:: cpp

    void LogCounts::report(const el::String &label) const {
        el::io::printLine(label, "  INFO="_el, _info, " WARN="_el, _warn, " ERROR="_el, _error, " OTHER="_el, _other);
        el::stdOut()->flush();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/pipe-log-summary service.txt

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFinal  INFO=2 WARN=1 ERROR=1 OTHER=0␛[0m

.. erbsland-demo-end::

Connect the Timer and Input to the Same Wait
============================================

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

The application creates a standalone loop with its input driver.
It attaches a backend that recognizes the driver's readiness flag and creates a timer on this particular loop.
Both consumers use the same counter object on the calling thread.
A timer created on the application's regular main loop would have no dispatcher during this run.

Give Each Chunk One Turn
========================

.. erbsland-demo::
    :source: event/apps/PipeLogSummary/src/PipeBackend.cpp
    :show: f:PipeBackend::attach, f:PipeBackend::poll, f:PipeBackend::handleEvent
    :sha256: de75addb7742b215cc6ecaffe9247bba79d54f1ff5522572ba30a23f57c347a0

.. code-block:: cpp

    void PipeBackend::attach(el::EventBackendTargetWeakPtr target, el::EventLoopDriverWeakPtr driver) {
        _target = std::move(target);
        _driver = std::dynamic_pointer_cast<PosixPipeDriver>(driver.lock());
        if (!_driver) {
            throw el::LogicFailure{"PipeBackend requires a PosixPipeDriver."_el};
        }
    }

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

The attachment checks that the loop supplied the expected driver.
After a native wait, the backend can obtain one owned block and queue an event identifying that block.
The queued event holds its place among other ready work.
A pending block also prevents another backend poll from reading ahead of its consumer.

.. erbsland-demo::
    :source: event/apps/PipeLogSummary/src/LogCounts.cpp
    :show: f:LogCounts::consume, f:LogCounts::endLine
    :sha256: b373c08dc65f2117d964af0daf66ecc208ef069963635a4b79e56f3780ef3c98

.. code-block:: cpp

    void LogCounts::consume(const el::ByteBlock &bytes) {
        for (const auto byte : bytes.span()) {
            if (byte == el::Byte{'\n'}) {
                endLine();
            } else if (_prefix.length() < el::ByteCount{6U}) {
                // Only ASCII severity labels are meaningful. The rest of a line can be arbitrary bytes.
                _prefix.append(el::Char{static_cast<char32_t>(byte.toUInt8())});
            }
        }
        if (bytes.isEmpty() && !_prefix.isEmpty()) {
            endLine();
        }
    }

    void LogCounts::endLine() {
        const auto prefix = el::String{_prefix};
        if (prefix.startsWith("INFO "_el)) {
            ++_info;
        } else if (prefix.startsWith("WARN "_el)) {
            ++_warn;
        } else if (prefix.startsWith("ERROR "_el)) {
            ++_error;
        } else {
            ++_other;
        }
        _prefix.clear();
    }

.. erbsland-demo-end::

Only the first six ASCII bytes can determine whether a line begins with ``INFO ``, ``WARN ``, or ``ERROR ``.
The consumer retains a short prefix across chunks and increments a counter when the newline arrives.
Every other line, including an empty one, contributes to ``OTHER``.
Even a long message without a newline needs only bounded memory.

Finish on EOF
=============

The driver reports EOF only after reading the last available bytes.
The consumer counts any unterminated final line, stops the reporting timer, and prints the final totals before quitting.
A read or polling error instead reaches the application's diagnostic path.
The standalone loop also captures callback failures for the caller to rethrow after dispatch returns.

:doc:`/topics/event/integrating_event_loop_drivers` follows the POSIX calls that keep input, timer deadlines, and cross-thread wakes responsive.
