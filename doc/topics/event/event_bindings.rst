..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Bindings
    single: Callbacks; Bound Continuations
    single: Events; Weak Bindings

****************************************
Delivering Callbacks with Event Bindings
****************************************

When background work finishes, its result often belongs to an object whose state is managed on an event loop.
The worker can report the result through a callback, but that callback must arrange for the receiving code to run on the
right loop.
An event binding packages this return path into a reusable callable.
With :cpp:func:`bindEvent() <erbsland::event::bindEvent>` and
:cpp:func:`bindWeakEvent() <erbsland::event::bindWeakEvent>`, you can choose where results arrive and whether a bound
recipient must stay alive to receive them.
This page explains those choices, how result arguments survive deferred delivery, and how to keep required completion
independent of optional notifications.

Turning a Callback into Queued Delivery
=======================================

:cpp:func:`invoke() <erbsland::event::EventTarget::invoke>` queues a callback whose work and captured values are already
known.
A binding prepares a callback for values that will arrive later.
You supply the receiving loop and a handler; the producer supplies the handler's arguments when it calls the binding.
The handler can be a lambda or another callable, or a member function paired with its recipient.
It must return ``void``.

Creating the binding does not queue work or start a worker.
Calling the returned callable stores the supplied arguments and queues a delivery on the receiving loop.
The loop then invokes the handler with those arguments.
Delivery is always deferred, even when the binding is called on the receiving loop itself, so the call does not return a
result from the handler.
You can call the same binding repeatedly; each call creates a separate delivery.

This makes bindings useful at ordinary callback boundaries.
A producer can accept a compatible ``std::function<void(...)>`` and call it from its worker thread without knowing
anything about event loops.
The receiver supplies a binding as that callback and keeps its result handling on its own loop.
You can also keep the returned callable directly in a variable declared with ``auto``.

.. erbsland-in-practice-begin:: Returning a File Scan to Its Owner

The checksum packer scans files on a worker thread and reports a file list and an optional error to ``Workflow``.
The application owns the workflow through a shared pointer, and ``Workflow`` inherits
``std::enable_shared_from_this<Workflow>`` so its methods can obtain that shared ownership with ``shared_from_this()``.
Its ``start()`` method runs on the main event loop.
The version using explicit invocation prepares the return path like this:

.. erbsland-demo::
    :source: event/apps/ChecksumPackerInvoke/src/Workflow.hpp
    :show: f:start(-3,-1)
    :sha256: f245d8dc9aad3abb4841da5b90cdd3e8c5c2fdd64442a4c98ed69952a162c50e

.. code-block:: cpp

    auto events = el::currentLoop();
    auto workflow = shared_from_this();
    _scanner.scan([events, workflow](FileWithDigestList fileList, std::exception_ptr error) -> void {
        events->invoke([workflow, fileList = std::move(fileList), error = std::move(error)]() mutable -> void {
            workflow->onScanned(std::move(fileList), std::move(error));
        });
    });

.. erbsland-demo-end::

The outer callback runs when the scanner reports its result; the inner callback waits for dispatch on main.
A member binding expresses the same return path:

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/Workflow.hpp
    :show: f:start(-3,-1)
    :sha256: 80e107731789a49de93896b0dde89c0d1f1b33264c64e3f53a753354dbe76534

.. code-block:: cpp

    _scanner.scan(el::bindEvent(shared_from_this(), &Workflow::onScanned));

.. erbsland-demo-end::

Here, omitting the target captures the main loop because it is running on the calling thread when the binding is
created.
The binding and any queued delivery retain the workflow.
The scanner still accepts an ordinary ``std::function<void(FileWithDigestList, std::exception_ptr)>``; the binding
supplies the queuing and retention that previously required explicit captures.
The error argument is empty on success.
The complete applications are described in :doc:`/demo_apps/event/checksum_packer_invoke` and
:doc:`/demo_apps/event/checksum_packer`.

.. erbsland-in-practice-end::

Choosing the Receiving Loop
===========================

The receiving loop is selected when you create a binding, not when the producer calls it.
Without an explicit target, both binding functions capture
:cpp:func:`currentLoop() <erbsland::event::currentLoop>`.
Create such a binding while the intended receiving loop is running on the calling thread, before passing it to a worker.
Creating it outside a running loop is an error; creating it inside another loop selects that other loop.

Pass an :cpp:type:`EventTargetPtr <erbsland::event::EventTargetPtr>` as the first argument when you already know the
receiving target.
This lets you create the binding before the loop starts or from code running on a different thread.
Later calls still deliver to the captured target, regardless of which thread submits them.
The same explicit-target form is available for callable, strong member, and weak member bindings.

.. erbsland-in-practice-begin:: Supplying the Workflow's Target Explicitly

If the checksum workflow already has its receiving target in ``events``, it can prepare its scan callback explicitly:

.. code-block:: cpp

    auto onScanned = el::bindEvent(events, shared_from_this(), &Workflow::onScanned);

This callback can be passed to the scanner even when its creation does not take place inside the receiving loop.

.. erbsland-in-practice-end::

A binding retains the target object, but it does not start the loop or keep its thread running.
Your application must keep dispatch running until required results have been handled.
Target retention alone does not guarantee that a submission is accepted or that its handler will run.

Choosing Strong or Weak Recipient Ownership
===========================================

When you defer a method call, its recipient may otherwise disappear before dispatch.
Choosing a strong or weak member binding determines whether pending work keeps that recipient alive, independently of
choosing the receiving loop.

A strong member binding made with :cpp:func:`bindEvent() <erbsland::event::bindEvent>` takes a ``std::shared_ptr`` and a
member-function pointer.
The binding retains the recipient, and every copy of the binding and every queued delivery shares that retention.
Releasing your original pointer therefore does not destroy the recipient while any of those owners remain.
This is useful for an operation owner that must process results, record failures, or complete cleanup after background
work finishes.

A weak member binding made with :cpp:func:`bindWeakEvent() <erbsland::event::bindWeakEvent>` takes a ``std::weak_ptr``
and a member-function pointer.
It queues delivery without retaining the recipient, then attempts to lock the weak pointer on the receiving loop just
before calling the method.
If the recipient has expired, the method is skipped, including when the recipient was still alive at submission time.
If locking succeeds, the recipient stays alive until the method returns or throws.
This is useful for a display or observer whose notifications are no longer needed after it disappears.

Both forms use the same deferred delivery and argument storage.
The difference is whether pending callbacks extend the recipient's lifetime.
A weak binding still queues a delivery when its recipient has expired; it does not cancel the producer's work or report
completion back to it.
Required completion therefore needs an owner that remains alive, while weak delivery is appropriate for notifications
that may safely be omitted.

.. erbsland-in-practice-begin:: Keeping a Display Optional

The pi calculator keeps required calculation state separate from its optional display:

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiCalculatorApp.hpp
    :show: c:Required data(+0,+2)
    :sha256: 9301b6a61aa3882807293dd42fba2a1662f7cd51388d1ea1812b9f2c74571336

.. code-block:: cpp

    std::shared_ptr<DigitHistory> _history; ///< Required data, independent of presentation.
    std::shared_ptr<PiSession> _session;    ///< Retained until the calculation thread has joined.
    std::shared_ptr<PiView> _view;          ///< Optional recipient, released when the user quits.

.. erbsland-demo-end::

The application creates the following callback while its main loop is running:

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiCalculatorApp.cpp
    :show: c:Optional presentation(-1,+1)
    :sha256: 3ff5c7e7df3741c6bce0eb1bc4a55fc113c14b9fed09e2e803afb96db5e7b417

.. code-block:: cpp

    updated = el::bindWeakEvent(std::weak_ptr<PiView>{_view}, &PiView::onUpdated);

.. erbsland-demo-end::

The session receives this callback for display updates.
Releasing ``_view`` allows the display to disappear even if updates are already queued; those deliveries skip the
expired recipient.
The independently owned history and session remain available to accept required results.
Binding ``_view`` strongly here would instead keep it alive while the callback or its deliveries retained it.

.. erbsland-in-practice-end::

Passing Results and Owning Their Data
=====================================

A producer may finish using a result before the receiving loop gets to it.
To bridge that interval, each invocation of a binding owns storage for its supplied arguments.
Lvalues are copied into that storage, and rvalues are moved into it.
Passing an owned result with ``std::move`` therefore allows its contents to move into the pending delivery.
The producer must treat that result as moved from after the call.

At dispatch, the stored arguments are passed to the handler as rvalues.
A handler can take a result by value and move it onward to the next operation, or take a const reference when it only
needs to inspect the result during the call.
Each invocation has independent argument storage, even when several deliveries wait in the queue at once.

Owning the argument does not necessarily mean owning the data it refers to.
Copying a pointer, a view, or an explicit reference wrapper still borrows the underlying object.
That object must outlive delivery, and shared access still needs appropriate synchronization.
Prefer an owning value when the producer cannot provide that lifetime guarantee.
:doc:`callback_lifetimes` explains borrowed captures and shared state in more detail.

Copies of a binding share one handler, including its captured values and mutable state.
Copying a binding into ``std::function`` does not create a fresh handler for that producer.
Move-only captures remain in the shared handler; they are not automatically consumed by each delivery.
This shared context is separate from the independently stored arguments supplied on each call.

.. erbsland-in-practice-begin:: Moving a Received Result to the Next Step

The checksum workflow receives the scan result as a value in ``onScanned()``.
After checking the error, it transfers the file list to the packer and supplies a binding for the final notification:

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/Workflow.hpp
    :show: f:onScanned(-8,-1)
    :sha256: cff883d6701f640d490d00026ecd0cdc39039f4fe29dc842dd2c17b0b081ce42

.. code-block:: cpp

    _packer.pack(std::move(fileList), el::bindEvent(shared_from_this(), &Workflow::onPacked));

.. erbsland-demo-end::

The list belongs to the receiving method at this point, so it can move the result onward.
The bound ``onPacked()`` method will receive the packer's ``std::exception_ptr`` argument on the same main loop.
:doc:`event_workflows` develops the operation's state transitions and failure handling.

.. erbsland-in-practice-end::

Capturing Request Context
=========================

A member binding is convenient when the producer's result arguments already match the receiving method.
A bound lambda is useful when you also need context known at the time you start the request, such as a request
identifier.
Capture that context when creating the handler, then let the producer supply only the result values when calling the
binding.
The handler can combine both when the receiving loop dispatches it.

Captures follow the usual C++ ownership rules.
A value capture retains its value, and a shared-pointer capture retains its recipient.
Capturing ``this`` or a reference only borrows the object; binding the lambda does not extend that object's lifetime.
Its owner must ensure it survives every delivery that may access it.

.. erbsland-in-practice-begin:: Recognizing a Search Reply

The file search application creates this binding on main before submitting work.
The handler retains the current query generation, while the worker supplies a ``SearchUpdate`` containing its results:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: c:Bind on main(+1,+6)
    :sha256: 868f84ddf2a0bc29f7a24c3acaf5dbd3738a80f1cbf91a7deb5299d88f48aa0c

.. code-block:: cpp

    const auto generation = _results.generation();
    // Bind on main before crossing to the worker. The reply retains this request's identity.
    auto reply = el::bindEvent([this, generation](SearchUpdate update) -> void {
        if (!_exiting) {
            _results.receive(generation, std::move(update));
            _dirty = true;
        }
    });

.. erbsland-demo-end::

``SearchResults::receive()`` checks the retained generation so an older request cannot replace the current query's
results.
The captured ``this`` is borrowed: the application survives its worker and the reply dispatch that can access it.
For the request/reply design and why cancellation alone does not exclude stale results, see
:doc:`communicating_between_loops`; :doc:`/demo_apps/event/file_search` covers the complete application.

.. erbsland-in-practice-end::

Completing Work Independently of Its View
=========================================

Strong and weak bindings work together when an operation can outlive the interface displaying its progress.
Keep required results and completion state in a retained operation owner.
That owner receives the worker's required callbacks through strong bindings and can notify an optional view through a
weak binding.
A skipped display update must not lose a result, prevent completion, or withhold permission for the worker to continue.
For example, if a worker waits for each result to be accepted before producing another, the operation owner must request
the next result independently of whether the view receives its notification.

Closing the view and stopping the operation are separate decisions.
If closing the view also requests cancellation, retain the operation owner and keep the receiving loop dispatching until
required completion has arrived.
Join the worker before releasing state it may still access, then end dispatch when no remaining operation needs it.
The binding manages callback ownership; your application still coordinates cancellation and shutdown.

.. erbsland-in-practice-begin:: Retaining Completion After the Display Closes

The pi calculator's session stores the main-loop target in ``_events`` and inherits
``std::enable_shared_from_this<PiSession>``.
The application retains the session through worker shutdown.
When starting calculation, the session supplies strong bindings for both result batches and final completion:

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiSession.cpp
    :show: c:Required results retain(-1,+4)
    :sha256: 5ff4e25e96defca852138211a3486f02e418e9e07d5ab8b217616631da11b53f

.. code-block:: cpp

    _worker.start(
        limit,
        el::bindEvent(_events, shared_from_this(), &PiSession::onDigits),
        el::bindEvent(_events, shared_from_this(), &PiSession::onFinished));

.. erbsland-demo-end::

``onDigits()`` stores each batch in the retained history and requests further work independently of the optional display
callback.
On exit, the application releases its view and requests cancellation while leaving main available for completion:

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

If calculation is still active, the final strong delivery reaches ``PiSession::onFinished()``, which joins the worker
before notifying the application that it can quit.
The weak display callback can expire without interrupting that sequence.
See :doc:`/demo_apps/event/pi_calculator` for the complete application.

.. erbsland-in-practice-end::

Destroying a binding does not cancel deliveries that are already queued.
Also, the last owner of a handler or recipient may be released outside the receiving loop, so their destruction has no
receiving-thread guarantee.
:doc:`shutting_down_event_work` explains shutdown ordering and cleanup when dispatch ends because of a failure.
For the full argument-transfer, threading, and failure contracts, see :ref:`event-bound-callbacks`.
