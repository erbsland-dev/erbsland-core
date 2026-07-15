***************************
Event Domain API Guidelines
***************************

These guidelines extend the Common API Guidelines for public APIs around the event system.

The purpose of this document is to define a base naming vocabulary for the event system.
It is intentionally plain, technical and list based to get a quick overview of method names and their usage patterns.
If you introduce new vocabulary, update this page to provide a good reference for future extensions.

Core Semantics
==============

*   **post**: send raw event objects.
*   **invoke**: callback/message execution
*   **timer**: for scheduled work needs with explicit lifetime
*   **events**: method/API name to access event functionality of an object
*   **on❮What❯**: registering callbacks for events

Primary Types
=============

.. code-block:: text

    Event // a single event with optional event data
    EventBackendId // A backend identifier (concrete instances in namespace event::id)
    EventData // the base of all event data types
    EventId // A event identifier (concrete instances in namespace event::id)
    EventLoop // The event loop interface
    EventLoopErrorAction // The action an event loop takes after a handled exception
    EventLoopErrorHandler // A callback for event-loop exception handling
    EventPipe // A event driven pipe for thread to thread communication
    EventRegistry // The event and backend id registry (singleton managed by Application)
    Events // The target for posting events and invoking callbacks
    EventTimer // A thread-safe timer for scheduled callbacks
    EventThread // A lightweight event driver thread for user code

Secondary Types
===============

.. code-block:: text

    EventBackend // The interface of an event backend
    EventBackendIdInfo // Information about registered backend identifiers
    EventBackendTarget // The backend-facing target for posting and waking the loop
    EventCallback // A callback executed by an event loop
    EventIdInfo // Information about registered event identifiers
    EventPipeReceiver // A receiver of data from a pipe
    EventPipeSender // A sender of data to a pipe
    EventScheduler // The frontend interface of the scheduler backend
    EventSource // The source for events.
    EventTimerMode // The scheduling mode of a timer

Event Patterns
==============

.. code-block:: text

    T(EventId [, EventData]) // create an event
    o.time() // get the time point of the event
    o.identifier() // get the event identifier
    o.data() // access the event data

Events Patterns
===============

.. code-block:: text

    o.post(event) // post an event
    o.invoke(callback) // queue a callback to execute in the target event loop
    o.invokeAfter(delay, callback) // queue a callback after a delay
    o.get<T>() -> T& // access a backend frontend interface
    o.createTimer(callback) -> EventTimerPtr // create an inactive timer with a fixed callback
    currentEvents() -> EventsPtr // access the events interface for the current managed event loop

Event Attachment Patterns
=========================

.. code-block:: text

    o.events() -> EventsPtr // access a core event target on Application/EventThread
    o.events() -> TEventEditor // access event callbacks on a domain object
    o.on❮Event❯(callback) -> T& // add a callback and return the editor

Event Loop Patterns
===================

.. code-block:: text

    o.run() // run until stopped
    o.runOnce() -> bool // run one event-loop cycle
    o.runOnce(maximumWait) -> bool // run one event-loop cycle with a maximum wait time
    o.runUntilIdle() -> std::size_t // run all immediately available events
    o.stop() // request loop stop
    o.quit() // post a terminal quit event
    o.isRunning() -> bool // test if the loop is running
    o.isQuitRequested() -> bool // test if the loop is terminating
    o.hasError() -> bool // test if a callback error is queued
    o.takeError() -> std::exception_ptr // take the oldest callback error
    o.setErrorHandler(handler) // configure event-loop exception handling
    o.registerBackend(backend) // register an event backend before the loop runs

Event Thread Patterns
=====================

.. code-block:: text

    Application::createEventThread() -> ManagedEventThreadPtr // create an application-managed event thread
    UnmanagedEventThread::create() -> UnmanagedEventThreadPtr // create a standalone event thread
    o.start() // start the event loop in a new thread
    o.quit() // request graceful event-loop quit
    o.join() // wait until the thread has finished
    o.isStarted() -> bool // test if the thread was started
    o.isRunning() -> bool // test if the event loop is running
    o.eventLoop() -> EventLoop& // access the event loop
    o.events() -> EventsPtr // access the events

Timer Patterns
==============

.. code-block:: text

    o.startOnce(delay) // run once after a delay while the timer is kept alive
    o.startFixedDelay(interval) // repeat interval after callback completion
    o.startFixedRate(interval) // repeat on a fixed cadence
    o.stop() // stop a timer
    o.isActive() -> bool // test if a timer is scheduled or pending
    o.mode() -> EventTimerMode // get the current timer mode
    o.interval() -> TimeDelta // get the current delay or interval

Backend Patterns
================

.. code-block:: text

    o.attach(target) // attach a target for backend events
    o.backendId() -> EventBackendId // get the unique backend identifier
    o.wake() // wake a backend from blocking work if supported
    o.poll(now) // poll for due backend events
    o.handleEvent(event) -> bool // handle an event owned by the backend
    o.nextWakeTime() -> optional<TimePoint> // get the next requested poll time

Backend Target Patterns
=======================

.. code-block:: text

    o.postFromBackend(event) // post a backend-generated event to the loop
    o.wakeFromBackend() // wake the loop after a backend state change
