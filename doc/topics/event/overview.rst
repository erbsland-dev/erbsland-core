..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Overview

*********************
Event System Overview
*********************

The event system connects callbacks, timers, and background work through event loops.
Start with the core concepts and application startup, then learn to connect sources, manage callback lifetimes, and
schedule work.
Bindings, event threads, and application parts provide the tools for organizing work around persistent state and
exchanging requests and results between loops.
The later topics explain error handling, orderly shutdown, and responsiveness under load.
Worker pools, direct loop execution, and extension APIs offer further paths for specific application needs.

Core Concepts of the Event System
=================================

:doc:`events_in_erbsland_core` introduces event loops and callbacks from the perspective of procedural C++ code.
It establishes waiting and dispatch, separate loop contexts, and the relationship between a target, a backend's public
interface, an event source, and its editor.

Running an Application with Event Loops
=======================================

:doc:`using_event_loops` connects application startup hooks to running event loops.
It explains direct and derived applications, validated configuration, workers in procedural programs, and registered
parts, with compile-only templates for each startup design.
The lifecycle continues through completion, managed shutdown, and cleanup.

Driving Event Loops Directly
============================

:doc:`running_event_loops_directly` shows how a procedural host dispatches ready work, waits for a delayed result in
short turns, and decides when to stop or close its loop.

Connecting to Event Sources
===========================

:doc:`using_event_apis` develops the source and editor concepts through direct and lazy loop binding, callback
registration, and independent subscriptions.
It explains why callbacks run on the owner loop, how to send setup there, and how direct invocations and bindings fit
the same execution model.

Callback Lifetimes and Ownership
================================

:doc:`callback_lifetimes` explains why callbacks can disappear, access destroyed objects, or keep objects alive forever.
Complete lookup programs show how source ownership, lambda captures, and weak recipients prevent these mistakes.
The page also explains subscription handles and why editor references cannot replace owning source pointers.

Creating Event Sources and Notifications
========================================

:doc:`creating_event_sources` follows an inbox arrival from a directory scan into a consumer's handler.
It shows where to bind the source, who owns its editor, and how to deliver a batch when a handler can replace itself or
stop the watch.

Scheduling Work and Timers
==========================

:doc:`scheduling_work` shows how to defer a callback, choose a delayed invocation or retained timer, and cancel or
replace a deadline.
It compares repeating timer modes and develops a debounce for repeated input.

Delivering Callbacks with Event Bindings
========================================

:doc:`event_bindings` turns nested result callbacks into reusable continuations bound to their receiving loop.
It compares explicit invocation with bindings and separates required operation completion from optional notifications.

Processing Work on Event Threads
================================

:doc:`processing_work_on_event_threads` explains thread creation, posting work, result ownership, and synchronization
through thread joining.
It compares managed and unmanaged ownership, explains startup and shutdown, and makes success and failure completion
paths explicit.

Processing Work with a Worker Pool
==================================

:doc:`processing_work_with_a_worker_pool` follows a batch of independent calculations from submission to its ordered
results.
It shows how to deliver the batch to an owner loop, finish pool shutdown, and consume the same results from a future in
procedural code.

Using Event Loops in Application Parts
======================================

:doc:`using_event_loops_in_application_parts` explains the execution and lifecycle model of a part's dedicated loop.
It covers source ownership, readiness, cross-thread service calls, and asynchronous shutdown.

Designing Event-Driven Applications Around State
================================================

:doc:`event_workflows` explains how persistent state connects callbacks after a procedural call sequence has ended.
It develops decisions based on events and current state, ownership by one loop, and explicit completion and failure.

Sending Requests and Results Between Event Loops
================================================

:doc:`communicating_between_loops` explains queued notifications, requests, and replies by comparing them with
synchronous function calls.
It shows how bindings carry parameters and results, why a reply is a separate delivery, and how receivers retain data
and accept only relevant results.

Handling Event Errors and Callback Exceptions
=============================================

:doc:`handling_event_errors` distinguishes synchronous exceptions, operation error notifications, and callback failures.
It shows how to retrieve captured exceptions, choose a loop policy, and end a managed worker's task without leaving its
caller waiting for a missing result.

Cancelling Event Work and Shutting Down
=======================================

:doc:`shutting_down_event_work` distinguishes cancellation requests, operation completion, loop termination, and
thread joining.
It develops cooperative cancellation, required completion paths, dependency-aware shutdown, and cleanup when closure
fails or takes too long.

Event Loop Responsiveness and Overload Control
==============================================

:doc:`controlling_event_load` explains how callback duration and excess incoming work affect a loop differently.
It develops bounded chunks, outstanding-request limits, queue capacity policies, and coalesced progress updates, then
explains blocking admission and how to interpret queue counts.

Defining and Posting Typed Events
=================================

:doc:`defining_custom_events` follows a cue boundary from its producer into the queue and out to a typed callback.
It explains when a subsystem benefits from a custom event protocol.

Writing an Event Backend
========================

:doc:`writing_event_backends` shows how a subsystem can report its next deadline, queue due changes, and finish after the
last change reaches its consumer.

Integrating Native Events with Loop Drivers
===========================================

:doc:`integrating_event_loop_drivers` follows a POSIX log pipeline through native waiting, backend polling, and ordinary dispatch.
It explains how an idle pipe, a scheduler deadline, and a wake from another thread share a single wait.

Demo Application Walkthroughs
=============================

:doc:`/demo_apps/event/index` follows the complete event applications, from small monitors to multi-stage workflows.
Use these walkthroughs to connect the focused examples in the topics to each application's ownership and completion
strategy.

Related Topics
==============

For the surrounding lifecycle and component framework, see :doc:`/topics/core/choosing_an_application_design` and
:doc:`/topics/core/application_parts`.
