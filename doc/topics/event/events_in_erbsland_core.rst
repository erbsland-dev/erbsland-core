..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Introduction
    single: Event Programming
    single: Event Loops; Concepts

***********************
Events in Erbsland Core
***********************

Erbsland Core's event system lets an application respond to timers, completed operations, and requests from other
threads without writing its own waiting and dispatch logic.
You describe each response in a callback, and an event loop calls it when work is ready.
Several operations can be pending while one thread handles their callbacks in turn.

This page introduces the facilities that make up that model: loops and targets, invocations and timers, event sources,
and the application structures that run them.
It explains how execution and ownership fit together so you can choose an arrangement for your own application.

Event Loops and Targets
=======================

An :cpp:class:`EventLoop <erbsland::event::EventLoop>` waits for work and dispatches callbacks on the thread running it.
A callback is ordinary C++ code passed to another object to be called later.
After a callback returns, the loop can process other ready work or wait until something becomes ready.
The wait belongs to the loop; each callback only needs to describe its response.

An :cpp:class:`Events <erbsland::event::Events>` target provides access to a loop's scheduling methods and services.
For example, ``app.events()`` returns the application's main target, while an event thread or application part has a
target of its own.
Choosing a target determines where queued work will execute.
Holding that target does not start dispatch: some thread must be running its loop.

For most applications, :cpp:class:`Application <erbsland::core::Application>` supplies the main loop and connects it to
startup, error handling, and shutdown.
:doc:`using_event_loops` explains that lifecycle.
An application with its own lifecycle can also own and run a loop directly; that integration belongs in
:doc:`running_event_loops_directly`.

Callback Execution and Responsiveness
=====================================

A loop calls one callback at a time.
The next callback on that loop cannot run until the current one returns, so state accessed exclusively by that loop
needs no mutex to protect it from other callbacks on the same loop.
This guarantee does not cover access from other threads.

A slow callback delays every other activity on its loop.
Neither a due timer nor a shutdown request interrupts a running callback.
Keep callbacks short when the loop must remain responsive, and move expensive independent work to a worker thread.
Queuing a callback on the current loop changes when it runs; it does not make it run concurrently.

Invocations and Timers
======================

An invocation queues a function on a target.
``invoke()`` is useful for a step that should run after the current callback returns, or for sending work to another
thread's loop.
``invokeAfter()`` queues a delayed action and retains its callback without requiring a stored handle.
Neither method calls the function inline.

A timer is useful when you need to stop, restart, or repeat scheduled work.
``createTimer()`` returns an owning timer pointer, and the timer's start methods select its timing behavior.
For example, ``startFixedDelay()`` schedules each repetition relative to the previous callback's completion.
Timer deadlines make work eligible for dispatch; they do not promise an exact execution time.
:doc:`scheduling_work` explains delayed invocations, timer modes, cancellation, and debouncing.

State and Callback Lifetime
===========================

Setup usually returns before deferred work executes.
Any data needed later must therefore survive outside that setup function, either in a callback's value captures or in an
object whose lifetime covers the operation.
Some scheduled work also has an owning handle: releasing the last timer pointer cancels that timer's activity.

The file-size monitor illustrates both kinds of state.
Its stored path identifies the file to inspect, and its stored timer keeps the repeated callback active:

.. erbsland-demo::
    :source: event/FileSizeMonitor/main.cpp
    :show: c:This method is called(+5,-2), f:initialize, f:poll
    :sha256: a993237fdd9554e986fa875e75f478174c7442cdec81425ea639eb7a161ae023

.. code-block:: cpp

    struct AppData {
        el::EventTimerPtr pollTimer;
        el::PathInfo pathInfo;
    } appData;

    void initialize() {
        el::stdOut()->printLine("Monitoring size of file: "_el, appData.pathInfo.resolvedPath());
        poll();
        appData.pollTimer = el::application().events()->createTimer(poll);
        appData.pollTimer->startFixedDelay(el::Seconds{1});
    }

    void poll() {
        appData.pathInfo.reload();
        if (!appData.pathInfo.exists()) {
            el::stdOut()->printLine("File not found"_el);
        } else {
            el::stdOut()->printLine("Size: "_el, appData.pathInfo.fileSize(), " bytes"_el);
        }
    }

.. erbsland-demo-end::

``initialize()`` reads the size once, creates the timer, and returns.
The loop then calls ``poll()`` repeatedly, with a one-second delay after each timer callback completes.
``appData`` survives for the application run; a larger component would normally keep this state in its own members.
A timer pointer local to ``initialize()`` would be destroyed on return and would not sustain the repeated work.

Object lifetime and safe access from multiple threads are separate questions.
A shared pointer keeps an object alive, but it does not protect that object's mutable data.
:doc:`callback_lifetimes` explains value captures, borrowed pointers, weak recipients, and subscription ownership.
The application's setup for this monitor is developed in :doc:`using_event_loops`.

Event Sources and Notifications
===============================

An event source represents something that produces notifications, such as a host lookup or an HTTP server.
You create the source on its owning loop, register handlers through its event editor, and start the operation.
The source delivers its notifications on that loop, allowing your application to handle other work while it waits.
:doc:`using_event_apis` develops this pattern with a host lookup.

A source's ``events()`` editor and a loop's ``events()`` target have different roles.
The editor configures handlers, such as ``onResolved()``; the target schedules work and provides loop services.
Source-owned handlers remain attached to their source.
APIs with independent observers instead return subscription handles that control individual registrations, as explained
in :doc:`callback_lifetimes`.

Worker Threads and Application Parts
====================================

An event thread runs another loop so that its callbacks can execute concurrently with work on the main loop.
You send it input through its target and deliver results to whichever loop owns the receiving state.
This fits independent computations or blocking work that would otherwise delay a responsive main loop.
It also lets a batch tool keep a procedural main function while workers process jobs.
:doc:`processing_work_on_event_threads` explains value transfer, result ownership, and joining completed work.

An application part provides a dedicated loop together with a component lifecycle.
It is useful for a long-lived service that owns event sources, depends on other services, and needs time to finish
operations during shutdown.
The framework starts and stops the part; the part creates its resources and defines when its asynchronous shutdown is
complete.
:doc:`using_event_loops_in_application_parts` explains that arrangement.

Choosing an Event-Based Design
==============================

Event programming is useful when the next action depends on what becomes ready: a timer, a connection, or a background
result.
Many operations can wait without each needing its own waiting thread.
A converter that simply reads, transforms, and writes data may already be clearer as a procedural function.
The useful question is what else your program should be able to do while an operation is pending.

A larger task may span several callbacks and need explicit state for completion, retries, and cancellation.
:doc:`event_workflows` explains how to organize that state.
If your component needs to publish notifications of its own, continue with :doc:`creating_event_sources`.
The chapter's :doc:`overview` also lists topics for overload handling and extending the event system.
