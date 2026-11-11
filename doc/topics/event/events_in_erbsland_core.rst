..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Introduction
    single: Event Loops; Concepts
    single: Event Sources; Editors

.. _events-in-erbsland-core:

*********************************
Core Concepts of the Event System
*********************************

An event-driven program describes how to react when work becomes ready.
Instead of waiting for each operation in turn, it gives callbacks to an event system that waits and dispatches them.
This page introduces the vocabulary behind that arrangement: loops, targets, backends, sources, and editors.
Once these roles are clear, :doc:`using_event_apis` explains how to bind sources to a loop and connect your reactions.

A Loop Waits and Dispatches Work
================================

An :cpp:class:`EventLoop <erbsland::event::EventLoop>` runs on one thread and calls ready callbacks one at a time.
Ready work might be a function submitted by your application or a notification that an operation has made progress.
While one callback runs, the next callback on that loop waits.
When the callback returns, the loop can select more ready work or wait until there is something to do.

.. mermaid::

    flowchart LR
        A{"Ready work?"} -->|"Yes"| B["Execute callback"]
        B -->|"Callback returns"| A
        A -->|"No"| C["Wait for work or shutdown"]
        C -->|"Work becomes ready"| A
        C -->|"Shutdown completes"| D["Loop returns"]

An idle loop has not finished.
It remains available for future work until its owner ends dispatch.
Conversely, a callback that blocks for a long time delays everything else on that loop, including shutdown handling.
Returning control to the loop is therefore part of every callback's job.
Scheduling another callback on the same loop defers execution; it does not make the two callbacks concurrent.

The Loop as an Event Target
===========================

``EventLoop`` implements :cpp:class:`EventTarget <erbsland::event::EventTarget>`, the interface used to post work and
access loop-associated services.
``app.loop()`` returns a shared pointer to this interface for the application's main loop.
The target answers *where should this work run?*; obtaining it does not start dispatch.
The application's default main supplies that dispatch on the main thread.

Consider ``el::application().loop()->invoke(callback)``.
``el::application()`` obtains the current application, ``loop()`` selects its main target, and ``invoke()`` queues the
callback on that target.
The call returns without executing the callback inline.
When the main loop dispatches the accepted invocation, it calls the callback on the main thread.
A producer on another thread uses the same target to send work to that loop.

Inside dispatched code, :cpp:func:`el::currentLoop() <erbsland::event::currentLoop>` identifies the loop whose callback
is executing.
Merely being on the main thread, or holding ``app.loop()``, does not establish this context.
Use a queued startup callback when setup needs the intended loop to be current.

.. erbsland-in-practice-begin:: Queueing a Startup Callback

This compile-only startup template borrows a ``Work`` object whose lifetime encloses ``app.run()``.
The initialization hook queues ``work.begin()`` and returns; the framework's default main later dispatches it.
The reader supplies the business operation in ``Work::begin()`` and connects its completion to the template's completion
hook.
The framework supplies startup, dispatch, and shutdown.

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/DirectInitialization/main.cpp
    :show: f:main
    :sha256: b37775b5e56a24298f0e9485e60d61189feaf560f7fb2c5c01476cebaa7b1710

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        auto work = demo::Work{app};
        app.setInitializeFn([&app, &work]() -> void {
            // Queue startup; the default main enters dispatch later.
            app.loop()->invoke([&work]() -> void { work.begin(); });
        });
        return app.run();
    }

.. erbsland-demo-end::

.. erbsland-in-practice-end::

Multiple Loops in One Application
=================================

An application can have a main loop and additional loops running on event threads or in application parts.
Each is a separate execution context.
Callbacks dispatched by one loop are serialized, while callbacks on different threads can run concurrently.
Data used only by callbacks on one loop needs no mutex against those callbacks; access from another thread still needs
synchronization or a transfer of ownership.

.. mermaid::

    flowchart TB
        subgraph M["Main thread"]
            m1["Main callback A"] --> m2["Main callback B"]
        end
        subgraph W["Worker thread"]
            w1["Worker callback A"] --> w2["Worker callback B"]
        end
        m1 -.->|"Post work to worker target"| w2
        w1 -.->|"Post result to main target"| m2

The arrows within each context show sequential execution; they do not impose an order between the two threads.
Choosing a different target selects a different context, not a separate thread for every operation.
Many sources can share one loop and deliver their notifications there.
:doc:`processing_work_on_event_threads` explains when an additional loop is useful and how to own it.

Backends Provide Loop-Associated Facilities
===========================================

A loop also gives access to facilities such as subprocesses or networking.
A *backend* connects a facility to the loop, while its public interface exposes the operations an application uses.
For example, ``loop->get<el::System>()`` returns the system backend's public
:cpp:class:`System <erbsland::system::System>` interface.
It returns a reference, so a factory call is written as ``loop->get<el::System>().createSubprocess(executable)``.

The application does not need to implement a backend to use these facilities.
The interface makes the facility available through the selected loop and its factories associate new sources with that
loop.
Additional backend interfaces can supply other facilities without redesigning the application's event loop.

Sources Produce Notifications
=============================

An :cpp:class:`EventSource <erbsland::event::EventSource>` is an object that produces notifications about an operation
or resource.
Host lookups, network connections, and subprocesses are examples of event sources.
A backend's public interface provides access to a facility; a source created through that interface represents one
particular operation or resource and delivers its notifications through the selected owner loop.
The factory and the event producer therefore have distinct roles.

For sources created through loop-associated factories, perform setup inside a callback on the intended owner loop.
Retain the returned source pointer in an object that lives for the operation, so the source and its handlers remain
available after setup returns.
Creating a source and starting its operation are separate steps: prepare the source, connect the application's
reactions, then use the source's API to start the operation.
Many sources can share the same loop, which dispatches their ready notifications in turn.

An application can also submit work directly through ``invoke()``.
This queues an application callback on the selected target without requiring an event-source object.

Editors Connect Application Reactions
=====================================

A source's ``events()`` returns a reference to its source-owned
:cpp:class:`EventEditor <erbsland::event::EventEditor>`.
Each concrete source exposes a typed editor for the notifications that source supports.
An ``on...()`` method replaces a handler, while an ``add...()`` method creates an independent subscription that you
retain for as long as you need it.
The editor provides access to configure these connections.
Registration does not execute a callback and does not start the operation.
Configure the editor on the owner loop before starting the operation.
As notifications become ready, that loop dispatches the registered handlers in its own execution context.

.. mermaid::

    flowchart TD
        target["EventTarget: chosen loop"] -->|"get&lt;...&gt;()"| backend["Backend's public interface"]
        backend -->|"Factory creates"| source["EventSource bound to this loop"]
        source -->|"events()"| editor["Source-owned typed event editor"]
        editor -->|"on...() registers a callback"| handler["Application handler"]
        source -.->|"After start: notifications become ready"| dispatch["Owner loop dispatches"]
        dispatch -.->|"Calls registered handler"| handler

These roles keep the setup explicit: the target selects the execution context, the backend interface creates a source
associated with it, and the editor exposes that source's notifications.
An application's ``loop()`` target and a source's ``events()`` editor serve different purposes.
The editor is not another loop and does not own a thread.

.. erbsland-in-practice-begin:: Preparing a Subprocess Source

This compile-only template retains the executable configuration and the subprocess in application members.
``handleCommandLineOptions()`` queues the preparation below on the main target, then starts the prepared process in that
same callback.
The queued callback makes the owner-loop context explicit.
Source preparation connects the reactions before launch.

.. erbsland-demo::
    :source: event/templates/EventConcepts/SubprocessSource/ProcessApplication.hpp
    :show: f:handleCommandLineOptions, f:prepareSource, c:Configuration retained(+0,+2)
    :sha256: 8046173729aa6d59fa1a64f583dbc7783d29bea3addf5a7f083f7c571d374a21

.. code-block:: cpp

    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        _executable = el::Path::fromNativeOrThrow(values->getText("executable"_el));
        loop()->invoke([this]() -> void {
            // Source preparation and launch both happen inside the owner loop.
            prepareSource();
            _process->start();
        });
    }

    void prepareSource() {
        _process = loop()->get<el::System>().createSubprocess(_executable);
        _process->events()
            .onExit([this](const el::ExitCode code) -> void { _exitCode = code; })
            .onError([](const el::PlatformErrorContext &error) -> void {
                throw el::ApplicationError{"Child process failed."_el, error.toString()};
            })
            .onFinal([this]() -> void { quit(_exitCode); });
        // TODO: Connect output handlers and other business reactions before launch.
    }

    el::Path _executable;                            ///< Configuration retained until source preparation.
    el::SubprocessPtr _process;                      ///< Retains the source and its source-owned handlers.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Completion status, accessed only on the main loop.

.. erbsland-demo-end::

The reader supplies output processing and other business reactions for the intended operation.
The framework supplies source binding and notification dispatch; the final handler requests application completion.
``onExit()`` records an ordinary exit code, while ``onError()`` sends an operational failure to application diagnostics.
Registering those handlers leaves the subprocess inactive until ``start()`` is called.

.. erbsland-in-practice-end::

Putting the Concepts Together
=============================

The event system separates where work runs, what produces notifications, and how your application responds:

* The **loop** waits and dispatches ready callbacks one at a time. An idle loop is still available for future work.
* The **target** selects that loop for queued work and service access. Obtaining it does not start dispatch, and
  ``invoke()`` queues a callback rather than calling it inline.
* A **backend's public interface** makes a facility available through the loop. Its factories associate new sources with
  that loop.
* A **source** represents an operation or resource that produces notifications. Retaining it keeps its resource and
  registered handlers available while the operation is needed.
* An **editor** connects your callbacks to the source's notifications. Registration configures a reaction; the owner
  loop executes it later when the notification is dispatched.

For source-based work, the usual sequence is to queue setup on the intended loop, create and retain the source, register
its handlers, start the operation, and return control to dispatch.
For a direct invocation, the queued callback itself is the work.
In either case, callbacks on one loop execute sequentially; work on separate loops can run concurrently.

Continue with :doc:`using_event_apis` for source binding, editor conventions, and callback execution.
:doc:`using_event_loops` then helps you choose an application startup hook, and :doc:`callback_lifetimes` explains
more involved ownership arrangements.
