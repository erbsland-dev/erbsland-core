***************************
Event Domain API Guidelines
***************************

Core Semantics
==============

Dispatch Model
--------------

.. code-block:: text

    owner loop = one loop that serializes a source's state and callbacks

Subscriptions and Scheduling
----------------------------

.. code-block:: text

    fixed delay = next interval starts after callback completion
    fixed rate = callbacks follow a stable cadence

Primary Types
=============

.. code-block:: text

    Event // one typed event with optional data and posting time
    Events // target for posting events, invoking callbacks, and accessing backends
    EventLoop // serialized event dispatch and native wait loop
    EventTimer // thread-safe retained scheduled-callback handle
    EventThread // event loop running on a dedicated thread

Event Value Types
=================

.. code-block:: text

    EventId, EventBackendId // registered event and backend identifiers
    EventData // base for typed event payloads
    EventSource, EventEditor // event-loop-owned source and source-owned handler editor
    EventIdInfo, EventBackendIdInfo // registered identifier metadata
    EventCallback // callback executed by an event loop
    EventRegistry // application-managed identifier registry

Scheduling and Thread Types
===========================

.. code-block:: text

    EventScheduler // scheduling frontend supplied by the scheduler backend
    EventTimerMode // inactive, once, fixed-delay, or fixed-rate scheduling mode
    ManagedEventThread // application-owned event thread
    UnmanagedEventThread // standalone event thread
    EventLoopErrorAction, EventLoopErrorHandler // callback-failure handling policy

Backend Types
=============

.. code-block:: text

    EventBackend // native or domain event backend interface
    EventBackendTarget // backend-facing loop target for posting and waking
    EventLoopDriver // injectable native wait and wake abstraction

Pattern Definitions
===================

.. code-block:: text

    Ep = EventsPtr/EventLoopPtr // event target or concrete loop pointer

Event Value Patterns
====================

.. code-block:: text

    T(identifier[, data]) // create an event with its posting time
    o.identifier()/time()/data() -> T // inspect event identity, time, or payload
    T::register❮Kind❯(name) -> T // register a stable event or backend identifier
    o.info(identifier) -> T // inspect registered identifier metadata

Dispatch Patterns
=================

.. code-block:: text

    o.post(event) // enqueue a raw event
    o.invoke(callback) // enqueue callback execution in the target loop
    o.invokeAfter(delay, callback) // enqueue a fire-and-forget delayed callback
    o.get❮Backend❯() -> T& // access one backend frontend interface
    currentEvents() -> EventsPtr // access the loop currently running on this thread

Subscription Patterns
=====================

.. code-block:: text

    o.events() -> TEventEditor& // access a source-owned editor on the owner loop
    o.on❮Event❯(callback) -> T& // replace a source-owned handler through an editor
    o.source() -> EventSourcePtr // retain the source that owns an editor
    o.target() -> EventsPtr // retain the target that dispatches the callbacks
    o.ownerEvents() -> EventsPtr // access the loop that owns an event source

Event Loop Patterns
===================

.. code-block:: text

    T::create([driver-or-backend]) -> EventLoopPtr // create a loop with optional integration
    o.run() // run until stopped or quit
    o.runOnce([maximumWait]) -> bool // perform one bounded event-loop cycle
    o.runUntilIdle() -> std::size_t // dispatch all immediately available work
    o.stop()/quit() // request immediate stop or queued graceful termination
    o.isRunning()/isQuitRequested() -> bool // inspect loop lifecycle state
    o.hasError() -> bool // test whether a callback failure is queued
    o.takeError() -> std::exception_ptr // consume the oldest callback failure
    o.setErrorHandler(handler) // configure callback-failure handling
    o.registerBackend(backend) // attach a backend before the loop runs

Timer and Thread Patterns
=========================

.. code-block:: text

    o.createTimer(callback) -> EventTimerPtr // create an inactive retained timer
    o.startOnce/startFixedDelay/startFixedRate(interval) // schedule retained work
    o.stop() // cancel scheduled timer work
    o.isActive() -> bool // test whether a timer is scheduled or pending
    o.mode()/interval() -> T // inspect timer scheduling
    o.createEventThread() -> ManagedEventThreadPtr // create an application-owned event thread
    T::create() -> UnmanagedEventThreadPtr // create a standalone event thread
    o.start()/quit()/join() // control an event thread lifecycle
    o.eventLoop()/events() -> Ep // access a thread's loop or event target

Backend Patterns
================

.. code-block:: text

    o.attach(target, driver) // attach the backend to one event loop
    o.poll(now)/handleEvent(event) -> bool // collect or handle backend work
    o.nextWakeTime() -> time::TimePoint // report the next requested poll time
    o.wait([maximumWait])/wake() // wait for or signal native activity
    o.postFromBackend(event)/wakeFromBackend() // notify the owning loop from a backend
