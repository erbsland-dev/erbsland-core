..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Overview

*********************
Event System Overview
*********************

The event system connects callbacks, timers, and background work through event loops.
These topics explain the programming model, help you choose where loops run, and show how to connect callbacks safely.
They then develop scheduling and the state needed to coordinate a complete task.
For work spanning several loops, continue with communication, shutdown, and load control.
Failure handling supports both paths; direct loop ownership and extension APIs form separate paths.

Event System Concepts and Facilities
====================================

:doc:`events_in_erbsland_core` introduces event loops and callbacks from the perspective of procedural C++ code.
It explains how targets, invocations, timers, and event sources fit together, and when a main loop, worker thread, or
application part is useful.

Application Event Loops
=======================

:doc:`using_event_loops` builds a file monitor around the application's main loop, then shows how a derived
application can run setup and cleanup around that loop.
It also explains when worker threads or application parts are useful next steps.

Processing Work on Event Threads
================================

:doc:`processing_work_on_event_threads` explains thread creation, posting work, result ownership, and synchronization
through thread joining.
A file-hashing batch illustrates how exclusive access and value transfer avoid shared mutable result state.

Event Loops in Application Parts
================================

:doc:`using_event_loops_in_application_parts` explains the execution and lifecycle model of a part's dedicated loop.
It uses a file server to demonstrate source ownership, readiness, cross-thread service calls, and asynchronous shutdown.

Connecting to Event Sources
===========================

:doc:`using_event_apis` follows a source from setup to result and explains the shared conventions behind targets,
source-owned editors, replaceable handlers, and independent observers.
It also shows how to send setup to the owner loop and access its network service.

Callback Ownership and Lifetime
===============================

:doc:`callback_lifetimes` explains what retains a source, timer, callback, and observer registration.
You will see when value and strong captures are useful, how weak captures let recipients disappear, and why cancellation
and callback completion are different lifetime questions.

Invocations and Timers
======================

:doc:`scheduling_work` shows how to defer a callback, choose a delayed invocation or retained timer, and cancel or
replace a deadline.
It compares repeating timer modes and develops a debounce for repeated input.

Simplifying Event Code with Bindings
====================================

:doc:`event_bindings` turns nested result callbacks into reusable continuations bound to their receiving loop.
Two checksum applications show the same operation with explicit invocation and event bindings.
A pi calculator then separates required operation completion from an optional display.

Asynchronous Task Workflows
===========================

:doc:`event_workflows` uses the Checksum Packer to connect scanning and packing into one operation.
It explains state transitions, result ownership, and success and failure paths coordinated by callbacks on the main
loop.

Communication Between Loops
===========================

:doc:`communicating_between_loops` routes requests to a worker and replies to a running main loop while keeping
mutable state with its owner.
It explains transferred values, recipient lifetime, obsolete replies, and completion paths that remain responsive.

Cancellation and Shutdown
=========================

:doc:`shutting_down_event_work` follows resources through cancellation or close, final callbacks, loop termination,
and thread joining.
It distinguishes draining accepted callbacks from finishing asynchronous operations, and explains failure and deadline
paths for shutdown.

Handling Event Failures
=======================

:doc:`handling_event_errors` distinguishes synchronous exceptions, operation error notifications, and callback failures.
It shows how to retrieve captured exceptions, choose a loop policy, and end a managed worker's task without leaving its
caller waiting for a missing result.

Keeping Loops Responsive Under Load
===================================

:doc:`controlling_event_load` starts with bounded outstanding requests and short callbacks, then develops queue
capacity as a separate overload policy.
It explains disposable updates, coalescing, blocking admission, and the limits of queue counts and timer deadlines.

Owning Loop Execution
=====================

The planned topic :doc:`running_event_loops_directly` covers dispatch controlled by a procedural host and standalone
worker ownership.
It forms an optional path for applications that need to drive execution themselves.

Extending the Event System
==========================

The extension tutorials are planned as a progression from a component's public notifications to native integration.
:doc:`creating_event_sources` will start with an object that publishes events.
:doc:`defining_custom_events` will develop typed event data and routing, and :doc:`writing_event_backends` will connect
custom work to the loop.
:doc:`integrating_event_loop_drivers` will cover native waiting and custom drivers.
These destinations are marked as planned topics until their tutorials are written.

Related Topics
==============

For the surrounding lifecycle and component framework, see :doc:`/topics/core/choosing_an_application_design` and
:doc:`/topics/core/application_parts`.
