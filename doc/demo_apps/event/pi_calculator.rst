..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*************************************************
PiCalculator: Required Results and Optional Views
*************************************************

``PiCalculator`` shows how a continuing computation can deliver required results while its display remains optional.
It separates application lifetime, calculation state, background arithmetic, and presentation.
That separation permits batching, cancellation, and reliable joining even when the user closes the view.
The digit-generation algorithm is incidental here; the walkthrough focuses on the event and ownership protocol.

Four Responsibilities
=====================

``PiCalculatorApp`` owns the main loop lifecycle, configuration, timers, session, and optional view.
``PiSession`` owns the calculation's main-loop state and retains ``DigitHistory`` independently of the view.
``PiWorker`` owns a regular ``std::thread`` that performs arithmetic and waits for demand between batches.
It is not an ``EventThread`` and does not dispatch an event loop.
``PiView`` reads history on main, handles display navigation, and renders the current viewport.

.. mermaid::

    flowchart LR
        subgraph M["Main-loop context"]
            app["Application"] --> session["Session"]
            session --> history["Digit history"]
            app --> view["Optional view"]
            view --> history
        end
        subgraph W["Ordinary worker thread"]
            worker["PiWorker arithmetic and demand wait"]
        end
        session -->|"Request next batch or cancellation"| worker
        worker -->|"Required bound result and completion"| session
        session -.->|"Optional weak update"| view

Command-line handling validates the digit limit and timeout, then queues ``begin()`` on main.
The default application main supplies dispatch.
``begin()`` creates the history, chooses plain output or a view, and constructs the session.
Interactive presentation uses a weak binding, so pending display updates do not keep a closed view alive.
Plain output uses an application callback instead.

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiCalculatorApp.cpp
    :show: f:PiCalculatorApp::begin
    :sha256: 8b0f967231f0680505ca860345dddb4af76d1b462833a8924acf90e7319cb5ff

.. code-block:: cpp

    void PiCalculatorApp::begin() {
        _history = std::make_shared<DigitHistory>();
        PiSession::OnUpdated updated;
        if (_plain) {
            terminal()->print("3."_el);
            updated = [this]() -> void { printNewDigits(); };
        } else {
            terminal()->input().setMode(el::cterm::Input::Mode::Key);
            _view = std::make_shared<PiView>(terminal(), _history);
            // Optional presentation must not keep the view alive after Q or timeout releases it.
            updated = el::bindWeakEvent(std::weak_ptr<PiView>{_view}, &PiView::onUpdated);
        }
        _session = std::make_shared<PiSession>(
            el::currentLoop(), _history, std::move(updated), [this](std::exception_ptr error) -> void {
                onCalculationFinished(std::move(error));
            });
        if (_view) {
            _view->setSession(_session);
        }
        _session->start(_limit);
        _refresh = loop()->createTimer([this]() -> void { tick(); });
        _refresh->startFixedDelay(el::Milliseconds{33});
        if (_timeout.isPositive()) {
            _deadline = loop()->createTimer([this]() -> void { requestExit(); });
            _deadline->startOnce(_timeout);
        }
    }

.. erbsland-demo-end::

Required Delivery Keeps the Operation Alive
===========================================

``PiSession::start()`` passes two strongly bound callbacks to the worker: one accepts digits, the other handles final
completion.
Both retain the session and target its main loop.
These are required operation callbacks, independent of whether a display exists.
The application also retains the session until shutdown has joined its worker.

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiSession.cpp
    :show: f:PiSession::start, f:PiSession::onDigits
    :sha256: 334717d41abc6a89c9f38bb637d793d3d2b0d1c3e1d7ca5f7d69379854c7a65a

.. code-block:: cpp

    void PiSession::start(const std::optional<el::CpCount> limit) {
        if (_started) {
            throw el::LogicFailure{"A pi calculation session starts once."_el};
        }
        // Required results retain the session and always return to its owner loop.
        _worker.start(
            limit,
            el::bindEvent(_events, shared_from_this(), &PiSession::onDigits),
            el::bindEvent(_events, shared_from_this(), &PiSession::onFinished));
        _started = true;
    }

    void PiSession::onDigits(el::String digits) {
        _history->append(std::move(digits));
        if (_onUpdated) {
            _onUpdated();
        }
        // Main accepts the result before asking for more. Optional display delivery never releases a worker slot.
        if (!_stopping) {
            _worker.requestNextBatch();
        }
    }

.. erbsland-demo-end::

Receiving digits appends them to the retained history before requesting another batch.
Optional display notification does not release the worker's next batch: accepting the required result does.
This prevents a disappearing view from stopping progress or losing the completion path.
:doc:`/topics/event/event_bindings` explains strong and weak bound recipients in more detail.

One Outstanding Batch
=====================

The worker starts with demand for one batch.
It produces at most 32 fractional digits per batch, with a time check after arithmetic steps to keep batches responsive.
It then waits on a condition variable until the session requests more or cancellation wakes it.
The time check is not a hard deadline for an individual arithmetic step.
This demand protocol bounds outstanding batch delivery even when main is slower than computation.

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiWorker.cpp
    :show: f:PiWorker::requestNextBatch, f:PiWorker::waitForDemand, f:PiWorker::requestStop
    :sha256: da67924c1a34b2dbca37edc2607b3490f75d1f4d12783fc06964790f0e237126

.. code-block:: cpp

    void PiWorker::requestNextBatch() {
        if (!_started) {
            throw el::LogicFailure{"Cannot request work before starting the calculation."_el};
        }
        const auto lock = std::lock_guard{_mutex};
        if (_stop) {
            return;
        }
        if (_requested) {
            throw el::LogicFailure{"Only one calculation batch may be requested."_el};
        }
        _requested = true;
        _demand.notify_one();
    }

    auto PiWorker::waitForDemand() -> bool {
        auto lock = std::unique_lock{_mutex};
        _demand.wait(lock, [this]() -> bool { return _requested || _stop; });
        if (_stop) {
            return false;
        }
        _requested = false;
        return true;
    }

    void PiWorker::requestStop() noexcept {
        // Take the demand mutex so cancellation cannot arrive between the predicate check and waiting.
        const auto lock = std::lock_guard{_mutex};
        _stop = true;
        _demand.notify_one();
    }

.. erbsland-demo-end::

The demand mutex protects request/wakeup ordering, and the atomic stop flag lets arithmetic notice cancellation.
The worker's arithmetic state is accessed only by that worker.
Main accesses digit history only after a value has crossed the event queue.
The algorithm generating the digits does not change these ownership rules.

Cancellation Leaves Required Dispatch Open
==========================================

An interactive stop request asks the session to stop calculation but keeps the view available.
Quit or timeout calls ``requestExit()``, which immediately releases the optional view and requests cancellation if the
session is still active.
It does not quit the main loop while required completion is still pending.

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiCalculatorApp.cpp
    :show: f:PiCalculatorApp::requestExit
    :sha256: 46f7283fdbafb7c06429ff6eb54574015659de3a9e94fa029d6ac81449e9911e

.. code-block:: cpp

    void PiCalculatorApp::requestExit() {
        _exitRequested = true;
        // Release optional UI immediately, but keep main open for strongly bound required completion.
        _view.reset();
        if (_session->isFinished()) {
            quit();
        } else {
            _session->requestStop();
        }
    }

.. erbsland-demo-end::

The worker wakes from its demand wait or notices cancellation between arithmetic steps, then sends its final callback.
``PiSession::onFinished()`` joins the native worker before notifying the application.
It retires callbacks and its target reference once the operation is terminal.
The application can now quit, or leave the stopped interactive view available.

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiSession.cpp
    :show: f:PiSession::onFinished, f:PiSession::shutdown
    :sha256: fd458e53ab5e1faa7e0038319df090fa688c5233b283a6b7fe6a2a86539f3ae1

.. code-block:: cpp

    void PiSession::onFinished(std::exception_ptr error) {
        _worker.join();
        _finished = true;
        if (_onUpdated) {
            _onUpdated();
        }
        // Retire target-retaining callbacks once the operation is terminal, including on exceptional shutdown.
        _onUpdated = {};
        _events.reset();
        if (auto completion = std::move(_onFinished)) {
            completion(std::move(error));
        }
    }

    void PiSession::shutdown() {
        // Exception cleanup can run after dispatch has ended; joining must not depend on a pending main-loop callback.
        _stopping = true;
        _worker.requestStop();
        _worker.join();
        // A stopped loop may still own callbacks retaining this session. Break the return ownership path.
        _onUpdated = {};
        _onFinished = {};
        _events.reset();
    }

.. erbsland-demo-end::

Operational arithmetic failures are captured as ``exception_ptr`` on the worker and delivered by the same required
completion path.
The application rethrows them on main for diagnostics.
If an exception ends dispatch before that path completes, application cleanup calls ``shutdown()``: it requests stop and
joins without waiting for another main-loop callback, then breaks retained callback/target paths.
This is why cancellation must be able to wake the ordinary worker independently of event dispatch.

A Finite Calculation
====================

The following run calculates 100 fractional digits and exits automatically in plain mode.
It exercises the session/worker protocol without requiring interactive terminal input.
Interactive use also supports stop, quit, follow, and scrolling; those presentation controls do not own operation
completion.

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiCalculatorApp.cpp
    :show: f:PiCalculatorApp::onCalculationFinished
    :exec: event/pi_calculator --plain --digits 100
    :show-cmd-line:
    :sha256: 442130f1c8dce3e37d1c6f7c7e554c056f941dfa6ae8b15dcb90ad24145ea32f

.. code-block:: cpp

    void PiCalculatorApp::onCalculationFinished(std::exception_ptr error) {
        if (error) {
            std::rethrow_exception(error);
        }
        if (_plain) {
            printNewDigits();
            terminal()->printLine();
        }
        // S leaves the stopped view available even with --digits. Only normal finite completion exits automatically.
        const auto finiteCompletion = _limit.has_value() && !_session->isStopping();
        if (_exitRequested || finiteCompletion || _plain) {
            quit();
        }
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/pi_calculator --plain --digits 100

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348
        253421170679␛[0m

.. erbsland-demo-end::

For the general completion rules, see :doc:`/topics/event/shutting_down_event_work`; for bounded producer demand, see
:doc:`/topics/event/controlling_event_load`.
