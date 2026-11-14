.. index::
    single: Event System

************
Event System
************

Usage Notes
===========

See :doc:`/topics/event/events_in_erbsland_core` for the loop, target, backend, source, and editor model, and
:doc:`/topics/event/using_event_loops` for application and worker-loop designs.

``EventTarget::post`` accepts raw event objects; ``invoke`` queues callbacks, and ``invokeAfter`` queues delayed
callbacks.
``EventTimer`` supplies cancellation and repetition through a retained handle.
See :doc:`/topics/event/scheduling_work` for delay, timer lifetime, repetition, and debounce, and
:doc:`/topics/event/event_workflows` for designing event-driven operations around persistent state.
For sending requests and results between loops, see :doc:`/topics/event/communicating_between_loops`.
For resource completion before loop termination and thread joins, see :doc:`/topics/event/shutting_down_event_work`.
``currentLoop()`` identifies the loop currently running on the calling thread.
Applications, application parts, and event threads expose their target through ``loop()``.
The target supports posting, scheduling, and backend access.
Derived applications can control the main loop through ``mainEventLoop()`` and enter it through ``runMainEventLoop()``;
``EventThread::workerEventLoop()`` exposes the worker's full loop interface.
``EventLoopDriver`` provides the native wait and wake interface for custom reactors and tests.

.. _event-bound-callbacks:

Bound Callbacks
===============

:cpp:func:`bindEvent() <erbsland::event::bindEvent>` creates a reusable callback that queues its handler on a captured
``EventTargetPtr``.
Supply an explicit target, or omit it to capture :cpp:func:`currentLoop() <erbsland::event::currentLoop>` when creating
the binding.
The returned object has an implementation-defined type, can be stored with ``auto``, and converts to compatible
``std::function<void(...)>`` callback parameters.
The handler must return ``void`` and be callable as an lvalue.
Free functions, function objects, generic lambdas, and move-only captures are supported.

.. code-block:: cpp

    auto completed = el::bindEvent(target, [](el::String result) -> void {
        el::io::printLine(result);
    });
    // Any producer thread can submit arguments; the target loop executes the handler.
    completed("Finished"_el);

The member-function overload takes a shared recipient and a member-function pointer.
It supports inherited, const, and ``noexcept`` methods where the supplied recipient and arguments are compatible.
An implicit target must be captured while the intended receiving loop is running on the calling thread:

.. code-block:: cpp

    // Inside a Workflow method running on its receiving loop:
    scanner.scan(el::bindEvent(shared_from_this(), &Workflow::onScanned));

:cpp:func:`bindWeakEvent() <erbsland::event::bindWeakEvent>` accepts a weak recipient and a member-function pointer.
It queues each invocation, then locks the recipient immediately before dispatching the method on the receiving loop.
An empty or expired recipient suppresses the method call; a successful lock retains the recipient until the method
returns or throws.

.. code-block:: cpp

    scanner.scan(el::bindWeakEvent(weak_from_this(), &Workflow::onScanned));

Arguments and Handler State
---------------------------

Binding queues no work.
Every call to the returned callback creates an independent delivery, including calls made on the receiving loop itself.
There is no inline execution, synchronous result, or cancellation handle.
Sequential submissions retain the target's queue order; concurrent producers have no additional ordering guarantee.

Arguments are stored using their decayed types: lvalues are copied and rvalues are moved.
Dispatch passes the stored arguments as rvalues, supporting value, const-reference, and rvalue-reference parameters.
A non-const reference parameter requires an explicit ``std::ref`` or ``std::reference_wrapper`` argument.
Pointers, views, and reference wrappers still borrow their referents; copying them does not copy the underlying data.
The caller must keep that data alive and synchronize any shared access.
Conversions to handler parameter types occur during dispatch, after storing the supplied argument types.

Copies of a binding, including copies stored in ``std::function``, share one handler and its mutable state.
Move-only captures remain in that shared handler and are not consumed by delivery.
Only the receiving loop invokes the handler; simultaneous submissions through a stable binding or its copies are safe.
Concurrent assignment or destruction of the same binding object requires caller synchronization.
Moved-from bindings may be destroyed or reassigned; calling one throws ``LogicFailure``.

Lifetime and Failure
--------------------

A binding retains its target, but does not start the loop, keep its thread running, or guarantee acceptance or delivery.
Strong member bindings retain their recipient while any binding copy or queued delivery exists.
Weak bindings never retain the recipient during submission.
An expired weak recipient still consumes a queue entry until dispatch, and does not report completion to its producer.

Destroying the binding does not cancel submitted work.
Queued transport retains the handler and arguments without retaining the target through the binding itself.
Captures supplied by the caller can still create ownership cycles.
Destruction of handlers, arguments, and recipients has no thread-affinity guarantee: final ownership can be released by
dispatch, rejection, target destruction, or destruction of the last binding copy.

Null explicit targets, null strong recipients, null function or member pointers, and empty ``std::function`` handlers
produce ``ParameterFailure`` during binding.
Implicit binding outside a running loop produces ``LogicFailure`` through ``currentLoop()``.
Incompatible invocation arguments and non-void handler results are rejected at compile time when checking or using the
returned callable.
An overloaded method name must first be resolved to the intended member-function pointer type by the caller.

Handler construction, allocation, argument-storage, and submission exceptions propagate on the calling thread.
Argument storage completes before submission; if it fails, no delivery is queued, although supplied rvalues may already
have been moved from.
Exceptions from handler execution follow the receiving loop's error policy.
Queue capacity, blocking, and shutdown follow the existing ``invoke()`` contracts described in
:ref:`event-loop-options`; a returned invocation does not establish that the handler ran or will run.

Event Thread Lifecycle
======================

Event threads are one-shot objects.
After ``start()`` succeeds, ``isStarted()`` remains true even after the worker terminates and is joined.
``isRunning()`` only observes whether the event loop is currently executing and is not a synchronization barrier for
startup or completion.

``quit()`` is an idempotent asynchronous request.
It can be issued before startup and lets events queued before the quit request run before the loop terminates.
``join()`` is only a wait operation: it never requests termination, is a no-op before startup or after a previous join,
and must not be called from the event thread itself.
It does not transport exceptions from the worker thread; an exception that escapes the event loop terminates the
process.

Destroying an event thread requests termination and joins a started worker.
Therefore, the final owning pointer must be released from another thread, never from the event thread itself.

Cooperative Thread Cancellation
-------------------------------

``EventThread::stopToken()`` returns a ``StopToken`` observing the owned loop's permanent quit request.
Both managed and unmanaged threads provide it immediately after construction.
Copies share the request state, can be polled from any thread, and remain valid without retaining the thread or loop.
The source stays private; consumers can observe cancellation but cannot request it through the token.

Explicit thread or underlying-loop quit, posted quit events, quitting error or queue-overflow policies, and thread
destruction signal the token before waiting for callback completion.
Application shutdown signals its managed workers through the same mechanism, including shutdown after a main-loop
failure or a worker's ``FailApplication`` error.
A request before ``start()`` is visible to subsequently dispatched callbacks and is never reset.

The token does not change graceful draining: already accepted callbacks still run and may choose to finish early.
It does not report thread completion; ``join()`` still supplies that synchronization.
Polling is cooperative and cannot interrupt a blocked operating-system call.
``EventLoop::stop()`` and an error handler returning ``Stop`` end resumable dispatch without signalling the token.
``Store`` and ``Ignore`` policies do not request quit either.
See :doc:`/topics/event/shutting_down_event_work` for operation cancellation and shutdown ordering.

.. _event-loop-options:

Queue Capacity and Error Policies
=================================

For practical policy choices and completion paths, see :doc:`/topics/event/handling_event_errors` and
:doc:`/topics/event/controlling_event_load`.

``EventLoopOptions`` is a fluent value copied when a loop or thread is created.
Pass it to ``Application::createEventThread(options)``, ``UnmanagedEventThread::create(options)``, or any
``EventLoop::create`` overload; driver and backend overloads take options last.
The default queue is unlimited and callback failures are stored while dispatch continues.

``maximumQueuedEvents()`` limits ordinary waiting events; zero means unlimited.
``queuedEventCount()`` returns a synchronized snapshot of ordinary and reserved waiting events.
The executing callback, delayed scheduler callbacks, timers not yet queued, and reserved quit control are excluded.
This limits event count, not payload bytes, blocked producers' retained events, or the stored error queue.
Ordinary due callbacks become subject to capacity when the scheduler posts them.
Checking the count before posting cannot enforce capacity; admission itself performs the atomic check.

Background-operation completions and selected deadlines use separately reserved queue capacity, with at most 1,024 live
reservations per loop.
These callbacks count toward ``queuedEventCount()`` but do not consume the ordinary queue limit.
Ordinary and reserved callbacks alternate when both queues contain work.
An operation that cannot reserve its continuation capacity is rejected before it starts.
Shutdown discards pending completions and their retained resources; it does not run callbacks on a worker thread or wait
for an uninterruptible native call.

When capacity is reached, ``queueOverflowAction()`` selects one of these behaviors:

* ``Terminate`` calls ``std::terminate()``. This is the default overflow action.
* ``FailApplication`` rejects the incoming event and reports ``EventQueueFullError`` to the owning application.
* ``Quit`` rejects the incoming event, closes admission, and drains accepted work.
* ``Block`` waits for space; when the active consumer itself would wait, it calls ``std::terminate()``.
* ``Ignore`` discards the incoming event and preserves the accepted queue.

Blocking requires another thread to run or drain the loop, including before a worker starts.
Stop and quit wake blocked producers, which discard their pending events.
They cannot interrupt a callback that is blocked; joins still wait for that callback to return.
Quit control bypasses capacity and does not appear in the queue count.
Rejected repeating timer occurrences are skipped and rescheduled while admission remains open.

``errorPolicy()`` controls ``RuntimeError`` exceptions from invocation and backend event dispatch when no custom error
handler is installed:

* ``Ignore`` discards the exception and continues.
* ``Store`` retains the exception for ``hasError()`` and ``takeError()`` and continues.
* ``Quit`` stores the exception, closes admission, and drains accepted events.
* ``FailApplication`` stores the exception and forwards it to the owning application.

A custom ``setErrorHandler()`` replaces the default action; the exception is still stored before the handler runs.
Clearing the handler restores the configured default policy.
A handler throwing ``RuntimeError`` adds its own exception and stops the loop.
Programming errors, allocation failures, thread-resource exhaustion, and other exceptions bypass these policies and
handlers.
They propagate unchanged from a directly run loop and terminate an event worker if they escape its thread entry point.
Fatal failures in a custom error handler also terminate, because error handling cannot propagate.
Applications must use ``RuntimeError`` subclasses for operational callback failures that the loop should handle.
Queue overflow is governed independently and does not invoke the callback-error handler.

``FailApplication`` is available only on application-managed workers.
Standalone loop and unmanaged-thread factories reject options selecting it with ``ParameterFailure``.
Managed failures preserve the first application error, stop normal main-loop dispatch, and request graceful shutdown of
every managed event thread.
This also releases a procedural main that is waiting for a separate receiver thread to finish.
The retained failure is rethrown on the application thread after shutdown processing and worker joins.
This includes errors arriving before main-loop startup or while workers are being joined.
These runtime exceptions follow the application's diagnostic and exit-code handling.
If a worker outlives its application state, its failure is retained locally and its loop quits.

Source Editors and Subscriptions
================================

An ``EventSource`` owns its handlers and one stable ``EventEditor`` returned by reference.
Editor access and handler changes require the source's owner loop unless the concrete source explicitly documents
thread-safe access and registration.
Thread-safe registration does not change the callback target.
An ``on...()`` method replaces one handler; an empty callback clears it.
``EventEditor::source()`` and ``target()`` return owning pointers, while the editor itself remains borrowed.
Applications, parts, and event threads provide an ``EventTargetPtr`` through ``loop()`` for submitting work.
See :doc:`/topics/event/using_event_apis` for source binding, frontend access, and editor conventions.

Multiple-observer notifications use ``add...()`` and return move-only ``EventSubscription`` handles.
Retain each handle to keep its registration active; cancellation or destruction prevents future invocations, while an
already started callback may finish.
Publisher destruction invalidates its registrations.
A notification uses either single-handler or observer semantics.
See :doc:`/topics/event/callback_lifetimes` for ownership, weak captures, cancellation, and safe callback cleanup.

Default native-driver creation, native event registration, and failed waits raise ``PlatformError`` with the captured
POSIX or Windows error context.
Wait failures propagate from ``run()``, ``runOnce()``, and ``runUntilIdle()``; they are not callback failures governed
by ``errorPolicy()``.

Native wake operations retry interruptions and preserve already-pending wake notifications.
An unexpected wake failure throws ``PlatformFailure`` at the failed native operation, retaining its error context.
``wake()`` permits propagation.
Calls through ``stop()``, ``quit()``, or backend wake notifications terminate at their ``noexcept`` boundaries.
The fatal failure bypasses recoverable callback-error policies.
Posting queues the event before waking the driver; a wake failure does not roll back the queued work.
Failure to drain the Linux wake descriptor instead propagates as ``PlatformError`` from the wait operation.

POSIX TCP and UDP devices serialize native operations with shutdown.
Abort remains synchronous and thread-safe: it releases readiness registrations before closing the socket.
Readiness callbacks retain shared ownership of the native device while running.
Callbacks copied before shutdown hold only a weak reference and cannot access a destroyed device.
Native registration release tolerates an already-absent registration.
Unexpected removal errors throw ``PlatformFailure`` and terminate at the registration's ``noexcept`` cleanup boundary.
Callbacks into owning sources run outside the native operation and driver registration locks.

Interface
=========

.. doxygenfunction:: erbsland::event::bindEvent(EventTargetPtr target, tFunction &&callback)

.. doxygenfunction:: erbsland::event::bindEvent(tFunction &&callback)

.. doxygenfunction:: erbsland::event::bindEvent(EventTargetPtr target, std::shared_ptr<tRecipient> recipient, tMethod method)

.. doxygenfunction:: erbsland::event::bindEvent(std::shared_ptr<tRecipient> recipient, tMethod method)

.. doxygenfunction:: erbsland::event::bindWeakEvent(EventTargetPtr target, std::weak_ptr<tRecipient> weakRecipient, tMethod method)

.. doxygenfunction:: erbsland::event::bindWeakEvent(std::weak_ptr<tRecipient> weakRecipient, tMethod method)
.. doxygenfunction:: erbsland::event::currentLoop() -> EventTargetPtr
.. doxygenclass:: erbsland::event::Event
    :members:
.. doxygenclass:: erbsland::event::EventBackend
    :members:
.. doxygenclass:: erbsland::event::EventBackendId
    :members:
.. doxygenclass:: erbsland::event::EventBackendIdInfo
    :members:
.. doxygenclass:: erbsland::event::EventBackendTarget
    :members:
.. doxygentypedef:: erbsland::event::EventCallback
.. doxygenclass:: erbsland::event::EventData
    :members:
.. doxygenclass:: erbsland::event::EventEditor
    :members:
.. doxygenclass:: erbsland::event::EventId
    :members:
.. doxygenclass:: erbsland::event::EventIdInfo
    :members:
.. doxygenclass:: erbsland::event::EventLoop
    :members:
.. doxygenclass:: erbsland::event::EventLoopDriver
    :members:
.. doxygenenum:: erbsland::event::EventLoopErrorAction
.. doxygentypedef:: erbsland::event::EventLoopErrorHandler
.. doxygenenum:: erbsland::event::EventLoopErrorPolicy
.. doxygenclass:: erbsland::event::EventLoopOptions
    :members:
.. doxygenclass:: erbsland::event::EventQueueFullError
    :members:
.. doxygenenum:: erbsland::event::EventQueueOverflowAction
.. doxygenclass:: erbsland::event::EventRegistry
    :members:
.. doxygenclass:: erbsland::event::EventScheduler
    :members:
.. doxygenclass:: erbsland::event::EventSource
    :members:
.. doxygenclass:: erbsland::event::EventSubscription
    :members:
.. doxygenclass:: erbsland::event::EventTarget
    :members:
.. doxygenclass:: erbsland::event::EventThread
    :members:
.. doxygenclass:: erbsland::event::EventTimer
    :members:
.. doxygenenum:: erbsland::event::EventTimerMode
.. doxygenclass:: erbsland::event::ManagedEventThread
    :members:
.. doxygenclass:: erbsland::event::UnmanagedEventThread
    :members:
