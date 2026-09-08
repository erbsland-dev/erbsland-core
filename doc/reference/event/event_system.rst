.. index::
    single: Event System

************
Event System
************

Usage Notes
===========

``Events::post`` is for raw event objects.
Use ``Events::invoke`` to execute a callback in the target event loop, including thread-to-thread callback messages.
Use ``Events::invokeAfter`` for fire-and-forget delayed callbacks.
Use ``EventTimer`` when the caller needs to keep a cancellation object for one-shot or repeated scheduled work.
Keep the returned ``EventTimer`` pointer for as long as the scheduled work shall remain active.
Use ``currentEvents()`` from domain event editors to attach to the event loop currently running on the thread.
``EventLoopDriver`` provides the single native wait and wake path and can be injected for tests or custom reactors.
Use ``ManagedEventThread`` for application-owned worker event loops and ``UnmanagedEventThread`` for standalone worker
event loops.

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

Source-Owned Event Editors
==========================

An ``EventSource`` owns its handlers and one stable ``EventEditor``.
Call the source's ``events()`` method on its owner loop and use the typed ``on...()`` methods to replace handlers.
Passing an empty callback clears the corresponding handler.
The returned reference is borrowed and remains valid only while the source remains alive.

Returning a reference makes the editor's management role explicit: it is neither a callback subscription nor an
independently retained object.
It also keeps fluent setup natural, for example ``lookup->events().onResolved(...).onError(...)``.
An implementation can construct its editor together with the source or lazily on the first ``events()`` call, but every
call returns the same editor.

``EventEditor::source()`` and ``EventEditor::target()`` form the small common interface needed by generic code that
works with different editor types.
``source()`` returns a shared pointer so such code can deliberately keep the source alive.
The common implementation stores that source weakly to avoid a source/editor ownership cycle and treats an expired
source as an internal logic error.
``target()`` retains the event collection that dispatches the callbacks.
Most application code does not need either accessor and works directly with the typed editor methods.

The name ``events()`` also appears on ``Application`` and ``EventThread``, where it returns an ``EventsPtr`` event-loop
target.
Those classes are not event sources, so there is no editor involved and the existing name keeps its distinct meaning.

Observer Subscriptions
======================

Events that naturally have multiple independent observers use ``add...()`` methods.
Each registration returns a move-only ``EventSubscription``.
Retain that handle for as long as the callback shall remain active; destroying or cancelling it prevents future callback
invocations.
A callback that has already started may finish.

An event offers either a single replaceable ``on...()`` handler or multiple ``add...()`` subscriptions.
It does not combine both semantics for the same notification.

Interface
=========

.. doxygenfunction:: erbsland::event::currentEvents() -> EventsPtr
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
.. doxygenclass:: erbsland::event::EventRegistry
    :members:
.. doxygenclass:: erbsland::event::Events
    :members:
.. doxygenclass:: erbsland::event::EventScheduler
    :members:
.. doxygenclass:: erbsland::event::EventSource
    :members:
.. doxygenclass:: erbsland::event::EventSubscription
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
