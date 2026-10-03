..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Bindings
    single: Callbacks; Bound Continuations
    single: Events; Weak Bindings

************************************
Simplifying Event Code with Bindings
************************************

You already know how ``invoke`` queues a call and how captured values survive until dispatch.
Returning results from a worker often repeats the same two lambdas: one receives the result, and another calls its
recipient on the right loop.
An event binding packages that return path into a reusable callback.
This page develops that return path in two checksum applications, then separates required calculation results from
optional display updates in a pi calculator.

From Nested Callbacks to a Bound Continuation
=============================================

The checksum applications scan files on an ordinary worker thread and return a file list to their main-loop workflow.
``ChecksumPackerInvoke`` spells out the return path with ``invoke``:

.. erbsland-demo::
    :source: event/ChecksumPackerInvoke/src/Workflow.hpp
    :show: f:start(-3,-1)
    :sha256: 3cc5d7a8a2f82f3f17ae925517e37d136db385b7aa8d04d8745397df87115df3

.. code-block:: cpp

    auto events = el::currentEvents();
    auto workflow = shared_from_this();
    _scanner.scan([events, workflow](FileWithDigestList fileList, std::exception_ptr error) -> void {
        events->invoke([workflow, fileList = std::move(fileList), error = std::move(error)]() mutable -> void {
            workflow->onScanned(std::move(fileList), std::move(error));
        });
    });

.. erbsland-demo-end::

The outer callback receives the worker's result.
It moves the file list into the inner callback, which retains the workflow and waits in main's queue.
``ChecksumPacker`` expresses that same continuation with ``bindEvent``:

.. erbsland-demo::
    :source: event/ChecksumPacker/src/Workflow.hpp
    :show: f:start(-3,-1)
    :sha256: 94c65308894fbed42c7c1bef03f12d18c4b7d880f9d39a6a85b7ef5079aa8cfe

.. code-block:: cpp

    _scanner.scan(el::bindEvent(shared_from_this(), &Workflow::onScanned));

.. erbsland-demo-end::

The returned callable accepts the arguments for ``Workflow::onScanned``.
Calling it queues that method; it never calls the method synchronously, even when called on the receiving loop.
The member binding retains the shared workflow for as long as the binding or a queued delivery needs it.

Each call owns its argument storage: lvalues are copied, and rvalues are moved.
The scanner passes its file list with ``std::move``, so the queued delivery owns those files until main receives them.
Stored arguments are delivered as rvalues, allowing the continuation to move the list onward to the packer.
Pointers, views, and explicit reference wrappers still borrow their referents; wrapping them does not extend their
lifetime.

Choosing the Receiving Loop
===========================

``Workflow::start`` runs on main, so the binding without an explicit target captures main through ``currentEvents`` when
it is created.
Later calls from the scan worker return to that captured target.
Creating the same binding on a worker loop would capture the worker instead.

When you already have the intended ``EventsPtr``, pass it explicitly:

.. code-block:: cpp

    auto onScanned = el::bindEvent(events, shared_from_this(), &Workflow::onScanned);

This form also works outside a running loop.
Capturing a target keeps its object alive; it does not keep dispatch running.
The receiving application still has to run its loop until required results have been handled.

Passing Results to a Bound Method
=================================

The workers keep ordinary callback interfaces.
They need no knowledge of the receiving loop or event bindings:

.. erbsland-demo::
    :source: event/ChecksumPacker/src/Scanner.hpp
    :show: c:The callback if the scan is done(+0,+3), f:scan
    :sha256: a5986ec66934a4b111d6a94c40739b432164d901b24d6d0c9b35f6e22e050008

.. code-block:: cpp

    /// The callback if the scan is done.
    /// On success, nullptr is passed to indicate that no failures occurred.
    /// On failure, a std::exception_ptr containing an exception is passed to indicate why the scan failed.
    using OnDone = std::function<void(FileWithDigestList, std::exception_ptr)>;

    void scan(OnDone onDone) {
        _workerThread = std::thread{[onDone = std::move(onDone), this]() -> void { scanInThread(onDone); }};
    }

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/ChecksumPacker/src/Packer.hpp
    :show: c:The callback if the packing is done(+0,+3), f:pack
    :sha256: f2395903b67db6c0ebf1481e738f2eab59f03fa1420b88b66ef1963500a0bbab

.. code-block:: cpp

    /// The callback if the packing is done.
    /// On success, nullptr is passed to indicate that no failures occurred.
    /// On failure, a std::exception_ptr containing an exception is passed to indicate why packing failed.
    using OnDone = std::function<void(std::exception_ptr)>;

    void pack(FileWithDigestList fileList, OnDone onDone) {
        _workerThread = std::thread{[fileList = std::move(fileList), onDone = std::move(onDone), this]() -> void {
            packInThread(fileList, onDone);
        }};
    }

.. erbsland-demo-end::

A binding fits these ``std::function`` parameters through their normal constructors.
The pack callback uses the same transformation with just an error argument.
First, the explicit return path in ``ChecksumPackerInvoke``:

.. erbsland-demo::
    :source: event/ChecksumPackerInvoke/src/Workflow.hpp
    :show: f:onScanned(-8,-1)
    :sha256: 44c58073031f4a7fcbcaa69a90780575fce8297a20e4519ffc231b303dcdd96b

.. code-block:: cpp

    auto events = el::currentEvents();
    auto workflow = shared_from_this();
    _packer.pack(std::move(fileList), [events, workflow](std::exception_ptr error) -> void {
        events->invoke(
            [workflow, error = std::move(error)]() mutable -> void { workflow->onPacked(std::move(error)); });
    });

.. erbsland-demo-end::

Then the bound continuation in ``ChecksumPacker``:

.. erbsland-demo::
    :source: event/ChecksumPacker/src/Workflow.hpp
    :show: f:onScanned(-8,-1)
    :sha256: 0e3f4d4069d6adef77a6bd1c0eed6eb1fbf4272708236b991734e7a36c99d3bf

.. code-block:: cpp

    _packer.pack(std::move(fileList), el::bindEvent(shared_from_this(), &Workflow::onPacked));

.. erbsland-demo-end::

Both applications have the same state transitions, error reporting, archive contents, and worker shutdown ordering.
:doc:`event_workflows` follows those operation decisions without repeating the transport mechanics.

Binding a Callable with Additional Context
==========================================

A method binding passes result arguments directly to a method.
A bound lambda can also retain context that belongs to the request.
TextPreview creates its reply on main before submitting the worker job:

.. erbsland-demo::
    :source: event/TextPreview/TextPreviewApp.cpp
    :show: f:TextPreviewApp::requestPreview
    :sha256: bef1f36356e20a74158df63775569013d23090a7c3dab36ea6bf267439e83148

.. code-block:: cpp

    void TextPreviewApp::requestPreview(el::String text) {
        const auto requestId = ++_requestId;
        // Bind the reply on main before sending work to another loop.
        auto reply =
            el::bindEvent([this, requestId](el::String preview) -> void { receivePreview(requestId, std::move(preview)); });
        _worker->events()->invoke([reply = std::move(reply), text = std::move(text)]() -> void {
            // Worker uses only captured input; main owns every application-state update.
            reply(el::toUppercase(text));
        });
    }

.. erbsland-demo-end::

The bound handler keeps the request identifier; the worker supplies only the computed preview.
``invoke`` remains useful for the immediate worker submission, while the bound reply makes the return path reusable.
The application outlives the worker, so its borrowed ``this`` remains valid throughout the exchange.
:doc:`communicating_between_loops` develops the obsolete-result checks and failure paths.

Copies of a binding share one handler, including its mutable state and move-only captures.
Each call nevertheless creates an independent argument delivery.
That distinction matters when a callback is copied into ``std::function`` or retained by several producers.

Retaining an Operation While Its Display Disappears
===================================================

The pi calculator continuously calculates decimal digits on an ordinary thread.
Its main loop stores every accepted batch in chunked history and displays a scrollable window into that prefix.
Press **S** to stop calculation permanently and browse the retained digits, or **Q** / **Escape** to quit.
Arrow keys, PageUp/PageDown, Home/End scroll; **F** follows the newest digits again.
``--timeout N`` stops and exits after N seconds, including while browsing; zero means unlimited.
``--digits N`` calculates exactly N fractional digits and exits.

``PiCalculatorApp`` inherits ``Application::main()`` to run the main loop and propagate callback failures.
Option processing queues setup before that loop starts dispatching:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiCalculatorApp.cpp
    :show: c:Start the application as soon(+0,+1)
    :sha256: b113b85382f3b2c50c83a3d47397d6827d159ae71885cf7529c442ad8e3f0d27

.. code-block:: cpp

    // Start the application as soon the main loop has started.
    events()->invoke([this]() -> void { begin(); });

.. erbsland-demo-end::

``begin()`` therefore creates its implicit-target bindings while main is the current loop.
The application owns required operation state separately from its optional view:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiCalculatorApp.hpp
    :show: c:Required data(+0,+2)
    :sha256: 6ef126dd9671e459c2c89cb599f38312030fb3daedd34e3d5d67160890e613ca

.. code-block:: cpp

    std::shared_ptr<DigitHistory> _history; ///< Required data, independent of presentation.
    std::shared_ptr<PiSession> _session;    ///< Retained until the calculation thread has joined.
    std::shared_ptr<PiView> _view;          ///< Optional recipient, released when the user quits.

.. erbsland-demo-end::

``PiSession`` binds batch and final callbacks strongly to itself on main.
The worker's interfaces remain ordinary ``std::function`` callbacks:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiWorker.hpp
    :show: c:Receive one owned(+0,+1), c:Receive exactly one(+0,+1)
    :sha256: da3d7f7a42c4c93061ead861854e067f3a58cefe985f76c0f981ea90e7898687

.. code-block:: cpp

    /// Receive one owned batch of fractional digits on the calculation thread.
    using OnDigits = std::function<void(el::String)>;

    /// Receive exactly one final notification; nullptr means ordinary completion or cancellation.
    using OnFinished = std::function<void(std::exception_ptr)>;

.. erbsland-demo-end::

The session supplies both receiving methods:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiSession.cpp
    :show: c:Required results retain(+4,+6)
    :sha256: 73cb8814d0fb11df3d8466e77eb3a109881d2b5a82e601e6e8614a26225b3a9f

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

.. erbsland-demo-end::

After main stores each batch, the session requests the next one.
At most one batch is outstanding, so calculation cannot accumulate a backlog while main is busy.
Batches contain at most 32 fractional digits; a nonempty partial batch is flushed after roughly 20 ms of calculation.
Optional display notification does not control demand or completion.

The application supplies that notification through ``bindWeakEvent``:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiCalculatorApp.cpp
    :show: c:Optional presentation(+0,+1)
    :sha256: 3477dd02d35ed9d669406fbcaa338efc8ee3d0205e90fab8fa8fa6d9c957029a

.. code-block:: cpp

    // Optional presentation must not keep the view alive after Q or timeout releases it.
    updated = el::bindWeakEvent(std::weak_ptr<PiView>{_view}, &PiView::onUpdated);

.. erbsland-demo-end::

The weak member binding checks its recipient on main during dispatch.
If the view has disappeared, the queued delivery makes no call.
Once a weak recipient is locked successfully, it stays alive through that method call.
The session holds no strong path back to the view, and losing that view cannot discard a required calculation result.

The arithmetic is isolated in ``PiDigitGenerator``, using growing exact ``BigInteger`` values and the streaming
algorithm from section 5 of `Gibbons's corrected paper
<https://www.cs.ox.ac.uk/people/jeremy.gibbons/publications/spigot.pdf>`_.
History retains the entire computed prefix, so memory use grows as calculation continues.
Plain mode provides deterministic output without interactive screen controls:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiCalculatorApp.cpp
    :show: c:S leaves the stopped view(+8,+5)
    :exec: event/pi_calculator --plain --digits 100
    :show-cmd-line:
    :sha256: 762124eb4e9d22e7acf792138f2e015c0d54bc0fd1f80a4c23df271eba5d122b

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

An interactive capture after **S** shows the retained prefix ready for browsing.
The number of completed digits depends on when you stop:

.. erbsland-ansi::
    :escape-char: ␛

    ␛[96;40mPi = 3. │ 255 fractional digits │ Stopped - browse the retained digits

    ␛[32m       1␛[90m | ␛[93m1415926535␛[37m ␛[93m8979323846␛[37m ␛[93m2643383279␛[37m ␛[93m5028841971␛[37m ␛[93m6939937510
    ␛[32m      51␛[90m | ␛[93m5820974944␛[37m ␛[93m5923078164␛[37m ␛[93m0628620899␛[37m ␛[93m8628034825␛[37m ␛[93m3421170679
    ␛[32m     101␛[90m | ␛[93m8214808651␛[37m ␛[93m3282306647␛[37m ␛[93m0938446095␛[37m ␛[93m5058223172␛[37m ␛[93m5359408128
    ␛[32m     151␛[90m | ␛[93m4811174502␛[37m ␛[93m8410270193␛[37m ␛[93m8521105559␛[37m ␛[93m6446229489␛[37m ␛[93m5493038196
    ␛[32m     201␛[90m | ␛[93m4428810975␛[37m ␛[93m6659334461␛[37m ␛[93m2847564823␛[37m ␛[93m3786783165␛[37m ␛[93m2712019091
    ␛[32m     251␛[90m | ␛[93m45648
    ␛[32m     301␛[90m |
    ␛[32m     351␛[90m |
    ␛[32m     401␛[90m |
    ␛[32m     451␛[90m |
    ␛[32m     501␛[90m |
    ␛[32m     551␛[90m |
    ␛[32m     601␛[90m |
    ␛[32m     651␛[90m |
    ␛[32m     701␛[90m |
    ␛[32m     751␛[90m |

    [␛[92mS␛[90m]␛[37m Stop ␛[90m[␛[92mQ␛[90m/␛[92mEsc␛[90m]␛[37m Quit ␛[90m[␛[92m↑␛[90m/␛[92m↓␛[90m/␛[92mPgUp␛[90m/␛[92mPgDown␛[90m/␛[92mHome␛[90m/␛[92mEnd␛[90m]␛[37m Scroll ␛[90m[␛[92mF␛[90m]␛[37m Follow ␛[90m(␛[33mfollow␛[90m)␛[0m

Finishing Work After the User Quits
===================================

The quit path releases the optional view first, requests cancellation, and keeps main dispatching:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiCalculatorApp.cpp
    :show: c:Release optional UI(+2,+7)
    :sha256: 5abad4b50ee1c8e37d2c6d10ebd8e9f34db13d2b964c236c75c30e13310860e0

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

The worker checks cancellation between arithmetic advancements, reports final completion through its strong binding, and
is joined by the session on main.
Only then does the application quit its event loop and release the session.
One large-integer operation remains non-interruptible; cancellation does not promise a fixed shutdown latency.
:doc:`shutting_down_event_work` explains this completion sequence and its exception cleanup in detail.

Weak delivery is suitable for optional presentation, while required completion needs an owner that stays alive.
Destroying a binding does not cancel its queued deliveries, and handler destruction has no receiving-thread guarantee.
For complete ownership, threading, argument-transfer, and failure contracts, see :ref:`event-bound-callbacks`.
