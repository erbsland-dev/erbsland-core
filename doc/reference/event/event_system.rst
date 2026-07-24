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
Retaining an ``EventEditor`` retains its complete callback subscription; releasing or disconnecting it removes that
subscription.
``EventLoopDriver`` provides the single native wait and wake path and can be injected for tests or custom reactors.
Use ``ManagedEventThread`` for application-owned worker event loops and ``UnmanagedEventThread`` for standalone worker
event loops.

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
.. doxygenclass:: erbsland::event::EventThread
    :members:
.. doxygenclass:: erbsland::event::EventTimer
    :members:
.. doxygenenum:: erbsland::event::EventTimerMode
.. doxygenclass:: erbsland::event::ManagedEventThread
    :members:
.. doxygenclass:: erbsland::event::UnmanagedEventThread
    :members:
